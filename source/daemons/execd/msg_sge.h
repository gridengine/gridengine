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
#define MSG_SGE_TABINFOXFAILEDY_SS  _MESSAGE(31000, _("tabinfo(\"%s\", ...) failed, %s\n"))
#define MSG_MEMORY_MALLOCXFAILED_D  _MESSAGE(31001, _("malloc("U32CFormat") failed"))
#define MSG_SGE_SKIPPINGREADOFCORRUPTEDPACCTFILE   _MESSAGE(31002, _("skipping read of corrupted pacct file"))
#define MSG_SGE_FREADOFHEADERFAILEDPACCTFILECORRUPTED _MESSAGE(31003, _("fread of header failed, pacct file corrupted"))
#define MSG_SGE_FREADOFFLAGFAILEDPACCTFILECORRUPTED   _MESSAGE(31004, _("fread of flag failed, pacct file corrupted"))
#define MSG_SGE_BADACCOUNTINGRECORDPACCTFILECORRUPTED _MESSAGE(31005, _("bad accounting record, pacct file corrupted"))
#define MSG_SGE_FREADOFACCTRECORDFAILEDPACCTFILECORRUPTED _MESSAGE(31006, _("fread of acct record failed, pacct file corrupted"))
#define MSG_SGE_UNABLETOOPENNEWPACCTFILE              _MESSAGE(31007, _("unable to open new pacct file"))
#define MSG_SGE_READPACCTRECORDSFORXPROCESSESANDYJOBS_II _MESSAGE(31008, _("read pacct records for %d processes, %d jobs"))
#define MSG_SGE_SYSMP_MP_SAGETXFAILEDY_SS             _MESSAGE(31009, _("sysmp(MP_SAGET, %s, ...) failed, %s\n"))
#define MSG_SGE_SWAPCTL_XFAILEDY_SS                   _MESSAGE(31010, _("swapctl(%s, ...) failed, %s\n"))
#define MSG_SGE_PSRETRIEVESYSTEMDATASUCCESSFULLYCOMPLETED   _MESSAGE(31011, _("psRetrieveSystemData() successfully completed"))
#define MSG_MEMORY_MALLOCFAILURE                      _MESSAGE(31012, _("malloc failure"))
#define MSG_SGE_PSRETRIEVEOSJOBDATASUCCESSFULLYCOMPLETED _MESSAGE(31013, _("psRetrieveOSJobData() successfully completed"))
#define MSG_SGE_GETSYSINFO_GSI_PHYSMEM_FAILEDX_S         _MESSAGE(31014, _("getsysinfo(GSI_PHYSMEM) failed, %s\n"))
#define MSG_SGE_NLISTFAILEDX_S         _MESSAGE(31015, _("nlist failed: %s\n"))
#define MSG_SGE_VM_PERFSUM_NOTFOUND     _MESSAGE(31016, _("vm_perfsum not found\n"  ))
#define MSG_SGE_PSSTARTCOLLECTORSUCCESSFULLYCOMPLETED _MESSAGE(31017, _("psStartCollector() successfully completed"))
#define MSG_SGE_PSSTOPCOLLECTORSUCCESSFULLYCOMPLETED  _MESSAGE(31018, _("psStopCollector() successfully completed"))
#define MSG_SGE_PSWATCHJOBSUCCESSFULLYCOMPLETED       _MESSAGE(31019, _("psWatchJob() successfully completed"))
#define MSG_SGE_PSIGNOREJOBSUCCESSFULLYCOMPLETED      _MESSAGE(31020, _("psIgnoreJob() successfully completed"))
#define MSG_SGE_PSTATUSSUCCESSFULLYCOMPLETED          _MESSAGE(31021, _("psStatus() successfully completed"))
#define MSG_SGE_PSGETONEJOBSUCCESSFULLYCOMPLETED      _MESSAGE(31022, _("psGetOneJob() successfully completed"))
#define MSG_SGE_PSGETALLJOBSSUCCESSFULLYCOMPLETED     _MESSAGE(31023, _("psGetAllJobs() successfully completed"))
#define MSG_SGE_PSGETSYSDATASUCCESSFULLYCOMPLETED     _MESSAGE(31024, _("psGetSysdata() successfully completed"))
#define MSG_SGE_USAGE         _MESSAGE(31025, _("\nusage: pdc [-snpgj] [-kK signo] [-iJPS secs] job_id [ ... ]\n\n"))
#define MSG_SGE_s_OPT_USAGE   _MESSAGE(31026, _("\t-s\tshow system data\n"  ))
#define MSG_SGE_n_OPT_USAGE   _MESSAGE(31027, _("\t-n\tno output\n"  ))
#define MSG_SGE_p_OPT_USAGE   _MESSAGE(31028, _("\t-p\tshow process information\n"  ))
#define MSG_SGE_i_OPT_USAGE   _MESSAGE(31029, _("\t-i\tinterval in seconds (default is 2)\n"  ))
#define MSG_SGE_g_OPT_USAGE   _MESSAGE(31030, _("\t-g\tproduce output for gr_osview(1)\n"  ))
#define MSG_SGE_j_OPT_USAGE   _MESSAGE(31031, _("\t-j\tprovide job name for gr_osview display (used only with -g)\n"  ))
#define MSG_SGE_J_OPT_USAGE   _MESSAGE(31032, _("\t-J\tjob data collection interval in seconds (default is 0)\n"    ))
#define MSG_SGE_k_OPT_USAGE   _MESSAGE(31033, _("\t-k\tkill job using signal signo\n"    ))
#define MSG_SGE_K_OPT_USAGE   _MESSAGE(31034, _("\t-K\tkill job using signal signo (loop until all processes are dead)\n"    ))
#define MSG_SGE_P_OPT_USAGE   _MESSAGE(31035, _("\t-P\tprocess data collection interval in seconds (default is 0)\n"    ))
#define MSG_SGE_S_OPT_USAGE   _MESSAGE(31036, _("\t-S\tsystem data collection interval in seconds (default is 15)\n\n"    ))
#define MSG_SGE_JOBDATA       _MESSAGE(31037, _("******** Job Data **********\n"))
#define MSG_SGE_SYSTEMDATA    _MESSAGE(31038, _("******** System Data ***********\n"))
#define MSG_SGE_STATUS        _MESSAGE(31039, _("********** Status **************\n"))
#define MSG_SGE_CPUUSAGE      _MESSAGE(31040, _("CPU Usage:"))
#define MSG_SGE_SGEJOBUSAGECOMPARSION     _MESSAGE(31041, _("Job Usage Comparison"))
#define MSG_SGE_XISNOTAVALIDSIGNALNUMBER_S   _MESSAGE(31042, _("%s is not a valid signal number\n"))
#define MSG_SGE_XISNOTAVALIDINTERVAL_S       _MESSAGE(31043, _("%s is not a valid interval\n"))
#define MSG_SGE_XISNOTAVALIDJOBID_S          _MESSAGE(31044, _("%s is not a valid job_id\n"))
#define MSG_SGE_GROSVIEWEXPORTFILE           _MESSAGE(31045, _("gr_osview export file\n"))
#define MSG_SGE_PERMISSIONDENIED             _MESSAGE(31046, _("permission denied\n"))
#define MSG_SGE_NOJOBS                       _MESSAGE(31047, _("No jobs\n"))


/* 
** sge/procfs.c
*/ 
#define MSG_SGE_NGROUPS_MAXOSRECONFIGURATIONNECESSARY    _MESSAGE(31048, _("NGROUPS_MAX <= 0: os reconfiguration necessary\n"))
#define MSG_SGE_PROCFSKILLADDGRPIDMALLOCFAILED           _MESSAGE(31049, _("procfs_kill_addgrpid(): malloc failed" ))
#define MSG_SGE_KILLINGPIDXY_UI                          _MESSAGE(31050, _("killing pid "U32CFormat"/%d" ))
#define MSG_SGE_DONOTKILLROOTPROCESSXY_UI                _MESSAGE(31051, _("do not kill root process "U32CFormat"/%d"   ))
#define MSG_SGE_PTDISPATCHPROCTOJOBMALLOCFAILED          _MESSAGE(31052, _("pt_dispatch_proc_to_job: malloc failed\n" ))




#endif /* __MSG_SGE_H */
