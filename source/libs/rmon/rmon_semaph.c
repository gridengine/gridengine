/*___INFO__MARK_BEGIN__*/
/*************************************************************************
 * 
 *  The Contents of this file are made available subject to the terms of
 *  the Sun Industry Standards Source License Version 1.2
 * 
 *  Sun Microsystems Inc., March, 2001
 * 
 * 
 *  Sun Industry Standards Source License Version 1.2
 *  =================================================
 *  The contents of this file are subject to the Sun Industry Standards
 *  Source License Version 1.2 (the "License"); You may not use this file
 *  except in compliance with the License. You may obtain a copy of the
 *  License at http://www.gridengine.sunsource.net/license.html
 * 
 *  Software provided under this License is provided on an "AS IS" basis,
 *  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
 *  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
 *  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
 *  See the License for the specific provisions governing your rights and
 *  obligations concerning the Software.
 * 
 *   The Initial Developer of the Original Code is: Sun Microsystems, Inc.
 * 
 *   Copyright: 2001 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/
#ifndef RMONLIGHT

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
/* !!!!                                                                                 !!!! */
/* !!!!                           N E V E R                                     !!!! */
/* !!!!  use a Debug-Macro ( DPRINTF or so... ) !!!! */
/* !!!!                     in THIS module !                    !!!! */
/* !!!!                                                                                 !!!! */
/* !!!! Don't even define DEBUG !                               !!!! */
/* !!!!                                                                                 !!!! */
/* !!!! It would end in an infinite recursion ! !!!! */
/* !!!!                                                                                 !!!! */
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

#include "rmon_h.h"
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>
#include "basis_types.h"
#include "rmon_def.h"
#include "rmon_semaph.h"
#include "msg_rmon.h"
#define BIGCOUNT        10000   /* initial value of process counter */

/*
 * Define the semaphore operation arrays for the semop() calls.
 */
#if defined(bsd4_2) || defined(MACH) || defined(__hpux) || defined(_AIX) || defined(SOLARIS) || defined(SINIX) || (defined(LINUX) && defined(_SEM_SEMUN_UNDEFINED))
union semun {
   int val;                     /* value for SETVAL */
   struct semid_ds *buf;        /* buffer for IPC_STAT & IPC_SET */
   ushort *array;               /* array for GETALL & SETALL */
};

#endif

static struct sembuf op_lock[2] =
{
   {2, 0, 0},                   /* wait for [2] (lock) to equal 0 */
   {2, 1, SEM_UNDO}             /* then increment [2] to 1 - this locks it */

/* UNDO to release the lock if processes exits */

/* before explicitly unlocking */
};

static struct sembuf op_endcreate[2] =
{
   {1, -1, SEM_UNDO},           /* decrement [1] (proc counter) with undo on exit */
                                                /* UNDO to adjust proc counter if process */
                                                /* exits before explicitly calling rmon_sem_close() */
   {2, -1, SEM_UNDO}            /* then decrement [2] (lock) back to 0 */
};

static struct sembuf op_open[1] =
{
   {1, -1, SEM_UNDO}            /* decrement [1] (proc counter) with undo on exit */
};

static struct sembuf op_close[3] =
{
   {2, 0, 0},                   /* wait for [2] (lock) to equal 0 */
   {2, 1, SEM_UNDO},            /* then increment [2] to 1 - this locks it */
   {1, 1, SEM_UNDO}             /* then increment [1] (proc counter) */
};

static struct sembuf op_unlock[1] =
{
   {2, -1, SEM_UNDO}            /* decrement [2] (lock) back to 0 */
};

static struct sembuf op_op[1] =
{
   {0, 99, SEM_UNDO}            /* decrement or increment [0] with undo on exit */

/* the 99 is set to the actual amount to add */

/* or subtract (positive or negative) */
};

/****************************************************************************
 * Create a semaphore with a specified initial value.
 * If the semaphore already exists, we don't initialize it (of course).
 * We return the semaphore ID if all OK, else -1.
 */

int rmon_sem_create(
key_t key,
int initval,                    /* used if we create the semaphore */
int more_flags                  /* used for semget() with IPC_EXCL */
) {
   register int id, semval;
   union semun semctl_arg;

   if (key == IPC_PRIVATE)
      return (-1);              /* not intended for private semaphores */

   else if (key == (key_t) - 1)
      return (-1);              /* probably an ftok() error by caller */

 again:
   if ((id = semget(key, 3, 0666 | IPC_CREAT | more_flags)) < 0)
      return (-1);              /* permission problem or tables full */

   /*
    * When the semaphore is created, we know that the value of all
    * 3 members is 0.
    * Get a lock on the semaphore by waiting for [2] to equal 0,
    * then increment it.
    *
    * There is a race condition here.  There is a possibility that
    * between the semget() above and the semop() below, another
    * process can call our rmon_sem_close() function which can remove
    * the semaphore if that process is the last one using it.
    * Therefore, we handle the error condition of an invalid
    * semaphore ID specially below, and if it does happen, we just
    * go back and create it again.
    */

   if (semop(id, &op_lock[0], 2) < 0) {
      if (errno == EINVAL)
         goto again;
   }

   /*
    * Get the value of the process counter.  If it equals 0,
    * then no one has initialized the semaphore yet.
    */

   if ((semval = semctl(id, 1, GETVAL, semctl_arg)) < 0)
      printf(MSG_RMON_CANTGETVAL);

   if (semval == 0) {
      /*
       * We could initialize by doing a SETALL, but that
       * would clear the adjust value that we set when we
       * locked the semaphore above.  Instead, we'll do 2
       * system calls to initialize [0] and [1].
       */

      semctl_arg.val = initval;
      if (semctl(id, 0, SETVAL, semctl_arg) < 0)
         printf(MSG_RMON_CANSETVAL0);

      semctl_arg.val = BIGCOUNT;
      if (semctl(id, 1, SETVAL, semctl_arg) < 0)
         printf(MSG_RMON_CANSETVAL1);
   }

   /*
    * Decrement the process counter and then release the lock.
    */

   if (semop(id, &op_endcreate[0], 2) < 0)
      printf(MSG_RMON_CANTENDCREATE);

   return (id);
}

/****************************************************************************
 * Open a semaphore that must already exist.
 * This function should be used, instead of rmon_sem_create(), if the caller
 * knows that the semaphore must already exist.  For example a client
 * from a client-server pair would use this, if its the server's
 * responsibility to create the semaphore.
 * We return the semaphore ID if all OK, else -1.
 */

int rmon_sem_open(
key_t key 
) {
   register int id;

   if (key == IPC_PRIVATE)
      return (-1);              /* not intended for private semaphores */

   else if (key == (key_t) - 1)
      return (-1);              /* probably an ftok() error by caller */

   if ((id = semget(key, 3, 0)) < 0)
      return (-1);              /* doesn't exist, or tables full */

   /*
    * Decrement the process counter.  We don't need a lock
    * to do this.
    */

   if (semop(id, &op_open[0], 1) < 0)
      printf(MSG_RMON_CANTOPEN);

   return (id);
}

/****************************************************************************
 * Remove a semaphore.
 * This call is intended to be called by a server, for example,
 * when it is being shut down, as we do an IPC_RMID on the semaphore,
 * regardless whether other processes may be using it or not.
 * Most other processes should use rmon_sem_close() below.
 */

int rmon_sem_rm(
int id 
) {
   union semun semctl_arg;
   if (semctl(id, 0, IPC_RMID, semctl_arg) < 0) {
      printf(MSG_RMON_CANTIPCRMID);
      return 0;
   }

   return 1;
}

/****************************************************************************
 * Close a semaphore.
 * Unlike the remove function above, this function is for a process
 * to call before it exits, when it is done with the semaphore.
 * We "decrement" the counter of processes using the semaphore, and
 * if this was the last one, we can remove the semaphore.
 */

int rmon_sem_close(
int id 
) {
   register int semval;
   union semun semctl_arg;

   /*
    * The following semop() first gets a lock on the semaphore,
    * then increments [1] - the process counter.
    */

   if (semop(id, &op_close[0], 3) < 0) {
      printf(MSG_RMON_CANTSEMOP);
      return 0;
   }

   /*
    * Now that we have a lock, read the value of the process
    * counter to see if this is the last reference to the
    * semaphore.
    * There is a race condition here - see the comments in
    * rmon_sem_create().
    */

   if ((semval = semctl(id, 1, GETVAL, semctl_arg)) < 0) {
      printf(MSG_RMON_CANTGETVAL);
      return 0;
   }

   if (semval > BIGCOUNT) {
      printf(MSG_RMON_SEM1GRTBIGCOUNT);
      return 0;
   }

   if (semval == BIGCOUNT) {
      rmon_sem_rm(id);
      return 0;
   }

   if (semop(id, &op_unlock[0], 1) < 0) {
      printf(MSG_RMON_CANTUNLOCK); /* unlock */
      return 0;
   }

   return 1;
}

/****************************************************************************
 * Wait until a semaphore's value is greater than 0, then decrement
 * it by 1 and return.
 * Dijkstra's P operation.  Tanenbaum's DOWN operation.
 */

int rmon_sem_wait(
int id 
) {
   return rmon_sem_op(id, -1);
}

/****************************************************************************
 * Increment a semaphore by 1.
 * Dijkstra's V operation.  Tanenbaum's UP operation.
 */

int rmon_sem_signal(
int id 
) {
   return rmon_sem_op(id, 1);
}

/****************************************************************************
 * General semaphore operation.  Increment or decrement by a user-specified
 * amount (positive or negative; amount can't be zero).
 */

int rmon_sem_op(
int id,
int value 
) {
   if ((op_op[0].sem_op = value) == 0) {
      printf(MSG_RMON_CANTHAVVALUEIS0);
      return 0;
   }

   if (semop(id, &op_op[0], 1) < 0) {
      printf(MSG_RMON_RMONSEMOPIDXPIDYERRORZ_IIS,
         id, (int)getpid(), strerror(errno));
      return 0;
   }

   return 1;
}

#endif /* RMONLIGHT */
