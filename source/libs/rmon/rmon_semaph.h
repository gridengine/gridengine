#ifndef __RMON_SEMAPH_H
#define __RMON_SEMAPH_H
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
 *  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
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

#include <sys/ipc.h>
#include "rmon_monitoring_level.h"

/* key for common acces on shared memory between spy & knecht */
#define SHMKEY_BASE 7891
#define SEMKEY_BASE 1111
#define PERMS  0666

typedef struct monitoring_box_type {
   u_long valid;
   u_long count;
   u_long quit;
   monitoring_level level;
   string programname;
   int programname_is_valid;
} monitoring_box_type;

#if defined(CRAY) || defined(NECSX4) || defined(NECSX5)
union semun {
   int val;
   struct semid_ds *buf;
   ushort *array;
};
#endif

int rmon_sem_create(key_t key, int initval, int more_flags);
int rmon_sem_open(key_t key);
int rmon_sem_rm(int id);
int rmon_sem_close(int id);
int rmon_sem_wait(int id);
int rmon_sem_signal(int id);
int rmon_sem_op(int id, int value);

#endif /* __RMON_SEMAPH_H */

