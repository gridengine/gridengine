#ifndef __SGE_GDI_INTERN_H
#define __SGE_GDI_INTERN_H
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

 

#ifdef WIN32NATIVE
#	include "win32nativetypes.h"
#endif

#include "cull.h"
#include "sge_gdi.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* v5.0:       0x10000000 */
/* v5.1:       0x10000001 */
/* v5.2:       0x10000002 */
/* v5.2.3:     0x10000003 */
/* v5.3 alpha1 0x100000F0 */
/* before hash 0x100000F1 */
/* v5.3beta1   0x100000F2 */
#define GRM_GDI_VERSION 0x100000F3

/* sge_gdi_request.c */
typedef struct _sge_gdi_request sge_gdi_request;
struct _sge_gdi_request {
   u_long32         op;
   u_long32         target;

   char             *host;
   char             *commproc;
   u_short          id;

   u_long32         version;
   lList            *lp;
   lList            *alp;
   lCondition       *cp;
   lEnumeration     *enp;
   char             *auth_info;     
   u_long32         sequence_id;
   u_long32         request_id;
   sge_gdi_request  *next;   
};


int sge_send_gdi_request(int sync, const char *rhost, const char *commproc, int id, sge_gdi_request *head);

int sge_unpack_gdi_request(sge_pack_buffer *pb, sge_gdi_request **arp);

int sge_pack_gdi_request(sge_pack_buffer *pb, sge_gdi_request *ar);

sge_gdi_request* free_gdi_request(sge_gdi_request *ar);

sge_gdi_request* new_gdi_request(void);


#define INIT_ALPP(alpp) (alpp && !*alpp)?((*alpp=lCreateList("answers", AN_Type))!=NULL):0

/* sge_send_reports */
int sge_send_reports(const char *rhost, const char *commproc, int id, lList *report_list, int synchron, u_long32 *mid);

/* sge_any_request.c */
enum {
   TAG_NONE            = 0,     /* usable e.g. as delimiter in a tag array */
   TAG_OLD_REQUEST,
   TAG_GDI_REQUEST,
   TAG_ACK_REQUEST,
   TAG_REPORT_REQUEST,
   TAG_FINISH_REQUEST,
   TAG_JOB_EXECUTION,
   TAG_SLAVE_ALLOW,
   TAG_CHANGE_TICKET,
   TAG_SIGJOB,
   TAG_SIGQUEUE,
   TAG_KILL_EXECD,
   TAG_NEW_FEATURES,
   TAG_GET_NEW_CONF,
   TAG_JOB_REPORT,              /* cull based job reports */
   TAG_QSTD_QSTAT,
   TAG_TASK_EXIT,
   TAG_TASK_TID,
   TAG_EVENT_CLIENT_EXIT

#ifdef SECURE
  ,TAG_SEC_ANNOUNCE,
   TAG_SEC_RESPOND,
   TAG_SEC_ERROR
#endif /* SECURE */

#ifdef KERBEROS
  ,TAG_AUTH_FAILURE
#endif

};

enum {
   ACK_JOB_DELIVERY,     /* sent back by execd, when master gave him a job    */
   ACK_SIGNAL_DELIVERY,  /* sent back by execd, when master sends a queue     */
   ACK_JOB_EXIT,         /* sent back by qmaster, when execd sends a job_exit */
   ACK_SIGNAL_JOB,       /* sent back by qmaster, when execd reports a job as */
                         /* running - that was not supposed to be there       */
   ACK_EVENT_DELIVERY    /* sent back by schedd, when master sends events     */
};

/* sending/receiving any request */

void prepare_enroll(const char *name, u_short id, int *tag_priority_list);

int do_enroll(int);

int sge_send_any_request(int synchron, u_long32 *mid, const char *rhost, const char *commproc, int id, sge_pack_buffer *pb, int tag);

int sge_get_any_request(char *rhost, char *commproc, u_short *id, sge_pack_buffer *pb, int *tag, int synchron);

int gdi_send_message_pb(int synchron, const char *tocomproc, int toid, const char *tohost, int tag, sge_pack_buffer *pb, u_long32 *mid);

/* setup.c */
void sge_setup(u_long32 sge_formal_prog_name, lList **alpp);
int reresolve_me_qualified_hostname(void);

/* sge_ack.c */
int sge_send_ack_to_qmaster(int sync, u_long32 type, u_long32 ulong_val, u_long32 ulong_val_2);

u_long32 sge_get_recoverable(lListElem *aep);  

const char *quality_text(lListElem *aep);

#ifdef  __cplusplus
}
#endif

#endif /* __SGE_GDI_INTERN_H */

