#ifndef _QMON_SUBMIT_H_
#define _QMON_SUBMIT_H_
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

#include <Xm/Xm.h>
#include "qmon_proto.h"

typedef struct _tSMEntry {
   String   job_script;
   String   job_tasks;
   String   job_name;
   String   project;
   String   ckpt_obj;
   String   directive_prefix;
   String   cell;
   String   account_string;
   String   pe;
   lList    *task_range;            /* RN_Type */
   lList    *job_args;              /* ST_Type */
   lList    *shell_list;            /* PN_Type */
   lList    *mail_list;             /* MR_Type */
   lList    *stdoutput_path_list;   /* PN_Type */
   lList    *stdinput_path_list;    /* PN_Type */
   lList    *stderror_path_list;    /* PN_Type */   
   lList    *hard_resource_list;     
   lList    *soft_resource_list;
   lList    *hard_queue_list;       /* QR_Type */
   lList    *soft_queue_list;       /* QR_Type */
   lList    *master_queue_list;       /* QR_Type */
   lList    *env_list;              /* Environment */
   lList    *ctx_list;              /* Context */
   lList    *hold_jid;              /* JB_jid_predecessor_list */
   int      mail_options;
   int      merge_output;
   int      priority;
   int      jobshare;
   Cardinal execution_time;
   Cardinal deadline;
   int      hold;
   int      now;
   int      notify;
   int      restart;
   int      cwd;
   int      checkpoint_attr;
   int      checkpoint_interval;
   int      verify_mode;
} tSMEntry;

typedef struct _tSubmitMode {
   int mode;
   int sub_mode;
   u_long32 job_id;
} tSubmitMode;

enum _tSubmitSensitivityMode {
   SUBMIT_NORMAL           = 0x01,
   SUBMIT_QSH              = 0x02,
   SUBMIT_BINARY           = 0x04,
   SUBMIT_SCRIPT           = 0x08,
   SUBMIT_QALTER_PENDING   = 0x10,
   SUBMIT_QALTER_RUNNING   = 0x20
};

void qmonSubmitPopup(Widget w, XtPointer cld, XtPointer cad);
void qmonSubmitSetResources(lList **hr, lList **sr);
lList *qmonSubmitHR(void);
lList *qmonSubmitSR(void);
String qmonSubmitRequestType(void);

#endif /* _QMON_SUBMIT_H_ */

