#ifndef __SGE_GDI_H
#define __SGE_GDI_H
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

/* may be this should be included by the gdi user */
#include "cull.h"
#include "gdi_setup.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* allowed values for op field of sge_gdi_request */
enum { 
   /* OPERATION -------------- */
   SGE_GDI_GET = 1, 
   SGE_GDI_ADD, 
   SGE_GDI_DEL, 
   SGE_GDI_MOD,
   SGE_GDI_TRIGGER,
   SGE_GDI_PERMCHECK,
   SGE_GDI_SPECIAL,
   SGE_GDI_COPY,

   /* SUB COMMAND  ----------- */

   /* for SGE_JOB_LIST => SGE_GDI_ADD */
   SGE_GDI_RETURN_NEW_VERSION = (1<<8),

   /* for SGE_JOB_LIST => SGE_GDI_DEL, SGE_GDI_MOD */
   SGE_GDI_ALL_JOBS  = (1<<9),
   SGE_GDI_ALL_USERS = (1<<10),

   /* for SGE_QUEUE_LIST, SGE_EXECHOST_LIST => SGE_GDI_MOD */
   SGE_GDI_SET    = 0,        /* overwrite the sublist with given values */
   SGE_GDI_CHANGE = (1<<11),  /* change the given elements */
   SGE_GDI_APPEND = (1<<12),  /* add some elements into a sublist */
   SGE_GDI_REMOVE = (1<<13)   /* remove some elements from a sublist */
};

#define SGE_GDI_OPERATION (0xFF)
#define SGE_GDI_SUBCOMMAND (~SGE_GDI_OPERATION)

#define SGE_GDI_GET_OPERATION(x) ((x)&SGE_GDI_OPERATION)
#define SGE_GDI_GET_SUBCOMMAND(x) ((x)&SGE_GDI_SUBCOMMAND)


/* allowed values for target field of sge_gdi_request */
enum {
   SGE_ADMINHOST_LIST = 1,
   SGE_SUBMITHOST_LIST,
   SGE_EXECHOST_LIST,
   SGE_QUEUE_LIST,
   SGE_JOB_LIST,
   SGE_EVENT_LIST,
   SGE_CENTRY_LIST,
   SGE_ORDER_LIST,
   SGE_MASTER_EVENT,
   SGE_CONFIG_LIST,
   SGE_MANAGER_LIST,
   SGE_OPERATOR_LIST,
   SGE_PE_LIST,
   SGE_SC_LIST,          /* schedconf list */
   SGE_USER_LIST,
   SGE_USERSET_LIST,
   SGE_PROJECT_LIST,
   SGE_SHARETREE_LIST,
   SGE_CKPT_LIST,
   SGE_CALENDAR_LIST,
   SGE_JOB_SCHEDD_INFO,
   SGE_ZOMBIE_LIST,
   SGE_USER_MAPPING_LIST,
   SGE_HGROUP_LIST,
   SGE_DUMMY_LIST
};

/*
** Special target numbers for ´complex requests´ which shall be handled
** directly at the master in order to reduce network traffic and replication
** of master lists. They are only useful for SGE_GDI_GET requests
** ATTENTION: the numbers below must not collide with the numbers in the enum
**            directly above
*/
enum {
   SGE_QSTAT = 1024,
   SGE_QHOST
};

/* sge_gdi_multi enums */
enum {
   SGE_GDI_RECORD,
   SGE_GDI_SEND,
   SGE_GDI_SHOW
};

/* preserves state between multiple calls to sge_gdi_multi() */
typedef struct _sge_gdi_request sge_gdi_request;
typedef struct {
   sge_gdi_request *first;
   sge_gdi_request *last;
   u_long32 sequence_id;
} state_gdi_multi;
/* to be used for initializing state_gdi_multi */
#define STATE_GDI_MULTI_INIT { NULL, NULL, 0 }

lList *sge_gdi(u_long32 target, u_long32 cmd, lList **lpp, lCondition *cp, lEnumeration *enp);

int sge_gdi_multi(lList **alpp, int mode, u_long32 target, u_long32 cmd, lList *lp, lCondition *cp, lEnumeration *enp, lList **malpp, state_gdi_multi *state);


lList *sge_gdi_extract_answer(u_long32 cmd, u_long32 target, int id, lList *mal, lList **olpp);

#ifdef  __cplusplus
}
#endif

#endif /* __SGE_GDI_H */
