/* 
 * File:   test_ff_cl.c
 * Author: dant
 *
 * Created on January 30, 2004, 1:22 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <ctype.h>

#include "sge_unistd.h"
#include "sge_gdi.h"
#include "setup.h"
#include "sig_handlers.h"
#include "commlib.h"
#include "sge_prog.h"
#include "msg_clients_common.h"
#include "msg_common.h"
#include "sge_answer.h"
#include "sge_mt_init.h"
#include "sge_log.h"
#include "cull/cull_multitype.h"
#include "cull/cull_list.h"
#include "rmon/sgermon.h"
#include "sgeobj/config.h"
#include "sgeobj/sge_all_listsL.h"
#include "sgeobj/sge_answer.h"
#include "sgeobj/sge_attrL.h"
#include "sgeobj/sge_calendarL.h"
#include "sgeobj/sge_centryL.h"
#include "sgeobj/sge_ckptL.h"
#include "sgeobj/sge_cqueueL.h"
#include "sgeobj/sge_hostL.h"
#include "sgeobj/sge_hrefL.h"
#include "sgeobj/sge_feature.h"
#include "sgeobj/sge_peL.h"
#include "sgeobj/sge_range.h"
#include "sgeobj/sge_strL.h"
#include "sgeobj/sge_sharetreeL.h"
#include "sgeobj/sge_subordinateL.h"
#include "sgeobj/sge_usageL.h"
#include "sgeobj/sge_userprj.h"
#include "sgeobj/sge_usersetL.h"
#include "spool/classic/read_write_pe.h"
#include "spool/classic/read_write_cal.h"
#include "spool/classic/read_write_centry.h"
#include "spool/classic/read_write_complex.h"
#include "spool/classic/read_write_ckpt.h"
#include "spool/classic/read_write_cqueue.h"
#include "spool/classic/read_write_host.h"
#include "spool/classic/read_write_host_group.h"
#include "spool/classic/read_write_qinstance.h"
#include "spool/classic/read_write_ume.h"
#include "spool/classic/read_write_userprj.h"
#include "spool/classic/read_write_userset.h"
#include "spool/classic/read_write_sharetree.h"
#include "spool/classic/rw_configuration.h"
#include "spool/classic/sched_conf.h"
#include "spool/sge_spooling_utilities.h"
#include "spool/flatfile/sge_flatfile.h"
#include "spool/flatfile/sge_flatfile_obj.h"
#include "spool/flatfile/sge_spooling_flatfile_scanner.h"
#include "uti/sge_prog.h"
#include "uti/sge_tmpnam.h"

#define allow_delete_time_modification

static void diff (const char *file1, const char *file2);
static void PE_test (void);
static void CAL_test (void);
static void CK_test (void);
static void STN_test (void);
static void CE_test (void);
static void CEL_test (void);
static void UP_test (void);
static void US_test (void);
static void EH_test (void);
static void CQ_test (void);
static void SC_test (void);
static void QU_test (void);
static void HGRP_test (void);
static void CU_test (void);
static void CONF_test (void);

const spool_flatfile_instr qconf_sub_name_value_space_sfi = 
{
   NULL,
   false,
   false,
   false,
   false,
   false,
   '\0',
   '=',
   ' ',
   '\0',
   '\0',
   &qconf_sub_name_value_space_sfi,
   { NoName, NoName, NoName }
};

const spool_flatfile_instr qconf_sfi = 
{
   NULL,
   true,
   false,
   false,
   true,
   false,
   ' ',
   '\n',
   '\0',
   '\0',
   '\0',
   &qconf_sub_name_value_space_sfi,
   { NoName, NoName, NoName }
};

const spool_flatfile_instr qconf_sub_param_sfi = 
{
   NULL,
   false,
   false,
   false,
   false,
   true,
   '\0',
   ' ',
   '\0',
   '\0',
   '\n',
   NULL,
   { NoName, NoName, NoName }
};

const spool_flatfile_instr qconf_sub_stn_sfi = 
{
   NULL,
   false,
   false,
   false,
   false,
   false,
   '\0',
   ',',
   '\0',
   '\0',
   '\0',
   NULL,
   {NoName, NoName, NoName}
};

const spool_flatfile_instr qconf_stn_sfi = 
{
   NULL,
   true,
   false,
   false,
   false,
   false,
   '=',
   '\n',
   ',',
   '\0',
   '\0',
   &qconf_sub_stn_sfi,
   {
      STN_children,
      STN_id,
      STN_version
   }
};

const spool_flatfile_instr qconf_ce_sfi = 
{
   NULL,
   false,
   true,
   true,
   false,
   true,
   '\0',
   ' ',
   '\0',
   '\0',
   '\n',
   NULL,
   { NoName, NoName, NoName }
};

const spool_flatfile_instr qconf_sub_up_sfi = 
{
   NULL,
   false,
   false,
   false,
   false,
   false,
   '\0',
   '\0',
   ',',
   '\0',
   '\0',
   &qconf_sub_name_value_space_sfi,
   { NoName, NoName, NoName }
};

const spool_flatfile_instr qconf_sub_ua_sfi = 
{
   NULL,
   true,
   false,
   false,
   false,
   false,
   '\0',
   '=',
   ',',
   '[',
   ']',
   NULL,
   { NoName, NoName, NoName }
};

const spool_flatfile_instr qconf_sub_upp_sfi = 
{
   NULL,
   true,
   false,
   false,
   false,
   false,
   '\0',
   '&',
   '\n',
   '{',
   '}',
   &qconf_sub_ua_sfi,
   { NoName, NoName, NoName }
};

const spool_flatfile_instr qconf_up_sfi = 
{
   NULL,
   true,
   false,
   false,
   false,
   false,
   ' ',
   '\n',
   '\0',
   '\0',
   '\0',
   &qconf_sub_up_sfi,
   { NoName, NoName, NoName }
};

const spool_flatfile_instr qconf_sub_name_value_comma_sfi = 
{
   NULL,
   false,
   false,
   false,
   false,
   false,
   '\0',
   '=',
   ',',
   '\0',
   '\0',
   NULL,
   { NoName, NoName, NoName }
};

const spool_flatfile_instr qconf_sub_name_value_comma_braced_sfi = 
{
   NULL,
   false,
   false,
   false,
   false,
   false,
   '\0',
   '=',
   ',',
   '[',
   ']',
   &qconf_sub_name_value_comma_sfi,
   { NoName, NoName, NoName }
};

const spool_flatfile_instr qconf_comma_sfi = 
{
   NULL,
   true,
   false,
   false,
   true,
   false,
   ' ',
   '\n',
   '\0',
   '\0',
   '\0',
   &qconf_sub_name_value_comma_sfi,
   { NoName, NoName, NoName }
};

const spool_flatfile_instr qconf_comma_braced_sfi = 
{
   NULL,
   true,
   false,
   false,
   true,
   false,
   ' ',
   '\n',
   '\0',
   '\0',
   '\0',
   &qconf_sub_name_value_comma_braced_sfi,
   { NoName, NoName, NoName }
};

/*
 * 
 */
int main(int argc, char** argv)
{
   DENTER_MAIN(TOP_LAYER, "test_ff_cl");   

   if (argc > 1) {
      int ret;

      while((ret = spool_lex())) {
         printf("line %3d: token %3d: %s\n", spool_line, ret, spool_text);
      }
   }
   else {
      lList *alp = NULL;
      int ret = 0;

      sge_mt_init();

      lInit(nmv);

      sge_gdi_param(SET_MEWHO, QCONF, NULL);
      if (sge_gdi_setup(prognames[QCONF], &alp)!=AE_OK) {
         answer_exit_if_not_recoverable(lFirst(alp));
         SGE_EXIT(1);
      }

      sge_setup_sig_handlers(QCONF);

   #ifdef ENABLE_NGC
      if ((ret = reresolve_me_qualified_hostname()) != CL_RETVAL_OK) {
         fprintf(stderr, cl_get_error_text(ret));
         SGE_EXIT(1);
      }
   #else
      if ((ret = reresolve_me_qualified_hostname()) != CL_OK) {
         SGE_ADD_MSG_ID(generate_commd_port_and_service_status_message(ret, SGE_EVENT));
         fprintf(stderr, SGE_EVENT);
         SGE_EXIT(1);
      }  
   #endif    

      PE_test ();
      CAL_test ();
      CK_test ();
      STN_test ();
      CE_test ();
      CEL_test ();
      UP_test ();
      US_test ();
      EH_test ();
      CQ_test ();
      SC_test ();
      HGRP_test ();
      CU_test ();
      CONF_test ();
   }

   DEXIT;
   return (EXIT_SUCCESS);
}

static void PE_test (void)
{
   lList *lp = NULL;
   lList *lp2 = NULL;
   lList *alp = NULL;
   lListElem *ep = lCreateElem (PE_Type);
   lListElem *ep2 = NULL;
   lListElem *ep3 = NULL;
   spooling_field *fields = NULL;
   const char *file1 = NULL;
   const char *file2 = NULL;
   
   /* Build a PE structure */
   
   lSetString (ep, PE_name, "Test_Name");
   lSetUlong (ep, PE_slots, 7);
   lSetString (ep, PE_start_proc_args, "start_args");
   lSetString (ep, PE_stop_proc_args, "stop_args");
   lSetString (ep, PE_allocation_rule, "allocation_rule");
   lSetBool (ep, PE_control_slaves, true);
   lSetBool (ep, PE_job_is_first_task, true);
   lSetString (ep, PE_urgency_slots, "urgency_slots");
   
   lp = lCreateList ("Resource Utilization List", RUE_Type);
   
   ep2 = lCreateElem (RUE_Type);
   lSetString (ep2, RUE_name, "Test_Name2");
   lSetDouble (ep2, RUE_utilized_now, 12.345);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (RUE_Type);
   lSetString (ep2, RUE_name, "Test_Name3");
   lSetDouble (ep2, RUE_utilized_now, 678.9);
   lAppendElem (lp, ep2);
   
   lSetList (ep, PE_resource_utilization, lp);
   
   lp = lCreateList ("User_List", US_Type);
   
   ep2 = lCreateElem (US_Type);   
   lSetString (ep2, US_name, "First_User_List");
   lSetUlong (ep2, US_type, 101);
   lSetUlong (ep2, US_fshare, 303);
   lSetUlong (ep2, US_oticket, 505);
   lSetUlong (ep2, US_job_cnt, 707);
   lSetUlong (ep2, US_pending_job_cnt, 909);
   
   lp2 = lCreateList ("User_Entry_List", UE_Type);
   
   ep3 = lCreateElem (UE_Type);
   lSetString (ep3, UE_name, "First_User");
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (UE_Type);
   lSetString (ep3, UE_name, "Second_User");
   lAppendElem (lp2, ep3);
   
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (US_Type);   
   lSetString (ep2, US_name, "Second_User_List");
   lSetUlong (ep2, US_type, 202);
   lSetUlong (ep2, US_fshare, 404);
   lSetUlong (ep2, US_oticket, 606);
   lSetUlong (ep2, US_job_cnt, 808);
   lSetUlong (ep2, US_pending_job_cnt, 0);
   
   lp2 = lCreateList ("User_Entry_List", UE_Type);
   
   ep3 = lCreateElem (UE_Type);
   lSetString (ep3, UE_name, "Third_User");
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (UE_Type);
   lSetString (ep3, UE_name, "Fourth_User");
   lAppendElem (lp2, ep3);
   
   lAppendElem (lp, ep2);
   
   lSetList (ep, PE_user_list, lp);
   
   lp = lCreateList ("XUser_List", US_Type);
   
   ep2 = lCreateElem (US_Type);   
   lSetString (ep2, US_name, "First_XUser_List");
   lSetUlong (ep2, US_type, 101);
   lSetUlong (ep2, US_fshare, 303);
   lSetUlong (ep2, US_oticket, 505);
   lSetUlong (ep2, US_job_cnt, 707);
   lSetUlong (ep2, US_pending_job_cnt, 909);
   
   lp2 = lCreateList ("XUser_Entry_List", UE_Type);
   
   ep3 = lCreateElem (UE_Type);
   lSetString (ep3, UE_name, "First_XUser");
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (UE_Type);
   lSetString (ep3, UE_name, "Second_XUser");
   lAppendElem (lp2, ep3);
   
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (US_Type);   
   lSetString (ep2, US_name, "Second_XUser_List");
   lSetUlong (ep2, US_type, 202);
   lSetUlong (ep2, US_fshare, 404);
   lSetUlong (ep2, US_oticket, 606);
   lSetUlong (ep2, US_job_cnt, 808);
   lSetUlong (ep2, US_pending_job_cnt, 0);
   
   lp2 = lCreateList ("XUser_Entry_List", UE_Type);
   
   ep3 = lCreateElem (UE_Type);
   lSetString (ep3, UE_name, "Third_XUser");
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (UE_Type);
   lSetString (ep3, UE_name, "Fourth_XUser");
   lAppendElem (lp2, ep3);
   
   lAppendElem (lp, ep2);
   
   lSetList (ep, PE_xuser_list, lp);

   ep2 = lCopyElem (ep);
   
   printf ("PE: spool = 0\n");
   /* Write a PE file using classic spooling */
   file1 = write_pe (0, 1, ep);
   
   /* Read a PE file using flatfile spooling */
   fields = sge_build_PE_field_list (false, false);
   ep = lFreeElem (ep);
   ep = spool_flatfile_read_object(&alp, PE_Type, NULL,
                                   fields, NULL, true, &qconf_sfi,
                                   SP_FORM_ASCII, NULL, file1);
   
   /* Write a PE file using flatfile spooling */
   file2 = spool_flatfile_write_object(&alp, ep, false,
                                       fields,
                                       &qconf_sfi,
                                       SP_DEST_TMP,
                                       SP_FORM_ASCII, 
                                       file2, false);
   
   ep = lFreeElem (ep);
   ep = cull_read_in_pe (NULL, file2, 0, 0, NULL, NULL);
   unlink (file2);
   FREE (file2);
   
   file2 = write_pe (0, 1, ep);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   FREE (file1);
   FREE (file2);
   FREE (fields);

   ep = lFreeElem (ep);
   ep = ep2;
   
   printf ("PE: spool = 1\n");
   /* Write a PE file using classic spooling */
   file1 = write_pe (1, 1, ep);
   
   /* Read a PE file using flatfile spooling */
   fields = sge_build_PE_field_list (true, false);
   ep = lFreeElem (ep);
   ep = spool_flatfile_read_object(&alp, PE_Type, NULL,
                                   fields, NULL, true, &qconf_sfi,
                                   SP_FORM_ASCII, NULL, file1);
   
   /* Write a PE file using flatfile spooling */
   file2 = spool_flatfile_write_object(&alp, ep, false,
                                       fields,
                                       &qconf_sfi,
                                       SP_DEST_TMP,
                                       SP_FORM_ASCII, 
                                       file2, true);
   
   ep = lFreeElem (ep);
   ep = cull_read_in_pe (NULL, file2, 0, 0, NULL, NULL);
   unlink (file2);
   FREE (file2);
   
   file2 = write_pe (1, 1, ep);
      
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   FREE (file1);
   FREE (file2);
   FREE (fields);
   
   ep = lFreeElem (ep);
   
   answer_list_output(&alp);   
   alp = lFreeList (alp);
   
   return;
}

static void CAL_test (void)
{
   lListElem *ep = NULL;
   lListElem *ep2 = NULL;
   lList *alp = NULL;
   const char *file1 = NULL, *file2 = NULL;
   
   ep = lCreateElem (CAL_Type);   
   lSetString (ep, CAL_name, "Test_Name");
   lSetString (ep, CAL_year_calendar, "2004");
   lSetString (ep, CAL_week_calendar, "KW 05");

   ep2 = lCopyElem (ep);
   
   printf ("CAL: spool = 0\n");
   /* Write a CAL file using classic spooling */
   file1 = write_cal (0, 1, ep);
   
   /* Read a CAL file using flatfile spooling */
   ep = lFreeElem (ep);
   ep = spool_flatfile_read_object(&alp, CAL_Type, NULL,
                                   CAL_fields, NULL, true, &qconf_sfi,
                                   SP_FORM_ASCII, NULL, file1);
   
   /* Write a CAL file using flatfile spooling */
   file2 = spool_flatfile_write_object(&alp, ep, false,
                                       CAL_fields,
                                       &qconf_sfi,
                                       SP_DEST_TMP,
                                       SP_FORM_ASCII, 
                                       file2, false);
   
   ep = lFreeElem (ep);
   ep = cull_read_in_cal (NULL, file2, 0, 0, NULL, NULL);
   unlink (file2);
   FREE (file2);
   
   file2 = write_cal (0, 1, ep);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   FREE (file1);
   FREE (file2);
   
   ep = lFreeElem (ep);
   ep = ep2;
   
   printf ("CAL: spool = 1\n");
   /* Write a CAL file using classic spooling */
   file1 = write_cal (1, 1, ep);
   
   /* Read a CAL file using flatfile spooling */
   ep = spool_flatfile_read_object(&alp, CAL_Type, NULL,
                                   CAL_fields, NULL, true, &qconf_sfi,
                                   SP_FORM_ASCII, NULL, file1);
   
   /* Write a CAL file using flatfile spooling */
   file2 = spool_flatfile_write_object(&alp, ep, false,
                                       CAL_fields,
                                       &qconf_sfi,
                                       SP_DEST_TMP,
                                       SP_FORM_ASCII, 
                                       file2, true);
   
   ep = lFreeElem (ep);
   ep = cull_read_in_cal (NULL, file2, 0, 0, NULL, NULL);
   unlink (file2);
   FREE (file2);
   
   file2 = write_cal (1, 1, ep);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   FREE (file1);
   FREE (file2);
   
   ep = lFreeElem (ep);
   
   answer_list_output(&alp);   
   alp = lFreeList (alp);
   
   return;
}

static void CK_test (void)
{
   lListElem *ep = NULL;
   lListElem *ep2 = NULL;
   lList *alp = NULL;
   const char *file1 = NULL, *file2 = NULL;
   
   ep = lCreateElem (CK_Type);
   lSetString (ep, CK_name, "Test_Name");
   lSetString (ep, CK_interface, "interface");
   lSetString (ep, CK_ckpt_command, "ckpt_command");
   lSetString (ep, CK_migr_command, "migr_command");
   lSetString (ep, CK_rest_command, "rest_command");
   lSetString (ep, CK_ckpt_dir, "ckpt_dir");
   lSetString (ep, CK_when, "when");
   lSetString (ep, CK_signal, "signal");
   lSetUlong (ep, CK_job_pid, 1313);
   lSetString (ep, CK_clean_command, "clean_command");
   
   ep2 = lCopyElem (ep);
   
   printf ("CK: spool = 0\n");
   /* Write a CK file using classic spooling */
   file1 = write_ckpt (0, 1, ep);
   
   /* Read a CK file using flatfile spooling */
   ep = lFreeElem (ep);
   ep = spool_flatfile_read_object(&alp, CK_Type, NULL,
                                   CK_fields, NULL, true, &qconf_sfi,
                                   SP_FORM_ASCII, NULL, file1);
      
   /* Write a CK file using flatfile spooling */
   file2 = spool_flatfile_write_object(&alp, ep, false,
                                       CK_fields,
                                       &qconf_sfi,
                                       SP_DEST_TMP,
                                       SP_FORM_ASCII, 
                                       file2, false);
   
   ep = lFreeElem (ep);
   ep = cull_read_in_ckpt (NULL, file2, 0, 0, NULL, NULL);
   unlink (file2);
   FREE (file2);
   
   file2 = write_ckpt (0, 1, ep);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   FREE (file1);
   FREE (file2);

   ep = lFreeElem (ep);
   ep = ep2;
   
   printf ("CK: spool = 1\n");
   /* Write a CK file using classic spooling */
   file1 = write_ckpt (1, 1, ep);
   
   /* Read a CK file using flatfile spooling */
   ep = lFreeElem (ep);
   ep = spool_flatfile_read_object(&alp, CK_Type, NULL,
                                   CK_fields, NULL, true, &qconf_sfi,
                                   SP_FORM_ASCII, NULL, file1);
   
   /* Write a CK file using flatfile spooling */
   file2 = spool_flatfile_write_object(&alp, ep, false,
                                       CK_fields,
                                       &qconf_sfi,
                                       SP_DEST_TMP,
                                       SP_FORM_ASCII, 
                                       file2, true);
   
   ep = lFreeElem (ep);
   ep = cull_read_in_ckpt (NULL, file2, 0, 0, NULL, NULL);
   unlink (file2);
   FREE (file2);
   
   file2 = write_ckpt (1, 1, ep);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   FREE (file1);
   FREE (file2);
   
   ep = lFreeElem (ep);
   
   answer_list_output(&alp);   
   alp = lFreeList (alp);
   
   return;
}

static void STN_test () {
   lListElem *ep = NULL;
   lListElem *ep2 = NULL;
   lListElem *ep3 = NULL;
   lList *lp = NULL;
   lList *lp2 = NULL;
   lList *alp = NULL;
   spooling_field *fields = NULL;
   const char *file1 = "/var/tmp/STN_cl";
   const char *file2 = NULL;
   char buffer[512];
   
   ep = lCreateElem (STN_Type);
   lSetString (ep, STN_name, "Test_Name");
   lSetUlong (ep, STN_type, 1);
   lSetUlong (ep, STN_id, 0);
   lSetUlong (ep, STN_shares, 99);
   lSetUlong (ep, STN_version, 1);
   
   lp = lCreateList ("Sharetree List", STN_Type);
   
   ep2 = lCreateElem (STN_Type);
   lSetString (ep2, STN_name, "Test_Name2");
   lSetUlong (ep2, STN_type, 1);
   lSetUlong (ep2, STN_id, 1);
   lSetUlong (ep2, STN_shares, 99);
   
   lp2 = lCreateList ("Sharetree List", STN_Type);
   
   ep3 = lCreateElem (STN_Type);
   lSetString (ep3, STN_name, "Test_Name3");
   lSetUlong (ep3, STN_type, 1);
   lSetUlong (ep3, STN_id, 2);
   lSetUlong (ep3, STN_shares, 99);
   lSetList (ep3, STN_children, NULL);
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (STN_Type);
   lSetString (ep3, STN_name, "Test_Name4");
   lSetUlong (ep3, STN_type, 1);
   lSetUlong (ep3, STN_id, 4);
   lSetUlong (ep3, STN_shares, 99);
   lSetList (ep3, STN_children, NULL);
   lAppendElem (lp2, ep3);
   
   lSetList (ep2, STN_children, lp2);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (STN_Type);
   lSetString (ep2, STN_name, "Test_Name5");
   lSetUlong (ep2, STN_type, 1);
   lSetUlong (ep2, STN_id, 5);
   lSetUlong (ep2, STN_shares, 99);
   
   lp2 = lCreateList ("Sharetree List", STN_Type);
   
   ep3 = lCreateElem (STN_Type);
   lSetString (ep3, STN_name, "Test_Name6");
   lSetUlong (ep3, STN_type, 1);
   lSetUlong (ep3, STN_id, 7);
   lSetUlong (ep3, STN_shares, 99);
   lSetList (ep3, STN_children, NULL);
   lAppendElem (lp2, ep3);
   
   lSetList (ep2, STN_children, lp2);
   lAppendElem (lp, ep2);
   
   lSetList (ep, STN_children, lp);

   ep2 = lCopyElem (ep);
   
   printf ("STN: spool = 0, recurse = 0, root_node = 0\n");
   /* Write a STN file using classic spooling */
   write_sharetree (&alp, ep, (char *)file1, NULL, 0, 0, 0);

   /* Read a STN file using flatfile spooling */
   fields = sge_build_STN_field_list (0, 0);
   ep = lFreeElem (ep);
   ep = spool_flatfile_read_object(&alp, STN_Type, NULL,
                                   fields, NULL, true, &qconf_stn_sfi,
                                   SP_FORM_ASCII, NULL, file1);
   
   /* Write a STN file using flatfile spooling */
   file2 = spool_flatfile_write_object(&alp, ep, false,
                                       fields,
                                       &qconf_stn_sfi,
                                       SP_DEST_TMP,
                                       SP_FORM_ASCII, 
                                       file2, false);

   ep = lFreeElem (ep);
   ep = read_sharetree ((char *)file2, NULL, 0, buffer, 0, NULL);
   unlink (file2);
   FREE (file2);
   
   file2 = "/var/tmp/STN_ff";
   write_sharetree (&alp, ep, (char *)file2, NULL, 0, 0, 0);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   FREE (fields);
   
   printf ("STN: spool = 0, recurse = 0, root_node = 1 -- INVALID\n");

   ep = lFreeElem (ep);
   ep = lCopyElem (ep2);
   
   printf ("STN: spool = 0, recurse = 1, root_node = 0\n");
   /* Write a STN file using classic spooling */
   write_sharetree (&alp, ep, (char *)file1, NULL, 0, 1, 0);
   
   /* Read a STN file using flatfile spooling */
   fields = sge_build_STN_field_list (0, 1);
   ep = lFreeElem (ep);
   ep = spool_flatfile_read_object(&alp, STN_Type, NULL,
                                   fields, NULL, true, &qconf_stn_sfi,
                                   SP_FORM_ASCII, NULL, file1);
   
   /* Write a STN file using flatfile spooling */
   file2 = spool_flatfile_write_object(&alp, ep, false,
                                       fields,
                                       &qconf_stn_sfi,
                                       SP_DEST_TMP,
                                       SP_FORM_ASCII, 
                                       file2, false);
   ep = lFreeElem (ep);
   ep = read_sharetree ((char *)file2, NULL, 0, buffer, 1, NULL);
   unlink (file2);
   FREE (file2);
   
   file2 = "/var/tmp/STN_ff";
   write_sharetree (&alp, ep, (char *)file2, NULL, 0, 1, 0);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   FREE (fields);

   ep = lFreeElem (ep);
   ep = lCopyElem (ep2);
   
   printf ("STN: spool = 0, recurse = 1, root_node = 1\n");
   /* Write a STN file using classic spooling */
   write_sharetree (&alp, ep, (char *)file1, NULL, 0, 1, 1);
   
   /* Read a STN file using flatfile spooling */
   fields = sge_build_STN_field_list (0, 1);
   ep = lFreeElem (ep);
   ep = spool_flatfile_read_object(&alp, STN_Type, NULL,
                                   fields, NULL, true, &qconf_stn_sfi,
                                   SP_FORM_ASCII, NULL, file1);
   
   /* Write a STN file using flatfile spooling */
   file2 = spool_flatfile_write_object(&alp, ep, true,
                                       fields,
                                       &qconf_stn_sfi,
                                       SP_DEST_TMP,
                                       SP_FORM_ASCII, 
                                       file2, false);

   ep = lFreeElem (ep);
   ep = read_sharetree ((char *)file2, NULL, 0, buffer, 1, NULL);
   unlink (file2);
   FREE (file2);
   
   file2 = "/var/tmp/STN_ff";
   write_sharetree (&alp, ep, (char *)file2, NULL, 0, 1, 1);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   FREE (fields);

   ep = lFreeElem (ep);
   ep = lCopyElem (ep2);
   
   printf ("STN: spool = 1, recurse = 0, root_node = 0\n");
   /* Write a STN file using classic spooling */
   write_sharetree (&alp, ep, (char *)file1, NULL, 1, 0, 0);
   
   /* Read a STN file using flatfile spooling */
   fields = sge_build_STN_field_list (1, 0);
   ep = lFreeElem (ep);
   ep = spool_flatfile_read_object(&alp, STN_Type, NULL,
                                   fields, NULL, true, &qconf_stn_sfi,
                                   SP_FORM_ASCII, NULL, file1);
   
   /* Write a STN file using flatfile spooling */
   file2 = spool_flatfile_write_object(&alp, ep, false,
                                       fields,
                                       &qconf_stn_sfi,
                                       SP_DEST_TMP,
                                       SP_FORM_ASCII, 
                                       file2, false);

   ep = lFreeElem (ep);
   ep = read_sharetree ((char *)file2, NULL, 1, buffer, 0, NULL);
   unlink (file2);
   FREE (file2);
   
   file2 = "/var/tmp/STN_ff";
   write_sharetree (&alp, ep, (char *)file2, NULL, 1, 0, 0);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   FREE (fields);
   
   printf ("STN: spool = 1, recurse = 0, root_node = 1 -- INVALID\n");

   ep = lFreeElem (ep);
   ep = lCopyElem (ep2);
   
   printf ("STN: spool = 1, recurse = 1, root_node = 0\n");
   /* Write a STN file using classic spooling */
   write_sharetree (&alp, ep, (char *)file1, NULL, 1, 1, 0);
   
   /* Read a STN file using flatfile spooling */
   fields = sge_build_STN_field_list (1, 1);
   ep = lFreeElem (ep);
   ep = spool_flatfile_read_object(&alp, STN_Type, NULL,
                                   fields, NULL, true, &qconf_stn_sfi,
                                   SP_FORM_ASCII, NULL, file1);
   
   /* Write a STN file using flatfile spooling */
   file2 = spool_flatfile_write_object(&alp, ep, false,
                                       fields,
                                       &qconf_stn_sfi,
                                       SP_DEST_TMP,
                                       SP_FORM_ASCII, 
                                       file2, false);

   ep = lFreeElem (ep);
   ep = read_sharetree ((char *)file2, NULL, 1, buffer, 1, NULL);
   unlink (file2);
   FREE (file2);
   
   file2 = "/var/tmp/STN_ff";
   write_sharetree (&alp, ep, (char *)file2, NULL, 1, 1, 0);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   FREE (fields);

   ep = lFreeElem (ep);
   ep = ep2;
   
   printf ("STN: spool = 1, recurse = 1, root_node = 1\n");
   /* Write a STN file using classic spooling */
   write_sharetree (&alp, ep, (char *)file1, NULL, 1, 1, 1);
   
   /* Read a STN file using flatfile spooling */
   fields = sge_build_STN_field_list (1, 1);
   ep = lFreeElem (ep);
   ep = spool_flatfile_read_object(&alp, STN_Type, NULL,
                                   fields, NULL, true, &qconf_stn_sfi,
                                   SP_FORM_ASCII, NULL, file1);
   
   /* Write a STN file using flatfile spooling */
   file2 = spool_flatfile_write_object(&alp, ep, true,
                                       fields,
                                       &qconf_stn_sfi,
                                       SP_DEST_TMP,
                                       SP_FORM_ASCII, 
                                       file2, true);

   ep = lFreeElem (ep);
   ep = read_sharetree ((char *)file2, NULL, 1, buffer, 1, NULL);
   unlink (file2);
   FREE (file2);
   
   file2 = "/var/tmp/STN_ff";
   write_sharetree (&alp, ep, (char *)file2, NULL, 1, 1, 1);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   FREE (fields);
   
   ep = lFreeElem (ep);
   
   answer_list_output(&alp);   
   alp = lFreeList (alp);
   
   return;
}

static void CE_test (void)
{
   lListElem *ep = NULL;
   lListElem *ep2 = NULL;
   lList *alp = NULL;
   const char *file1 = NULL, *file2 = NULL;
   
   ep = lCreateElem (CE_Type);
   lSetString (ep, CE_name, "Test_Name");
   lSetString (ep, CE_shortcut, "shortcut");
   lSetUlong (ep, CE_valtype, 1);
   lSetUlong (ep, CE_relop, 5);
   lSetBool (ep, CE_consumable, true);
   lSetString (ep, CE_default, "10");
   lSetUlong (ep, CE_requestable, REQU_NO);
   lSetString (ep, CE_urgency_weight, "20");
   
   ep2 = lCopyElem (ep);
   
   printf ("CE: spool = 0\n");
   /* Write a CE file using classic spooling */
   file1 = write_centry (0, 1, ep);
   
   /* Read a CE file using flatfile spooling */
   ep = lFreeElem (ep);
   ep = spool_flatfile_read_object(&alp, CE_Type, NULL,
                                   CE_fields, NULL, true, &qconf_sfi,
                                   SP_FORM_ASCII, NULL, file1);
         
   /* Write a CE file using flatfile spooling */
   file2 = spool_flatfile_write_object(&alp, ep, false,
                                       CE_fields,
                                       &qconf_sfi,
                                       SP_DEST_TMP,
                                       SP_FORM_ASCII, 
                                       file2, false);

   ep = lFreeElem (ep);
   ep = cull_read_in_centry (NULL, file2, 0, NULL, NULL);
   unlink (file2);
   FREE (file2);
   
   file2 = write_centry (0, 1, ep);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   FREE (file1);
   FREE (file2);
   
   ep = lFreeElem (ep);
   ep = ep2;
   
   printf ("CE: spool = 1\n");
   /* Write a CE file using classic spooling */
   file1 = write_centry (1, 1, ep);
   
   /* Read a CE file using flatfile spooling */
   ep = lFreeElem (ep);
   ep = spool_flatfile_read_object(&alp, CE_Type, NULL,
                                   CE_fields, NULL, true, &qconf_sfi,
                                   SP_FORM_ASCII, NULL, file1);
         
   /* Write a CE file using flatfile spooling */
   file2 = spool_flatfile_write_object(&alp, ep, false,
                                       CE_fields,
                                       &qconf_sfi,
                                       SP_DEST_TMP,
                                       SP_FORM_ASCII, 
                                       file2, true);

   ep = lFreeElem (ep);
   ep = cull_read_in_centry (NULL, file2, 1, NULL, NULL);
   unlink (file2);
   FREE (file2);
   
   file2 = write_centry (1, 1, ep);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   FREE (file1);
   FREE (file2);
   
   ep = lFreeElem (ep);
   
   answer_list_output(&alp);   
   alp = lFreeList (alp);
   
   return;
}

static void CEL_test (void)
{
   lListElem *ep = NULL;
   lList *lp = NULL;
   lList *lp2 = NULL;
   lList *alp = NULL;
   const char *file1 = "/var/tmp/CEL_ff";
   const char *file2 = NULL;
   
   lp = lCreateList ("Complex List", CE_Type);
   
   ep = lCreateElem (CE_Type);
   lSetString (ep, CE_name, "Test_Name");
   lSetString (ep, CE_shortcut, "shortcut");
   lSetUlong (ep, CE_valtype, 1);
   lSetUlong (ep, CE_relop, 5);
   lSetBool (ep, CE_consumable, true);
   lSetString (ep, CE_default, "15");
   lSetUlong (ep, CE_requestable, REQU_NO);
   lSetString (ep, CE_urgency_weight, "25");
   lAppendElem (lp, ep);
   
   ep = lCreateElem (CE_Type);
   lSetString (ep, CE_name, "Test_Name");
   lSetString (ep, CE_shortcut, "shortcut");
   lSetUlong (ep, CE_valtype, 1);
   lSetUlong (ep, CE_relop, 5);
   lSetBool (ep, CE_consumable, false);
   lSetString (ep, CE_default, "15");
   lSetUlong (ep, CE_requestable, REQU_YES);
   lSetString (ep, CE_urgency_weight, "25");
   lAppendElem (lp, ep);
   
   spool_flatfile_align_list(&alp, (const lList *)lp, CE_fields, 0);
   
   lp2 = lCopyList ("Complex List", lp);
   
   printf ("CEL: spool = 0\n");
   /* Write a CEL file using classic spooling */
   write_cmplx (0, (char *)file1, lp, NULL, &alp);
   
   /* Read a CE file using flatfile spooling */
   lp = lFreeList (lp);
   lp = spool_flatfile_read_list(&alp, CE_Type,
                                   CE_fields, NULL, true, &qconf_ce_sfi,
                                   SP_FORM_ASCII, NULL, file1);
   
   /* Write a CEL file using flatfile spooling */
   file2 = spool_flatfile_write_list(&alp, lp,
                                     CE_fields,
                                     &qconf_ce_sfi,
                                     SP_DEST_TMP,
                                     SP_FORM_ASCII, 
                                     file2, false);

   lp = lFreeList (lp);
   lp = read_cmplx (file2, NULL, NULL);
   unlink (file2);
   FREE (file2);
   
   file2 = "/var/tmp/CEL_cl";
   write_cmplx (0, (char *)file2, lp, NULL, &alp);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   
   lp = lFreeList (lp);
   lp = lp2;
   
   printf ("CEL: spool = 1\n");
   /* Write a CEL file using classic spooling */
   write_cmplx (1, (char *)file1, lp, NULL, &alp);
   
   /* Read a CE file using flatfile spooling */
   lp = lFreeList (lp);
   lp = spool_flatfile_read_list(&alp, CE_Type,
                                   CE_fields, NULL, true, &qconf_ce_sfi,
                                   SP_FORM_ASCII, NULL, file1);
   
   /* Write a CEL file using flatfile spooling */
   file2 = spool_flatfile_write_list(&alp, lp,
                                     CE_fields,
                                     &qconf_ce_sfi,
                                     SP_DEST_TMP,
                                     SP_FORM_ASCII, 
                                     file2, true);

   lp = lFreeList (lp);
   lp = read_cmplx (file2, NULL, NULL);
   unlink (file2);
   FREE (file2);
   
   file2 = "/var/tmp/CEL_cl";
   write_cmplx (1, (char *)file2, lp, NULL, &alp);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   
   lp = lFreeList (lp);
   
   answer_list_output(&alp);   
   alp = lFreeList (alp);
   
   return;
}

static void diff (const char *file1, const char *file2)
{
   char **argv = (char **)malloc (sizeof (char *) * 4);
   char *path = "/usr/bin/diff";
/*   printf ("diff %s %s\n", file1, file2);*/
   
   if (!fork ()) {
      argv[1] = path;
      argv[1] = (char *)file1;
      argv[2] = (char *)file2;
      argv[3] = NULL;
      
      execv (path, argv);
   }
   else {
      int stat_loc = 0;
      
      wait (&stat_loc);
   }
   
   FREE (argv);
}

static void UP_test (void) {
   lListElem *ep = NULL;
   lListElem *ep2 = NULL;
   lListElem *ep3 = NULL;
   lList *lp = NULL;
   lList *lp2 = NULL;
   lList *alp = NULL;
   spooling_field *fields = NULL;
   const char *file1 = "/var/tmp/UP_cl";
   const char *file2 = NULL;
   
   ep = lCreateElem (UP_Type);
   lSetString (ep, UP_name, "Test_Name");
   lSetUlong (ep, UP_oticket, 100);
   lSetUlong (ep, UP_fshare, 50);
   lSetUlong (ep, UP_delete_time, 123456789);
   lSetString (ep, UP_default_project, "default_project");
   lSetUlong (ep, UP_usage_time_stamp, 987654321);
   
   lp = lCreateList ("Usage List", UA_Type);
   
   ep2 = lCreateElem (UA_Type);
   lSetString (ep2, UA_name, "Test_Name2");
   lSetDouble (ep2, UA_value, 11.22);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (UA_Type);
   lSetString (ep2, UA_name, "Test_Name3");
   lSetDouble (ep2, UA_value, 33.44);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (UA_Type);
   lSetString (ep2, UA_name, "Test_Name4");
   lSetDouble (ep2, UA_value, 55.66);
   lAppendElem (lp, ep2);
      
   lSetList (ep, UP_usage, lp);
   
   lp = lCreateList ("LT Usage List", UA_Type);
   
   ep2 = lCreateElem (UA_Type);
   lSetString (ep2, UA_name, "Test_Name5");
   lSetDouble (ep2, UA_value, 11.22);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (UA_Type);
   lSetString (ep2, UA_name, "Test_Name6");
   lSetDouble (ep2, UA_value, 33.44);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (UA_Type);
   lSetString (ep2, UA_name, "Test_Name7");
   lSetDouble (ep2, UA_value, 55.66);
   lAppendElem (lp, ep2);
      
   lSetList (ep, UP_long_term_usage, lp);
   
   lp = lCreateList ("Project List", UPP_Type);
   
   ep2 = lCreateElem (UPP_Type);
   lSetString (ep2, UPP_name, "Test_Name8");
   
   lp2 = lCreateList ("Usage List", UA_Type);
   
   ep3 = lCreateElem (UA_Type);
   lSetString (ep3, UA_name, "Test_Name9");
   lSetDouble (ep3, UA_value, 11.22);
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (UA_Type);
   lSetString (ep3, UA_name, "Test_Name10");
   lSetDouble (ep3, UA_value, 33.44);
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (UA_Type);
   lSetString (ep3, UA_name, "Test_Name11");
   lSetDouble (ep3, UA_value, 55.66);
   lAppendElem (lp2, ep3);
      
   lSetList (ep2, UPP_usage, lp2);
   
   lp2 = lCreateList ("LT Usage List", UA_Type);
   
   ep3 = lCreateElem (UA_Type);
   lSetString (ep3, UA_name, "Test_Name12");
   lSetDouble (ep3, UA_value, 11.22);
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (UA_Type);
   lSetString (ep3, UA_name, "Test_Name13");
   lSetDouble (ep3, UA_value, 33.44);
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (UA_Type);
   lSetString (ep3, UA_name, "Test_Name14");
   lSetDouble (ep3, UA_value, 55.66);
   lAppendElem (lp2, ep3);
      
   lSetList (ep2, UPP_long_term_usage, lp2);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (UPP_Type);
   lSetString (ep2, UPP_name, "Test_Name15");
   
   lp2 = lCreateList ("Usage List", UA_Type);
   
   ep3 = lCreateElem (UA_Type);
   lSetString (ep3, UA_name, "Test_Name16");
   lSetDouble (ep3, UA_value, 11.22);
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (UA_Type);
   lSetString (ep3, UA_name, "Test_Name17");
   lSetDouble (ep3, UA_value, 33.44);
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (UA_Type);
   lSetString (ep3, UA_name, "Test_Name18");
   lSetDouble (ep3, UA_value, 55.66);
   lAppendElem (lp2, ep3);
      
   lSetList (ep2, UPP_usage, lp2);
   
   lp2 = lCreateList ("LT Usage List", UA_Type);
   
   ep3 = lCreateElem (UA_Type);
   lSetString (ep3, UA_name, "Test_Name19");
   lSetDouble (ep3, UA_value, 11.22);
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (UA_Type);
   lSetString (ep3, UA_name, "Test_Name20");
   lSetDouble (ep3, UA_value, 33.44);
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (UA_Type);
   lSetString (ep3, UA_name, "Test_Name21");
   lSetDouble (ep3, UA_value, 55.66);
   lAppendElem (lp2, ep3);
      
   lSetList (ep2, UPP_long_term_usage, lp2);
   lAppendElem (lp, ep2);
   
   lSetList (ep, UP_project, lp);
   
   lp = lCreateList ("ACL List", US_Type);
   
   ep2 = lCreateElem (US_Type);
   lSetString (ep2, US_name, "Test_Name22");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (US_Type);
   lSetString (ep2, US_name, "Test_Name23");
   lAppendElem (lp, ep2);
   
   lSetList (ep, UP_acl, lp);
   
   lp = lCreateList ("XACL List", US_Type);
   
   ep2 = lCreateElem (US_Type);
   lSetString (ep2, US_name, "Test_Name24");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (US_Type);
   lSetString (ep2, US_name, "Test_Name25");
   lAppendElem (lp, ep2);
   
   lSetList (ep, UP_xacl, lp);
   
   lp = lCreateList ("Debited Usage List", UPU_Type);
   
   ep2 = lCreateElem (UPU_Type);
   lSetUlong (ep2, UPU_job_number, 13579);
   
   lp2 = lCreateList ("Old Usage List", UA_Type);
   
   ep3 = lCreateElem (UA_Type);
   lSetString (ep3, UA_name, "Test_Name26");
   lSetDouble (ep3, UA_value, 11.22);
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (UA_Type);
   lSetString (ep3, UA_name, "Test_Name27");
   lSetDouble (ep3, UA_value, 33.44);
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (UA_Type);
   lSetString (ep3, UA_name, "Test_Name28");
   lSetDouble (ep3, UA_value, 55.66);
   lAppendElem (lp2, ep3);
      
   lSetList (ep2, UPU_old_usage_list, lp2);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (UPU_Type);
   lSetUlong (ep2, UPU_job_number, 97531);
   
   lp2 = lCreateList ("Old Usage List", UA_Type);
   
   ep3 = lCreateElem (UA_Type);
   lSetString (ep3, UA_name, "Test_Name29");
   lSetDouble (ep3, UA_value, 11.22);
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (UA_Type);
   lSetString (ep3, UA_name, "Test_Name30");
   lSetDouble (ep3, UA_value, 33.44);
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (UA_Type);
   lSetString (ep3, UA_name, "Test_Name31");
   lSetDouble (ep3, UA_value, 55.66);
   lAppendElem (lp2, ep3);
      
   lSetList (ep2, UPU_old_usage_list, lp2);
   lAppendElem (lp, ep2);
   
   lSetList (ep, UP_debited_job_usage, lp);

   ep2 = lCopyElem (ep);
   
   printf ("UP: spool = 0, user = 0\n");
   /* Write a UP file using classic spooling */
   write_userprj (&alp, ep, (char *)file1, NULL, 0, 0);
   
   /* Read a UP file using flatfile spooling */
   ep = lFreeElem (ep);
   fields = sge_build_UP_field_list (0, 0);
   ep = spool_flatfile_read_object(&alp, UP_Type, NULL,
                                   fields, NULL, true, &qconf_sfi,
                                   SP_FORM_ASCII, NULL, file1);
         
   /* Write a UP file using flatfile spooling */
   file2 = spool_flatfile_write_object(&alp, ep, false,
                                     fields,
                                     &qconf_up_sfi,
                                     SP_DEST_TMP,
                                     SP_FORM_ASCII, 
                                     file2, false);

   ep = lFreeElem (ep);
   ep = cull_read_in_userprj (NULL, file2, 0, 0, NULL);
   unlink (file2);
   FREE (file2);

   file2 = "/var/tmp/UP_ff";
   write_userprj (&alp, ep, (char *)file2, NULL, 0, 0);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   FREE (fields);
   
   ep = lFreeElem (ep);
   ep = ep2;
   
   printf ("UP: spool = 0, user = 1\n");
   /* Write a UP file using classic spooling */
   write_userprj (&alp, ep, (char *)file1, NULL, 0, 1);
   
   /* Read a UP file using flatfile spooling */
   ep = lFreeElem (ep);
   fields = sge_build_UP_field_list (0, 1);
   ep = spool_flatfile_read_object(&alp, UP_Type, NULL,
                                   fields, NULL, true, &qconf_sfi,
                                   SP_FORM_ASCII, NULL, file1);
         
   /* Write a UP file using flatfile spooling */
   file2 = spool_flatfile_write_object(&alp, ep, false,
                                     fields,
                                     &qconf_up_sfi,
                                     SP_DEST_TMP,
                                     SP_FORM_ASCII, 
                                     file2, false);

   ep = lFreeElem (ep);
   ep = cull_read_in_userprj (NULL, file2, 0, 1, NULL);
   unlink (file2);
   FREE (file2);

   file2 = "/var/tmp/UP_ff";
   write_userprj (&alp, ep, (char *)file2, NULL, 0, 1);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   FREE (fields);
   
   ep = lFreeElem (ep);
   
   printf ("UP: spool = 1, user = 0 -- INVALID\n");
   printf ("UP: spool = 1, user = 1 -- INVALID\n");
   
   answer_list_output(&alp);   
   alp = lFreeList (alp);
   
   return;
}

static void US_test(void)
{
   lListElem *ep = NULL;
   lListElem *ep2 = NULL;
   lList *lp = NULL;
   lList *alp = NULL;
   const char *file1 = "/var/tmp/US_cl";
   const char *file2 = NULL;
   
   ep = lCreateElem (US_Type);
   lSetString (ep, US_name, "Test_Name");
   lSetUlong (ep, US_type, 1);
   lSetUlong (ep, US_fshare, 10);
   lSetUlong (ep, US_oticket, 100);
   
   lp = lCreateList ("Entry List", UE_Type);
   
   ep2 = lCreateElem (UE_Type);
   lSetString (ep2, UE_name, "Test_Name2");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (UE_Type);
   lSetString (ep2, UE_name, "Test_Name3");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (UE_Type);
   lSetString (ep2, UE_name, "Test_Name4");
   lAppendElem (lp, ep2);
   
   lSetList (ep, US_entries, lp);
   
   printf ("US: NO ARGS\n");
   /* Write a CK file using classic spooling */
   write_userset (&alp, ep, (char *)file1, NULL, 0);
   
   /* Read a CK file using flatfile spooling */
   ep = lFreeElem (ep);
   ep = spool_flatfile_read_object(&alp, US_Type, NULL,
                                   US_fields, NULL, true, &qconf_sfi,
                                   SP_FORM_ASCII, NULL, file1);
      
   /* Write a CK file using flatfile spooling */
   file2 = spool_flatfile_write_object(&alp, ep, false,
                                       US_fields,
                                       &qconf_sfi,
                                       SP_DEST_TMP,
                                       SP_FORM_ASCII, 
                                       file2, false);
   
   ep = lFreeElem (ep);
   ep = cull_read_in_userset (NULL, file2, 0, 0, NULL);
   unlink (file2);
   FREE (file2);

   file2 = "/var/tmp/US_ff";
   write_userset (&alp, ep, (char *)file2, NULL, 0);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   
   ep = lFreeElem (ep);
   
   answer_list_output(&alp);   
   alp = lFreeList (alp);
   
   return;
}

static void EH_test (void)
{
   lListElem *ep = NULL;
   lListElem *ep2 = NULL;
   lList *lp = NULL;
   lList *alp = NULL;
   spooling_field *fields = NULL;
   const char *file1 = NULL;
   const char *file2 = NULL;
   
   ep = lCreateElem (EH_Type);
   lSetHost (ep, EH_name, "Test_Name");
   
   lp = lCreateList ("Load Scaling List", HS_Type);
   
   ep2 = lCreateElem (HS_Type);
   lSetString (ep2, HS_name, "Test_Name2");
   lSetDouble (ep2, HS_value, 1234.567);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (HS_Type);
   lSetString (ep2, HS_name, "Test_Name3");
   lSetDouble (ep2, HS_value, 6.7);
   lAppendElem (lp, ep2);
   
   lSetList (ep, EH_scaling_list, lp);
   
   lp = lCreateList ("Consumable Config List", CE_Type);
   
   ep2 = lCreateElem (CE_Type);
   lSetString (ep2, CE_name, "Test_Name4");
   lSetString (ep2, CE_shortcut, "shortcut");
   lSetUlong (ep2, CE_valtype, 1);
   lSetUlong (ep2, CE_relop, 5);
   lSetBool (ep2, CE_consumable, true);
   lSetString (ep2, CE_default, "15");
   lSetUlong (ep2, CE_requestable, REQU_NO);
   lSetString (ep2, CE_urgency_weight, "25");
   lSetString (ep2, CE_stringval, "stringval");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (CE_Type);
   lSetString (ep2, CE_name, "Test_Name5");
   lSetString (ep2, CE_shortcut, "shortcut");
   lSetUlong (ep2, CE_valtype, 1);
   lSetUlong (ep2, CE_relop, 5);
   lSetBool (ep2, CE_consumable, false);
   lSetString (ep2, CE_default, "15");
   lSetUlong (ep2, CE_requestable, REQU_YES);
   lSetString (ep2, CE_urgency_weight, "25");
   lSetDouble (ep2, CE_doubleval, 6969.9696);
   lAppendElem (lp, ep2);
   
   lSetList (ep, EH_consumable_config_list, lp);
   
   lp = lCreateList ("Actual Complex Values List", CE_Type);

   ep2 = lCreateElem (CE_Type);
   lSetString (ep2, CE_name, "Test_Name6");
   lSetString (ep2, CE_shortcut, "shortcut");
   lSetUlong (ep2, CE_valtype, 1);
   lSetUlong (ep2, CE_relop, 5);
   lSetBool (ep2, CE_consumable, true);
   lSetString (ep2, CE_default, "15");
   lSetUlong (ep2, CE_requestable, REQU_NO);
   lSetString (ep2, CE_urgency_weight, "25");
   lSetString (ep2, CE_stringval, "stringval");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (CE_Type);
   lSetString (ep2, CE_name, "Test_Name7");
   lSetString (ep2, CE_shortcut, "shortcut");
   lSetUlong (ep2, CE_valtype, 1);
   lSetUlong (ep2, CE_relop, 5);
   lSetBool (ep2, CE_consumable, false);
   lSetString (ep2, CE_default, "15");
   lSetUlong (ep2, CE_requestable, REQU_YES);
   lSetString (ep2, CE_urgency_weight, "25");
   lSetDouble (ep2, CE_doubleval, 6969.9696);
   lAppendElem (lp, ep2);

   lSetList (ep, EH_resource_utilization, lp);
   
   lp = lCreateList ("Load Values List", HL_Type);
   
   ep2 = lCreateElem (HL_Type);
   lSetString (ep2, HL_name, "Test_Name8");
   lSetString (ep2, HL_value, "1234.567");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (HL_Type);
   lSetString (ep2, HL_name, "Test_Name9");
   lSetString (ep2, HL_value, "6.7");
   lAppendElem (lp, ep2);
   
   lSetList (ep, EH_load_list, lp);

   lSetUlong (ep, EH_processors, 64);
   
   lp = lCreateList ("Resched Unknown List", RU_Type);
   
   ep2 = lCreateElem (RU_Type);
   lSetUlong (ep2, RU_job_number, 1);
   lSetUlong (ep2, RU_task_number, 1);
   lSetUlong (ep2, RU_state, 0);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (RU_Type);
   lSetUlong (ep2, RU_job_number, 1);
   lSetUlong (ep2, RU_task_number, 2);
   lSetUlong (ep2, RU_state, 1);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (RU_Type);
   lSetUlong (ep2, RU_job_number, 2);
   lSetUlong (ep2, RU_task_number, 1);
   lSetUlong (ep2, RU_state, 2);
   lAppendElem (lp, ep2);

   lSetList (ep, EH_reschedule_unknown_list, lp);
   
   lp = lCreateList ("User List", US_Type);
   
   ep2 = lCreateElem (US_Type);
   lSetString (ep2, US_name, "Test_Name9");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (US_Type);
   lSetString (ep2, US_name, "Test_Name10");
   lAppendElem (lp, ep2);
   
   lSetList (ep, EH_acl, lp);
   
   lp = lCreateList ("XUser List", US_Type);
   
   ep2 = lCreateElem (US_Type);
   lSetString (ep2, US_name, "Test_Name11");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (US_Type);
   lSetString (ep2, US_name, "Test_Name12");
   lAppendElem (lp, ep2);
   
   lSetList (ep, EH_xacl, lp);
   
   lp = lCreateList ("Projects List", UP_Type);
   
   ep2 = lCreateElem (UP_Type);
   lSetString (ep2, UP_name, "Test_Name13");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (UP_Type);
   lSetString (ep2, UP_name, "Test_Name14");
   lAppendElem (lp, ep2);
   
   lSetList (ep, EH_prj, lp);
   
   lp = lCreateList ("XProjects List", UP_Type);
   
   ep2 = lCreateElem (UP_Type);
   lSetString (ep2, UP_name, "Test_Name15");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (UP_Type);
   lSetString (ep2, UP_name, "Test_Name16");
   lAppendElem (lp, ep2);
   
   lSetList (ep, EH_xprj, lp);
   
   lp = lCreateList ("Usage Scaling List", HS_Type);
   
   ep2 = lCreateElem (HS_Type);
   lSetString (ep2, HS_name, "Test_Name17");
   lSetDouble (ep2, HS_value, 1234.567);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (HS_Type);
   lSetString (ep2, HS_name, "Test_Name18");
   lSetDouble (ep2, HS_value, 6.7);
   lAppendElem (lp, ep2);
   
   lSetList (ep, EH_usage_scaling_list, lp);
   
   lp = lCreateList ("Report Vars List", STU_Type);
   
   ep2 = lCreateElem (STU_Type);
   lSetString (ep2, STU_name, "Test_Name19");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (STU_Type);
   lSetString (ep2, STU_name, "Test_Name20");
   lAppendElem (lp, ep2);
   
   lSetList (ep, EH_report_variables, lp);

   printf ("EH: spool = 0\n");
   /* Write a EH file using classic spooling */
   file1 = write_host (0, 1, ep, EH_name, NULL);
   
   /* Read a EH file using flatfile spooling */
   ep = lFreeElem (ep);
   fields = sge_build_EH_field_list (false, false, false);
   ep = spool_flatfile_read_object(&alp, EH_Type, NULL,
                                   fields, NULL, true, &qconf_sfi,
                                   SP_FORM_ASCII, NULL, file1);
      
   /* Write a EH file using flatfile spooling */
   file2 = spool_flatfile_write_object(&alp, ep, false,
                                       fields,
                                       &qconf_sfi,
                                       SP_DEST_TMP,
                                       SP_FORM_ASCII, 
                                       file2, false);
   
   ep = lFreeElem (ep);
   ep = cull_read_in_host (NULL, file2, 0, EH_name, 0, NULL);
   unlink (file2);
   FREE (file2);

   file2 = write_host (0, 1, ep, EH_name, NULL);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);   
   FREE (file1);
   FREE (file2);
   FREE (fields);

   ep = lFreeElem (ep);
   
   printf ("EH: spool = 1 -- INVALID\n");
   
   answer_list_output(&alp);   
   alp = lFreeList (alp);
   
   return;
}

static void CQ_test (void) {
   lListElem *ep = NULL;
   lListElem *ep2 = NULL;
   lListElem *ep3 = NULL;
   lList *lp = NULL;
   lList *lp2 = NULL;
   lList *alp = NULL;
   const char *file1 = NULL, *file2 = NULL;
   
   ep = lCreateElem (CQ_Type);
   
   lSetString (ep, CQ_name, "Test_Name");
   
   lp = lCreateList ("Host List", HR_Type);
   
   ep2 = lCreateElem (HR_Type);
   lSetHost (ep2, HR_name, "Test_Name2");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (HR_Type);
   lSetHost (ep2, HR_name, "Test_Name3");
   lAppendElem (lp, ep2);
   
   lSetList (ep, CQ_hostlist, lp);
   
   lp = lCreateList ("Seq No. List", AULNG_Type);
   
   ep2 = lCreateElem (AULNG_Type);
   lSetHost (ep2, AULNG_href, "Test_Name4");
   lSetUlong (ep2, AULNG_value, 12);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AULNG_Type);
   lSetHost (ep2, AULNG_href, "Test_Name5");
   lSetUlong (ep2, AULNG_value, 16);
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_seq_no, lp);

   lp = lCreateList ("Load Thresholds", ACELIST_Type);
   
   ep2 = lCreateElem (ACELIST_Type);
   lSetHost (ep2, ACELIST_href, "Test_Name6");

   lp2 = lCreateList ("Complex List", CE_Type);
   
   ep3 = lCreateElem (CE_Type);
   lSetString (ep3, CE_name, "Test_Name7");
   lSetString (ep3, CE_stringval, "stringval");
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (CE_Type);
   lSetString (ep3, CE_name, "Test_Name8");
   lSetDouble (ep3, CE_doubleval, 3.14);
   lAppendElem (lp2, ep3);
   
   lSetList (ep2, ACELIST_value, lp2);
   
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ACELIST_Type);
   lSetHost (ep2, ACELIST_href, "Test_Name9");

   lp2 = lCreateList ("Complex List", CE_Type);
   
   ep3 = lCreateElem (CE_Type);
   lSetString (ep3, CE_name, "Test_Name10");
   lSetString (ep3, CE_stringval, "stringval");
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (CE_Type);
   lSetString (ep3, CE_name, "Test_Name11");
   lSetDouble (ep3, CE_doubleval, 3.14);
   lAppendElem (lp2, ep3);
   
   lSetList (ep2, ACELIST_value, lp2);
   
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_load_thresholds, lp);
   
   lp = lCreateList ("Suspend Thresholds", ACELIST_Type);
   
   ep2 = lCreateElem (ACELIST_Type);
   lSetHost (ep2, ACELIST_href, "Test_Name12");

   lp2 = lCreateList ("Complex List", CE_Type);
   
   ep3 = lCreateElem (CE_Type);
   lSetString (ep3, CE_name, "Test_Name13");
   lSetString (ep3, CE_stringval, "stringval");
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (CE_Type);
   lSetString (ep3, CE_name, "Test_Name14");
   lSetDouble (ep3, CE_doubleval, 3.14);
   lAppendElem (lp2, ep3);
   
   lSetList (ep2, CQ_load_thresholds, lp2);
   
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ACELIST_Type);
   lSetHost (ep2, ACELIST_href, "Test_Name15");

   lp2 = lCreateList ("Complex List", CE_Type);
   
   ep3 = lCreateElem (CE_Type);
   lSetString (ep3, CE_name, "Test_Name16");
   lSetString (ep3, CE_stringval, "stringval");
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (CE_Type);
   lSetString (ep3, CE_name, "Test_Name17");
   lSetDouble (ep3, CE_doubleval, 3.14);
   lAppendElem (lp2, ep3);
   
   lSetList (ep2, ACELIST_value, lp2);
   
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_suspend_thresholds, lp);
   
   lp = lCreateList ("NSuspend List", AULNG_Type);
   
   ep2 = lCreateElem (AULNG_Type);
   lSetHost (ep2, AULNG_href, "Test_Name18");
   lSetUlong (ep2, AULNG_value, 12);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AULNG_Type);
   lSetHost (ep2, AULNG_href, "Test_Name19");
   lSetUlong (ep2, AULNG_value, 16);
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_nsuspend, lp);

   lp = lCreateList ("Suspend Intervals", AINTER_Type);
   
   ep2 = lCreateElem (AINTER_Type);
   lSetHost (ep2, AINTER_href, "Test_Name20");
   lSetString (ep2, AINTER_value, "1");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AINTER_Type);
   lSetHost (ep2, AINTER_href, "Test_Name21");
   lSetString (ep2, AINTER_value, "2");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_suspend_interval, lp);

   lp = lCreateList ("Priority List", ASTR_Type);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name22");
   lSetString (ep2, ASTR_value, "3");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name23");
   lSetString (ep2, ASTR_value, "4");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_priority, lp);

   lp = lCreateList ("Min CPU Intervals", AINTER_Type);
   
   ep2 = lCreateElem (AINTER_Type);
   lSetHost (ep2, AINTER_href, "Test_Name24");
   lSetString (ep2, AINTER_value, "5");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AINTER_Type);
   lSetHost (ep2, AINTER_href, "Test_Name25");
   lSetString (ep2, AINTER_value, "6");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_min_cpu_interval, lp);

   lp = lCreateList ("Processor List", ASTR_Type);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name26");
   lSetString (ep2, ASTR_value, "7");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name27");
   lSetString (ep2, ASTR_value, "8");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_processors, lp);

   lp = lCreateList ("Q-Type List", AQTLIST_Type);
   
   ep2 = lCreateElem (AQTLIST_Type);
   lSetHost (ep2, AQTLIST_href, "Test_Name28");
   lSetUlong (ep2, AQTLIST_value, 12);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AQTLIST_Type);
   lSetHost (ep2, AQTLIST_href, "Test_Name29");
   lSetUlong (ep2, AQTLIST_value, 16);
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_qtype, lp);

   lp = lCreateList ("Checkpoint List", ASTRLIST_Type);
   
   ep2 = lCreateElem (ASTRLIST_Type);
   lSetHost (ep2, ASTRLIST_href, "Test_Name30");
   
   lp2 = lCreateList ("String List", ST_Type);
   
   ep3 = lCreateElem (ST_Type);
   lSetString (ep3, ST_name, "Test_Name31");
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (ST_Type);
   lSetString (ep3, ST_name, "Test_Name32");
   lAppendElem (lp2, ep3);
   
   lSetList (ep2, ASTRLIST_value, lp2);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ASTRLIST_Type);
   lSetHost (ep2, ASTRLIST_href, "Test_Name33");
   
   lp2 = lCreateList ("String List", ST_Type);
   
   ep3 = lCreateElem (ST_Type);
   lSetString (ep3, ST_name, "Test_Name34");
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (ST_Type);
   lSetString (ep3, ST_name, "Test_Name35");
   lAppendElem (lp2, ep3);
   
   lSetList (ep2, ASTRLIST_value, lp2);
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_ckpt_list, lp);

   lp = lCreateList ("PE List", ASTRLIST_Type);
   
   ep2 = lCreateElem (ASTRLIST_Type);
   lSetHost (ep2, ASTRLIST_href, "Test_Name36");
   
   lp2 = lCreateList ("String List", ST_Type);
   
   ep3 = lCreateElem (ST_Type);
   lSetString (ep3, ST_name, "Test_Name37");
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (ST_Type);
   lSetString (ep3, ST_name, "Test_Name38");
   lAppendElem (lp2, ep3);
   
   lSetList (ep2, ASTRLIST_value, lp2);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ASTRLIST_Type);
   lSetHost (ep2, ASTRLIST_href, "Test_Name39");
   
   lp2 = lCreateList ("String List", ST_Type);
   
   ep3 = lCreateElem (ST_Type);
   lSetString (ep3, ST_name, "Test_Name40");
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (ST_Type);
   lSetString (ep3, ST_name, "Test_Name41");
   lAppendElem (lp2, ep3);
   
   lSetList (ep2, ASTRLIST_value, lp2);
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_pe_list, lp);

   lp = lCreateList ("Rerun List", ABOOL_Type);
   
   ep2 = lCreateElem (ABOOL_Type);
   lSetHost (ep2, ABOOL_href, "Test_Name42");
   lSetBool (ep2, ABOOL_value, true);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ABOOL_Type);
   lSetHost (ep2, ABOOL_href, "Test_Name43");
   lSetBool (ep2, ABOOL_value, false);
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_rerun, lp);

   lp = lCreateList ("Job Slots", AULNG_Type);
   
   ep2 = lCreateElem (AULNG_Type);
   lSetHost (ep2, AULNG_href, "Test_Name44");
   lSetUlong (ep2, AULNG_value, 12);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AULNG_Type);
   lSetHost (ep2, AULNG_href, "Test_Name45");
   lSetUlong (ep2, AULNG_value, 16);
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_job_slots, lp);

   lp = lCreateList ("Tmp Dir List", ASTR_Type);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name46");
   lSetString (ep2, ASTR_value, "value");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name47");
   lSetString (ep2, ASTR_value, "value");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_tmpdir, lp);

   lp = lCreateList ("Shell List", ASTR_Type);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name48");
   lSetString (ep2, ASTR_value, "value");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name49");
   lSetString (ep2, ASTR_value, "value");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_shell, lp);

   lp = lCreateList ("Prolog List", ASTR_Type);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name50");
   lSetString (ep2, ASTR_value, "value");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name51");
   lSetString (ep2, ASTR_value, "value");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_prolog, lp);

   lp = lCreateList ("Epilog List", ASTR_Type);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name52");
   lSetString (ep2, ASTR_value, "value");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name53");
   lSetString (ep2, ASTR_value, "value");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_epilog, lp);

   lp = lCreateList ("Shell Start Modes", ASTR_Type);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name54");
   lSetString (ep2, ASTR_value, "value");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name55");
   lSetString (ep2, ASTR_value, "value");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_shell_start_mode, lp);

   lp = lCreateList ("Shell Start Methods", ASTR_Type);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name56");
   lSetString (ep2, ASTR_value, "value");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name57");
   lSetString (ep2, ASTR_value, "value");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_starter_method, lp);

   lp = lCreateList ("Shell Suspend Methods", ASTR_Type);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name58");
   lSetString (ep2, ASTR_value, "value");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name59");
   lSetString (ep2, ASTR_value, "value");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_suspend_method, lp);

   lp = lCreateList ("Shell Resume Methods", ASTR_Type);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name60");
   lSetString (ep2, ASTR_value, "value");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name61");
   lSetString (ep2, ASTR_value, "value");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_resume_method, lp);

   lp = lCreateList ("Shell Terminate Methods", ASTR_Type);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name62");
   lSetString (ep2, ASTR_value, "value");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name63");
   lSetString (ep2, ASTR_value, "value");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_terminate_method, lp);

   lp = lCreateList ("Notify List", AINTER_Type);
   
   ep2 = lCreateElem (AINTER_Type);
   lSetHost (ep2, AINTER_href, "Test_Name64");
   lSetString (ep2, AINTER_value, "46");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AINTER_Type);
   lSetHost (ep2, AINTER_href, "Test_Name65");
   lSetString (ep2, AINTER_value, "77");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_notify, lp);

   lp = lCreateList ("Owner List", AUSRLIST_Type);
   
   ep2 = lCreateElem (AUSRLIST_Type);
   lSetHost (ep2, AUSRLIST_href, "Test_Name66");
   
   lp2 = lCreateList ("Userset List", US_Type);
   
   ep3 = lCreateElem (US_Type);
   lSetString (ep3, US_name, "Test_Name67");
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (US_Type);
   lSetString (ep3, US_name, "Test_Name68");
   lAppendElem (lp2, ep3);
   
   lSetList (ep2, AUSRLIST_value, lp2);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AUSRLIST_Type);
   lSetHost (ep2, AUSRLIST_href, "Test_Name69");
   
   lp2 = lCreateList ("Userset List", US_Type);
   
   ep3 = lCreateElem (US_Type);
   lSetString (ep3, US_name, "Test_Name70");
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (US_Type);
   lSetString (ep3, US_name, "Test_Name71");
   lAppendElem (lp2, ep3);
   
   lSetList (ep2, AUSRLIST_value, lp2);
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_owner_list, lp);

   lp = lCreateList ("ACL List", AUSRLIST_Type);
   
   ep2 = lCreateElem (AUSRLIST_Type);
   lSetHost (ep2, AUSRLIST_href, "Test_Name72");
   
   lp2 = lCreateList ("Userset List", US_Type);
   
   ep3 = lCreateElem (US_Type);
   lSetString (ep3, US_name, "Test_Name73");
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (US_Type);
   lSetString (ep3, US_name, "Test_Name74");
   lAppendElem (lp2, ep3);
   
   lSetList (ep2, AUSRLIST_value, lp2);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AUSRLIST_Type);
   lSetHost (ep2, AUSRLIST_href, "Test_Name75");
   
   lp2 = lCreateList ("Userset List", US_Type);
   
   ep3 = lCreateElem (US_Type);
   lSetString (ep3, US_name, "Test_Name76");
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (US_Type);
   lSetString (ep3, US_name, "Test_Name77");
   lAppendElem (lp2, ep3);
   
   lSetList (ep2, AUSRLIST_value, lp2);
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_acl, lp);

   lp = lCreateList ("XUser List", AUSRLIST_Type);
   
   ep2 = lCreateElem (AUSRLIST_Type);
   lSetHost (ep2, AUSRLIST_href, "Test_Name78");
   
   lp2 = lCreateList ("Userset List", US_Type);
   
   ep3 = lCreateElem (US_Type);
   lSetString (ep3, US_name, "Test_Name79");
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (US_Type);
   lSetString (ep3, US_name, "Test_Name80");
   lAppendElem (lp2, ep3);
   
   lSetList (ep2, AUSRLIST_value, lp2);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AUSRLIST_Type);
   lSetHost (ep2, AUSRLIST_href, "Test_Name81");
   
   lp2 = lCreateList ("Userset List", US_Type);
   
   ep3 = lCreateElem (US_Type);
   lSetString (ep3, US_name, "Test_Name82");
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (US_Type);
   lSetString (ep3, US_name, "Test_Name83");
   lAppendElem (lp2, ep3);
   
   lSetList (ep2, AUSRLIST_value, lp2);
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_xacl, lp);

   lp = lCreateList ("Subordinate List", ASOLIST_Type);
   
   ep2 = lCreateElem (ASOLIST_Type);
   lSetHost (ep2, ASOLIST_href, "Test_Name84");

   lp2 = lCreateList ("Subordinate List", SO_Type);
   
   ep3 = lCreateElem (SO_Type);
   lSetString (ep3, SO_name, "Test_Name85");
   lSetUlong (ep3, SO_threshold, 13);
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (SO_Type);
   lSetString (ep3, SO_name, "Test_Name86");
   lSetUlong (ep3, SO_threshold, 16);
   lAppendElem (lp2, ep3);
   
   lSetList (ep2, ASOLIST_value, lp2);
   
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ASOLIST_Type);
   lSetHost (ep2, ASOLIST_href, "Test_Name87");

   lp2 = lCreateList ("Subordinate List", SO_Type);
   
   ep3 = lCreateElem (SO_Type);
   lSetString (ep3, SO_name, "Test_Name85");
   lSetUlong (ep3, SO_threshold, 13);
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (SO_Type);
   lSetString (ep3, SO_name, "Test_Name86");
   lSetUlong (ep3, SO_threshold, 16);
   lAppendElem (lp2, ep3);
   
   lSetList (ep2, ASOLIST_value, lp2);
   
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_subordinate_list, lp);
   
   lp = lCreateList ("Consumable Configs List", ACELIST_Type);
   
   ep2 = lCreateElem (ACELIST_Type);
   lSetHost (ep2, ACELIST_href, "Test_Name90");

   lp2 = lCreateList ("Complex List", CE_Type);
   
   ep3 = lCreateElem (CE_Type);
   lSetString (ep3, CE_name, "Test_Name91");
   lSetString (ep3, CE_stringval, "stringval");
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (CE_Type);
   lSetString (ep3, CE_name, "Test_Name92");
   lSetDouble (ep3, CE_doubleval, 3.14);
   lAppendElem (lp2, ep3);
   
   lSetList (ep2, CQ_load_thresholds, lp2);
   
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ACELIST_Type);
   lSetHost (ep2, ACELIST_href, "Test_Name93");

   lp2 = lCreateList ("Complex List", CE_Type);
   
   ep3 = lCreateElem (CE_Type);
   lSetString (ep3, CE_name, "Test_Name94");
   lSetString (ep3, CE_stringval, "stringval");
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (CE_Type);
   lSetString (ep3, CE_name, "Test_Name95");
   lSetDouble (ep3, CE_doubleval, 3.14);
   lAppendElem (lp2, ep3);
   
   lSetList (ep2, ACELIST_value, lp2);
   
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_consumable_config_list, lp);
   
   lp = lCreateList ("Projects List", APRJLIST_Type);
   
   ep2 = lCreateElem (APRJLIST_Type);
   lSetHost (ep2, APRJLIST_href, "Test_Name96");
   
   lp2 = lCreateList ("Project List", UP_Type);
   
   ep3 = lCreateElem (UP_Type);
   lSetString (ep3, UP_name, "Test_Name97");
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (UP_Type);
   lSetString (ep3, UP_name, "Test_Name98");
   lAppendElem (lp2, ep3);
   
   lSetList (ep2, APRJLIST_value, lp2);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (APRJLIST_Type);
   lSetHost (ep2, APRJLIST_href, "Test_Name99");
   
   lp2 = lCreateList ("Project List", UP_Type);
   
   ep3 = lCreateElem (UP_Type);
   lSetString (ep3, UP_name, "Test_Name100");
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (UP_Type);
   lSetString (ep3, UP_name, "Test_Name101");
   lAppendElem (lp2, ep3);
   
   lSetList (ep2, APRJLIST_value, lp2);
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_projects, lp);

   lp = lCreateList ("XProjects List", APRJLIST_Type);
   
   ep2 = lCreateElem (APRJLIST_Type);
   lSetHost (ep2, APRJLIST_href, "Test_Name102");
   
   lp2 = lCreateList ("Project List", UP_Type);
   
   ep3 = lCreateElem (UP_Type);
   lSetString (ep3, UP_name, "Test_Name103");
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (UP_Type);
   lSetString (ep3, UP_name, "Test_Name104");
   lAppendElem (lp2, ep3);
   
   lSetList (ep2, APRJLIST_value, lp2);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (APRJLIST_Type);
   lSetHost (ep2, APRJLIST_href, "Test_Name105");
   
   lp2 = lCreateList ("Project List", UP_Type);
   
   ep3 = lCreateElem (UP_Type);
   lSetString (ep3, UP_name, "Test_Name106");
   lAppendElem (lp2, ep3);
   
   ep3 = lCreateElem (UP_Type);
   lSetString (ep3, UP_name, "Test_Name107");
   lAppendElem (lp2, ep3);
   
   lSetList (ep2, APRJLIST_value, lp2);
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_xprojects, lp);

   lp = lCreateList ("Calendar List", ASTR_Type);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name108");
   lSetString (ep2, ASTR_value, "value");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name109");
   lSetString (ep2, ASTR_value, "value");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_calendar, lp);

   lp = lCreateList ("Initial State List", ASTR_Type);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name110");
   lSetString (ep2, ASTR_value, "value");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name111");
   lSetString (ep2, ASTR_value, "value");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_initial_state, lp);

   lp = lCreateList ("s_rt List", ATIME_Type);
   
   ep2 = lCreateElem (ATIME_Type);
   lSetHost (ep2, ATIME_href, "Test_Name112");
   lSetString (ep2, ATIME_value, "00:00:00");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ATIME_Type);
   lSetHost (ep2, ATIME_href, "Test_Name113");
   lSetString (ep2, ATIME_value, "23:59:59");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_s_rt, lp);

   lp = lCreateList ("h_rt List", ATIME_Type);
   
   ep2 = lCreateElem (ATIME_Type);
   lSetHost (ep2, ATIME_href, "Test_Name114");
   lSetString (ep2, ATIME_value, "00:00:00");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ATIME_Type);
   lSetHost (ep2, ATIME_href, "Test_Name115");
   lSetString (ep2, ATIME_value, "23:59:59");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_h_rt, lp);

   lp = lCreateList ("s_cpu List", ATIME_Type);
   
   ep2 = lCreateElem (ATIME_Type);
   lSetHost (ep2, ATIME_href, "Test_Name116");
   lSetString (ep2, ATIME_value, "00:00:00");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ATIME_Type);
   lSetHost (ep2, ATIME_href, "Test_Name117");
   lSetString (ep2, ATIME_value, "23:59:59");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_s_cpu, lp);

   lp = lCreateList ("h_cpu List", ATIME_Type);
   
   ep2 = lCreateElem (ATIME_Type);
   lSetHost (ep2, ATIME_href, "Test_Name118");
   lSetString (ep2, ATIME_value, "00:00:00");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ATIME_Type);
   lSetHost (ep2, ATIME_href, "Test_Name119");
   lSetString (ep2, ATIME_value, "23:59:59");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_h_cpu, lp);

   lp = lCreateList ("s_fsize List", AMEM_Type);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name120");
   lSetString (ep2, AMEM_value, "1048576");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name121");
   lSetString (ep2, AMEM_value, "1024");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_s_fsize, lp);

   lp = lCreateList ("h_fsize List", AMEM_Type);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name122");
   lSetString (ep2, AMEM_value, "1048576");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name123");
   lSetString (ep2, AMEM_value, "1024");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_h_fsize, lp);

   lp = lCreateList ("s_data List", AMEM_Type);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name124");
   lSetString (ep2, AMEM_value, "1048576");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name125");
   lSetString (ep2, AMEM_value, "1024");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_s_data, lp);

   lp = lCreateList ("h_data List", AMEM_Type);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name126");
   lSetString (ep2, AMEM_value, "1048576");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name127");
   lSetString (ep2, AMEM_value, "1024");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_h_data, lp);

   lp = lCreateList ("s_stack List", AMEM_Type);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name128");
   lSetString (ep2, AMEM_value, "1048576");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name129");
   lSetString (ep2, AMEM_value, "1024");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_s_stack, lp);

   lp = lCreateList ("h_stack List", AMEM_Type);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name130");
   lSetString (ep2, AMEM_value, "1048576");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name131");
   lSetString (ep2, AMEM_value, "1024");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_h_stack, lp);

   lp = lCreateList ("s_core List", AMEM_Type);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name132");
   lSetString (ep2, AMEM_value, "1048576");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name133");
   lSetString (ep2, AMEM_value, "1024");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_s_core, lp);

   lp = lCreateList ("h_core List", AMEM_Type);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name134");
   lSetString (ep2, AMEM_value, "1048576");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name135");
   lSetString (ep2, AMEM_value, "1024");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_h_core, lp);

   lp = lCreateList ("s_rss List", AMEM_Type);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name136");
   lSetString (ep2, AMEM_value, "1048576");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name137");
   lSetString (ep2, AMEM_value, "1024");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_s_rss, lp);

   lp = lCreateList ("h_rss List", AMEM_Type);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name138");
   lSetString (ep2, AMEM_value, "1048576");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name139");
   lSetString (ep2, AMEM_value, "1024");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_h_rss, lp);

   lp = lCreateList ("s_vmem List", AMEM_Type);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name140");
   lSetString (ep2, AMEM_value, "1048576");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name141");
   lSetString (ep2, AMEM_value, "1024");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_s_vmem, lp);

   lp = lCreateList ("h_vmem List", AMEM_Type);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name142");
   lSetString (ep2, AMEM_value, "1048576");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name143");
   lSetString (ep2, AMEM_value, "1024");
   lAppendElem (lp, ep2);

   lSetList (ep, CQ_h_vmem, lp);

   printf ("CQ: No Args\n");   
   /* Write a CQ file using classic spooling */
   file1 = write_cqueue (0, 1, ep);
   
   /* Read a CQ file using flatfile spooling */
   ep = lFreeElem (ep);
   ep = spool_flatfile_read_object(&alp, CQ_Type, NULL,
                                   CQ_fields, NULL, true, &qconf_sfi,
                                   SP_FORM_ASCII, NULL, file1);
   
   /* Write a CQ file using flatfile spooling */
   file2 = spool_flatfile_write_object(&alp, ep, false,
                                       CQ_fields,
                                       &qconf_sfi,
                                       SP_DEST_TMP,
                                       SP_FORM_ASCII, 
                                       file2, false);
   
   ep = lFreeElem (ep);
   ep = cull_read_in_cqueue (NULL, file2, 0, 0, NULL, NULL);
   unlink (file2);
   FREE (file2);
   
   file2 = write_cqueue (0, 1, ep);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   FREE (file1);
   FREE (file2);
   
   ep = lFreeElem (ep);
   
   answer_list_output(&alp);   
   alp = lFreeList (alp);
   
   return;
}

static void SC_test (void)
{
   lListElem *ep = NULL;
   lListElem *ep2 = NULL;
   lList *lp = NULL;
   lList *alp = NULL;
   const char *file1 = NULL, *file2 = NULL;
   
   ep = lCreateElem (SC_Type);
   lSetString (ep, SC_algorithm, "algorithm");
   lSetString (ep, SC_schedule_interval, "30");
   lSetUlong (ep, SC_maxujobs, 1024);
   lSetUlong (ep, SC_queue_sort_method, 1);

   lp = lCreateList ("Job Load Adjustments", CE_Type);

   ep2 = lCreateElem (CE_Type);
   lSetString (ep2, CE_name, "Test_Name1");
   lSetString (ep2, CE_shortcut, "shortcut");
   lSetUlong (ep2, CE_valtype, 1);
   lSetUlong (ep2, CE_relop, 5);
   lSetBool (ep2, CE_consumable, true);
   lSetString (ep2, CE_default, "15");
   lSetUlong (ep2, CE_requestable, REQU_NO);
   lSetString (ep2, CE_urgency_weight, "25");
   lSetString (ep2, CE_stringval, "stringval");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (CE_Type);
   lSetString (ep2, CE_name, "Test_Name2");
   lSetString (ep2, CE_shortcut, "shortcut");
   lSetUlong (ep2, CE_valtype, 1);
   lSetUlong (ep2, CE_relop, 5);
   lSetBool (ep2, CE_consumable, false);
   lSetString (ep2, CE_default, "15");
   lSetUlong (ep2, CE_requestable, REQU_YES);
   lSetString (ep2, CE_urgency_weight, "25");
   lSetString (ep2, CE_stringval, "stringval");
   lAppendElem (lp, ep2);

   lSetList (ep, SC_job_load_adjustments, lp);
   lSetString (ep, SC_load_adjustment_decay_time, "45");
   lSetString (ep, SC_load_formula, "load_formula");
   lSetString (ep, SC_schedd_job_info, "schedd_job_info");
   lSetUlong (ep, SC_flush_submit_sec, 128);
   lSetUlong (ep, SC_flush_finish_sec, 512);
   lSetString (ep, SC_params, "params");
   lSetString (ep, SC_reprioritize_interval, "15");
   lSetUlong (ep, SC_halftime, 32);
   
   lp = lCreateList ("Usage Weight List", UA_Type);
   
   ep2 = lCreateElem (UA_Type);
   lSetString (ep2, UA_name, "Test_Name3");
   lSetDouble (ep2, UA_value, 11.22);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (UA_Type);
   lSetString (ep2, UA_name, "Test_Name4");
   lSetDouble (ep2, UA_value, 33.44);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (UA_Type);
   lSetString (ep2, UA_name, "Test_Name5");
   lSetDouble (ep2, UA_value, 55.66);
   lAppendElem (lp, ep2);
      
   lSetList (ep, SC_usage_weight_list, lp);
   
   lSetDouble (ep, SC_compensation_factor, 11.22);
   lSetDouble (ep, SC_weight_user, 11.22);
   lSetDouble (ep, SC_weight_project, 33.44);
   lSetDouble (ep, SC_weight_department, 66.77);
   lSetDouble (ep, SC_weight_job, 88.99);
   lSetUlong (ep, SC_weight_tickets_functional, 36);
   lSetUlong (ep, SC_weight_tickets_share, 18);
   lSetBool (ep, SC_share_override_tickets, true);
   lSetBool (ep, SC_share_functional_shares, true);
   lSetUlong (ep, SC_max_functional_jobs_to_schedule, 2048);
   lSetBool (ep, SC_report_pjob_tickets, true);
   lSetUlong (ep, SC_max_pending_tasks_per_job, 256);
   lSetString (ep, SC_halflife_decay_list, "halflife_decay_list");
   lSetString (ep, SC_policy_hierarchy, "policy_hierarchy");
   lSetDouble (ep, SC_weight_ticket, 11.99);
   lSetDouble (ep, SC_weight_waiting_time, 22.88);
   lSetDouble (ep, SC_weight_deadline, 33.77);
   lSetDouble (ep, SC_weight_urgency, 44.66);
   

   printf ("SC: No Args\n");   
   /* Write a SC file using classic spooling */
   file1 = write_sched_configuration (0, 1, NULL, ep);
   
   /* Read a SC file using flatfile spooling */
   ep = lFreeElem (ep);
   ep = spool_flatfile_read_object(&alp, SC_Type, NULL,
                                   SC_fields, NULL, true, &qconf_comma_sfi,
                                   SP_FORM_ASCII, NULL, file1);
   
   /* Write a SC file using flatfile spooling */
   file2 = spool_flatfile_write_object(&alp, ep, false,
                                       SC_fields,
                                       &qconf_comma_sfi,
                                       SP_DEST_TMP,
                                       SP_FORM_ASCII, 
                                       file2, false);
   
   ep = lFreeElem (ep);
   lp = read_sched_configuration (NULL, file2, 0, &alp);
   ep = lFirst (lp);
   unlink (file2);
   FREE (file2);
   
   file2 = write_sched_configuration (0, 1, NULL, ep);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   FREE (file1);
   FREE (file2);
   
   lp = lFreeList (lp);
   
   answer_list_output(&alp);   
   alp = lFreeList (alp);
   
   return;
}

static void QU_test (void)
{
   lListElem *ep = NULL;
   lList * alp = NULL;
   spooling_field *fields = sge_build_QU_field_list (false, false);
   const char *file1 = NULL, *file2 = NULL;
   
   ep = lCreateElem (QU_Type);
   lSetHost (ep, QU_qhostname, "Test_Name");
   lSetString (ep, QU_qname, "Test_Name2");

   printf ("QU: No Args\n");   
   
   /* Write a QU file using classic spooling */
   file1 = write_qinstance (0, 1, ep);
   
   /* Read a QU file using flatfile spooling */
   ep = lFreeElem (ep);
   ep = spool_flatfile_read_object(&alp, QU_Type, NULL,
                                   fields, NULL, true, &qconf_sfi,
                                   SP_FORM_ASCII, NULL, file1);
   
   /* Write a QU file using flatfile spooling */
   file2 = spool_flatfile_write_object(&alp, ep, false,
                                       fields,
                                       &qconf_sfi,
                                       SP_DEST_TMP,
                                       SP_FORM_ASCII, 
                                       file2, false);
   
   ep = lFreeElem (ep);
   ep = cull_read_in_qinstance (NULL, file2, 0, 0, NULL, NULL);
   unlink (file2);
   FREE (file2);
   
   file2 = write_qinstance (0, 1, ep);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   FREE (file1);
   FREE (file2);
   FREE (fields);
   
   ep = lFreeElem (ep);
   
   answer_list_output(&alp);   
   alp = lFreeList (alp);
   
   return;
}

static void HGRP_test (void)
{
   lListElem *ep = NULL;
   lListElem *ep2 = NULL;
   lList * lp = NULL;
   lList * alp = NULL;
   const char *file1 = NULL, *file2 = NULL;
   
   ep = lCreateElem (HGRP_Type);
   lSetHost (ep, HGRP_name, "Test_Name");
   
   lp = lCreateList ("Host List", HR_Type);
   
   ep2 = lCreateElem (HR_Type);
   lSetHost (ep2, HR_name, "Test_Name2");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (HR_Type);
   lSetHost (ep2, HR_name, "Test_Name3");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (HR_Type);
   lSetHost (ep2, HR_name, "Test_Name4");
   lAppendElem (lp, ep2);
   
   lSetList (ep, HGRP_host_list, lp);
   
   printf ("HGRP: No Args\n");   
   
   /* Write a HGRP file using classic spooling */
   file1 = write_host_group (0, 1, ep);
   
   /* Read a HGRP file using flatfile spooling */
   ep = lFreeElem (ep);
   ep = spool_flatfile_read_object(&alp, HGRP_Type, NULL,
                                   HGRP_fields, NULL, true, &qconf_sfi,
                                   SP_FORM_ASCII, NULL, file1);
   
   /* Write a HGRP file using flatfile spooling */
   file2 = spool_flatfile_write_object(&alp, ep, false,
                                       HGRP_fields,
                                       &qconf_sfi,
                                       SP_DEST_TMP,
                                       SP_FORM_ASCII, 
                                       file2, false);
   
   ep = lFreeElem (ep);
   ep = cull_read_in_host_group (NULL, file2, 0, 0, NULL, NULL);
   unlink (file2);
   FREE (file2);
   
   file2 = write_host_group (0, 1, ep);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   FREE (file1);
   FREE (file2);
   
   ep = lFreeElem (ep);
   
   answer_list_output(&alp);   
   alp = lFreeList (alp);
   
   return;
}

static void CU_test(void)
{
   lListElem *ep = NULL;
   lListElem *ep2 = NULL;
   lList * lp = NULL;
   lList * alp = NULL;
   const char *file1 = NULL, *file2 = NULL;
   
   ep = lCreateElem (CU_Type);
   lSetString (ep, CU_name, "Test_Name");
   
   lp = lCreateList ("Remote User List", ASTR_Type);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name2");
   lSetString (ep2, ASTR_value, "value");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ASTR_Type);
   lSetHost (ep2, ASTR_href, "Test_Name3");
   lSetString (ep2, ASTR_value, "value");
   lAppendElem (lp, ep2);
   
   lSetList (ep, CU_ruser_list, lp);
   
   lp = lCreateList ("Ulong32 List", AULNG_Type);
   
   ep2 = lCreateElem (AULNG_Type);
   lSetHost (ep2, AULNG_href, "Test_Name4");
   lSetUlong (ep2, AULNG_value, 13);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AULNG_Type);
   lSetHost (ep2, AULNG_href, "Test_Name5");
   lSetUlong (ep2, AULNG_value, 14);
   lAppendElem (lp, ep2);
   
   lSetList (ep, CU_ulong32, lp);
   
   lp = lCreateList ("Boolean List", ABOOL_Type);
   
   ep2 = lCreateElem (ABOOL_Type);
   lSetHost (ep2, ABOOL_href, "Test_Name6");
   lSetBool (ep2, ABOOL_value, true);
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ABOOL_Type);
   lSetHost (ep2, ABOOL_href, "Test_Name7");
   lSetBool (ep2, ABOOL_value, false);
   lAppendElem (lp, ep2);
   
   lSetList (ep, CU_bool, lp);
   
   lp = lCreateList ("Time List", ATIME_Type);
   
   ep2 = lCreateElem (ATIME_Type);
   lSetHost (ep2, ATIME_href, "Test_Name8");
   lSetString (ep2, ATIME_value, "6");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (ATIME_Type);
   lSetHost (ep2, ATIME_href, "Test_Name9");
   lSetString (ep2, ATIME_value, "5");
   lAppendElem (lp, ep2);
   
   lSetList (ep, CU_time, lp);
   
   lp = lCreateList ("Memory List", AMEM_Type);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name10");
   lSetString (ep2, AMEM_value, "4");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AMEM_Type);
   lSetHost (ep2, AMEM_href, "Test_Name11");
   lSetString (ep2, AMEM_value, "3");
   lAppendElem (lp, ep2);
   
   lSetList (ep, CU_mem, lp);
   
   lp = lCreateList ("Inter List", AINTER_Type);
   
   ep2 = lCreateElem (AINTER_Type);
   lSetHost (ep2, AINTER_href, "Test_Name10");
   lSetString (ep2, AINTER_value, "2");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (AINTER_Type);
   lSetHost (ep2, AINTER_href, "Test_Name11");
   lSetString (ep2, AINTER_value, "1");
   lAppendElem (lp, ep2);
   
   lSetList (ep, CU_inter, lp);
   
   printf ("CU: No Args\n");   
   
   /* Write a CU file using classic spooling */
   file1 = write_ume (0, 1, ep);
   
   /* Read a CU file using flatfile spooling */
   ep = lFreeElem (ep);
   ep = spool_flatfile_read_object(&alp, CU_Type, NULL,
                                   CU_fields, NULL, true, &qconf_comma_braced_sfi,
                                   SP_FORM_ASCII, NULL, file1);
   
   /* Write a CU file using flatfile spooling */
   file2 = spool_flatfile_write_object(&alp, ep, false,
                                       CU_fields,
                                       &qconf_comma_braced_sfi,
                                       SP_DEST_TMP,
                                       SP_FORM_ASCII, 
                                       file2, false);
   
   ep = lFreeElem (ep);
   ep = cull_read_in_ume (NULL, file2, 0, 0, NULL, NULL);
   unlink (file2);
   FREE (file2);
   
   file2 = write_ume (0, 1, ep);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   FREE (file1);
   FREE (file2);
   
   ep = lFreeElem (ep);
   
   answer_list_output(&alp);   
   alp = lFreeList (alp);
   
   return;
}

static void CONF_test(void)
{
   lListElem *ep = NULL;
   lListElem *ep2 = NULL;
   lList * lp = NULL;
   lList * alp = NULL;
   spooling_field *fields = sge_build_CONF_field_list (false);
   const char *file1 = "/var/tmp/CONF_cl";
   const char *file2 = NULL;
   
   ep = lCreateElem (CONF_Type);
   lSetUlong (ep, CONF_version, 101);
   
   lp = lCreateList ("Config List", CF_Type);
   
   ep2 = lCreateElem (CF_Type);
   lSetString (ep2, CF_name, "gid_range");
   lSetString (ep2, CF_value, "1000-1100");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (CF_Type);
   lSetString (ep2, CF_name, "gid_range");
   lSetString (ep2, CF_value, "1001");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (CF_Type);
   lSetString (ep2, CF_name, "admin_user");
   lSetString (ep2, CF_value, "testuser");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (CF_Type);
   lSetString (ep2, CF_name, "user_lists");
   lSetString (ep2, CF_value, "me you someone_else");
   lAppendElem (lp, ep2);
   
   ep2 = lCreateElem (CF_Type);
   lSetString (ep2, CF_name, "login_shells");
   lSetString (ep2, CF_value, "sh,ksh,csh,tcsh");
   lAppendElem (lp, ep2);
   
   lSetList (ep, CONF_entries, lp);
   
   printf ("CONF: No Args\n");   
   
   /* Write a CU file using classic spooling */
   write_configuration (0, &alp, (char *)file1, ep, NULL, 0L);
   
   /* Read a CU file using flatfile spooling */
   ep = lFreeElem (ep);
   ep = spool_flatfile_read_object(&alp, CONF_Type, NULL,
                                   fields, NULL, false, &qconf_sfi,
                                   SP_FORM_ASCII, NULL, file1);
   
   /* Write a CU file using flatfile spooling */
   file2 = spool_flatfile_write_object(&alp, ep, false,
                                       fields,
                                       &qconf_sfi,
                                       SP_DEST_TMP,
                                       SP_FORM_ASCII, 
                                       file2, false);
   
   ep = lFreeElem (ep);
   ep = read_configuration (file2, "Test_Name", 0L);
   unlink (file2);
   FREE (file2);
   
   file2 = "/var/tmp/CONF_ff";
   write_configuration (0, &alp, (char *)file2, ep, NULL, 0L);
   
   diff (file1, file2);
   
   unlink (file1);
   unlink (file2);
   FREE (fields);
   
   ep = lFreeElem (ep);
   
   answer_list_output(&alp);   
   alp = lFreeList (alp);
   
   return;
}
