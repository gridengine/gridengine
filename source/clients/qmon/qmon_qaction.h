#ifndef _QMON_QACTION_H_
#define _QMON_QACTION_H_
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

/* 
 * typedefs for all the resources connected to the dialogs 
 * additional typedefs for transfer cull <--> structs
 */

/* queue configuration parameters -> gdilib/sge_queueL.h */

typedef struct _tQCEntry {
/*---- general queue config -----*/
   char qname[100];
   char qhostname[100];
   int  qtype;
   char processors[256];
   int  priority;
   int  job_slots;
   int  rerun;
   char tmpdir[256];
   char calendar[256];
   char shell[256];
   int  shell_start_mode;
   char notify[20];
   int  seq_no;
   int initial_state;
/*---- methods -----*/
   const char *prolog;
   const char *epilog;
   const char *starter_method;
   const char *suspend_method;
   const char *resume_method;
   const char *terminate_method;
/*---- checkpointing -----*/
   char max_migr_time[20];
   char max_no_migr[20];
   char min_cpu_interval[20];
   lList* migr_load_thresholds;
/*---- load thresholds ----*/
   lList* load_thresholds;
/*---- suspend thresholds ----*/
   lList* suspend_thresholds;
   char suspend_interval[20];
   int    nsuspend;
/*---- queue limits  ----*/
   lList* limits_hard;
   lList* limits_soft;
   /*
   char s_rt[20];
   char h_rt[20];
   char s_cpu[20];
   char h_cpu[20];
   char s_fsize[20];
   char h_fsize[20];
   char s_data[20];
   char h_data[20];
   char s_stack[20];
   char h_stack[20];
   char s_core[20];
   char h_core[20];
   char s_rss[20];
   char h_rss[20];
   char s_vmem[20];
   char h_vmem[20];
   */
/*---- complexes  ----*/
   lList *complexes;
   lList *consumable_config_list;
/*---- user access ---*/
   lList *acl;
   lList *xacl;
/*---- project access ---*/
   lList *prj;
   lList *xprj;
/*---- owner list ----*/
   lList *owner_list;
/*---- subordinate list ----*/
   lList *subordinates;
}  tQCEntry;

   
typedef struct _tAction {
   String label;
   XtCallbackProc callback;
} tAction;

enum {
   QC_ADD,
   QC_MODIFY,
   QC_DELETE
};

/*-------------------------------------------------------------------------*/
void qmonQCPopup(Widget w, XtPointer cld, XtPointer cad);
void updateQCQ(void);
void updateQCC(void);
void updateQCA(void);
void updateQCP(void);
void qmonQCPopdown(Widget w, XtPointer cld, XtPointer cad);

#endif /* _QMON_QACTION_H_ */

