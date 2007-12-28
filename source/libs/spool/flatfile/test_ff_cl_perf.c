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
#include <strings.h>
#include <unistd.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>

#include "sge_unistd.h"
#include "gdi/sge_gdi.h"
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
#include "sgeobj/sge_resource_quotaL.h"
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
#include "spool/classic/read_write_resource_quota.h"
#include "spool/classic/rw_configuration.h"
#include "spool/classic/sched_conf.h"
#include "spool/sge_spooling_utilities.h"
#include "spool/flatfile/sge_flatfile.h"
#include "spool/flatfile/sge_flatfile_obj.h"
#include "spool/flatfile/sge_spooling_flatfile_scanner.h"
#include "uti/sge_prog.h"
#include "uti/sge_tmpnam.h"

#define LOOP 5000

struct rusage before;
struct rusage after;
double classic_time = 0;
double flatfile_time = 0;
int i = 0;

static int PE_test(void);
static int CAL_test(void);
static int EH_test(void);
static int CQ_test(void);
static int RQS_test(void);

#ifndef __SGE_NO_USERMAPPING__
const spool_flatfile_instr qconf_comma_braced_sfi = 
{
   NULL,
   true,
   false,
   false,
   true,
   false,
   false,
   true,
   ' ',
   '\n',
   '\0',
   '\0',
   '\0',
   &qconf_sub_name_value_comma_braced_sfi,
   { NoName, NoName, NoName }
};
#endif

typedef int (*func)(void);

/*
 * 
 */
int main(int argc, char** argv)
{
   int i = 0;

   const func test_array[] = {
                               RQS_test,
                               CQ_test,
                               EH_test,
                               CAL_test,
                               PE_test,
                               NULL };


   DENTER_MAIN(TOP_LAYER, "test_ff_cl_perf");   

   sge_mt_init();

   while (test_array[i] != NULL) {
      if (test_array[i]() != 0) {
         SGE_EXIT(NULL, 1);
      }
      i++;
   }
   
   DRETURN(EXIT_SUCCESS);
}

void print_result(const char *test, double value_classic, double value_flatfile) {
   if (value_flatfile< value_classic) {
      printf("%s: flatfile is %.2fs faster than classic (%.2f%%)\n", test, value_classic - value_flatfile, ((value_classic-value_flatfile)/value_flatfile)*100);

   } else {
      printf("%s: classic is %.2fs faster than flatfile (%.2f%%)\n", test, value_flatfile - value_classic, ((value_flatfile-value_classic)/value_classic)*100);
   }
}

static int PE_test(void)
{
   int ret = 0;
   lList *lp = NULL;
   lList *lp2 = NULL;
   lList *alp = NULL;
   lListElem *ep = lCreateElem(PE_Type);
   lListElem *ep2 = NULL;
   lListElem *ep3 = NULL;
   spooling_field *fields = NULL;
   const char *file1 = NULL;

   
   /* Build a PE structure */
   
   lSetString(ep, PE_name, "Test_Name");
   lSetUlong(ep, PE_slots, 7);
   lSetString(ep, PE_start_proc_args, "start_args");
   lSetString(ep, PE_stop_proc_args, "stop_args");
   lSetString(ep, PE_allocation_rule, "allocation_rule");
   lSetBool(ep, PE_control_slaves, true);
   lSetBool(ep, PE_job_is_first_task, true);
   lSetString(ep, PE_urgency_slots, "urgency_slots");
#ifdef SGE_PQS_API
   lSetString(ep, PE_qsort_args, "qsort_args");
#endif
   lSetBool(ep, PE_accounting_summary, true);
   
   lp = lCreateList("Resource Utilization List", RUE_Type);
   
   ep2 = lCreateElem(RUE_Type);
   lSetString(ep2, RUE_name, "Test_Name2");
   lSetDouble(ep2, RUE_utilized_now, 12.345);
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(RUE_Type);
   lSetString(ep2, RUE_name, "Test_Name3");
   lSetDouble(ep2, RUE_utilized_now, 678.9);
   lAppendElem(lp, ep2);
   
   lSetList(ep, PE_resource_utilization, lp);
   
   lp = lCreateList("User_List", US_Type);

   ep2 = lCreateElem(US_Type);   
   lSetString(ep2, US_name, "First_User_List");
   lSetUlong(ep2, US_type, 101);
   lSetUlong(ep2, US_fshare, 303);
   lSetUlong(ep2, US_oticket, 505);
   lSetUlong(ep2, US_job_cnt, 707);
   lSetUlong(ep2, US_pending_job_cnt, 909);
   
   lp2 = lCreateList("User_Entry_List", UE_Type);
   
   ep3 = lCreateElem(UE_Type);
   lSetString(ep3, UE_name, "First_User");
   lAppendElem(lp2, ep3);
   
   ep3 = lCreateElem(UE_Type);
   lSetString(ep3, UE_name, "Second_User");
   lAppendElem(lp2, ep3);

   lSetList(ep2, US_entries, lp2);
   
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(US_Type);   
   lSetString(ep2, US_name, "Second_User_List");
   lSetUlong(ep2, US_type, 202);
   lSetUlong(ep2, US_fshare, 404);
   lSetUlong(ep2, US_oticket, 606);
   lSetUlong(ep2, US_job_cnt, 808);
   lSetUlong(ep2, US_pending_job_cnt, 0);
   
   lp2 = lCreateList("User_Entry_List", UE_Type);
   
   ep3 = lCreateElem(UE_Type);
   lSetString(ep3, UE_name, "Third_User");
   lAppendElem(lp2, ep3);
   
   ep3 = lCreateElem(UE_Type);
   lSetString(ep3, UE_name, "Fourth_User");
   lAppendElem(lp2, ep3);

   lSetList(ep2, US_entries, lp2);
   
   lAppendElem(lp, ep2);
   
   lSetList(ep, PE_user_list, lp);
   
   lp = lCreateList("XUser_List", US_Type);
   
   ep2 = lCreateElem(US_Type);   
   lSetString(ep2, US_name, "First_XUser_List");
   lSetUlong(ep2, US_type, 101);
   lSetUlong(ep2, US_fshare, 303);
   lSetUlong(ep2, US_oticket, 505);
   lSetUlong(ep2, US_job_cnt, 707);
   lSetUlong(ep2, US_pending_job_cnt, 909);
   
   lp2 = lCreateList("XUser_Entry_List", UE_Type);
   
   ep3 = lCreateElem(UE_Type);
   lSetString(ep3, UE_name, "First_XUser");
   lAppendElem(lp2, ep3);
   
   ep3 = lCreateElem(UE_Type);
   lSetString(ep3, UE_name, "Second_XUser");
   lAppendElem(lp2, ep3);

   lSetList(ep2, US_entries, lp2);
   
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(US_Type);   
   lSetString(ep2, US_name, "Second_XUser_List");
   lSetUlong(ep2, US_type, 202);
   lSetUlong(ep2, US_fshare, 404);
   lSetUlong(ep2, US_oticket, 606);
   lSetUlong(ep2, US_job_cnt, 808);
   lSetUlong(ep2, US_pending_job_cnt, 0);
   
   lp2 = lCreateList("XUser_Entry_List", UE_Type);
   
   ep3 = lCreateElem(UE_Type);
   lSetString(ep3, UE_name, "Third_XUser");
   lAppendElem(lp2, ep3);
   
   ep3 = lCreateElem(UE_Type);
   lSetString(ep3, UE_name, "Fourth_XUser");
   lAppendElem(lp2, ep3);

   lSetList(ep2, US_entries, lp2);
   
   lAppendElem(lp, ep2);
   
   lSetList(ep, PE_xuser_list, lp);

   fields = sge_build_PE_field_list(false, false);

   printf("\nPE_test:\n");

   /* do classic writing */
   getrusage(RUSAGE_SELF, &before);
   for (i=0; i < LOOP; i++) {
      file1 = write_pe(0, 1, ep);
      unlink(file1);
      FREE(file1);
   }
   getrusage(RUSAGE_SELF, &after);
   classic_time = (after.ru_utime.tv_usec + after.ru_stime.tv_usec) - (before.ru_utime.tv_usec + before.ru_stime.tv_usec);
   classic_time = (after.ru_utime.tv_sec + after.ru_stime.tv_sec) - (before.ru_utime.tv_sec + before.ru_stime.tv_sec) + (classic_time/1000000);
   printf("classic writing took %.2fs\n", classic_time);

   /* do flatfile writing */
   getrusage(RUSAGE_SELF, &before);
   for (i=0; i < LOOP; i++) {
      char filename[SGE_PATH_MAX];
      sge_tmpnam(filename);
      file1 = spool_flatfile_write_object(&alp, ep, false,
                                       fields,
                                       &qconf_sfi,
                                       SP_DEST_SPOOL,
                                       SP_FORM_ASCII, 
                                       filename, false);
      unlink(filename);
      FREE(file1);
   }
   getrusage(RUSAGE_SELF, &after);
   flatfile_time = (after.ru_utime.tv_usec + after.ru_stime.tv_usec) - (before.ru_utime.tv_usec + before.ru_stime.tv_usec);
   flatfile_time = (after.ru_utime.tv_sec + after.ru_stime.tv_sec) - (before.ru_utime.tv_sec + before.ru_stime.tv_sec) + (flatfile_time/1000000);

   printf("write error:\n");
   answer_list_output(&alp);   
   printf("flatfile writing took %.2fs\n",flatfile_time);
   print_result("writing", classic_time, flatfile_time);

   /* do classic reading */
   file1 = write_pe(0, 1, ep);
   getrusage(RUSAGE_SELF, &before);
   for (i=0; i < LOOP; i++) {
      lFreeElem(&ep);
      ep = cull_read_in_pe(NULL, file1, 0, 0, NULL, NULL);
   }
   getrusage(RUSAGE_SELF, &after);
   unlink(file1);
   FREE(file1);
   classic_time = (after.ru_utime.tv_usec + after.ru_stime.tv_usec) - (before.ru_utime.tv_usec + before.ru_stime.tv_usec);
   classic_time = (after.ru_utime.tv_sec + after.ru_stime.tv_sec) - (before.ru_utime.tv_sec + before.ru_stime.tv_sec) + (classic_time/1000000);

   printf("classic reading took %.2fs\n", classic_time);

   /* do flatfile reading */
   file1 = spool_flatfile_write_object(&alp, ep, false,
                                    fields,
                                    &qconf_sfi,
                                    SP_DEST_TMP,
                                    SP_FORM_ASCII, 
                                    file1, false);
   getrusage(RUSAGE_SELF, &before);
   for (i=0; i < LOOP; i++) {
      lFreeElem(&ep);
      ep = spool_flatfile_read_object(&alp, PE_Type, NULL,
                                      fields, NULL, true, &qconf_sfi,
                                      SP_FORM_ASCII, NULL, file1);
   }
   getrusage(RUSAGE_SELF, &after);
   unlink(file1);
   FREE(file1);
   lFreeElem(&ep);
   flatfile_time = (after.ru_utime.tv_usec + after.ru_stime.tv_usec) - (before.ru_utime.tv_usec + before.ru_stime.tv_usec);
   flatfile_time = (after.ru_utime.tv_sec + after.ru_stime.tv_sec) - (before.ru_utime.tv_sec + before.ru_stime.tv_sec) + (flatfile_time/1000000);

   printf("read error:\n");
   answer_list_output(&alp);   
   printf("flatfile reading took %.2fs\n", flatfile_time);
   print_result("reading", classic_time, flatfile_time);

   printf("\n");


   FREE(fields);
   
   lFreeList(&alp);
   
   return ret;
}

static int CAL_test(void)
{
   int ret = 0;
   lListElem *ep = NULL;
   lList *alp = NULL;
   const char *file1 = NULL;
   
   ep = lCreateElem(CAL_Type);   
   lSetString(ep, CAL_name, "Test_Name");
   lSetString(ep, CAL_year_calendar, "2004");
   lSetString(ep, CAL_week_calendar, "KW 05");


   printf("\nCAL_test:\n");

   /* do classic writing */
   getrusage(RUSAGE_SELF, &before);
   for (i=0; i < LOOP; i++) {
      file1 = write_cal(0, 1, ep);
      unlink(file1);
      FREE(file1);
   }
   getrusage(RUSAGE_SELF, &after);
   classic_time = (after.ru_utime.tv_usec + after.ru_stime.tv_usec) - (before.ru_utime.tv_usec + before.ru_stime.tv_usec);
   classic_time = (after.ru_utime.tv_sec + after.ru_stime.tv_sec) - (before.ru_utime.tv_sec + before.ru_stime.tv_sec) + (classic_time/1000000);
   printf("classic writing took %.2fs\n", classic_time);

   /* do flatfile writing */
   getrusage(RUSAGE_SELF, &before);
   for (i=0; i < LOOP; i++) {
      char filename[SGE_PATH_MAX];
      sge_tmpnam(filename);
      file1 = spool_flatfile_write_object(&alp, ep, false,
                                          CAL_fields,
                                          &qconf_sfi,
                                          SP_DEST_SPOOL,
                                          SP_FORM_ASCII, 
                                          filename, false);
      unlink(filename);
      FREE(file1);
   }
   getrusage(RUSAGE_SELF, &after);
   flatfile_time = (after.ru_utime.tv_usec + after.ru_stime.tv_usec) - (before.ru_utime.tv_usec + before.ru_stime.tv_usec);
   flatfile_time = (after.ru_utime.tv_sec + after.ru_stime.tv_sec) - (before.ru_utime.tv_sec + before.ru_stime.tv_sec) + (flatfile_time/1000000);

   printf("write error:\n");
   answer_list_output(&alp);   
   printf("flatfile writing took %.2fs\n",flatfile_time);
   print_result("writing", classic_time, flatfile_time);
   
   lFreeElem(&ep);
   
   answer_list_output(&alp);   
   lFreeList(&alp);
   
   return ret;
}

static int EH_test(void)
{
   int ret = 0;
   lListElem *ep = NULL;
   lListElem *ep2 = NULL;
   lList *lp = NULL;
   lList *alp = NULL;
   spooling_field *fields = NULL;
   const char *file1 = NULL;
   
   ep = lCreateElem(EH_Type);
   lSetHost(ep, EH_name, "Test_Name");
   
   lp = lCreateList("Load Scaling List", HS_Type);
   
   ep2 = lCreateElem(HS_Type);
   lSetString(ep2, HS_name, "Test_Name2");
   lSetDouble(ep2, HS_value, 1234.567);
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(HS_Type);
   lSetString(ep2, HS_name, "Test_Name3");
   lSetDouble(ep2, HS_value, 6.7);
   lAppendElem(lp, ep2);
   
   lSetList(ep, EH_scaling_list, lp);
   
   lp = lCreateList("Consumable Config List", CE_Type);
   
   ep2 = lCreateElem(CE_Type);
   lSetString(ep2, CE_name, "Test_Name4");
   lSetString(ep2, CE_shortcut, "shortcut");
   lSetUlong(ep2, CE_valtype, 1);
   lSetUlong(ep2, CE_relop, 5);
   lSetBool(ep2, CE_consumable, true);
   lSetString(ep2, CE_default, "15");
   lSetUlong(ep2, CE_requestable, REQU_NO);
   lSetString(ep2, CE_urgency_weight, "25");
   lSetString(ep2, CE_stringval, "stringval");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(CE_Type);
   lSetString(ep2, CE_name, "Test_Name5");
   lSetString(ep2, CE_shortcut, "shortcut");
   lSetUlong(ep2, CE_valtype, 1);
   lSetUlong(ep2, CE_relop, 5);
   lSetBool(ep2, CE_consumable, false);
   lSetString(ep2, CE_default, "15");
   lSetUlong(ep2, CE_requestable, REQU_YES);
   lSetString(ep2, CE_urgency_weight, "25");
   lSetDouble(ep2, CE_doubleval, 6969.9696);
   lAppendElem(lp, ep2);
   
   lSetList(ep, EH_consumable_config_list, lp);
   
   lp = lCreateList("Actual Complex Values List", CE_Type);

   ep2 = lCreateElem(CE_Type);
   lSetString(ep2, CE_name, "Test_Name6");
   lSetString(ep2, CE_shortcut, "shortcut");
   lSetUlong(ep2, CE_valtype, 1);
   lSetUlong(ep2, CE_relop, 5);
   lSetBool(ep2, CE_consumable, true);
   lSetString(ep2, CE_default, "15");
   lSetUlong(ep2, CE_requestable, REQU_NO);
   lSetString(ep2, CE_urgency_weight, "25");
   lSetString(ep2, CE_stringval, "stringval");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(CE_Type);
   lSetString(ep2, CE_name, "Test_Name7");
   lSetString(ep2, CE_shortcut, "shortcut");
   lSetUlong(ep2, CE_valtype, 1);
   lSetUlong(ep2, CE_relop, 5);
   lSetBool(ep2, CE_consumable, false);
   lSetString(ep2, CE_default, "15");
   lSetUlong(ep2, CE_requestable, REQU_YES);
   lSetString(ep2, CE_urgency_weight, "25");
   lSetDouble(ep2, CE_doubleval, 6969.9696);
   lAppendElem(lp, ep2);

   lSetList(ep, EH_resource_utilization, lp);
   
   lp = lCreateList("Load Values List", HL_Type);
   
   ep2 = lCreateElem(HL_Type);
   lSetString(ep2, HL_name, "Test_Name8");
   lSetString(ep2, HL_value, "1234.567");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(HL_Type);
   lSetString(ep2, HL_name, "Test_Name9");
   lSetString(ep2, HL_value, "6.7");
   lAppendElem(lp, ep2);
   
   lSetList(ep, EH_load_list, lp);

   lSetUlong(ep, EH_processors, 64);
   
   lp = lCreateList("Resched Unknown List", RU_Type);
   
   ep2 = lCreateElem(RU_Type);
   lSetUlong(ep2, RU_job_number, 1);
   lSetUlong(ep2, RU_task_number, 1);
   lSetUlong(ep2, RU_state, 0);
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(RU_Type);
   lSetUlong(ep2, RU_job_number, 1);
   lSetUlong(ep2, RU_task_number, 2);
   lSetUlong(ep2, RU_state, 1);
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(RU_Type);
   lSetUlong(ep2, RU_job_number, 2);
   lSetUlong(ep2, RU_task_number, 1);
   lSetUlong(ep2, RU_state, 2);
   lAppendElem(lp, ep2);

   lSetList(ep, EH_reschedule_unknown_list, lp);
   
   lp = lCreateList("User List", US_Type);
   
   ep2 = lCreateElem(US_Type);
   lSetString(ep2, US_name, "Test_Name9");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(US_Type);
   lSetString(ep2, US_name, "Test_Name10");
   lAppendElem(lp, ep2);
   
   lSetList(ep, EH_acl, lp);
   
   lp = lCreateList("XUser List", US_Type);
   
   ep2 = lCreateElem(US_Type);
   lSetString(ep2, US_name, "Test_Name11");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(US_Type);
   lSetString(ep2, US_name, "Test_Name12");
   lAppendElem(lp, ep2);
   
   lSetList(ep, EH_xacl, lp);
   
   lp = lCreateList("Projects List", PR_Type);
   
   ep2 = lCreateElem(PR_Type);
   lSetString(ep2, PR_name, "Test_Name13");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(PR_Type);
   lSetString(ep2, PR_name, "Test_Name14");
   lAppendElem(lp, ep2);
   
   lSetList(ep, EH_prj, lp);
   
   lp = lCreateList("XProjects List", PR_Type);
   
   ep2 = lCreateElem(PR_Type);
   lSetString(ep2, PR_name, "Test_Name15");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(PR_Type);
   lSetString(ep2, PR_name, "Test_Name16");
   lAppendElem(lp, ep2);
   
   lSetList(ep, EH_xprj, lp);
   
   lp = lCreateList("Usage Scaling List", HS_Type);
   
   ep2 = lCreateElem(HS_Type);
   lSetString(ep2, HS_name, "Test_Name17");
   lSetDouble(ep2, HS_value, 1234.567);
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(HS_Type);
   lSetString(ep2, HS_name, "Test_Name18");
   lSetDouble(ep2, HS_value, 6.7);
   lAppendElem(lp, ep2);
   
   lSetList(ep, EH_usage_scaling_list, lp);
   
   lp = lCreateList("Report Vars List", STU_Type);
   
   ep2 = lCreateElem(STU_Type);
   lSetString(ep2, STU_name, "Test_Name19");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(STU_Type);
   lSetString(ep2, STU_name, "Test_Name20");
   lAppendElem(lp, ep2);
   
   lSetList(ep, EH_report_variables, lp);

   printf("\nEH_test:\n");

   fields = sge_build_EH_field_list(false, false, false);

   /* do classic writing */
   getrusage(RUSAGE_SELF, &before);
   for (i=0; i < LOOP; i++) {
      file1 = write_host(0, 1, ep, EH_name, NULL);
      unlink(file1);
      FREE(file1);
   }
   getrusage(RUSAGE_SELF, &after);
   classic_time = (after.ru_utime.tv_usec + after.ru_stime.tv_usec) - (before.ru_utime.tv_usec + before.ru_stime.tv_usec);
   classic_time = (after.ru_utime.tv_sec + after.ru_stime.tv_sec) - (before.ru_utime.tv_sec + before.ru_stime.tv_sec) + (classic_time/1000000);
   printf("classic writing took %.2fs\n", classic_time);

   /* do flatfile writing */
   getrusage(RUSAGE_SELF, &before);
   for (i=0; i < LOOP; i++) {
      char filename[SGE_PATH_MAX];
      sge_tmpnam(filename);
      file1 = spool_flatfile_write_object(&alp, ep, false,
                                          fields,
                                          &qconf_sfi,
                                          SP_DEST_SPOOL,
                                          SP_FORM_ASCII, 
                                          filename, false);
      unlink(filename);
      FREE(file1);
   }
   getrusage(RUSAGE_SELF, &after);
   flatfile_time = (after.ru_utime.tv_usec + after.ru_stime.tv_usec) - (before.ru_utime.tv_usec + before.ru_stime.tv_usec);
   flatfile_time = (after.ru_utime.tv_sec + after.ru_stime.tv_sec) - (before.ru_utime.tv_sec + before.ru_stime.tv_sec) + (flatfile_time/1000000);

   printf("write error:\n");
   answer_list_output(&alp);   
   printf("flatfile writing took %.2fs\n", flatfile_time);
   print_result("writing", classic_time, flatfile_time);

   return ret;
}

static int CQ_test(void) {
   int ret = 0;
   lListElem *ep = NULL;
   lListElem *ep2 = NULL;
   lListElem *ep3 = NULL;
   lList *lp = NULL;
   lList *lp2 = NULL;
   lList *alp = NULL;
   const char *file1 = NULL;

   ep = lCreateElem(CQ_Type);
   
   lSetString(ep, CQ_name, "Test_Name");
   
   lAddSubHost(ep, HR_name, "Test_Name2", CQ_hostlist, HR_Type);
   lAddSubHost(ep, HR_name, "Test_Name3", CQ_hostlist, HR_Type);

   lp = lCreateList("Seq No. List", AULNG_Type);
   
   ep2 = lCreateElem(AULNG_Type);
   lSetHost(ep2, AULNG_href, "Test_Name4");
   lSetUlong(ep2, AULNG_value, 12);
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(AULNG_Type);
   lSetHost(ep2, AULNG_href, "Test_Name5");
   lSetUlong(ep2, AULNG_value, 16);
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_seq_no, lp);

   lp = lCreateList("Load Thresholds", ACELIST_Type);
   
   ep2 = lCreateElem(ACELIST_Type);
   lSetHost(ep2, ACELIST_href, "Test_Name6");

   lp2 = lCreateList("Complex List", CE_Type);
   
   ep3 = lCreateElem(CE_Type);
   lSetString(ep3, CE_name, "Test_Name7");
   lSetString(ep3, CE_stringval, "stringval");
   lAppendElem(lp2, ep3);
   
   lSetList(ep2, ACELIST_value, lp2);
   
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_load_thresholds, lp);
   
   lp = lCreateList("Suspend Thresholds", ACELIST_Type);

   ep2 = lCreateElem(ACELIST_Type);
   lSetHost(ep2, ACELIST_href, "Test_Name15");

   lp2 = lCreateList("Complex_List2", CE_Type);
   
   ep3 = lCreateElem(CE_Type);
   lSetString(ep3, CE_name, "Test_Name16");
   lSetString(ep3, CE_stringval, "stringval");
   lAppendElem(lp2, ep3);

   lSetList(ep2, ACELIST_value, lp2);
   
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_suspend_thresholds, lp);
   
   lp = lCreateList("NSuspend List", AULNG_Type);
   
   ep2 = lCreateElem(AULNG_Type);
   lSetHost(ep2, AULNG_href, "Test_Name18");
   lSetUlong(ep2, AULNG_value, 12);
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(AULNG_Type);
   lSetHost(ep2, AULNG_href, "Test_Name19");
   lSetUlong(ep2, AULNG_value, 16);
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_nsuspend, lp);

   lp = lCreateList("Suspend Intervals", AINTER_Type);
   
   ep2 = lCreateElem(AINTER_Type);
   lSetHost(ep2, AINTER_href, "Test_Name20");
   lSetString(ep2, AINTER_value, "1");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(AINTER_Type);
   lSetHost(ep2, AINTER_href, "Test_Name21");
   lSetString(ep2, AINTER_value, "2");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_suspend_interval, lp);

   lp = lCreateList("Priority List", ASTR_Type);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name22");
   lSetString(ep2, ASTR_value, "3");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name23");
   lSetString(ep2, ASTR_value, "4");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_priority, lp);

   lp = lCreateList("Min CPU Intervals", AINTER_Type);
   
   ep2 = lCreateElem(AINTER_Type);
   lSetHost(ep2, AINTER_href, "Test_Name24");
   lSetString(ep2, AINTER_value, "5");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(AINTER_Type);
   lSetHost(ep2, AINTER_href, "Test_Name25");
   lSetString(ep2, AINTER_value, "6");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_min_cpu_interval, lp);

   lp = lCreateList("Processor List", ASTR_Type);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name26");
   lSetString(ep2, ASTR_value, "7");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name27");
   lSetString(ep2, ASTR_value, "8");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_processors, lp);

   lp = lCreateList("Q-Type List", AQTLIST_Type);
   
   ep2 = lCreateElem(AQTLIST_Type);
   lSetHost(ep2, AQTLIST_href, "Test_Name28");
   lSetUlong(ep2, AQTLIST_value, 12);
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(AQTLIST_Type);
   lSetHost(ep2, AQTLIST_href, "Test_Name29");
   lSetUlong(ep2, AQTLIST_value, 16);
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_qtype, lp);

   lp = lCreateList("Checkpoint List", ASTRLIST_Type);
   
   ep2 = lCreateElem(ASTRLIST_Type);
   lSetHost(ep2, ASTRLIST_href, "Test_Name30");
   
   lp2 = lCreateList("String List", ST_Type);
   
   ep3 = lCreateElem(ST_Type);
   lSetString(ep3, ST_name, "Test_Name31");
   lAppendElem(lp2, ep3);
   
   ep3 = lCreateElem(ST_Type);
   lSetString(ep3, ST_name, "Test_Name32");
   lAppendElem(lp2, ep3);
   
   lSetList(ep2, ASTRLIST_value, lp2);
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_ckpt_list, lp);

   lp = lCreateList("PE List", ASTRLIST_Type);
   
   ep2 = lCreateElem(ASTRLIST_Type);
   lSetHost(ep2, ASTRLIST_href, "Test_Name36");
   
   lp2 = lCreateList("String List", ST_Type);
   
   ep3 = lCreateElem(ST_Type);
   lSetString(ep3, ST_name, "Test_Name37");
   lAppendElem(lp2, ep3);

   ep3 = lCreateElem(ST_Type);
   lSetString(ep3, ST_name, "Test_Name38");
   lAppendElem(lp2, ep3);
   
   lSetList(ep2, ASTRLIST_value, lp2);
   lAppendElem(lp, ep2);
   
   lSetList(ep, CQ_pe_list, lp);

   lp = lCreateList("Rerun List", ABOOL_Type);
   
   ep2 = lCreateElem(ABOOL_Type);
   lSetHost(ep2, ABOOL_href, "Test_Name42");
   lSetBool(ep2, ABOOL_value, true);
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(ABOOL_Type);
   lSetHost(ep2, ABOOL_href, "Test_Name43");
   lSetBool(ep2, ABOOL_value, false);
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_rerun, lp);

   lp = lCreateList("Job Slots", AULNG_Type);
   
   ep2 = lCreateElem(AULNG_Type);
   lSetHost(ep2, AULNG_href, "Test_Name44");
   lSetUlong(ep2, AULNG_value, 12);
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(AULNG_Type);
   lSetHost(ep2, AULNG_href, "Test_Name45");
   lSetUlong(ep2, AULNG_value, 16);
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_job_slots, lp);

   lp = lCreateList("Tmp Dir List", ASTR_Type);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name46");
   lSetString(ep2, ASTR_value, "value");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name47");
   lSetString(ep2, ASTR_value, "value");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_tmpdir, lp);

   lp = lCreateList("Shell List", ASTR_Type);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name48");
   lSetString(ep2, ASTR_value, "value");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name49");
   lSetString(ep2, ASTR_value, "value");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_shell, lp);

   lp = lCreateList("Prolog List", ASTR_Type);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name50");
   lSetString(ep2, ASTR_value, "value");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name51");
   lSetString(ep2, ASTR_value, "value");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_prolog, lp);

   lp = lCreateList("Epilog List", ASTR_Type);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name52");
   lSetString(ep2, ASTR_value, "value");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name53");
   lSetString(ep2, ASTR_value, "value");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_epilog, lp);

   lp = lCreateList("Shell Start Modes", ASTR_Type);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name54");
   lSetString(ep2, ASTR_value, "value");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name55");
   lSetString(ep2, ASTR_value, "value");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_shell_start_mode, lp);

   lp = lCreateList("Shell Start Methods", ASTR_Type);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name56");
   lSetString(ep2, ASTR_value, "value");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name57");
   lSetString(ep2, ASTR_value, "value");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_starter_method, lp);

   lp = lCreateList("Shell Suspend Methods", ASTR_Type);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name58");
   lSetString(ep2, ASTR_value, "value");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name59");
   lSetString(ep2, ASTR_value, "value");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_suspend_method, lp);

   lp = lCreateList("Shell Resume Methods", ASTR_Type);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name60");
   lSetString(ep2, ASTR_value, "value");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name61");
   lSetString(ep2, ASTR_value, "value");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_resume_method, lp);

   lp = lCreateList("Shell Terminate Methods", ASTR_Type);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name62");
   lSetString(ep2, ASTR_value, "value");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name63");
   lSetString(ep2, ASTR_value, "value");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_terminate_method, lp);

   lp = lCreateList("Notify List", AINTER_Type);
   
   ep2 = lCreateElem(AINTER_Type);
   lSetHost(ep2, AINTER_href, "Test_Name64");
   lSetString(ep2, AINTER_value, "46");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(AINTER_Type);
   lSetHost(ep2, AINTER_href, "Test_Name65");
   lSetString(ep2, AINTER_value, "77");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_notify, lp);

   lp = lCreateList("Owner List", AUSRLIST_Type);
   
   ep2 = lCreateElem(AUSRLIST_Type);
   lSetHost(ep2, AUSRLIST_href, "Test_Name66");
   
   lp2 = lCreateList("Userset List", US_Type);
   
   ep3 = lCreateElem(US_Type);
   lSetString(ep3, US_name, "Test_Name67");
   lAppendElem(lp2, ep3);
   
   ep3 = lCreateElem(US_Type);
   lSetString(ep3, US_name, "Test_Name68");
   lAppendElem(lp2, ep3);
   
   lSetList(ep2, AUSRLIST_value, lp2);
   lAppendElem(lp, ep2);
 
   lSetList(ep, CQ_owner_list, lp);

   lp = lCreateList("ACL List", AUSRLIST_Type);
   
   ep2 = lCreateElem(AUSRLIST_Type);
   lSetHost(ep2, AUSRLIST_href, "Test_Name72");
   
   lp2 = lCreateList("Userset List", US_Type);
   
   ep3 = lCreateElem(US_Type);
   lSetString(ep3, US_name, "Test_Name73");
   lAppendElem(lp2, ep3);
   
   ep3 = lCreateElem(US_Type);
   lSetString(ep3, US_name, "Test_Name74");
   lAppendElem(lp2, ep3);
   
   lSetList(ep2, AUSRLIST_value, lp2);
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_acl, lp);

   lp = lCreateList("XUser List", AUSRLIST_Type);
   
   ep2 = lCreateElem(AUSRLIST_Type);
   lSetHost(ep2, AUSRLIST_href, "Test_Name78");
   
   lp2 = lCreateList("Userset List", US_Type);
   
   ep3 = lCreateElem(US_Type);
   lSetString(ep3, US_name, "Test_Name79");
   lAppendElem(lp2, ep3);
   
   ep3 = lCreateElem(US_Type);
   lSetString(ep3, US_name, "Test_Name80");
   lAppendElem(lp2, ep3);
   
   lSetList(ep2, AUSRLIST_value, lp2);
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_xacl, lp);

   lp = lCreateList("Subordinate List", ASOLIST_Type);
   
   ep2 = lCreateElem(ASOLIST_Type);
   lSetHost(ep2, ASOLIST_href, "Test_Name84");

   lp2 = lCreateList("Subordinate List", SO_Type);
   
   ep3 = lCreateElem(SO_Type);
   lSetString(ep3, SO_name, "Test_Name85");
   lSetUlong(ep3, SO_threshold, 13);
   lAppendElem(lp2, ep3);
   
   ep3 = lCreateElem(SO_Type);
   lSetString(ep3, SO_name, "Test_Name86");
   lSetUlong(ep3, SO_threshold, 16);
   lAppendElem(lp2, ep3);

   lSetList(ep2, ASOLIST_value, lp2);
   
   lAppendElem(lp, ep2);
   
   lSetList(ep, CQ_subordinate_list, lp);
  
   lp = lCreateList("Consumable Configs List", ACELIST_Type);
   
   ep2 = lCreateElem(ACELIST_Type);
   lSetHost(ep2, ACELIST_href, "Test_Name93");

   lp2 = lCreateList("Complex List", CE_Type);
   
   ep3 = lCreateElem(CE_Type);
   lSetString(ep3, CE_name, "Test_Name94");
   lSetString(ep3, CE_stringval, "stringval");
   lAppendElem(lp2, ep3);

   lSetList(ep2, ACELIST_value, lp2);
   
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_consumable_config_list, lp);
   
   lp = lCreateList("Projects List", APRJLIST_Type);
   
   ep2 = lCreateElem(APRJLIST_Type);
   lSetHost(ep2, APRJLIST_href, "Test_Name96");
   
   lp2 = lCreateList("Project List", PR_Type);
   
   ep3 = lCreateElem(PR_Type);
   lSetString(ep3, PR_name, "Test_Name97");
   lAppendElem(lp2, ep3);
   
   ep3 = lCreateElem(PR_Type);
   lSetString(ep3, PR_name, "Test_Name98");
   lAppendElem(lp2, ep3);
   
   lSetList(ep2, APRJLIST_value, lp2);
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_projects, lp);

   lp = lCreateList("XProjects List", APRJLIST_Type);
   
   ep2 = lCreateElem(APRJLIST_Type);
   lSetHost(ep2, APRJLIST_href, "Test_Name102");
   
   lp2 = lCreateList("Project List", PR_Type);
   
   ep3 = lCreateElem(PR_Type);
   lSetString(ep3, PR_name, "Test_Name103");
   lAppendElem(lp2, ep3);
   
   ep3 = lCreateElem(PR_Type);
   lSetString(ep3, PR_name, "Test_Name104");
   lAppendElem(lp2, ep3);
   
   lSetList(ep2, APRJLIST_value, lp2);
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_xprojects, lp);

   lp = lCreateList("Calendar List", ASTR_Type);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name108");
   lSetString(ep2, ASTR_value, "value");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name109");
   lSetString(ep2, ASTR_value, "value");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_calendar, lp);

   lp = lCreateList("Initial State List", ASTR_Type);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name110");
   lSetString(ep2, ASTR_value, "value");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(ASTR_Type);
   lSetHost(ep2, ASTR_href, "Test_Name111");
   lSetString(ep2, ASTR_value, "value");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_initial_state, lp);

   lp = lCreateList("s_rt List", ATIME_Type);
   
   ep2 = lCreateElem(ATIME_Type);
   lSetHost(ep2, ATIME_href, "Test_Name112");
   lSetString(ep2, ATIME_value, "00:00:00");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(ATIME_Type);
   lSetHost(ep2, ATIME_href, "Test_Name113");
   lSetString(ep2, ATIME_value, "23:59:59");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_s_rt, lp);

   lp = lCreateList("h_rt List", ATIME_Type);
   
   ep2 = lCreateElem(ATIME_Type);
   lSetHost(ep2, ATIME_href, "Test_Name114");
   lSetString(ep2, ATIME_value, "00:00:00");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(ATIME_Type);
   lSetHost(ep2, ATIME_href, "Test_Name115");
   lSetString(ep2, ATIME_value, "23:59:59");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_h_rt, lp);

   lp = lCreateList("s_cpu List", ATIME_Type);
   
   ep2 = lCreateElem(ATIME_Type);
   lSetHost(ep2, ATIME_href, "Test_Name116");
   lSetString(ep2, ATIME_value, "00:00:00");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(ATIME_Type);
   lSetHost(ep2, ATIME_href, "Test_Name117");
   lSetString(ep2, ATIME_value, "23:59:59");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_s_cpu, lp);

   lp = lCreateList("h_cpu List", ATIME_Type);
   
   ep2 = lCreateElem(ATIME_Type);
   lSetHost(ep2, ATIME_href, "Test_Name118");
   lSetString(ep2, ATIME_value, "00:00:00");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(ATIME_Type);
   lSetHost(ep2, ATIME_href, "Test_Name119");
   lSetString(ep2, ATIME_value, "23:59:59");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_h_cpu, lp);

   lp = lCreateList("s_fsize List", AMEM_Type);
   
   ep2 = lCreateElem(AMEM_Type);
   lSetHost(ep2, AMEM_href, "Test_Name120");
   lSetString(ep2, AMEM_value, "1048576");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(AMEM_Type);
   lSetHost(ep2, AMEM_href, "Test_Name121");
   lSetString(ep2, AMEM_value, "1024");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_s_fsize, lp);

   lp = lCreateList("h_fsize List", AMEM_Type);
   
   ep2 = lCreateElem(AMEM_Type);
   lSetHost(ep2, AMEM_href, "Test_Name122");
   lSetString(ep2, AMEM_value, "1048576");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(AMEM_Type);
   lSetHost(ep2, AMEM_href, "Test_Name123");
   lSetString(ep2, AMEM_value, "1024");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_h_fsize, lp);

   lp = lCreateList("s_data List", AMEM_Type);
   
   ep2 = lCreateElem(AMEM_Type);
   lSetHost(ep2, AMEM_href, "Test_Name124");
   lSetString(ep2, AMEM_value, "1048576");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(AMEM_Type);
   lSetHost(ep2, AMEM_href, "Test_Name125");
   lSetString(ep2, AMEM_value, "1024");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_s_data, lp);

   lp = lCreateList("h_data List", AMEM_Type);
   
   ep2 = lCreateElem(AMEM_Type);
   lSetHost(ep2, AMEM_href, "Test_Name126");
   lSetString(ep2, AMEM_value, "1048576");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(AMEM_Type);
   lSetHost(ep2, AMEM_href, "Test_Name127");
   lSetString(ep2, AMEM_value, "1024");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_h_data, lp);

   lp = lCreateList("s_stack List", AMEM_Type);
   
   ep2 = lCreateElem(AMEM_Type);
   lSetHost(ep2, AMEM_href, "Test_Name128");
   lSetString(ep2, AMEM_value, "1048576");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(AMEM_Type);
   lSetHost(ep2, AMEM_href, "Test_Name129");
   lSetString(ep2, AMEM_value, "1024");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_s_stack, lp);

   lp = lCreateList("h_stack List", AMEM_Type);
   
   ep2 = lCreateElem(AMEM_Type);
   lSetHost(ep2, AMEM_href, "Test_Name130");
   lSetString(ep2, AMEM_value, "1048576");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(AMEM_Type);
   lSetHost(ep2, AMEM_href, "Test_Name131");
   lSetString(ep2, AMEM_value, "1024");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_h_stack, lp);

   lp = lCreateList("s_core List", AMEM_Type);
   
   ep2 = lCreateElem(AMEM_Type);
   lSetHost(ep2, AMEM_href, "Test_Name132");
   lSetString(ep2, AMEM_value, "1048576");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(AMEM_Type);
   lSetHost(ep2, AMEM_href, "Test_Name133");
   lSetString(ep2, AMEM_value, "1024");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_s_core, lp);

   lp = lCreateList("h_core List", AMEM_Type);
   
   ep2 = lCreateElem(AMEM_Type);
   lSetHost(ep2, AMEM_href, "Test_Name134");
   lSetString(ep2, AMEM_value, "1048576");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(AMEM_Type);
   lSetHost(ep2, AMEM_href, "Test_Name135");
   lSetString(ep2, AMEM_value, "1024");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_h_core, lp);

   lp = lCreateList("s_rss List", AMEM_Type);
   
   ep2 = lCreateElem(AMEM_Type);
   lSetHost(ep2, AMEM_href, "Test_Name136");
   lSetString(ep2, AMEM_value, "1048576");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(AMEM_Type);
   lSetHost(ep2, AMEM_href, "Test_Name137");
   lSetString(ep2, AMEM_value, "1024");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_s_rss, lp);

   lp = lCreateList("h_rss List", AMEM_Type);
   
   ep2 = lCreateElem(AMEM_Type);
   lSetHost(ep2, AMEM_href, "Test_Name138");
   lSetString(ep2, AMEM_value, "1048576");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(AMEM_Type);
   lSetHost(ep2, AMEM_href, "Test_Name139");
   lSetString(ep2, AMEM_value, "1024");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_h_rss, lp);

   lp = lCreateList("s_vmem List", AMEM_Type);
   
   ep2 = lCreateElem(AMEM_Type);
   lSetHost(ep2, AMEM_href, "Test_Name140");
   lSetString(ep2, AMEM_value, "1048576");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(AMEM_Type);
   lSetHost(ep2, AMEM_href, "Test_Name141");
   lSetString(ep2, AMEM_value, "1024");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_s_vmem, lp);

   lp = lCreateList("h_vmem List", AMEM_Type);
   
   ep2 = lCreateElem(AMEM_Type);
   lSetHost(ep2, AMEM_href, "Test_Name142");
   lSetString(ep2, AMEM_value, "1048576");
   lAppendElem(lp, ep2);
   
   ep2 = lCreateElem(AMEM_Type);
   lSetHost(ep2, AMEM_href, "Test_Name143");
   lSetString(ep2, AMEM_value, "1024");
   lAppendElem(lp, ep2);

   lSetList(ep, CQ_h_vmem, lp);

   printf("\nCQ_test:\n");

   getrusage(RUSAGE_SELF, &before);
   for (i=0; i < LOOP; i++) {
      file1 = write_cqueue(0, 1, ep);
      unlink(file1);
      FREE(file1);
   }
   getrusage(RUSAGE_SELF, &after);
   classic_time = (after.ru_utime.tv_usec + after.ru_stime.tv_usec) - (before.ru_utime.tv_usec + before.ru_stime.tv_usec);
   classic_time = (after.ru_utime.tv_sec + after.ru_stime.tv_sec) - (before.ru_utime.tv_sec + before.ru_stime.tv_sec) + (classic_time/1000000);
   printf("classic writing took %.2fs\n", classic_time);

   /* do flatfile writing */
   getrusage(RUSAGE_SELF, &before);
   for (i=0; i < LOOP; i++) {
      char filename[SGE_PATH_MAX];
      sge_tmpnam(filename);
      file1 = spool_flatfile_write_object(&alp, ep, false,
                                       CQ_fields,
                                       &qconf_sfi,
                                       SP_DEST_SPOOL,
                                       SP_FORM_ASCII, 
                                       filename, false);
      unlink(filename);
      FREE(file1);
   }
   getrusage(RUSAGE_SELF, &after);
   flatfile_time = (after.ru_utime.tv_usec + after.ru_stime.tv_usec) - (before.ru_utime.tv_usec + before.ru_stime.tv_usec);
   flatfile_time = (after.ru_utime.tv_sec + after.ru_stime.tv_sec) - (before.ru_utime.tv_sec + before.ru_stime.tv_sec) + (flatfile_time/1000000);

   printf("write error:\n");
   answer_list_output(&alp);   
   printf("flatfile writing took %.2fs\n", flatfile_time);

   print_result("writing", classic_time, flatfile_time);
   
   unlink(file1);
   FREE(file1);
   
   lFreeElem(&ep);
   
   answer_list_output(&alp);   
   lFreeList(&alp);
   
   return ret;
}

static int RQS_test(void) {
   int ret = 0;
   lListElem *ep = NULL;
   lListElem *ep2 = NULL;
   lListElem *ep3 = NULL;
   lListElem *limit = NULL;
   lList *lp1 = NULL;
   lList *limit_list= NULL;
   lList *alp = NULL;
   spooling_field *fields = NULL;
   const char *file1 = "/var/tmp/RQS_cl";
   lList* rqs_list = lCreateList("test", RQS_Type);

   ep = lCreateElem(RQS_Type);
   lSetString(ep, RQS_name, "Test_Name1");
   lSetString(ep, RQS_description, "Test Description");
   lSetBool(ep, RQS_enabled, false);
   lp1 = lCreateList("Rule_List", RQR_Type);
   /* rule 1 */
   ep2 = lCreateElem(RQR_Type);
      ep3 = lCreateElem(RQRF_Type);
      lSetBool(ep3, RQRF_expand, true);
      lAddSubStr(ep3, ST_name, "Test_User1", RQRF_scope, ST_Type);

      lAddSubStr(ep3, ST_name, "Test_User2", RQRF_xscope, ST_Type);
      lAddSubStr(ep3, ST_name, "Test_User3", RQRF_xscope, ST_Type);
      lSetObject(ep2, RQR_filter_users, ep3);

      limit_list = lCreateList("Limit_List", RQRL_Type);
      limit = lCreateElem(RQRL_Type);
      lSetString(limit, RQRL_name, "slots");
      lSetString(limit, RQRL_value, "2*$num_proc");
      lAppendElem(limit_list, limit);
      lSetList(ep2, RQR_limit, limit_list);
   lAppendElem(lp1, ep2);
   /* rule 2 */
   ep2 = lCreateElem(RQR_Type);
      ep3 = lCreateElem(RQRF_Type);
      lSetBool(ep3, RQRF_expand, true);
      lAddSubStr(ep3, ST_name, "Test_Queue1", RQRF_scope, ST_Type);

      lAddSubStr(ep3, ST_name, "Test_Queue2", RQRF_xscope, ST_Type);
      lSetObject(ep2, RQR_filter_queues, ep3);

      limit_list = lCreateList("Limit_List", RQRL_Type);
      limit = lCreateElem(RQRL_Type);
      lSetString(limit, RQRL_name, "arch");
      lSetString(limit, RQRL_value, "lx24-amd64");
      lAppendElem(limit_list, limit);
      lSetList(ep2, RQR_limit, limit_list);
   lAppendElem(lp1, ep2);
   /* rule 3 */
   ep2 = lCreateElem(RQR_Type);
   lSetString(ep2, RQR_name, "rule3");
      ep3 = lCreateElem(RQRF_Type);
      lSetBool(ep3, RQRF_expand, true);
      lAddSubStr(ep3, ST_name, "Test_Pe1", RQRF_scope, ST_Type);

      lAddSubStr(ep3, ST_name, "Test_Pe2", RQRF_xscope, ST_Type);
      lSetObject(ep2, RQR_filter_pes, ep3);

      limit_list = lCreateList("Limit_List", RQRL_Type);
      limit = lCreateElem(RQRL_Type);
      lSetString(limit, RQRL_name, "mem");
      lSetString(limit, RQRL_value, "1G");
      lAppendElem(limit_list, limit);
      lSetList(ep2, RQR_limit, limit_list);
   lAppendElem(lp1, ep2);
   lSetList(ep, RQS_rule, lp1);
   lAppendElem(rqs_list, ep);

   /* rule 4 */
   ep = lCreateElem(RQS_Type);
   lSetString(ep, RQS_name, "Test_Name2");
   lSetString(ep, RQS_description, "Test Description");
   lSetBool(ep, RQS_enabled, true);
   lp1 = lCreateList("Rule_List", RQR_Type);
   ep2 = lCreateElem(RQR_Type);
      ep3 = lCreateElem(RQRF_Type);
      lSetBool(ep3, RQRF_expand, false);
      lAddSubStr(ep3, ST_name, "roland", RQRF_scope, ST_Type);

      lAddSubStr(ep3, ST_name, "andre", RQRF_xscope, ST_Type);
      lSetObject(ep2, RQR_filter_users, ep3);

      limit_list = lCreateList("Limit_List", RQRL_Type);
      /* first limit */
      limit = lCreateElem(RQRL_Type);
      lSetString(limit, RQRL_name, "mem");
      lSetString(limit, RQRL_value, "10G");
      lAppendElem(limit_list, limit);
      /* second limit */
      limit = lCreateElem(RQRL_Type);
      lSetString(limit, RQRL_name, "arch");
      lSetString(limit, RQRL_value, "sol-sparc64");
      lAppendElem(limit_list, limit);

      lSetList(ep2, RQR_limit, limit_list);
   lAppendElem(lp1, ep2);
   lSetList(ep, RQS_rule, lp1);
   lAppendElem(rqs_list, ep);

   fields = sge_build_RQS_field_list(false, false);

   printf("\nRQS_test:\n");
   /* do classic writing */
   getrusage(RUSAGE_SELF, &before);
   for (i=0; i < LOOP; i++) {
      file1 = write_rqs_list(0, 1, rqs_list);
      unlink(file1);
      FREE(file1);
   }
   getrusage(RUSAGE_SELF, &after);
   classic_time = (after.ru_utime.tv_usec + after.ru_stime.tv_usec) - (before.ru_utime.tv_usec + before.ru_stime.tv_usec);
   classic_time = (after.ru_utime.tv_sec + after.ru_stime.tv_sec) - (before.ru_utime.tv_sec + before.ru_stime.tv_sec) + (classic_time/1000000);
   printf("classic writing took %.2fs\n", classic_time);

   /* do flatfile writing */
   getrusage(RUSAGE_SELF, &before);
   for (i=0; i < LOOP; i++) {
      char filename[SGE_PATH_MAX];
      sge_tmpnam(filename);
/*       printf("filename: %s\n", filename); */
      file1 = spool_flatfile_write_list(&alp, rqs_list, fields, &qconf_rqs_sfi, SP_DEST_SPOOL, SP_FORM_ASCII, filename, false);
      unlink(filename);
      FREE(file1);
   }
   getrusage(RUSAGE_SELF, &after);
   flatfile_time = (after.ru_utime.tv_usec + after.ru_stime.tv_usec) - (before.ru_utime.tv_usec + before.ru_stime.tv_usec);
   flatfile_time = (after.ru_utime.tv_sec + after.ru_stime.tv_sec) - (before.ru_utime.tv_sec + before.ru_stime.tv_sec) + (flatfile_time/1000000);

   printf("write error:\n");
   answer_list_output(&alp);   
   printf("flatfile writing took %.2fs\n",flatfile_time);
   print_result("writing", classic_time, flatfile_time);

   lFreeList(&rqs_list);

   return ret;
}
