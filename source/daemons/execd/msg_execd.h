#ifndef __MSG_EXECD_H
#define __MSG_EXECD_H
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
**  global execd messages 
*/
#define MSG_COM_ACK_UNKNOWN          _MESSAGE(29000, _("unknown ack event\n"))


/*
** tmpdir.c
*/
#define MSG_FILE_RECURSIVERMDIR_SS       _MESSAGE(29001, _("recursive rmdir("SFN"): "SFN"\n"))

/*
** setup_execd.c
*/
#define MSG_JOB_XREGISTERINGJOBYATPTFDURINGSTARTUP_SU    _MESSAGE(29002, _(SFN" registering job \""U32CFormat"\" at ptf during startup"))
#define MSG_FAILED    _MESSAGE(29003, _("failed"))
#define MSG_DELAYED    _MESSAGE(29004, _("delayed"))
#define MSG_JOB_XREGISTERINGJOBYTASKZATPTFDURINGSTARTUP_SUS    _MESSAGE(29005, _(SFN" registering job \""U32CFormat"\" task "SFN" at ptf during startup"))
#define MSG_ANSWER_KEEPINGCHANNELFDXOPEN_I    _MESSAGE(29006, _("keeping channel fd #%d open\n"))
#define MSG_SGETEXT_NOCOMMD                     _MESSAGE(29007, _("can't connect commd.\n"))

/*
** reaper_execd.c
*/
#define MSG_SLAVE    _MESSAGE(29008, _("slave "))
#define MSG_COREDUMPED    _MESSAGE(29009, _("(core dumped) "))
#define MSG_WAITPIDNOSIGNOEXIT_UI   _MESSAGE(29010, _("waitpid() returned for pid "U32CFormat" status %d unequal WIFSIGNALED/WIFEXITED\n"))
#define MSG_SHEPHERD_VSHEPHERDOFJOBWXDIEDTHROUGHSIGNALYZ_SUUSI    _MESSAGE(29011, _(SFN"shepherd of job "U32CFormat"."U32CFormat" died through signal "SFN"= %d"))
#define MSG_SHEPHERD_WSHEPHERDOFJOBXYEXITEDWITHSTATUSZ_SUUI    _MESSAGE(29012, _(SFN"shepherd of job "U32CFormat"."U32CFormat" exited with exit status = %d\n"))
#define MSG_JOB_MISSINGJOBXYINJOBREPORTFOREXITINGJOBADDINGIT_UU    _MESSAGE(29013, _("missing job "U32CFormat"."U32CFormat" in job report for exiting job - adding it"))
#define MSG_STATUS_LOADSENSORDIEDWITHSIGNALXY_SI    _MESSAGE(29014, _("load sensor died through signal "SFN"= %d"))
#define MSG_STATUS_LOADSENSOREXITEDWITHEXITSTATUS_I    _MESSAGE(29015, _("load sensor exited with exit status = %d\n"))
#define MSG_STATUS_MAILERDIEDTHROUGHSIGNALXY_SI    _MESSAGE(29016, _("mailer died through signal "SFN" = %d"))
#define MSG_STATUS_MAILEREXITEDWITHEXITSTATUS_I    _MESSAGE(29017, _("mailer exited with exit status = %d\n"))
#define MSG_JOB_REAPINGJOBXPTFCOMPLAINSY_US    _MESSAGE(29018, _("reaping job \""U32CFormat"\" ptf complains: "SFN))
#define MSG_JOB_CLEANUPJOBCALLEDWITHINVALIDPARAMETERS    _MESSAGE(29019, _("clean_up_job() called with invalid parameters"))
#define MSG_JOB_CANTFINDDIRXFORREAPINGJOBYZ_SS _MESSAGE(29020, _("can't find directory "SFN" for reaping job "SFN"\n"))
#define MSG_JOB_CANTREADCONFIGFILEFORJOBXY_S    _MESSAGE(29021, _("can't read config file for job "SFN"\n"))
#define MSG_STATUS_ABNORMALTERMINATIONOFSHEPHERDFORJOBXY_S    _MESSAGE(29022, _("abnormal termination of shepherd for job "SFN": no \"exit_status\" file"))
#define MSG_STATUS_ABNORMALTERMINATIONFOSHEPHERDFORJOBXYEXITSTATEFILEISEMPTY_S    _MESSAGE(29023, _("abnormal termination of shepherd for job "SFN": \"exit_status\" file is empty"))
#define MSG_SHEPHERD_DIEDTHROUGHSIGNAL    _MESSAGE(29024, _("shepherd died through signal"))
#define MSG_SHEPHERD_NOPIDFILE    _MESSAGE(29025, _("no \"pid\" file for shepherd"))
#define MSG_SHEPHERD_EXITEDWISSTATUS_I    _MESSAGE(29026, _("shepherd exited with exit status %d"))
#define MSG_JOB_CANTREADERRORFILEFORJOBXY_S    _MESSAGE(29027, _("can't read error file for job "SFN"\n"))
#define MSG_JOB_CANTREADUSAGEFILEFORJOBXY_S    _MESSAGE(29028, _("can't read usage file for job "SFN"\n"))
#define MSG_JOB_WXDIEDTHROUGHSIGNALYZ_SSI    _MESSAGE(29029, _("job "SFN" died through signal "SFN" (%d)"))
#define MSG_JOB_CANTREADUSEDRESOURCESFORJOB    _MESSAGE(29030, _("can't read used resources for job"))
#define MSG_JOB_CANTOPENJOBPIDFILEFORJOBXY_S    _MESSAGE(29031, _("can't open \"job_pid\" file for job "SFN"\n"))
#define MSG_SHEPHERD_REMOVEACKEDJOBEXITCALLEDWITHX_U    _MESSAGE(29032, _("remove_acked_job_exit called with "U32CFormat".0"))
#define MSG_JOB_XYHASNOTASKZ_UUS    _MESSAGE(29033, _("job "U32CFormat"."U32CFormat" has no task "SFQ))
#define MSG_SHEPHERD_CANTSTARTXFORJOBY_SU    _MESSAGE(29034, _("can't start command "SFQ" for job " U32CFormat " to delete credentials"))
#define MSG_SHEPHERD_CANTDELCREDENTIALSFORJOBXCOMMANDYFAILEDWITHCODEZ_USI    _MESSAGE(29035, _("could not delete credentials for job " U32CFormat" - command "SFQ" failed with return code %d"))
#define MSG_SHEPHERD_CANTDELCREDENTIALSFORJOBXYBINARYNOTEXIST_US    _MESSAGE(29036, _("could not delete credentials for job " U32CFormat" - "SFN" binary does not exist"))
#define MSG_FILE_CANTREMOVEDIRECTORY_SS    _MESSAGE(29037, _("can't remove directory "SFQ": "SFN))
#define MSG_SHEPHERD_ACKNOWLEDGEFORUNKNOWNJOBXYZ_UUS    _MESSAGE(29038, _("acknowledge for unknown job "U32CFormat"."U32CFormat"/"SFN))
#define MSG_SHEPHERD_ACKNOWLEDGEFORUNKNOWNJOBEXIT    _MESSAGE(29039, _("acknowledge for unknown job's exit"))
#define MSG_SHEPHERD_CANTFINDACTIVEJOBSDIRXFORREAPINGJOBY_SU    _MESSAGE(29040, _("can't find active jobs directory "SFQ" for reaping job "U32CFormat ))
#define MSG_SHEPHERD_INCORRECTCONFIGFILEFORJOBXY_UU    _MESSAGE(29041, _("incorrect config file for job "U32CFormat"."U32CFormat""))
#define MSG_SHEPHERD_CANTSTARTJOBXY_US     _MESSAGE(29042, _("can't start job \""U32CFormat"\": "SFN))
#define MSG_SHEPHERD_PROBLEMSAFTERSTART_DS _MESSAGE(29043, _("problems after job start \""U32CFormat"\": "SFN))
#define MSG_SHEPHERD_JATASKXYISKNOWNREPORTINGITTOQMASTER    _MESSAGE(29044, _("ja-task \"" U32CFormat"."U32CFormat"\" is unknown - reporting it to qmaster"))
#define MSG_SHEPHERD_CKECKINGFOROLDJOBS    _MESSAGE(29045, _("checking for old jobs"))
#define MSG_SHEPHERD_NOOLDJOBSATSTARTUP    _MESSAGE(29046, _("no old jobs at startup"))
#define MSG_SHEPHERD_CANTGETPROCESSESFROMPSCOMMAND    _MESSAGE(29047, _("can't get processes from ps command"))
#define MSG_SHEPHERD_XISNOTAJOBDIRECTORY_S    _MESSAGE(29048, _(SFQ" is not a job directory"))
#define MSG_SHEPHERD_FOUNDACTIVEJOBDIRXWHILEMISSINGJOBDIRREMOVING_S    _MESSAGE(29049, _("found active job directory "SFQ" while missing job directory - removing"))
#define MSG_SHEPHERD_CANTSTATXY_SS    _MESSAGE(29050, _("can't stat "SFQ": "SFN))
#define MSG_FILE_XISNOTADIRECTORY_S    _MESSAGE(29051, _(SFQ" is not a directory"))
#define MSG_SHEPHERD_FOUNDDIROFJOBX_S    _MESSAGE(29052, _("found directory of job "SFQ))
#define MSG_SHEPHERD_CANTREADPIDFILEXFORJOBYSTARTTIMEZX_SSUS    _MESSAGE(29053, _("can't read pid file "SFQ" of shepherd for job "SFQ" - starttime: "U32CFormat" cleaning up: "SFN))
#define MSG_SHEPHERD_MISSINGJOBXINJOBREPORTFOREXITINGJOB_U    _MESSAGE(29054, _("missing job \""U32CFormat"\" in job report for exiting job"))
#define MSG_SHEPHERD_CANTREADPIDFROMPIDFILEXFORJOBY_SS    _MESSAGE(29055, _("can't read pid from pid file "SFQ" of shepherd for job "SFN))
#define MSG_SHEPHERD_SHEPHERDFORJOBXHASPIDYANDISZALIVE_SUS    _MESSAGE(29056, _("shepherd for job "SFN" has pid \""U32CFormat"\" and is "SFN" alive\n"))
#define MSG_NOT    _MESSAGE(29057, _("not"))
#define MSG_SHEPHERD_INCONSISTENTDATAFORJOBX_U    _MESSAGE(29058, _("inconsistent data for job \""U32CFormat"\""))
#define MSG_SHEPHERD_MISSINGJOBXYINJOBREPORT_UU    _MESSAGE(29059, _("Missing job "U32CFormat"."U32CFormat" in job report"))
#define MSG_SHEPHERD_CANTOPENPIDFILEXFORJOBYZ_SUU    _MESSAGE(29060, _("can't open pid file "SFQ" for job "U32CFormat"."U32CFormat))
#define MSG_SHEPHERD_CANTOPENUSAGEFILEXFORJOBYZX_SUUS    _MESSAGE(29061, _("can't open usage file "SFQ" for job "U32CFormat"."U32CFormat": "SFN))
#define MSG_SHEPHERD_EXECDWENTDOWNDURINGJOBSTART _MESSAGE(29062, _("execd went down during job start"))
#define MSG_SHEPHERD_REPORTINGJOBFINSIHTOQMASTER _MESSAGE(29063, _(" reporting job finish to qmaster"))
#define MSG_SHEPHERD_GOTACKFORJOBEXIT              _MESSAGE(29064, _(" got ack for job exit"))
#define MSG_SHEPHERD_REPORINGJOBSTARTFAILURETOQMASTER _MESSAGE(29065, _(" reporting job start failure to qmaster"))
#define MSG_SHEPHERD_REPORINGJOBPROBLEMTOQMASTER _MESSAGE(29066, _(" reporting problems with running job to qmaster"))
#define MSG_SHEPHERD_ACKINGUNKNWONJOB                 _MESSAGE(29067, _(" ack'ing unknown job"))
#define MSG_JR_ERRSTR_EXECDDONTKNOWJOB _MESSAGE(29068, _("execd doesn't know this job"))
#define MSG_EXECD_GOTACKFORPETASKBUTISNOTINSTATEEXITING_S _MESSAGE(29069, _("get exit ack for pe task "SFN" but task is not in state exiting"))


/*
** ptf.c
*/
#define MSG_SYSTEM_SYSINFO_SI_RELEASE_CALL_FAILED_S        _MESSAGE(29070, _("sysinfo(SI_RELEASE) call failed - "SFN))
#define MSG_SCHEDD_JOBXPIDYSCHEDCTLFAILUREX_UUS            _MESSAGE(29071, _("job "U32CFormat" pid "U32CFormat" schedctl failure: "SFN"\n"))
#define MSG_PRIO_JOBXPIDYSETPRIORITYFAILURE_UUS            _MESSAGE(29072, _("job "U32CFormat" pid "U32CFormat" setpriority failure: "SFN"\n"))
#define MSG_SCHEDD_JOBXPIDYSCHEDSETSCHEDULERFAILURE_UUS    _MESSAGE(29073, _("job "U32CFormat" pid "U32CFormat" sched_setscheduler failure: "SFN"\n"))
#define MSG_PRIO_JOBXNICEMFAILURE_S                        _MESSAGE(29074, _("job "U32CFormat" nicem failure: "SFN"\n"))
#define MSG_PRIO_JOBXNICEJFAILURE_S                        _MESSAGE(29075, _("job "U32CFormat" nicej failure: "SFN"\n"))
#define MSG_PRIO_JOBXSETPRIORITYFAILURE_US                 _MESSAGE(29076, _("job "U32CFormat" setpriority failure: "SFN"\n"))
#define MSG_WHERE_FAILEDTOBUILDWHERECONDITION              _MESSAGE(29077, _("failed to build where-condition"))
#define MSG_PRIO_PTFMINMAX_II                              _MESSAGE(29078, _("PTF_MAX_PRIORITY=%d, PTF_MIN_PRIORITY=%d\n"))
#define MSG_PRIO_NICEMFAILED_S                             _MESSAGE(29079, _("nicem failed: "SFN"\n"))
#define MSG_PRIO_SETPRIOFAILED_S                           _MESSAGE(29080, _("setpriority failed: "SFN"\n"))
#define MSG_ERROR_UNKNOWNERRORCODE                         _MESSAGE(29081, _("Unknown error code"))
#define MSG_ERROR_NOERROROCCURED                           _MESSAGE(29082, _("No error occurred"))
#define MSG_ERROR_INVALIDARGUMENT                          _MESSAGE(29083, _("Invalid argument"))
#define MSG_ERROR_JOBDOESNOTEXIST                          _MESSAGE(29084, _("Job does not exist"))
#define MSG_JOB_THEASHFORJOBXISY_UX                        _MESSAGE(29085, _("The ASH for job "U32CFormat" is "u64"\n"))
#define MSG_JOB_MYASHIS_X                                  _MESSAGE(29086, _("My ash is "u64"\n"))
#define MSG_ERROR_UNABLETODUMPJOBUSAGELIST                 _MESSAGE(29087, _("Unable to dump job usage list\n"))



/*
** src/load_avg.c
*/
#define MSG_LOAD_NOMEMINDICES          _MESSAGE(29088, _("failed retrieving memory indices\n"))
#define MSG_LOAD_NOPTFUSAGE_S          _MESSAGE(29089, _("ptf failed to determine job usage: "SFN))
#define MSG_SGETEXT_NO_LOAD                           _MESSAGE(29090, _("can't get load values\n"))



/*
** job_report_execd.c
*/
#define MSG_JOB_TYPEMALLOC            _MESSAGE(29091, _("runtime type error or malloc failure in add_job_report"))
#define MSG_PARSE_USAGEATTR_SSU       _MESSAGE(29092, _("failed parsing "SFQ" passed as usage attribute "SFQ" of job "U32CFormat) )      


/*
** get_path.c
*/
#define MSG_EXECD_INVALIDUSERNAME_S   _MESSAGE(29093, _("invalid user name "SFQ"\n"))
#define MSG_EXECD_NOHOMEDIR_S         _MESSAGE(29094, _("missing home directory for user "SFQ"\n"))
#define MSG_BUFFEROFSIZETOOSMALLFOR_DS _MESSAGE(29095, _("buffer of size %d is too small for path "SFN"...\n"))


/*
** execd_ticket.c
*/
#define MSG_JOB_TICKETFORMAT          _MESSAGE(29096, _("format error in ticket request"))
#define MSG_JOB_TICKETPASS2PTF_IS     _MESSAGE(29097, _("passing %d new tickets ptf complains: "SFN))


/*
** exec_job.c
*/
#define MSG_FILE_RMDIR_SS             _MESSAGE(29098, _("can't remove directory "SFQ": "SFN"\n"))
#define MSG_FILE_CREATEDIRDEL_SS      _MESSAGE(29099, _("can't create directory "SFN" after deletion: "SFN))
#define MSG_FILE_CREATEDIR_SS         _MESSAGE(29100, _("can't create directory "SFN": "SFN))
#define MSG_EXECD_NOSGID              _MESSAGE(29101, _("supplementary group ids could not be found in /proc"))
#define MSG_EXECD_NOPARSEGIDRANGE     _MESSAGE(29102, _("can not parse gid_range"))
#define MSG_EXECD_NOADDGID            _MESSAGE(29103, _("can not find an unused add_grp_id"))
#define MSG_MAIL_MAILLISTTOOLONG_U    _MESSAGE(29104, _("maillist for job " U32CFormat " too long\n"))
#define MSG_EXECD_NOXTERM             _MESSAGE(29105, _("unable to find xterm executable for interactive job, not configured\n"))
#define MSG_EXECD_NOSHEPHERD_SSS      _MESSAGE(29108, _("unable to find shepherd executable neither in architecture directory "SFN" nor in "SFN": "SFN))
#define MSG_EXECD_NOSHEPHERDWRAP_SS   _MESSAGE(29109, _("unable to find shepherd wrapper command "SFN": "SFN))
#define MSG_DCE_NOSHEPHERDWRAP_SS     _MESSAGE(29110, _("unable to find DCE shepherd wrapper command "SFN": "SFN))
#define MSG_EXECD_NOCOSHEPHERD_SSS    _MESSAGE(29111, _("unable to find coshepherd executable neither in architecture directory "SFN" nor in "SFN": "SFN))
#define MSG_EXECD_AFSCONFINCOMPLETE   _MESSAGE(29112, _("incomplete AFS configuration - set_token_cmd and token_extend_time must be configured"))
#define MSG_EXECD_NOCREATETOKENFILE_S _MESSAGE(29113, _("can't create token file: "SFN))
#define MSG_EXECD_TOKENZERO           _MESSAGE(29114, _("AFS token does not exist or has zero length"))
#define MSG_EXECD_NOWRITETOKEN_S      _MESSAGE(29115, _("can't write token to token file: "SFN))

/* CR: don't localize mail subject, until we send it in Mime format!
 *  The message definition is not l10n'ed (no _() macro used)!!!     
 */
#define MSG_MAIL_STARTSUBJECT_UUS     "Job-array task "U32CFormat"."U32CFormat" ("SFN") Started" 

#define MSG_MAIL_STARTBODY_UUSSSSS    _MESSAGE(29116, _("Job-array task "U32CFormat"."U32CFormat" ("SFN") Started\n User       = "SFN"\n Queue      = "SFN"\n Host       = "SFN"\n Start Time = "SFN) ) 

/* CR: don't localize mail subject, until we send it in Mime format!
 *  The message definition is not l10n'ed (no _() macro used)!!!     
 */
#define MSG_MAIL_STARTSUBJECT_US      "Job "U32CFormat" ("SFN") Started"

#define MSG_MAIL_STARTBODY_USSSSS     _MESSAGE(29117, _("Job "U32CFormat" ("SFN") Started\n User       = "SFN"\n Queue      = "SFN"\n Host       = "SFN"\n Start Time = "SFN) ) 
#define MSG_FILE_CHDIR_SS             _MESSAGE(29118, _("can't change dir to "SFN": "SFN))
#define MSG_EXECD_NOFORK_S            _MESSAGE(29119, _("fork failed: "SFN))
#define MSG_EXECD_NOSTARTSHEPHERD     _MESSAGE(29120, _("unable to start shepherd process"))
#define MSG_EXECD_NOWHERE4GDIL_S        _MESSAGE(29121, _("cannot build where for "SFN"\n"))
#define MSG_SYSTEM_CANTMAKETMPDIR     _MESSAGE(29122, _("can't make tmpdir"))
#define MSG_SYSTEM_CANTGETTMPDIR      _MESSAGE(29123, _("can't get tmpdir"))
#define MSG_EXECD_UNABLETOFINDSCRIPTFILE_SS  _MESSAGE(29124, _("unable to find script file "SFN": "SFN))
#define MSG_EXECD_NEEDATTRXINUSERDEFCOMPLOFYQUEUES_SS _MESSAGE(29125, _("need attribute "SFQ" in userdefined complex of "SFQ"-queues\n"))

/*
** execd_ck_to_do.c
*/
#define MSG_JOB_EXCEEDHLIM_USSFF      _MESSAGE(29126, _("job "U32CFormat" exceeds job hard limit "SFQ" of queue "SFQ" (%8.5f > limit:%8.5f) - sending SIGKILL\n"))
#define MSG_JOB_EXCEEDSLIM_USSFF      _MESSAGE(29127, _("job "U32CFormat" exceeds job soft limit "SFQ" of queue "SFQ" (%8.5f > limit:%8.5f) - sending SIGXCPU\n"))
#define MSG_EXECD_EXCEEDHWALLCLOCK_UU _MESSAGE(29128, _("job "U32CFormat"."U32CFormat" exceeded hard wallclock time - initiate terminate method"))
#define MSG_EXECD_EXCEEDSWALLCLOCK_UU _MESSAGE(29129, _("job "U32CFormat"."U32CFormat" exceeded soft wallclock time - initiate soft notify method"))
#define MSG_EXECD_NOADDGIDOPEN_SSS   _MESSAGE(29130, _("failed opening addgrpid file "SFN" of job "SFN": "SFN))
#define MSG_JOB_NOREGISTERPTF_SS     _MESSAGE(29131, _("failed registering job "SFN" at ptf: "SFN))
#define MSG_EXECD_NOOSJOBIDOPEN_SSS  _MESSAGE(29132, _("failed opening os jobid file "SFN" of job "SFN": "SFN))
#define MSG_EXECD_NOOSJOBIDREAD_SUUS  _MESSAGE(29133, _("failed reading os jobid file "SFN" of job "U32CFormat"."U32CFormat": "SFN))
/*
** execd_job_exec.c
*/
#define MSG_COM_UNPACKFEATURESET      _MESSAGE(29134, _("unpacking featureset from job execution message\n"))
#define MSG_COM_UNPACKJOB             _MESSAGE(29135, _("unpacking job from job execution message\n"))
#define MSG_COM_RECEIVED              _MESSAGE(29136, _("received"))
#define MSG_COM_UNPACKINGQ            _MESSAGE(29137, _("unpacking queue list from job execution message\n"))
#define MSG_JOB_MISSINGQINGDIL_SU     _MESSAGE(29138, _("missing queue "SFQ" found in gdil of job "U32CFormat"\n"))
#define MSG_EXECD_NOWRITESCRIPT_SIUS  _MESSAGE(29139, _("can't write script file "SFQ" wrote only %d of "U32CFormat" bytes: "SFN))
#define MSG_JOB_TASKWITHOUTJOB_U      _MESSAGE(29140, _("received task belongs to job "U32CFormat" but this job is not here\n"))
#define MSG_JOB_TASKNOTASKINJOB_UU    _MESSAGE(29141, _("received task belongs to job "U32CFormat" but this job is here but the JobArray task "U32CFormat" is not here\n"))
#define MSG_JOB_TASKNOSUITABLEJOB_U   _MESSAGE(29142, _("received task belongs to job "U32CFormat" but this job is not suited for starting tasks\n"))
#define MSG_JOB_TASKALREADYEXISTS_US  _MESSAGE(29143, _("received task "U32CFormat"/"SFN" which is already here\n"))
#define MSG_JOB_NOTASKPASSINGIF_SU    _MESSAGE(29144, _(SFN" does not fulfill task passing interface for job "U32CFormat"\n"))
#define MSG_JOB_NOFREEQ_USSS          _MESSAGE(29145, _("no free queue for job "U32CFormat" of user "SFN"@"SFN" (localhost = "SFN")\n"))
#define MSG_JOB_NOSUCHQ_SUSS          _MESSAGE(29146, _("no such queue "SFQ" as requested by job "U32CFormat" from user "SFN"@"SFN"\n"))
#define MSG_JOB_NOREQQONHOST_SSS      _MESSAGE(29147, _("requested queue "SFQ" resides not at this host "SFN" but at host "SFN"\n"))
#define MSG_JOB_REQQFULL_SII          _MESSAGE(29148, _("requested queue "SFQ" is already full (%d/%d)\n"))
/*
** execd_kill_execd.c
*/
#define MSG_JOB_INITCKPTSHUTDOWN_U    _MESSAGE(29149, _("initiate checkpoint at shutdown: job "U32CFormat""))
#define MSG_JOB_KILLSHUTDOWN_U        _MESSAGE(29150, _("killing job at shutdown: job "U32CFormat""))

/*
** execd_signal_queue.c
*/
#define MSG_JOB_INITMIGRSUSPQ_U       _MESSAGE(29151, _("initiate migration at queue suspend for job "U32CFormat))
#define MSG_JOB_SIGNALTASK_UUS        _MESSAGE(29152, _("SIGNAL jid: " U32CFormat " jatask: " U32CFormat " signal: "SFN"\n"))
#define MSG_EXECD_WRITESIGNALFILE_S   _MESSAGE(29153, _("error writing file "SFN" for signal transfer to shepherd"))
#define MSG_JOB_DELIVERSIGNAL_ISSIS   _MESSAGE(29154, _("failed to deliver signal %d to job "SFN" for "SFN" (shepherd with pid %d): "SFN))
#define MSG_JOB_INITMIGRSUSPJ_UU      _MESSAGE(29155, _("initiate migration at job suspend for job "U32CFormat" task "U32CFormat""))



/*
** dispatcher.c
*/
#define MSG_COM_NOCONNECT             _MESSAGE(29156, _("can't connect to commd"))
#define MSG_COM_RECONNECT             _MESSAGE(29157, _("can connect to commd again"))
#define MSG_COM_NORCVMSG_S            _MESSAGE(29158, _("error receiving message "SFN))
#define MSG_COM_NOACK_S               _MESSAGE(29159, _("error sending acknowledge: "SFN"\n"))
#define MSG_COM_INTERNALDISPATCHCALLWITHOUTDISPATCH _MESSAGE(29160, _("internal dispatcher called without s.th. to dispatch"))


/*
** sge_load_sensor.c
*/
#define MSG_LS_STOPLS_S               _MESSAGE(29161, _("stopping load sensor "SFN"\n"))
#define MSG_LS_STARTLS_S              _MESSAGE(29162, _("starting load sensor "SFN"\n"))
#define MSG_LS_RESTARTLS_S            _MESSAGE(29163, _("restarting load sensor "SFN"\n"))
#define MSG_LS_NORESTARTLS            _MESSAGE(29164, _("load sensor not restarted because load sensor file was not modified\n"))
#define MSG_LS_NOMODTIME_SS           _MESSAGE(29165, _("can't get mod_time from load sensor file "SFN": "SFN"\n"))
#define MSG_LS_FORMAT_ERROR_SS        _MESSAGE(29166, _("Format error of loadsensor "SFQ". Received: \"%100s\""))



/*
** execd.c
*/
#define MSG_EXECD_PROGINVALIDNAME_S   _MESSAGE(29167, _("program called with invalid name: "SFQ"\n"))
#define MSG_FILE_REDIRECTFD_I         _MESSAGE(29168, _("can't redirect file descriptor #%d\n"))
#define MSG_EXECD_NOPROGNAMEPROD_S    _MESSAGE(29169, _("program name "SFQ" does not match product mode") ) 
#define MSG_COM_CANTSTARTCOMMD        _MESSAGE(29170, _("can't start commd"))
#define MSG_COM_CANTENROLL2COMMD_S    _MESSAGE(29171, _("can't enroll to commd: "SFN))
#define MSG_EXECD_NOSTARTPTF          _MESSAGE(29172, _("could not start priority translation facility (ptf)"))
#define MSG_EXECD_STARTPDCANDPTF      _MESSAGE(29173, _("successfully started PDC and PTF"))
#define MSG_COM_RECEIVEREQUEST_S      _MESSAGE(29174, _("can't receive request: "SFN))
#define MSG_COM_CANTREGISTER_S        _MESSAGE(29175, _("can't register at \"qmaster\": "SFN))
#define MSG_COM_ERROR                 _MESSAGE(29176, _("communication error"))
#define MSG_COM_REGISTERDENIED_S      _MESSAGE(29177, _("registration at \"qmaster\" was denied: "SFN))
#define MSG_PARSE_INVALIDARG_S        _MESSAGE(29178, _("invalid command line argument "SFQ"\n"))
#define MSG_PARSE_TOOMANYARGS         _MESSAGE(29179, _("too many command line options\n"))

#define MSG_EXECD_INITPACKBUFFERFAILED_S _MESSAGE(29181, _("init_packbuffer failed: "SFN"\n"))

/*
 *
 */
#define MSG_EXECD_CMDNEXISTS          _MESSAGE(29182, _("The given command does not exist"))

#endif /* __MSG_EXECD_H */

