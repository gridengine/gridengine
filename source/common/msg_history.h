#ifndef __MSG_HISTORY_H
#define __MSG_HISTORY_H
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

#include "basis_types.h"

/* 
** history/qacct.c
*/ 
#define MSG_HISTORY_NOJOBSRUNNINGSINCESTARTUP      _MESSAGE(25066, _("no jobs running since startup\n"))
#define MSG_HISTORY_FAILEDRESOLVINGHOSTNAME_SS     _MESSAGE(25067, _("failed resolving hostname "SFQ": "SFN"\n"))
#define MSG_HISTORY_TOPTIONMASTHAVELISTOFTASKIDRANGES  _MESSAGE(25068, _("ERROR! -t option must have a list of task id ranges\n"))
#define MSG_HISTORY_INVALIDLISTOFTASKIDRANGES_S       _MESSAGE(25069, _("ERROR! invalid list of task id ranges: "SFN"\n"))
#define MSG_HISTORY_QMASTERISNOTALIVE                 _MESSAGE(25070, _("qmaster is not alive"))
#define MSG_HISTORY_ERRORUNABLETOOPENX_S     _MESSAGE(25074, _("error unable to open "SFN"\n"))
#define MSG_HISTORY_NOTENOUGTHMEMORYTOCREATELIST   _MESSAGE(25075, _("not enough memory to create list\n"))
#define MSG_HISTORY_IGNORINGINVALIDENTRYINLINEX_U  _MESSAGE(25076, _("ignoring invalid entry in line " U32CFormat "\n"))
#define MSG_HISTORY_IGNORINGJOBXFORACCOUNTINGMASTERQUEUEYNOTEXISTS_IS   _MESSAGE(25082, _("ignoring job %d for accounting: jobs master queue "SFN" does not longer exist\n"))
#define MSG_HISTORY_COMPLEXTEMPLATESCANTBEFILLEDCORRECTLYFORJOBX_D   _MESSAGE(25083, _("complex templates can't be filled correctly for job " U32CFormat"\n"))
#define MSG_HISTORY_JOBARRAYTASKSWXYZNOTFOUND_DDDD   _MESSAGE(25084, _("Job-array tasks "U32CFormat"."U32CFormat"-"U32CFormat":"U32CFormat" not found\n" ))
#define MSG_HISTORY_JOBIDXNOTFOUND_D          _MESSAGE(25085, _("job id " U32CFormat  " not found\n"  ))
#define MSG_HISTORY_JOBARRAYTASKSWXYZNOTFOUND_SDDD    _MESSAGE(25086, _("Job-array tasks "SFQ"."U32CFormat"-"U32CFormat":"U32CFormat" not found\n"))
#define MSG_HISTORY_JOBNAMEXNOTFOUND_S       _MESSAGE(25087, _("job name "SFN" not found\n"))
#define MSG_HISTORY_TOPTIONREQUIRESJOPTION   _MESSAGE(25088, _("-t option requires -j option\n"))
#define MSG_HISTORY_HOST            "HOST"
#define MSG_HISTORY_QUEUE           "QUEUE"
#define MSG_HISTORY_GROUP           "GROUP"
#define MSG_HISTORY_OWNER           "OWNER"
#define MSG_HISTORY_PROJECT         "PROJECT"
#define MSG_HISTORY_DEPARTMENT      "DEPARTMENT"
#define MSG_HISTORY_PE              "PE"
#define MSG_HISTORY_SLOTS           "SLOTS"
#define MSG_HISTORY_TOTSYSTEMUSAGE  _MESSAGE(25089, _("Total System Usage\n"))
#define MSG_HISTORY_USAGE           _MESSAGE(25090, _("usage:"    ))
#define MSG_HISTORY_A_OPT_USAGE     _MESSAGE(25091, _("jobs accounted to the given account\n"      ))
#define MSG_HISTORY_help_OPT_USAGE  _MESSAGE(25092, _("display this message\n"  ))
#define MSG_HISTORY_h_OPT_USAGE     _MESSAGE(25093, _("list [matching] host\n"))
#define MSG_HISTORY_q_OPT_USAGE     _MESSAGE(25094, _("list [matching] queue\n"))
#define MSG_HISTORY_g_OPT_USAGE     _MESSAGE(25095, _("list [matching] group\n"))
#define MSG_HISTORY_o_OPT_USAGE     _MESSAGE(25096, _("list [matching] owner\n"))
#define MSG_HISTORY_P_OPT_USAGE     _MESSAGE(25097, _("list [matching] project\n"))
#define MSG_HISTORY_D_OPT_USAGE     _MESSAGE(25098, _("list [matching] department\n"))
#define MSG_HISTORY_pe_OPT_USAGE    _MESSAGE(25099, _("list [matching] parallel environment\n"))
#define MSG_HISTORY_slots_OPT_USAGE _MESSAGE(25100, _("list [matching] job slots\n"   ))
#define MSG_HISTORY_l_OPT_USAGE     _MESSAGE(25101, _("request given complex attributes\n" ))
#define MSG_HISTORY_b_OPT_USAGE     _MESSAGE(25102, _("jobs started after\n"))
#define MSG_HISTORY_e_OPT_USAGE     _MESSAGE(25103, _("jobs started before\n"))
#define MSG_HISTORY_d_OPT_USAGE     _MESSAGE(25104, _("jobs started during the last d days\n"))
#define MSG_HISTORY_j_OPT_USAGE     _MESSAGE(25105, _("list all [matching] jobs\n"))
#define MSG_HISTORY_t_OPT_USAGE     _MESSAGE(25106, _("list all [matching] tasks (requires -j option)\n"))
#define MSG_HISTORY_f_OPT_USAGE           _MESSAGE(25109, _("use alternate accounting file\n"))
#define MSG_HISTORY_beginend_OPT_USAGE    _MESSAGE(25110, _("[[CC]YYMMDDhhmm[.SS]\n"))
#define MSG_HISTORY_SHOWJOB_QNAME             "qname"
#define MSG_HISTORY_SHOWJOB_HOSTNAME          "hostname"
#define MSG_HISTORY_SHOWJOB_GROUP             "group"
#define MSG_HISTORY_SHOWJOB_OWNER             "owner"
#define MSG_HISTORY_SHOWJOB_PROJECT           "project"
#define MSG_HISTORY_SHOWJOB_DEPARTMENT        "department"
#define MSG_HISTORY_SHOWJOB_JOBNAME           "jobname"
#define MSG_HISTORY_SHOWJOB_JOBNUMBER         "jobnumber"
#define MSG_HISTORY_SHOWJOB_TASKID            "taskid"
#define MSG_HISTORY_SHOWJOB_ACCOUNT           "account"
#define MSG_HISTORY_SHOWJOB_PRIORITY          "priority"
#define MSG_HISTORY_SHOWJOB_QSUBTIME          "qsub_time"
#define MSG_HISTORY_SHOWJOB_STARTTIME         "start_time"
#define MSG_HISTORY_SHOWJOB_ENDTIME           "end_time"
#define MSG_HISTORY_SHOWJOB_NULL              "(NULL)"
#define MSG_HISTORY_SHOWJOB_GRANTEDPE         "granted_pe"
#define MSG_HISTORY_SHOWJOB_SLOTS             "slots"
#define MSG_HISTORY_SHOWJOB_FAILED            "failed"
#define MSG_HISTORY_SHOWJOB_EXITSTATUS        "exit_status"
#define MSG_HISTORY_SHOWJOB_RUWALLCLOCK       "ru_wallclock"
#define MSG_HISTORY_SHOWJOB_RUUTIME           "ru_utime"
#define MSG_HISTORY_SHOWJOB_RUSTIME           "ru_stime"
#define MSG_HISTORY_SHOWJOB_VUTIME            "vutime"
#define MSG_HISTORY_SHOWJOB_VSTIME            "vstime"
#define MSG_HISTORY_SHOWJOB_MEMSIZE           "memsize"
#define MSG_HISTORY_SHOWJOB_RUMAXRSS          "ru_maxrss"
#define MSG_HISTORY_SHOWJOB_RUIXRSS           "ru_ixrss"
#define MSG_HISTORY_SHOWJOB_RUISMRSS          "ru_ismrss"
#define MSG_HISTORY_SHOWJOB_RUIDRSS           "ru_idrss"
#define MSG_HISTORY_SHOWJOB_RUISRSS           "ru_isrss"
#define MSG_HISTORY_SHOWJOB_RUMINFLT          "ru_minflt"
#define MSG_HISTORY_SHOWJOB_RUMAJFLT          "ru_majflt"
#define MSG_HISTORY_SHOWJOB_RUNSWAP           "ru_nswap"
#define MSG_HISTORY_SHOWJOB_RUINBLOCK         "ru_inblock"
#define MSG_HISTORY_SHOWJOB_RUOUBLOCK         "ru_oublock"
#define MSG_HISTORY_SHOWJOB_RUMSGSND          "ru_msgsnd"
#define MSG_HISTORY_SHOWJOB_RUMSGRCV          "ru_msgrcv"
#define MSG_HISTORY_SHOWJOB_RUNSIGNALS        "ru_nsignals"
#define MSG_HISTORY_SHOWJOB_RUNVCSW           "ru_nvcsw"
#define MSG_HISTORY_SHOWJOB_RUNIVCSW          "ru_nivcsw"
#define MSG_HISTORY_SHOWJOB_CPU               "cpu"
#define MSG_HISTORY_SHOWJOB_MEM               "mem"
#define MSG_HISTORY_SHOWJOB_IO                "io"
#define MSG_HISTORY_SHOWJOB_IOW               "iow"
#define MSG_HISTORY_SHOWJOB_MAXVMEM           "maxvmem"
#define MSG_HISTORY_GETALLLISTSGETCOMPLEXLISTFAILED      _MESSAGE(25111, _("get_all_lists: failed to get complex list\n"))
#define MSG_HISTORY_GETALLLISTSGETEXECHOSTLISTFAILED     _MESSAGE(25112, _("get_all_lists: failed to get exechost list\n"))
#define MSG_HISTORY_GETALLLISTSGETQUEUELISTFAILED        _MESSAGE(25113, _("get_all_lists: failed to get queue list\n"))




#endif /* __MSG_HISTORY_H   */
