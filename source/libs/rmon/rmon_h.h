#ifndef __RMON_H_H
#define __RMON_H_H
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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/param.h>          /* MAXHOSTNAMELEN */
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>

#ifdef _AIX
#include <sys/select.h>
#endif

#ifdef HPUX
#define TYPE_OF_READFDS         (int *)
#define TYPE_OF_WRITEFDS        (int *)
#else
#define TYPE_OF_READFDS		(fd_set *)
#define TYPE_OF_WRITEFDS	(fd_set *)
#endif

#ifdef SUN4
	/* should be in unistd.h */
	int gethostname(char *, size_t);

	/* should be in time.h */
	int select(size_t, fd_set *, fd_set *, fd_set *, const struct timeval *);

#include <sys/ipc.h>
#include <sys/shm.h>
	/* should be in shm.h */
	char *shmat(int shmid, char *shmaddr, int shmflg);
	int shmdt(char *shmaddr);
	int shmget(key_t key, int size, int shmflg);
	int shmctl(int shmid, int cmd, struct shmid_ds * buf);

#include <sys/sem.h>
	/* should be in sem.h */
	int semop (int semid, struct sembuf * sops, int nsops);
	int semctl (int semid, int semnum, int cmd, union semun arg);
	int semget (key_t key, int nsems, int semflg);

#endif
#if defined(ALPHA) || defined(ALPHA4)
	/* should be in sem.h */
	union semun {
		int val;                     /* value for SETVAL  */
		struct semid_ds *buf;        /* buffer for IPC_STAT & IPC_SET */
		ushort *array;               /* array for GETALL & SETALL */
	};
#endif

#endif /* __RMON_H_H */
