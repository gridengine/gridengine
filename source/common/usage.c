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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sge_stdio.h"

#include "usage.h"
#include "sge_feature.h"

#include "sge.h"
#include "sgermon.h"
#include "sge_prog.h"
#include "sge_options.h"
#include "sge_unistd.h"

#include "msg_common.h"
#include "msg_gdilib.h"

static void print_marked(FILE *fp);
static char* get_argument_syntax(int nr);
static void usage_silent(FILE *fp);

bool start_commd = true; 

static int marker[OA__END];

void mark_argument_syntax(
int argument_number 
) {
   marker[argument_number] = 1;
}


static char* get_argument_syntax(int nr)
{ 
   switch (nr)
   {
     case OA_ACCOUNT_STRING:
       return MSG_GDI_ARGUMENTSYNTAX_OA_ACCOUNT_STRING;
     case OA_COMPLEX_LIST:
       return MSG_GDI_ARGUMENTSYNTAX_OA_COMPLEX_LIST;
     case OA_CONTEXT_LIST:
       return MSG_GDI_ARGUMENTSYNTAX_OA_CONTEXT_LIST; 
     case OA_CKPT_SEL:
         return MSG_GDI_ARGUMENTSYNTAX_OA_CKPT_SEL; 
     case OA_DATE_TIME:
         return MSG_GDI_ARGUMENTSYNTAX_OA_DATE_TIME; 
     case OA_DESTIN_ID_LIST:
         return MSG_GDI_ARGUMENTSYNTAX_OA_DESTIN_ID_LIST; 
     case OA_DESTIN_ID_LIST2:
         return MSG_GDI_ARGUMENTSYNTAX_OA_DESTIN_ID_LIST2;
     case OA_HOLD_LIST:
         return MSG_GDI_ARGUMENTSYNTAX_OA_HOLD_LIST; 
     case OA_HOST_ID_LIST:
         return MSG_GDI_ARGUMENTSYNTAX_OA_HOST_ID_LIST;
     case OA_JOB_ID_LIST:
         return MSG_GDI_ARGUMENTSYNTAX_OA_JOB_ID_LIST; 
     case OA_JOB_IDENTIFIER_LIST:
         return MSG_GDI_ARGUMENTSYNTAX_OA_JOB_IDENTIFIER_LIST; 
     case OA_JOB_QUEUE_DEST:
         return MSG_GDI_ARGUMENTSYNTAX_OA_JOB_QUEUE_DEST; 
     case OA_LISTNAME_LIST:
         return MSG_GDI_ARGUMENTSYNTAX_OA_LISTNAME_LIST; 
     case OA_MAIL_ADDRESS:
         return MSG_GDI_ARGUMENTSYNTAX_OA_MAIL_ADDRESS; 
     case OA_MAIL_LIST:
         return MSG_GDI_ARGUMENTSYNTAX_OA_MAIL_LIST; 
     case OA_MAIL_OPTIONS:
         return MSG_GDI_ARGUMENTSYNTAX_OA_MAIL_OPTIONS; 
     case OA_NODE_LIST:
         return MSG_GDI_ARGUMENTSYNTAX_OA_NODE_LIST; 
     case OA_NODE_PATH:
         return MSG_GDI_ARGUMENTSYNTAX_OA_NODE_PATH; 
     case OA_NODE_SHARES_LIST:
         return MSG_GDI_ARGUMENTSYNTAX_OA_NODE_SHARES_LIST; 
     case OA_PATH_LIST:
         return MSG_GDI_ARGUMENTSYNTAX_OA_PATH_LIST; 
     case OA_FILE_LIST:
         return MSG_GDI_ARGUMENTSYNTAX_OA_FILE_LIST; 
     case OA_PRIORITY:
         return MSG_GDI_ARGUMENTSYNTAX_OA_PRIORITY; 
     case OA_RESOURCE_LIST:
         return MSG_GDI_ARGUMENTSYNTAX_OA_RESOURCE_LIST; 
     case OA_SERVER:
         return MSG_GDI_ARGUMENTSYNTAX_OA_SERVER; 
     case OA_SERVER_LIST:
         return MSG_GDI_ARGUMENTSYNTAX_OA_SERVER_LIST; 
     case OA_SIGNAL:
         return MSG_GDI_ARGUMENTSYNTAX_OA_SIGNAL; 
     case OA_SIMPLE_CONTEXT_LIST:
         return MSG_GDI_ARGUMENTSYNTAX_OA_SIMPLE_CONTEXT_LIST; 
     case OA_SLOT_RANGE:
         return MSG_GDI_ARGUMENTSYNTAX_OA_SLOT_RANGE; 
     case OA_STATES:
         return MSG_GDI_ARGUMENTSYNTAX_OA_STATES; 
     case OA_JOB_TASK_LIST:
         return MSG_GDI_ARGUMENTSYNTAX_OA_JOB_TASK_LIST; 
     case OA_JOB_TASKS:
         return MSG_GDI_ARGUMENTSYNTAX_OA_JOB_TASKS; 
     case OA_TASK_ID_RANGE:
         return MSG_GDI_ARGUMENTSYNTAX_OA_TASK_ID_RANGE; 
     case OA_USER_LIST:
         return MSG_GDI_ARGUMENTSYNTAX_OA_USER_LIST; 
     case OA_VARIABLE_LIST:
         return MSG_GDI_ARGUMENTSYNTAX_OA_VARIABLE_LIST;
     case OA_OBJECT_NAME:
         return MSG_GDI_ARGUMENTSYNTAX_OA_OBJECT_NAME;
     case OA_ATTRIBUTE_NAME:
         return MSG_GDI_ARGUMENTSYNTAX_OA_ATTRIBUTE_NAME;
     case OA_OBJECT_ID_LIST:
         return MSG_GDI_ARGUMENTSYNTAX_OA_OBJECT_ID_LIST;
     case OA_PROJECT_LIST:
         return MSG_GDI_ARGUMENTSYNTAX_OA_PROJECT_LIST;
     case OA_EVENTCLIENT_LIST:
         return MSG_GDI_ARGUMENTSYNTAX_OA_EVENTCLIENT_LIST;
     case OA_HOST_LIST:
         return MSG_GDI_ARGUMENTSYNTAX_OA_HOST_LIST;

     default:
         break; 
   }
  return ""; 
  /*(argument_syntax[nr]);*/
}

static void print_marked(
FILE *fp 
) {
   int i;
   for (i=0; i<OA__END; i++)
      if (marker[i]==1)
         fprintf(fp, "%s\n", get_argument_syntax(i));
}



void sge_usage(
FILE *fp 
) {

  char namebuf[128];
  dstring ds;
  char buffer[256];
  
#define PRINTITD(o,d) print_option_syntax(fp,o,d)
#define PRINTIT(o) print_option_syntax(fp,o,NULL)
#define MARK(n) mark_argument_syntax(n)

   DENTER(TOP_LAYER, "sge_usage");

   sge_dstring_init(&ds, buffer, sizeof(buffer));

   fprintf(fp, "%s\n", feature_get_product_name(FS_SHORT_VERSION, &ds));
   
   if (!strcmp(uti_state_get_sge_formal_prog_name(), "execd"))
      strcpy(namebuf, "sge_execd");
   else if (!strcmp(uti_state_get_sge_formal_prog_name(), "qmaster"))
      strcpy(namebuf, "sge_qmaster");
   else
      strcpy(namebuf, uti_state_get_sge_formal_prog_name());
         
   fprintf(fp, "%s %s [options]\n", MSG_GDI_USAGE_USAGESTRING , namebuf);

   /* reset all option markers */
   memset(marker, 0, sizeof(marker));

   if (VALID_OPT(a_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_a_OPT_DATE_TIME , MSG_GDI_UTEXT_a_OPT_DATE_TIME );
      MARK(OA_DATE_TIME);
   }

   if (VALID_OPT(aattr_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_aattr_OPT, MSG_GDI_UTEXT_aattr_OPT);
      MARK(OA_OBJECT_NAME); 
      MARK(OA_ATTRIBUTE_NAME); 
      MARK(OA_OBJECT_ID_LIST); 
   } 

   if (VALID_OPT(Aattr_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Aattr_OPT, MSG_GDI_UTEXT_Aattr_OPT);
      MARK(OA_OBJECT_NAME); 
      MARK(OA_ATTRIBUTE_NAME); 
      MARK(OA_OBJECT_ID_LIST); 
   }  

   if (VALID_OPT(ac_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_ac_OPT_CONTEXT_LIST, 
         MSG_GDI_UTEXT_ac_OPT_CONTEXT_LIST );
      MARK(OA_COMPLEX_LIST);
   }

   if (VALID_OPT(acal_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_acal_OPT_FNAME , MSG_GDI_UTEXT_acal_OPT_FNAME);
   }

   if (VALID_OPT(Acal_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Acal_OPT_FNAME, MSG_GDI_UTEXT_Acal_OPT_FNAME);
   }

   if (VALID_OPT(ackpt_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_ackpt_OPT_CKPT_NAME, 
         MSG_GDI_UTEXT_ackpt_OPT_CKPT_NAME);
   }

   if (VALID_OPT(Ackpt_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Ackpt_OPT_FNAME, 
         MSG_GDI_UTEXT_Ackpt_OPT_FNAME);
   }

   if (VALID_OPT(aconf_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_aconf_OPT_HOST_LIST, 
         MSG_GDI_UTEXT_aconf_OPT_HOST_LIST);
   }

   if (VALID_OPT(Aconf_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Aconf_OPT_FILE_LIST, MSG_GDI_UTEXT_Aconf_OPT_FILE_LIST );
   }

   if (VALID_OPT(ae_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_ae_OPT_EXEC_SERVER_TEMPLATE, MSG_GDI_UTEXT_ae_OPT_EXEC_SERVER_TEMPLATE );
   }

   if (VALID_OPT(Ae_OPT, uti_state_get_mewho())) {
      PRINTITD( MSG_GDI_USAGE_Ae_OPT_FNAME, MSG_GDI_UTEXT_Ae_OPT_FNAME );
   }

   if (VALID_OPT(ah_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_ah_OPT_HOSTNAME, MSG_GDI_UTEXT_ah_OPT_HOSTNAME );
   }

   if (VALID_OPT(ahgrp_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_ahgrp_OPT, MSG_GDI_UTEXT_ahgrp_OPT);
   }

   if (VALID_OPT(Ahgrp_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Ahgrp_OPT, MSG_GDI_UTEXT_Ahgrp_OPT);
   }

   if (VALID_OPT(am_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_am_OPT_USER_LIST, MSG_GDI_UTEXT_am_OPT_USER_LIST);
      MARK(OA_USER_LIST);
   }


   if (VALID_OPT(ao_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_ao_OPT_USER_LIST , MSG_GDI_UTEXT_ao_OPT_USER_LIST );
      MARK(OA_USER_LIST);
   }

   if (VALID_OPT(ap_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_ap_OPT_PE_NAME , MSG_GDI_UTEXT_ap_OPT_PE_NAME);
   }

   if (VALID_OPT(Ap_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Ap_OPT_FNAME , MSG_GDI_UTEXT_Ap_OPT_FNAME );
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(aprj_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_APRJ , MSG_GDI_UTEXT_APRJ );
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(Aprj_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Aprj , MSG_GDI_UTEXT_Aprj );
   }

   if (VALID_OPT(aq_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_aq_OPT_Q_TEMPLATE , MSG_GDI_UTEXT_aq_OPT_Q_TEMPLATE );
   }

   if (VALID_OPT(Aq_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Aq_OPT_FNAME , MSG_GDI_UTEXT_Aq_OPT_FNAME );
   }

   if (VALID_OPT(as_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_as_OPT_HOSTNAME , MSG_GDI_UTEXT_as_OPT_HOSTNAME);
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(astnode_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_ASTNODE_NODE_SHARES_LIST , MSG_GDI_UTEXT_ASTNODE_NODE_SHARES_LIST );
      MARK(OA_NODE_SHARES_LIST);
      MARK(OA_NODE_PATH);
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(astree_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_ASTREE , MSG_GDI_UTEXT_ASTREE);
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(Astree_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_ASTREE_FNAME, MSG_GDI_UTEXT_ASTREE_FNAME);
   }

   if (VALID_OPT(au_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_au_OPT_USER_LIST_LISTNAME_LIST , MSG_GDI_UTEXT_au_OPT_USER_LIST_LISTNAME_LIST );
      MARK(OA_USER_LIST);
      MARK(OA_LISTNAME_LIST);
   }

   if (VALID_OPT(Au_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Au_OPT_LISTNAME_LIST , MSG_GDI_UTEXT_Au_OPT_LISTNAME_LIST );
   }

#ifndef __SGE_NO_USERMAPPING__
   if (VALID_OPT(aumap_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_aumap_OPT, MSG_GDI_UTEXT_aumap_OPT );
   }

   if (VALID_OPT(Aumap_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Aumap_OPT, MSG_GDI_UTEXT_Aumap_OPT );
   }
#endif 
 
   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(aus_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_AUSER , MSG_GDI_UTEXT_AUSER );
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(Aus_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Auser , MSG_GDI_UTEXT_Auser );
   }

   if (VALID_OPT(A_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_A_OPT_ACCOUNT_STRING , MSG_GDI_UTEXT_A_OPT_ACCOUNT_STRING );
      MARK(OA_ACCOUNT_STRING);
   }

   if (VALID_OPT(b_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_b_OPT_YN, MSG_GDI_UTEXT_b_OPT_YN);
   }

   if (VALID_OPT(c_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_c_OPT_CKPT_SELECTOR ,  MSG_GDI_UTEXT_c_OPT_CKPT_SELECTOR );
      MARK(OA_CKPT_SEL);
   }

   if (VALID_OPT(cl_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_c_OPT , MSG_GDI_UTEXT_c_OPT );
   }

   if (VALID_OPT(ckptobj_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_ckpt_OPT_CKPT_NAME ,  MSG_GDI_UTEXT_ckpt_OPT_CKPT_NAME );
   }

   if (VALID_OPT(clear_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_clear_OPT, MSG_GDI_UTEXT_clear_OPT);
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(cu_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_clearusage_OPT, MSG_GDI_UTEXT_clearusage_OPT);
   }

   if (VALID_OPT(cwd_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_cwd_OPT, MSG_GDI_UTEXT_cwd_OPT);
   }

   if (VALID_OPT(cq_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_cq_OPT_DESTIN_ID_LIST, 
         MSG_GDI_UTEXT_cq_OPT_DESTIN_ID_LIST);
      MARK(OA_DESTIN_ID_LIST);
   }

   if (VALID_OPT(C_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_C_OPT_DIRECTIVE_PREFIX, 
         MSG_GDI_UTEXT_C_OPT_DIRECTIVE_PREFIX);
   }

   if (VALID_OPT(d_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_d_OPT, MSG_GDI_UTEXT_d_OPT);
   }

   if (VALID_OPT(dattr_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_dattr_OPT, MSG_GDI_UTEXT_dattr_OPT);
      MARK(OA_OBJECT_NAME);
      MARK(OA_ATTRIBUTE_NAME);
      MARK(OA_OBJECT_ID_LIST);  
   }

   if (VALID_OPT(Dattr_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Dattr_OPT, MSG_GDI_UTEXT_Dattr_OPT);
      MARK(OA_OBJECT_NAME);
      MARK(OA_ATTRIBUTE_NAME);
      MARK(OA_OBJECT_ID_LIST);  
   }    

   if (VALID_OPT(dc_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_dc_OPT_SIMPLE_COMPLEX_LIST , MSG_GDI_UTEXT_dc_OPT_SIMPLE_COMPLEX_LIST );
      MARK(OA_SIMPLE_CONTEXT_LIST);
   }

   if (VALID_OPT(dcal_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_dcal_OPT_CALENDAR_NAME , MSG_GDI_UTEXT_dcal_OPT_CALENDAR_NAME );
   }

   if (VALID_OPT(dckpt_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_dckpt_OPT_CKPT_NAME , MSG_GDI_UTEXT_dckpt_OPT_CKPT_NAME );
   }

   if (VALID_OPT(dconf_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_dconf_OPT_HOST_LIST , MSG_GDI_UTEXT_dconf_OPT_HOST_LIST );
   }

   if (VALID_OPT(de_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_de_OPT_HOST_LIST , MSG_GDI_UTEXT_de_OPT_HOST_LIST );
   }

   if (VALID_OPT(display_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_display_OPT_DISPLAY, MSG_GDI_UTEXT_display_OPT_DISPLAY );
   }

   if (VALID_OPT(dh_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_dh_OPT_HOST_LIST , MSG_GDI_UTEXT_dh_OPT_HOST_LIST );
   }

   if (VALID_OPT(dhgrp_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_dhgrp_OPT, MSG_GDI_UTEXT_dhgrp_OPT);
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(dl_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_dl_OPT_DATE_TIME , MSG_GDI_UTEXT_dl_OPT_DATE_TIME );
      MARK(OA_DATE_TIME);
   }

   if (VALID_OPT(dm_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_dm_OPT_USER_LIST , MSG_GDI_UTEXT_dm_OPT_USER_LIST );
      MARK(OA_USER_LIST);
   }

   if (VALID_OPT(do_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_do_OPT_USER_LIST , MSG_GDI_UTEXT_do_OPT_USER_LIST );
      MARK(OA_USER_LIST);
   }

   if (VALID_OPT(dp_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_dp_OPT_PE_NAME, MSG_GDI_UTEXT_dp_OPT_PE_NAME );
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(dprj_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_dprj_OPT_PROJECT, MSG_GDI_UTEXT_dprj_OPT_PROJECT );
      MARK(OA_PROJECT_LIST);
   }

   if (VALID_OPT(dq_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_dq_OPT_DESTIN_ID_LIST, 
         MSG_GDI_UTEXT_dq_OPT_DESTIN_ID_LIST);
      MARK(OA_DESTIN_ID_LIST);
   }

   if (VALID_OPT(ds_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_ds_OPT_HOST_LIST, MSG_GDI_UTEXT_ds_OPT_HOST_LIST);
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(dstnode_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_DSTNODE_NODELIST, MSG_GDI_UTEXT_DSTNODE_NODELIST);
      MARK(OA_NODE_LIST);
      MARK(OA_NODE_PATH);
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(dstree_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_DSTREE  , MSG_GDI_UTEXT_DSTREE );
   }

   if (VALID_OPT(du_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_du_OPT_USER_LIST_LISTNAME_LIST , MSG_GDI_UTEXT_du_OPT_USER_LIST_LISTNAME_LIST );
      MARK(OA_USER_LIST);
      MARK(OA_LISTNAME_LIST);
   }

   if (VALID_OPT(dul_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_dul_OPT_LISTNAME_LIST , MSG_GDI_UTEXT_dul_OPT_LISTNAME_LIST );
      MARK(OA_LISTNAME_LIST);
   }
   
#ifndef __SGE_NO_USERMAPPING__
   if (VALID_OPT(dumap_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_dumap_OPT, MSG_GDI_UTEXT_dumap_OPT );
   }
#endif
   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(dus_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_DUSER_USER, MSG_GDI_UTEXT_DUSER_USER );
      MARK(OA_USER_LIST);
   }


   if (VALID_OPT(e_OPT, uti_state_get_mewho())) {
      if (uti_state_get_mewho() == QMOD) {
         PRINTITD(MSG_GDI_USAGE_e_OPT , MSG_GDI_UTEXT_e_OPT);
      } else {
         PRINTITD(MSG_GDI_USAGE_e_OPT_PATH_LIST, MSG_GDI_UTEXT_e_OPT_PATH_LIST );
         MARK(OA_PATH_LIST);
      }
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(ext_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_ext_OPT, MSG_GDI_UTEXT_ext_OPT );
   }

   if (VALID_OPT(f_OPT, uti_state_get_mewho())) {
      if (uti_state_get_mewho() == QSTAT) 
         PRINTITD(MSG_GDI_USAGE_f_OPT,MSG_GDI_UTEXT_f_OPT_FULL_OUTPUT );
      else
         PRINTITD(MSG_GDI_USAGE_f_OPT,MSG_GDI_UTEXT_f_OPT_FORCE_ACTION );
   }

   if (VALID_OPT(h_OPT, uti_state_get_mewho())) {
      if ((uti_state_get_mewho() == QALTER) || (uti_state_get_mewho() == QRESUB)  || 
          (uti_state_get_mewho() == QHOLD)  || (uti_state_get_mewho() == QRLS)) {
         PRINTITD(MSG_GDI_USAGE_h_OPT_HOLD_LIST , MSG_GDI_UTEXT_h_OPT_HOLD_LIST );
         MARK(OA_HOLD_LIST);
         MARK(OA_JOB_TASK_LIST);
         MARK(OA_JOB_TASKS);
         MARK(OA_TASK_ID_RANGE);
      } else { /* QSUB */
         PRINTITD(MSG_GDI_USAGE_h_OPT, MSG_GDI_UTEXT_h_OPT);
      }
   }

   if (VALID_OPT(hard_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_hard_OPT , MSG_GDI_UTEXT_hard_OPT );
   }

   if (VALID_OPT(help_OPT, uti_state_get_mewho())) {

      PRINTITD(MSG_GDI_USAGE_help_OPT , MSG_GDI_UTEXT_help_OPT );

   }

   if (VALID_OPT(hold_jid_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_hold_jid_OPT , MSG_GDI_UTEXT_hold_jid_OPT );
      MARK(OA_JOB_IDENTIFIER_LIST);
   }
   
   if (VALID_OPT(i_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_i_OPT_PATH_LIST, MSG_GDI_UTEXT_i_OPT_PATH_LIST );
      MARK(OA_FILE_LIST);
   }

   if (VALID_OPT(inherit_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_inherit_OPT, MSG_GDI_UTEXT_inherit_OPT );
   }

   if (VALID_OPT(j_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_j_OPT_YN , MSG_GDI_UTEXT_j_OPT_YN );
   }

   if (VALID_OPT(jid_OPT, uti_state_get_mewho())) {
      if (uti_state_get_mewho() == QSTAT) {
         PRINTITD(MSG_GDI_USAGE_jid_OPT_JOB_ID_LIST , MSG_GDI_UTEXT_jid_OPT_JOB_ID_LIST_PRINTED );
         MARK(OA_JOB_ID_LIST);
      } else if (uti_state_get_mewho() == QALTER) {
         PRINTITD(MSG_GDI_USAGE_jid_OPT_JOB_ID_LIST , MSG_GDI_UTEXT_jid_OPT_JOB_ID_LIST_ALTERED );
         MARK(OA_JOB_ID_LIST);
      } else {
         PRINTIT(MSG_GDI_USAGE_jid_OPT_JID );
      }
   }

   if (VALID_OPT(ke_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_ke_OPT_HOSTS, MSG_GDI_UTEXT_ke_OPT_HOSTS );
      PRINTITD(MSG_GDI_USAGE_k_OPT_MASTERORSCHEDULINGDAEMON, MSG_GDI_UTEXT_k_OPT_MASTERORSCHEDULINGDAEMON );
      MARK(OA_HOST_LIST);
   }

   if (VALID_OPT(kec_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_kec_OPT, MSG_GDI_UTEXT_kec_OPT );
      MARK(OA_EVENTCLIENT_LIST);
   }

   if (VALID_OPT(l_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_l_OPT_RESOURCE_LIST , MSG_GDI_UTEXT_l_OPT_RESOURCE_LIST );
      MARK(OA_RESOURCE_LIST);
   }

   if (VALID_OPT(lj_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_lj_OPT_LOG_FILE , MSG_GDI_UTEXT_lj_OPT_LOG_FILE );
   }

   if (VALID_OPT(m_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_m_OPT_MAIL_OPTIONS, 
         MSG_GDI_UTEXT_m_OPT_MAIL_OPTIONS);
      MARK(OA_MAIL_OPTIONS);
   }

   if (VALID_OPT(masterq_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_masterq_OPT_DESTIN_ID_LIST, 
         MSG_GDI_UTEXT_masterq_OPT_DESTIN_ID_LIST_BIND );
      MARK(OA_DESTIN_ID_LIST2);
   }

   if (VALID_OPT(mattr_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_mattr_OPT, MSG_GDI_UTEXT_mattr_OPT);
      MARK(OA_OBJECT_NAME); 
      MARK(OA_ATTRIBUTE_NAME); 
      MARK(OA_OBJECT_ID_LIST); 
   }

   if (VALID_OPT(Mattr_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Mattr_OPT, MSG_GDI_UTEXT_Mattr_OPT);
      MARK(OA_OBJECT_NAME); 
      MARK(OA_ATTRIBUTE_NAME); 
      MARK(OA_OBJECT_ID_LIST); 
   }    

   if (VALID_OPT(mc_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_mc_OPT_COMPLEX, MSG_GDI_UTEXT_mc_OPT_COMPLEX);
   }

   if (VALID_OPT(mckpt_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_mckpt_OPT_CKPT_NAME, 
         MSG_GDI_UTEXT_mckpt_OPT_CKPT_NAME);
   }

   if (VALID_OPT(Mc_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Mc_OPT_COMPLEX_NAME_FNAME, 
         MSG_GDI_UTEXT_Mc_OPT_COMPLEX_NAME_FNAME);
   }

   if (VALID_OPT(mcal_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_mcal_OPT_CALENDAR_NAME, 
         MSG_GDI_UTEXT_mcal_OPT_CALENDAR_NAME);
   }

   if (VALID_OPT(Mcal_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Mcal_OPT_FNAME, MSG_GDI_UTEXT_Mcal_OPT_FNAME);
   }

   if (VALID_OPT(Mckpt_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Mckpt_OPT_FNAME , MSG_GDI_UTEXT_Mckpt_OPT_FNAME );
   }

   if (VALID_OPT(mconf_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_mconf_OPT_HOSTLISTORGLOBAL, 
         MSG_GDI_UTEXT_mconf_OPT_HOSTLISTORGLOBAL);
   }

   if (VALID_OPT(msconf_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_msconf_OPT, MSG_GDI_UTEXT_msconf_OPT);
   }
   
   if (VALID_OPT(Msconf_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Msconf_OPT, MSG_GDI_UTEXT_Msconf_OPT);
   }
   
   if (VALID_OPT(me_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_me_OPT_SERVER, MSG_GDI_UTEXT_me_OPT_SERVER);
   }

   if (VALID_OPT(Me_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Me_OPT_FNAME, MSG_GDI_UTEXT_Me_OPT_FNAME);
   }

   if (VALID_OPT(mhgrp_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_mhgrp_OPT, MSG_GDI_UTEXT_mhgrp_OPT);
   }

   if (VALID_OPT(Mhgrp_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Mhgrp_OPT, MSG_GDI_UTEXT_Mhgrp_OPT);
   }

   if (VALID_OPT(mp_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_mp_OPT_PE_NAME , MSG_GDI_UTEXT_mp_OPT_PE_NAME );
   }

   if (VALID_OPT(Mp_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Mp_OPT_FNAME , MSG_GDI_UTEXT_Mp_OPT_FNAME );
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(mprj_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_mprj_OPT_PROJECT, MSG_GDI_UTEXT_mprj_OPT_PROJECT);
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(Mprj_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Mprj_OPT_PROJECT, MSG_GDI_UTEXT_Mprj_OPT_PROJECT);
   }

   if (VALID_OPT(mq_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_mq_OPT_QUEUE , MSG_GDI_UTEXT_mq_OPT_QUEUE );
   }

   if (VALID_OPT(mqattr_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_mqattr_OPT_ATTR_NAME_VALUE_DESTIN_ID_LIST,
         MSG_GDI_UTEXT_mqattr_OPT_ATTR_NAME_VALUE_DESTIN_ID_LIST);
      MARK(OA_DESTIN_ID_LIST);
   }

   if (VALID_OPT(Mq_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Mq_OPT_FNAME, MSG_GDI_UTEXT_Mq_OPT_FNAME);
   }

   if (VALID_OPT(Mqattr_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Mqattr_OPT_FNAME_DESTIN_ID_LIST, 
         MSG_GDI_UTEXT_Mqattr_OPT_FNAME_DESTIN_ID_LIST );
      MARK(OA_DESTIN_ID_LIST);
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(mstnode_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_MSTNODE_NODE_SHARES_LIST, 
         MSG_GDI_UTEXT_MSTNODE_NODE_SHARES_LIST );
      MARK(OA_NODE_SHARES_LIST);
      MARK(OA_NODE_PATH);
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(Mstree_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_MSTREE_FNAME, MSG_GDI_UTEXT_MSTREE_FNAME);
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(mstree_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_MSTREE , MSG_GDI_UTEXT_MSTREE );
   }

   if (VALID_OPT(mu_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_mu_OPT_LISTNAME_LIST, 
         MSG_GDI_UTEXT_mu_OPT_LISTNAME_LIST);
      MARK(OA_LISTNAME_LIST);
   }
   
   if (VALID_OPT(Mu_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Mu_OPT_LISTNAME_LIST, 
         MSG_GDI_UTEXT_Mu_OPT_LISTNAME_LIST);
   }

#ifndef __SGE_NO_USERMAPPING__
   if (VALID_OPT(Mumap_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Mumap_OPT, MSG_GDI_UTEXT_Mumap_OPT);
   }

   if (VALID_OPT(mumap_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_mumap_OPT, MSG_GDI_UTEXT_mumap_OPT);
   }
#endif
   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(mus_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_muser_OPT_USER, MSG_GDI_UTEXT_muser_OPT_USER);
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(Mus_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Muser_OPT_USER, MSG_GDI_UTEXT_Muser_OPT_USER);
   }

   if (VALID_OPT(notify_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_notify_OPT, MSG_GDI_UTEXT_notify_OPT);
   }

   if (VALID_OPT(now_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_now_OPT_YN, MSG_GDI_UTEXT_now_OPT_YN);
   }
   
   if (VALID_OPT(M_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_M_OPT_MAIL_LIST, MSG_GDI_UTEXT_M_OPT_MAIL_LIST);
      MARK(OA_MAIL_LIST);
      MARK(OA_MAIL_ADDRESS);
   }

   if (VALID_OPT(N_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_N_OPT_NAME, MSG_GDI_UTEXT_N_OPT_NAME);
   }

   if (VALID_OPT(nostdin_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_nostdin_OPT, MSG_GDI_UTEXT_nostdin_OPT);
   }

   if (VALID_OPT(noshell_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_noshell_OPT, MSG_GDI_UTEXT_noshell_OPT);
   }

   if (VALID_OPT(o_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_o_OPT_PATH_LIST, MSG_GDI_UTEXT_o_OPT_PATH_LIST);
      MARK(OA_PATH_LIST);
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(ot_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_ot_OPT_TICKETS, MSG_GDI_UTEXT_ot_OPT_TICKETS);
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(P_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_P_OPT_PROJECT_NAME, 
         MSG_GDI_UTEXT_P_OPT_PROJECT_NAME);
   }
   if (VALID_OPT(p_OPT, uti_state_get_mewho())) {
      if (uti_state_get_mewho() == QSELECT || uti_state_get_mewho() == QSH || uti_state_get_mewho() == QLOGIN 
          || uti_state_get_mewho() == QRSH ||uti_state_get_mewho() == QSUB || uti_state_get_mewho() == QALTER) {
         PRINTITD(MSG_GDI_USAGE_p_OPT_PRIORITY, MSG_GDI_UTEXT_p_OPT_PRIORITY);
      } else {
         PRINTIT(MSG_GDI_USAGE_p_OPT_PRIORITY);
      }
      MARK(OA_PRIORITY);
   }

   if (VALID_OPT(pe_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_pe_OPT_PE_NAME_SLOT_RANGE, 
         MSG_GDI_UTEXT_pe_OPT_PE_NAME_SLOT_RANGE );
      MARK(OA_SLOT_RANGE);
   }

   if (VALID_OPT(q_OPT, uti_state_get_mewho())) {
      if (uti_state_get_mewho() == QSUB || uti_state_get_mewho() == QALTER || uti_state_get_mewho() == QSH 
          || uti_state_get_mewho() == QLOGIN || uti_state_get_mewho() == QRSH ) {
         PRINTITD(MSG_GDI_USAGE_q_OPT_DESTIN_ID_LIST, 
            MSG_GDI_UTEXT_q_OPT_DESTIN_ID_LIST_BIND);
      } else {
         PRINTITD(MSG_GDI_USAGE_q_OPT_DESTIN_ID_LIST, 
            MSG_GDI_UTEXT_q_OPT_DESTIN_ID_LIST_INFO);
      }
      MARK(OA_DESTIN_ID_LIST2);
   }

#if 0
   if (VALID_OPT(qs_args_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_qs_args_OPT_ARGS_QS_END, 
         MSG_GDI_UTEXT_qs_args_OPT_ARGS_QS_END);
   }
#endif

   if (VALID_OPT(r_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_r_OPT_YN, MSG_GDI_UTEXT_r_OPT_YN);
   }

   if (VALID_OPT(res_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_res_OPT, MSG_GDI_UTEXT_res_OPT);
   }

   if (VALID_OPT(s_OPT, uti_state_get_mewho())) {
      if (uti_state_get_mewho() == QMASTER || uti_state_get_mewho() == EXECD || uti_state_get_mewho() == SCHEDD) { 
         usage_silent(fp);
      } else 
      if (uti_state_get_mewho() == QSELECT) {
         PRINTIT(MSG_GDI_USAGE_s_OPT_STATES );
         MARK(OA_STATES);
      }
      else if (uti_state_get_mewho() == QMOD) {
         PRINTITD(MSG_GDI_USAGE_s_OPT ,MSG_GDI_UTEXT_s_OPT );
      }
      else {
         PRINTIT(MSG_GDI_USAGE_s_OPT_SIGNAL);
         MARK(OA_SIGNAL);
      }
   }

   if (VALID_OPT(rattr_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_rattr_OPT, MSG_GDI_UTEXT_rattr_OPT);
      MARK(OA_OBJECT_NAME);
      MARK(OA_ATTRIBUTE_NAME);
      MARK(OA_OBJECT_ID_LIST);  
   }

   if (VALID_OPT(Rattr_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_Rattr_OPT, MSG_GDI_UTEXT_Rattr_OPT);
      MARK(OA_OBJECT_NAME);
      MARK(OA_ATTRIBUTE_NAME);
      MARK(OA_OBJECT_ID_LIST);  
   }    

   if (VALID_OPT(sc_OPT, uti_state_get_mewho())) {
      if(uti_state_get_mewho() == QCONF) {
         PRINTITD(MSG_GDI_USAGE_sc_OPT_COMPLEX_LIST, 
            MSG_GDI_UTEXT_sc_OPT_COMPLEX_LIST_SHOW);
         MARK(OA_COMPLEX_LIST);
      }
      else {
         PRINTITD(MSG_GDI_USAGE_sc_OPT_CONTEXT_LIST, 
            MSG_GDI_UTEXT_sc_OPT_CONTEXT_LIST_SET);
         MARK(OA_CONTEXT_LIST);
      }
   }

   if (VALID_OPT(scal_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_scal_OPT_CALENDAR_NAME, 
         MSG_GDI_UTEXT_scal_OPT_CALENDAR_NAME);
   }

   if (VALID_OPT(scall_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_scall_OPT, MSG_GDI_UTEXT_scall_OPT);
   }

   if (VALID_OPT(sckpt_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_sckpt_OPT_CKPT_NAME, 
         MSG_GDI_UTEXT_sckpt_OPT_CKPT_NAME);
   }

   if (VALID_OPT(sckptl_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_sckptl_OPT, MSG_GDI_UTEXT_sckptl_OPT);
   }

   if (VALID_OPT(sconf_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_sconf_OPT_HOSTLISTORGLOBAL, 
         MSG_GDI_UTEXT_sconf_OPT_HOSTLISTORGLOBAL);
   }

   if (VALID_OPT(sconfl_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_sconfl_OPT, MSG_GDI_UTEXT_sconfl_OPT);
   }

   if (VALID_OPT(se_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_se_OPT_SERVER, MSG_GDI_UTEXT_se_OPT_SERVER);
   }

   if (VALID_OPT(secl_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_secl_OPT, MSG_GDI_UTEXT_secl_OPT );
   }

   if (VALID_OPT(sel_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_sel_OPT, MSG_GDI_UTEXT_sel_OPT);
   }

   if (VALID_OPT(sep_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_sep_OPT, MSG_GDI_UTEXT_sep_OPT);
   }

   if (VALID_OPT(sh_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_sh_OPT, MSG_GDI_UTEXT_sh_OPT);
   }

   if (VALID_OPT(shgrp_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_shgrp_OPT, MSG_GDI_UTEXT_shgrp_OPT);
   }

   if (VALID_OPT(shgrpl_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_shgrpl_OPT, MSG_GDI_UTEXT_shgrpl_OPT);
   }
 
   if (VALID_OPT(sm_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_sm_OPT , MSG_GDI_UTEXT_sm_OPT );
   }

   if (VALID_OPT(so_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_so_OPT , MSG_GDI_UTEXT_so_OPT );
   }

   if (VALID_OPT(soft_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_soft_OPT ,  MSG_GDI_UTEXT_soft_OPT );
   }

   if (VALID_OPT(sp_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_sp_OPT_PE_NAME , MSG_GDI_UTEXT_sp_OPT_PE_NAME );
   }

   if (VALID_OPT(spl_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_spl_OPT , MSG_GDI_UTEXT_spl_OPT );
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(sprj_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_sprj_OPT_PROJECT, MSG_GDI_UTEXT_sprj_OPT_PROJECT);
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(sprjl_OPT, uti_state_get_mewho())) {
      PRINTITD( MSG_GDI_USAGE_sprjl_OPT, MSG_GDI_UTEXT_sprjl_OPT);
   }

   if (VALID_OPT(sq_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_sq_OPT_DESTIN_ID_LIST, 
         MSG_GDI_UTEXT_sq_OPT_DESTIN_ID_LIST);
      MARK(OA_DESTIN_ID_LIST);
   }

   if (VALID_OPT(sql_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_sql_OPT , MSG_GDI_UTEXT_sql_OPT );
   }

   if (VALID_OPT(ss_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_ss_OPT, MSG_GDI_UTEXT_ss_OPT );
   } 

   if (VALID_OPT(sss_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_sss_OPT , MSG_GDI_UTEXT_sss_OPT );
   } 

   if (VALID_OPT(ssconf_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_ssconf_OPT , MSG_GDI_UTEXT_ssconf_OPT );
   } 

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(sstnode_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_sstnode_OPT_NODE_LIST, 
         MSG_GDI_UTEXT_sstnode_OPT_NODE_LIST);
      MARK(OA_NODE_LIST);
      MARK(OA_NODE_PATH);
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(rsstnode_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_rsstnode_OPT_NODE_LIST, 
         MSG_GDI_UTEXT_rsstnode_OPT_NODE_LIST);
      MARK(OA_NODE_LIST);
      MARK(OA_NODE_PATH);
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(sstree_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_sstree_OPT, MSG_GDI_UTEXT_sstree_OPT);
   }


   if (VALID_OPT(su_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_su_OPT_LISTNAME_LIST, 
         MSG_GDI_UTEXT_su_OPT_LISTNAME_LIST);
      MARK(OA_LISTNAME_LIST);
   }
#ifndef __SGE_NO_USERMAPPING__
   if (VALID_OPT(sumap_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_sumap_OPT, MSG_GDI_UTEXT_sumap_OPT);
   }

   if (VALID_OPT(sumapl_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_sumapl_OPT, MSG_GDI_UTEXT_sumapl_OPT);
   }
#endif
   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(sus_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_suser_OPT_USER, MSG_GDI_UTEXT_suser_OPT_USER);
      MARK(OA_USER_LIST);
   }

   if (VALID_OPT(sul_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_sul_OPT, MSG_GDI_UTEXT_sul_OPT);
   }

   if (feature_is_enabled(FEATURE_SGEEE) && VALID_OPT(susl_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_suserl_OPT, MSG_GDI_UTEXT_suserl_OPT);
   }

   if (VALID_OPT(S_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_S_OPT_PATH_LIST, MSG_GDI_UTEXT_S_OPT_PATH_LIST);
      MARK(OA_PATH_LIST);
   }


   if (VALID_OPT(t_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_t_OPT_TASK_ID_RANGE, 
         MSG_GDI_UTEXT_t_OPT_TASK_ID_RANGE );
      MARK(OA_TASK_ID_RANGE);
   }

   if (VALID_OPT(tsm_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_tsm_OPT , MSG_GDI_UTEXT_tsm_OPT );
   }

   if (VALID_OPT(u_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_u_OPT_USERLISTORUALL, 
         MSG_GDI_UTEXT_u_OPT_USERLISTORUALL );
      PRINTITD("", MSG_GDI_UTEXT_ATTACH__u_OPT_USERLISTORUALL );
      MARK(OA_USER_LIST);
   }
 
   if (VALID_OPT(us_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_us_OPT, MSG_GDI_UTEXT_us_OPT );
   }

   if (VALID_OPT(v_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_v_OPT_VARIABLE_LIST, 
         MSG_GDI_UTEXT_v_OPT_VARIABLE_LIST);
      MARK(OA_VARIABLE_LIST);
   }

   if (VALID_OPT(verify_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_verify_OPT, MSG_GDI_UTEXT_verify_OPT );
   }

   if (VALID_OPT(V_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_V_OPT, MSG_GDI_UTEXT_V_OPT );
   }

   if (VALID_OPT(w_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_w_OPT_EWNV, MSG_GDI_UTEXT_w_OPT_EWNV );
   }

   if (VALID_OPT(AT_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_AT_OPT_FILE, MSG_GDI_UTEXT_AT_OPT_FILE );
   }

   if (VALID_OPT(DESTIN_OPR, uti_state_get_mewho())) {
      PRINTIT(get_argument_syntax(OA_DESTIN_ID_LIST)); /* ??? used in qmove */
   }

   if (VALID_OPT(JQ_DEST_OPR, uti_state_get_mewho())) {
      PRINTIT(MSG_GDI_USAGE_JQ_DEST_OPR );
      MARK(OA_JOB_QUEUE_DEST);
   }

   if (VALID_OPT(MESSAGE_OPR, uti_state_get_mewho())) {
      PRINTIT(MSG_GDI_USAGE_MESSAGE_OPR );
   }

   if (VALID_OPT(JOB_ID_OPR, uti_state_get_mewho())) {
      PRINTIT(MSG_GDI_USAGE_JOB_ID_OPR );
      MARK(OA_JOB_ID_LIST);
   }

   if (VALID_OPT(SCRIPT_OPR, uti_state_get_mewho())) {
      if (uti_state_get_mewho() != QALTER) {
         PRINTIT(MSG_GDI_USAGE_SCRIPT_OPR );
      }
      else {
         PRINTITD(MSG_GDI_USAGE_JOB_ID_OPR , MSG_GDI_UTEXT_JOB_ID_OPR );
         PRINTITD(MSG_GDI_USAGE_SCRIPT_OPR_ARGS , MSG_GDI_UTEXT_SCRIPT_OPR_ARGS );
      }
   }

   if (VALID_OPT(nostart_commd_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_nostart_commd_OPT, MSG_GDI_UTEXT_nostart_commd_OPT );
   }

   if (VALID_OPT(verbose_OPT, uti_state_get_mewho())) {
      PRINTITD(MSG_GDI_USAGE_verbose_OPT, MSG_GDI_UTEXT_verbose_OPT );
   }

   print_marked(fp);

   if (uti_state_get_mewho() == QSTAT) {
      fprintf(fp, "\n\n");
      fprintf(fp, "%s %s [options]\n",MSG_GDI_USAGE_USAGESTRING , uti_state_get_sge_formal_prog_name());
                    
   }

   fflush(fp);

   SGE_EXIT(1);
}

static void usage_silent(
FILE *fp 
) {
   DENTER(TOP_LAYER, "usage_silent");
   print_option_syntax(fp, "[-s]", MSG_GDI_USAGE_SILENT);
   DEXIT;
}
