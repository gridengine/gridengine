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
#define MSG_COM_ACK_UNKNOWN          _("unknown ack event\n")


/*
** tmpdir.c
*/
#define MSG_FILE_RECURSIVERMDIR_SS       _("recursive rmdir(%s): %s\n")

/*
** setup_execd.c
*/
#define MSG_JOB_XREGISTERINGJOBYATPTFDURINGSTARTUP_SU    _("%s registering job \""U32CFormat"\" at ptf during startup")
#define MSG_FAILED    _("failed")
#define MSG_DELAYED    _("delayed")
#define MSG_JOB_XREGISTERINGJOBYTASKZATPTFDURINGSTARTUP_SUS    _("%s registering job \""U32CFormat"\" task %s at ptf during startup")
#define MSG_ANSWER_KEEPINGCHANNELFDXOPEN_I    _("keeping channel fd #%d open\n")
#define MSG_SGETEXT_NOCOMMD                     _("can't connect commd.\n")

/*
** reaper_execd.c
*/
#define MSG_SLAVE    _("slave ")
#define MSG_COREDUMPED    _("(core dumped) ")
#define MSG_WAITPIDNOSIGNOEXIT_PI   _("waitpid() returned for pid "pid_t_fmt" status %d unequal WIFSIGNALED/WIFEXITED\n")
#define MSG_SHEPHERD_VSHEPHERDOFJOBWXDIEDTHROUGHSIGNALYZ_SUUSI    _("%sshepherd of job "U32CFormat"."U32CFormat" died through signal %s= %d")
#define MSG_SHEPHERD_WSHEPHERDOFJOBXYEXITEDWITHSTATUSZ_SUUI    _("%sshepherd of job "U32CFormat"."U32CFormat" exited with exit status = %d\n")
#define MSG_JOB_MISSINGJOBXYINJOBREPORTFOREXITINGJOBADDINGIT_UU    _("missing job "U32CFormat"."U32CFormat" in job report for exiting job - adding it")
#define MSG_STATUS_LOADSENSORDIEDWITHSIGNALXY_SI    _("load sensor died through signal %s= %d")
#define MSG_STATUS_LOADSENSOREXITEDWITHEXITSTATUS_I    _("load sensor exited with exit status = %d\n")
#define MSG_STATUS_MAILERDIEDTHROUGHSIGNALXY_SI    _("mailer died through signal %s = %d")
#define MSG_STATUS_MAILEREXITEDWITHEXITSTATUS_I    _("mailer exited with exit status = %d\n")
#define MSG_JOB_REAPINGJOBXPTFCOMPLAINSY_US    _("reaping job \""U32CFormat"\" ptf complains: %s")
#define MSG_JOB_CLEANUPJOBCALLEDWITHINVALIDPARAMETERS    _("clean_up_job() called with invalid parameters")
#define MSG_JOB_CANTFINDDIRXFORREAPINGJOBYZ_SS _("can't find directory "SFN" for reaping job "SFN"\n")
#define MSG_JOB_CANTREADCONFIGFILEFORJOBXY_S    _("can't read config file for job "SFN"\n")
#define MSG_STATUS_ABNORMALTERMINATIONOFSHEPHERDFORJOBXY_S    _("abnormal termination of shepherd for job "SFN": no \"exit_status\" file")
#define MSG_STATUS_ABNORMALTERMINATIONFOSHEPHERDFORJOBXYEXITSTATEFILEISEMPTY_S    _("abnormal termination of shepherd for job "SFN": \"exit_status\" file is empty")
#define MSG_SHEPHERD_DIEDTHROUGHSIGNAL    _("shepherd died through signal")
#define MSG_SHEPHERD_NOPIDFILE    _("no \"pid\" file for shepherd")
#define MSG_SHEPHERD_EXITEDWISSTATUS_I    _("shepherd exited with exit status %d")
#define MSG_JOB_CANTREADERRORFILEFORJOBXY_S    _("can't read error file for job "SFN"\n")
#define MSG_JOB_CANTREADUSAGEFILEFORJOBXY_S    _("can't read usage file for job "SFN"\n")
#define MSG_JOB_WXDIEDTHROUGHSIGNALYZ_SSI    _("job "SFN" died through signal %s (%d)")
#define MSG_JOB_CANTREADUSEDRESOURCESFORJOB    _("can't read used resources for job")
#define MSG_JOB_CANTOPENJOBPIDFILEFORJOBXY_S    _("can't open \"job_pid\" file for job "SFN"\n")
#define MSG_SHEPHERD_REMOVEACKEDJOBEXITCALLEDWITHX_U    _("remove_acked_job_exit called with "U32CFormat".0")
#define MSG_JOB_XYHASNOTASKZ_UUS    _("job "U32CFormat"."U32CFormat" has no task \"%s\"")
#define MSG_SHEPHERD_CANTSTARTXFORJOBY_SU    _("can't start command \"%s\" for job " U32CFormat " to delete credentials")
#define MSG_SHEPHERD_CANTDELCREDENTIALSFORJOBXCOMMANDYFAILEDWITHCODEZ_USI    _("could not delete credentials for job " U32CFormat" - command \"%s\" failed with return code %d")
#define MSG_SHEPHERD_CANTDELCREDENTIALSFORJOBXYBINARYNOTEXIST_US    _("could not delete credentials for job " U32CFormat" - %s binary does not exist")
#define MSG_FILE_CANTREMOVEDIRECTORY_SS    _("can't remove directory \"%s\": %s")
#define MSG_SHEPHERD_ACKNOWLEDGEFORUNKNOWNJOBXYZ_UUS    _("acknowledge for unknown job "U32CFormat"."U32CFormat"/%s")
#define MSG_SHEPHERD_ACKNOWLEDGEFORUNKNOWNJOBEXIT    _("acknowledge for unknown job's exit")
#define MSG_SHEPHERD_CANTFINDACTIVEJOBSDIRXFORREAPINGJOBY_SU    _("can't find active jobs directory \"%s\" for reaping job "U32CFormat )
#define MSG_SHEPHERD_INCORRECTCONFIGFILEFORJOBXY_UU    _("incorrect config file for job "U32CFormat"."U32CFormat"")
#define MSG_SHEPHERD_CANTSTARTJOBXY_US     _("can't start job \""U32CFormat"\": %s")
#define MSG_SHEPHERD_PROBLEMSAFTERSTART_DS _("problems after job start \""U32CFormat"\": %s")
#define MSG_SHEPHERD_JATASKXYISKNOWNREPORTINGITTOQMASTER    _("ja-task \"" U32CFormat"."U32CFormat"\" is unknown - reporting it to qmaster")
#define MSG_SHEPHERD_CKECKINGFOROLDJOBS    _("checking for old jobs")
#define MSG_SHEPHERD_NOOLDJOBSATSTARTUP    _("no old jobs at startup")
#define MSG_SHEPHERD_CANTGETPROCESSESFROMPSCOMMAND    _("can't get processes from ps command")
#define MSG_SHEPHERD_XISNOTAJOBDIRECTORY_S    _("\"%s\" is not a job directory")
#define MSG_SHEPHERD_FOUNDACTIVEJOBDIRXWHILEMISSINGJOBDIRREMOVING_S    _("found active job directory \"%s\" while missing job directory - removing")
#define MSG_SHEPHERD_CANTSTATXY_SS    _("can't stat \"%s\": %s")
#define MSG_FILE_XISNOTADIRECTORY_S    _("\"%s\" is not a directory")
#define MSG_SHEPHERD_FOUNDDIROFJOBX_S    _("found directory of job \"%s\"")
#define MSG_SHEPHERD_CANTREADPIDFILEXFORJOBYSTARTTIMEZX_SSUS    _("can't read pid file \"%s\" of shepherd for job \"%s\" - starttime: "U32CFormat" cleaning up: %s")
#define MSG_SHEPHERD_MISSINGJOBXINJOBREPORTFOREXITINGJOB_U    _("missing job \""U32CFormat"\" in job report for exiting job")
#define MSG_SHEPHERD_CANTREADPIDFROMPIDFILEXFORJOBY_SS    _("can't read pid from pid file \"%s\" of shepherd for job %s")
#define MSG_SHEPHERD_SHEPHERDFORJOBXHASPIDYANDISZALIVE_SUS    _("shepherd for job %s has pid \""pid_t_fmt"\" and is %s alive\n")
#define MSG_NOT    _("not")
#define MSG_SHEPHERD_INCONSISTENTDATAFORJOBX_U    _("inconsistent data for job \""U32CFormat"\"")
#define MSG_SHEPHERD_MISSINGJOBXYINJOBREPORT_UU    _("Missing job "U32CFormat"."U32CFormat" in job report")
#define MSG_SHEPHERD_CANTOPENPIDFILEXFORJOBYZ_SUU    _("can't open pid file \"%s\" for job "U32CFormat"."U32CFormat)
#define MSG_SHEPHERD_CANTOPENUSAGEFILEXFORJOBYZX_SUUS    _("can't open usage file \"%s\" for job "U32CFormat"."U32CFormat": %s")
#define MSG_SHEPHERD_EXECDWENTDOWNDURINGJOBSTART _("execd went down during job start")
#define MSG_SHEPHERD_REPORTINGJOBFINSIHTOQMASTER _(" reporting job finish to qmaster")
#define MSG_SHEPHERD_GOTACKFORJOBEXIT              _(" got ack for job exit")
#define MSG_SHEPHERD_REPORINGJOBSTARTFAILURETOQMASTER _(" reporting job start failure to qmaster")
#define MSG_SHEPHERD_REPORINGJOBPROBLEMTOQMASTER _(" reporting problems with running job to qmaster")
#define MSG_SHEPHERD_ACKINGUNKNWONJOB                 _(" ack'ing unknown job")
#define MSG_JR_ERRSTR_EXECDDONTKNOWJOB _("execd doesn't know this job")
#define MSG_EXECD_GOTACKFORPETASKBUTISNOTINSTATEEXITING_S _("get exit ack for pe task %s but task is not in state exiting")


/*
** ptf.c
*/
#define MSG_SYSTEM_SYSINFO_SI_RELEASE_CALL_FAILED_S        _("sysinfo(SI_RELEASE) call failed - %s")
#define MSG_SCHEDD_JOBXPIDYSCHEDCTLFAILUREX_UUS            _("job "U32CFormat" pid "U32CFormat" schedctl failure: %s\n")
#define MSG_PRIO_JOBXPIDYSETPRIORITYFAILURE_UUS            _("job "U32CFormat" pid "U32CFormat" setpriority failure: %s\n")
#define MSG_SCHEDD_JOBXPIDYSCHEDSETSCHEDULERFAILURE_UUS    _("job "U32CFormat" pid "U32CFormat" sched_setscheduler failure: %s\n")
#define MSG_PRIO_JOBXNICEMFAILURE_S                        _("job "U32CFormat" nicem failure: %s\n")
#define MSG_PRIO_JOBXNICEJFAILURE_S                        _("job "U32CFormat" nicej failure: %s\n")
#define MSG_PRIO_JOBXSETPRIORITYFAILURE_US                 _("job "U32CFormat" setpriority failure: %s\n")
#define MSG_WHERE_FAILEDTOBUILDWHERECONDITION              _("failed to build where-condition")
#define MSG_PRIO_PTFMINMAX_II                              _("PTF_MAX_PRIORITY=%d, PTF_MIN_PRIORITY=%d\n")
#define MSG_PRIO_NICEMFAILED_S                             _("nicem failed: %s\n")
#define MSG_PRIO_SETPRIOFAILED_S                           _("setpriority failed: %s\n")
#define MSG_ERROR_UNKNOWNERRORCODE                         _("Unknown error code")
#define MSG_ERROR_NOERROROCCURED                           _("No error occurred")
#define MSG_ERROR_INVALIDARGUMENT                          _("Invalid argument")
#define MSG_ERROR_JOBDOESNOTEXIST                          _("Job does not exist")
#define MSG_JOB_THEASHFORJOBXISY_UX                        _("The ASH for job "U32CFormat" is "u64"\n")
#define MSG_JOB_MYASHIS_X                                  _("My ash is "u64"\n")
#define MSG_ERROR_UNABLETODUMPJOBUSAGELIST                 _("Unable to dump job usage list\n")



/*
** src/load_avg.c
*/
#define MSG_LOAD_NOMEMINDICES          _("failed retrieving memory indices\n")
#define MSG_LOAD_NOPTFUSAGE_S          _("ptf failed to determine job usage: %s")
#define MSG_SGETEXT_NO_LOAD                           _("can't get load values\n")



/*
** job_report_execd.c
*/
#define MSG_JOB_TYPEMALLOC            _("runtime type error or malloc failure in add_job_report")
#define MSG_PARSE_USAGEATTR_SSU       _("failed parsing \"%s\" passed as usage attribute \"%s\" of job "U32CFormat)      


/*
** get_path.c
*/
#define MSG_EXECD_INVALIDUSERNAME_S   _("invalid user name \"%s\"\n")
#define MSG_EXECD_NOHOMEDIR_S         _("missing home directory for user \"%s\"\n")
#define MSG_BUFFEROFSIZETOOSMALLFOR_DS _("buffer of size %d is too small for path "SFN"...\n")


/*
** execd_ticket.c
*/
#define MSG_JOB_TICKETFORMAT          _("format error in ticket request")
#define MSG_JOB_TICKETPASS2PTF_IS     _("passing %d new tickets ptf complains: %s")


/*
** exec_job.c
*/
#define MSG_FILE_RMDIR_SS             _("can't remove directory \"%s\": %s\n")
#define MSG_FILE_CREATEDIRDEL_SS      _("can't create directory %s after deletion: %s")
#define MSG_FILE_CREATEDIR_SS         _("can't create directory %s: %s")
#define MSG_EXECD_NOSGID              _("supplementary group ids could not be found in /proc")
#define MSG_EXECD_NOPARSEGIDRANGE     _("can not parse gid_range")
#define MSG_EXECD_NOADDGID            _("can not find an unused add_grp_id")
#define MSG_MAIL_MAILLISTTOOLONG_U    _("maillist for job " U32CFormat " too long\n")
#define MSG_EXECD_NOXTERM             _("unable to find xterm executable for interactive job, not configured\n")
#define MSG_EXECD_NODISPLAY           _("no DISPLAY variable found with interactive job")
#define MSG_EXECD_EMPTYDISPLAY        _("empty DISPLAY variable delivered with interactive job")
#define MSG_EXECD_NOSHEPHERD_SSS      _("unable to find shepherd executable neither in architecture directory %s nor in %s: %s")
#define MSG_EXECD_NOSHEPHERDWRAP_SS   _("unable to find shepherd wrapper command %s: %s")
#define MSG_DCE_NOSHEPHERDWRAP_SS     _("unable to find DCE shepherd wrapper command %s: %s")
#define MSG_EXECD_NOCOSHEPHERD_SSS    _("unable to find coshepherd executable neither in architecture directory %s nor in %s: %s")
#define MSG_EXECD_AFSCONFINCOMPLETE   _("incomplete AFS configuration - set_token_cmd and token_extend_time must be configured")
#define MSG_EXECD_NOCREATETOKENFILE_S _("can't create token file: %s")
#define MSG_EXECD_TOKENZERO           _("AFS token does not exist or has zero length")
#define MSG_EXECD_NOWRITETOKEN_S      _("can't write token to token file: %s")
#define MSG_MAIL_STARTSUBJECT_UUS     "Job-array task "U32CFormat"."U32CFormat" ("SFN") Started" 
#define MSG_MAIL_STARTBODY_UUSSSSS    _("Job-array task "U32CFormat"."U32CFormat" ("SFN") Started\n User       = %s\n Queue      = %s\n Host       = %s\n Start Time = %s") 
#define MSG_MAIL_STARTSUBJECT_US      "Job "U32CFormat" ("SFN") Started"
#define MSG_MAIL_STARTBODY_USSSSS     _("Job "U32CFormat" ("SFN") Started\n User       = %s\n Queue      = %s\n Host       = %s\n Start Time = %s") 
#define MSG_FILE_CHDIR_SS             _("can't change dir to %s: %s")
#define MSG_EXECD_NOFORK_S            _("fork failed: %s")
#define MSG_EXECD_NOSTARTSHEPHERD     _("unable to start shepherd process")
#define MSG_EXECD_NOWHERE4GDIL_S        _("cannot build where for %s\n")
#define MSG_SYSTEM_CANTMAKETMPDIR     _("can't make tmpdir")
#define MSG_SYSTEM_CANTGETTMPDIR      _("can't get tmpdir")
#define MSG_EXECD_UNABLETOFINDSCRIPTFILE_SS  _("unable to find script file %s: %s")
#define MSG_EXECD_NEEDATTRXINUSERDEFCOMPLOFYQUEUES_SS _("need attribute \"%s\" in userdefined complex of \"%s\"-queues\n")

/*
** execd_ck_to_do.c
*/
#define MSG_JOB_EXCEEDHLIM_USSFF      _("job "U32CFormat" exceeds job hard limit \"%s\" of queue \"%s\" (%8.5f > limit:%8.5f) - sending SIGKILL\n")
#define MSG_JOB_EXCEEDSLIM_USSFF      _("job "U32CFormat" exceeds job soft limit \"%s\" of queue \"%s\" (%8.5f > limit:%8.5f) - sending SIGXCPU\n")
#define MSG_EXECD_EXCEEDHWALLCLOCK_UU _("job "U32CFormat"."U32CFormat" exceeded hard wallclock time - initiate terminate method")
#define MSG_EXECD_EXCEEDSWALLCLOCK_UU _("job "U32CFormat"."U32CFormat" exceeded soft wallclock time - initiate soft notify method")
#define MSG_EXECD_NOADDGIDOPEN_SSS   _("failed opening addgrpid file %s of job %s: %s")
#define MSG_JOB_NOREGISTERPTF_SS     _("failed registering job %s at ptf: %s")
#define MSG_EXECD_NOOSJOBIDOPEN_SSS  _("failed opening os jobid file %s of job %s: %s")
#define MSG_EXECD_NOOSJOBIDREAD_SUUS  _("failed reading os jobid file %s of job "U32CFormat"."U32CFormat": %s")
/*
** execd_job_exec.c
*/
#define MSG_COM_UNPACKFEATURESET      _("unpacking featureset from job execution message\n")
#define MSG_COM_UNPACKJOB             _("unpacking job from job execution message\n")
#define MSG_COM_RECEIVED              _("received")
#define MSG_COM_UNPACKINGQ            _("unpacking queue list from job execution message\n")
#define MSG_JOB_MISSINGQINGDIL_SU     _("missing queue \"%s\" found in gdil of job "U32CFormat"\n")
#define MSG_EXECD_NOWRITESCRIPT_SIUS  _("can't write script file \"%s\" wrote only %d of "U32CFormat" bytes: %s")
#define MSG_JOB_TASKWITHOUTJOB_U      _("received task belongs to job "U32CFormat" but this job is not here\n")
#define MSG_JOB_TASKNOTASKINJOB_UU    _("received task belongs to job "U32CFormat" but this job is here but the JobArray task "U32CFormat" is not here\n")
#define MSG_JOB_TASKNOSUITABLEJOB_U   _("received task belongs to job "U32CFormat" but this job is not suited for starting tasks\n")
#define MSG_JOB_TASKALREADYEXISTS_US  _("received task "U32CFormat"/%s which is already here\n")
#define MSG_JOB_NOTASKPASSINGIF_SU    _("%s does not fulfill task passing interface for job "U32CFormat"\n")
#define MSG_JOB_NOFREEQ_USSS          _("no free queue for job "U32CFormat" of user %s@%s (localhost = %s)\n")
#define MSG_JOB_NOSUCHQ_SUSS          _("no such queue \"%s\" as requested by job "U32CFormat" from user %s@%s\n")
#define MSG_JOB_NOREQQONHOST_SSS      _("requested queue \"%s\" resides not at this host %s but at host %s\n")
#define MSG_JOB_REQQFULL_SII          _("requested queue \"%s\" is already full (%d/%d)\n")
/*
** execd_kill_execd.c
*/
#define MSG_JOB_INITCKPTSHUTDOWN_U    _("initiate checkpoint at shutdown: job "U32CFormat"")
#define MSG_JOB_KILLSHUTDOWN_U        _("killing job at shutdown: job "U32CFormat"")

/*
** execd_signal_queue.c
*/
#define MSG_JOB_INITMIGRSUSPQ_U       _("initiate migration at queue suspend for job "U32CFormat)
#define MSG_JOB_SIGNALTASK_UUS        _("SIGNAL jid: " U32CFormat " jatask: " U32CFormat " signal: %s\n")
#define MSG_EXECD_WRITESIGNALFILE_S   _("error writing file %s for signal transfer to shepherd")
#define MSG_JOB_DELIVERSIGNAL_ISSIS   _("failed to deliver signal %d to job %s for %s (shepherd with pid %d): %s")
#define MSG_JOB_INITMIGRSUSPJ_UU      _("initiate migration at job suspend for job "U32CFormat" task "U32CFormat"")



/*
** dispatcher.c
*/
#define MSG_COM_NOCONNECT             _("can't connect to commd")
#define MSG_COM_RECONNECT             _("can connect to commd again")
#define MSG_COM_NORCVMSG_S            _("error receiving message %s")
#define MSG_COM_NOACK_S               _("error sending acknowledge: %s\n")
#define MSG_COM_INTERNALDISPATCHCALLWITHOUTDISPATCH _("internal dispatcher called without s.th. to dispatch")


/*
** sge_load_sensor.c
*/
#define MSG_LS_STOPLS_S               _("stopping load sensor %s\n")
#define MSG_LS_STARTLS_S              _("starting load sensor %s\n")
#define MSG_LS_RESTARTLS_S            _("restarting load sensor %s\n")
#define MSG_LS_NORESTARTLS            _("load sensor not restarted because load sensor file was not modified\n")
#define MSG_LS_NOMODTIME_SS           _("can't get mod_time from load sensor file %s: %s\n")
#define MSG_LS_FORMAT_ERROR_SS        _("Format error of loadsensor "SFQ". Received: \"%100s\"")



/*
** execd.c
*/
#define MSG_EXECD_PROGINVALIDNAME_S   _("program called with invalid name: '%s'\n")
#define MSG_FILE_REDIRECTFD_I         _("can't redirect file descriptor #%d\n")
#define MSG_EXECD_NOPROGNAMEPROD_S    _("program name '%s' does not match product mode") 
#define MSG_COM_CANTSTARTCOMMD        _("can't start commd")
#define MSG_COM_CANTENROLL2COMMD_S    _("can't enroll to commd: %s")
#define MSG_EXECD_NOSTARTPTF          _("could not start priority translation facility (ptf)")
#define MSG_EXECD_STARTPDCANDPTF      _("successfully started PDC and PTF")
#define MSG_COM_RECEIVEREQUEST_S      _("can't receive request: %s")
#define MSG_COM_CANTREGISTER_S        _("can't register at \"qmaster\": %s")
#define MSG_COM_ERROR                 _("communication error")
#define MSG_COM_REGISTERDENIED_S      _("registration at \"qmaster\" was denied: %s")
#define MSG_PARSE_INVALIDARG_S        _("invalid command line argument \"%s\"\n")
#define MSG_PARSE_TOOMANYARGS         _("too many command line options\n")
#define MSG_GDI_ENROLLTOCOMMDFAILED_S _("can't enroll to commd: %s\n")  

#endif /* __MSG_EXECD_H */

