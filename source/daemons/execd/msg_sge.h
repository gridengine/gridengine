#ifndef __MSG_SGE_H
#define __MSG_SGE_H
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
** sge/pdc.c
*/ 
#define MSG_SGE_TABINFOXFAILEDY_SS  _("tabinfo(\"%s\", ...) failed, %s\n")
#define MSG_MEMORY_MALLOCXFAILED_D  _("malloc("U32CFormat") failed")
#define MSG_SGE_SKIPPINGREADOFCORRUPTEDPACCTFILE   _("skipping read of corrupted pacct file")
#define MSG_SGE_FREADOFHEADERFAILEDPACCTFILECORRUPTED _("fread of header failed, pacct file corrupted")
#define MSG_SGE_FREADOFFLAGFAILEDPACCTFILECORRUPTED   _("fread of flag failed, pacct file corrupted")
#define MSG_SGE_BADACCOUNTINGRECORDPACCTFILECORRUPTED _("bad accounting record, pacct file corrupted")
#define MSG_SGE_FREADOFACCTRECORDFAILEDPACCTFILECORRUPTED _("fread of acct record failed, pacct file corrupted")
#define MSG_SGE_UNABLETOOPENNEWPACCTFILE              _("unable to open new pacct file")
#define MSG_SGE_READPACCTRECORDSFORXPROCESSESANDYJOBS_II _("read pacct records for %d processes, %d jobs")
#define MSG_SGE_SYSMP_MP_SAGETXFAILEDY_SS             _("sysmp(MP_SAGET, %s, ...) failed, %s\n")
#define MSG_SGE_SWAPCTL_XFAILEDY_SS                   _("swapctl(%s, ...) failed, %s\n")
#define MSG_SGE_PSRETRIEVESYSTEMDATASUCCESSFULLYCOMPLETED   _("psRetrieveSystemData() successfully completed")
#define MSG_MEMORY_MALLOCFAILURE                      _("malloc failure")
#define MSG_SGE_PSRETRIEVEOSJOBDATASUCCESSFULLYCOMPLETED _("psRetrieveOSJobData() successfully completed")
#define MSG_SGE_GETSYSINFO_GSI_PHYSMEM_FAILEDX_S         _("getsysinfo(GSI_PHYSMEM) failed, %s\n")
#define MSG_SGE_NLISTFAILEDX_S         _("nlist failed: %s\n")
#define MSG_SGE_VM_PERFSUM_NOTFOUND     _("vm_perfsum not found\n"  )
#define MSG_SGE_PSSTARTCOLLECTORSUCCESSFULLYCOMPLETED _("psStartCollector() successfully completed")
#define MSG_SGE_PSSTOPCOLLECTORSUCCESSFULLYCOMPLETED  _("psStopCollector() successfully completed")
#define MSG_SGE_PSWATCHJOBSUCCESSFULLYCOMPLETED       _("psWatchJob() successfully completed")
#define MSG_SGE_PSIGNOREJOBSUCCESSFULLYCOMPLETED      _("psIgnoreJob() successfully completed")
#define MSG_SGE_PSTATUSSUCCESSFULLYCOMPLETED          _("psStatus() successfully completed")
#define MSG_SGE_PSGETONEJOBSUCCESSFULLYCOMPLETED      _("psGetOneJob() successfully completed")
#define MSG_SGE_PSGETALLJOBSSUCCESSFULLYCOMPLETED     _("psGetAllJobs() successfully completed")
#define MSG_SGE_PSGETSYSDATASUCCESSFULLYCOMPLETED     _("psGetSysdata() successfully completed")
#define MSG_SGE_USAGE         _("\nusage: pdc [-snpgj] [-kK signo] [-iJPS secs] job_id [ ... ]\n\n")
#define MSG_SGE_s_OPT_USAGE   _("\t-s\tshow system data\n"  )
#define MSG_SGE_n_OPT_USAGE   _("\t-n\tno output\n"  )
#define MSG_SGE_p_OPT_USAGE   _("\t-p\tshow process information\n"  )
#define MSG_SGE_i_OPT_USAGE   _("\t-i\tinterval in seconds (default is 2)\n"  )
#define MSG_SGE_g_OPT_USAGE   _("\t-g\tproduce output for gr_osview(1)\n"  )
#define MSG_SGE_j_OPT_USAGE   _("\t-j\tprovide job name for gr_osview display (used only with -g)\n"  )
#define MSG_SGE_J_OPT_USAGE   _("\t-J\tjob data collection interval in seconds (default is 0)\n"    )
#define MSG_SGE_k_OPT_USAGE   _("\t-k\tkill job using signal signo\n"    )
#define MSG_SGE_K_OPT_USAGE   _("\t-K\tkill job using signal signo (loop until all processes are dead)\n"    )
#define MSG_SGE_P_OPT_USAGE   _("\t-P\tprocess data collection interval in seconds (default is 0)\n"    )
#define MSG_SGE_S_OPT_USAGE   _("\t-S\tsystem data collection interval in seconds (default is 15)\n\n"    )
#define MSG_SGE_JOBDATA       _("******** Job Data **********\n")
#define MSG_SGE_SYSTEMDATA    _("******** System Data ***********\n")
#define MSG_SGE_STATUS        _("********** Status **************\n")
#define MSG_SGE_CPUUSAGE      _("CPU Usage:")
#define MSG_SGE_SGEJOBUSAGECOMPARSION     _("Job Usage Comparison")
#define MSG_SGE_XISNOTAVALIDSIGNALNUMBER_S   _("%s is not a valid signal number\n")
#define MSG_SGE_XISNOTAVALIDINTERVAL_S       _("%s is not a valid interval\n")
#define MSG_SGE_XISNOTAVALIDJOBID_S          _("%s is not a valid job_id\n")
#define MSG_SGE_GROSVIEWEXPORTFILE           _("gr_osview export file\n")
#define MSG_SGE_PERMISSIONDENIED             _("permission denied\n")
#define MSG_SGE_NOJOBS                       _("No jobs\n")


/* 
** sge/procfs.c
*/ 
#define MSG_SGE_NGROUPS_MAXOSRECONFIGURATIONNECESSARY    _("NGROUPS_MAX <= 0: os reconfiguration necessary\n")
#define MSG_SGE_PROCFSKILLADDGRPIDMALLOCFAILED           _("procfs_kill_addgrpid(): malloc failed" )
#define MSG_SGE_KILLINGPIDXY_UI                          _("killing pid "U32CFormat"/%d" )
#define MSG_SGE_DONOTKILLROOTPROCESSXY_UI                _("do not kill root process "U32CFormat"/%d"   )
#define MSG_SGE_PTDISPATCHPROCTOJOBMALLOCFAILED          _("pt_dispatch_proc_to_job: malloc failed\n" )
#define MSG_SGE_PIOCPSINFOFAILED                         _("PIOCPSINFO failed\n")




#endif /* __MSG_SGE_H */
