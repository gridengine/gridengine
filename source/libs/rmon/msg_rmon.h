#ifndef __MSG_RMON_H
#define __MSG_RMON_H
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
** rmon/src/rmon_c_c_client_register.c 
*/
#define MSG_RMON_NORMONDAVAILABLE               "no rmond available\n"
#define MSG_RMON_UNEXPECTEDREQUESTSTATUSX_D     "unexpected request status: ["U32CFormat"]\n"

/* 
** rmon/src/rmon_c_c_flush.c 
*/
#define MSG_RMON_THISRMONNRXWASQUITTEDBYRMOND_D "This rmon (Nr. "U32CFormat") was quitted by rmond\n"
#define MSG_RMON_UNEXPECTEDPROTOCOLVALUEX_D     "unexpected protocol value ["U32CFormat"]\n"
#define MSG_RMON_MALLOCFAILED                   "malloc failed!\n"

/* 
** rmon/src/rmon_c_c_monitoring_level.c 
*/
#define MSG_RMON_UNKNOWNSPY                     "unknown spy !\n"
#define MSG_RMON_UNKNOWNRMON                    "unknown rmon !\n"
#define MSG_RMON_VERYHEAVYERROR                 "very heavy error\n"
#define MSG_RMON_ILLEGALMONITORINGLEVEL         "illegal monitoring level\n"
#define MSG_RMON_NEWTRANSITIONLISTELEMENT       "new transition list element\n"
#define MSG_RMON_REMOVEDTRANSITIONLISTELEMENT   "removed transition list element\n"
#define MSG_RMON_ALTEREDTRANSITIONLISTELEMENT   "altered transition list element\n"
#define MSG_RMON_NEWWAITLISTELEMENT             "new wait list element\n"
#define MSG_RMON_REMOVEDWAITLISTELEMENT         "removed wait list element\n"
#define MSG_RMON_ALTEREDWAITINGLISTELEMENT      "altered waiting list element\n"

/* 
** rmon/src/rmon_conf.c 
*/
#define MSG_RMON_CANNOTOPENXFORCONFIGDATAEXIT_S "error: cannot open %s for configuration data. Exit.\n"
#define MSG_RMON_ERRORINXLINEYCHARACTERZ_SII    "Error in %s, Line %d, Character %d.\n"
#define MSG_RMON_UNKNOWNCONFIGURATIONTYPEX_S    "Unknown configuration type %s.\n"
#define MSG_RMON_CANTFINDRMONDHOSTNAMEINXEXIT_S "Can't find RMOND_HOST_NAME in %s. Exit.\n"
#define MSG_RMON_HOSTNAMEXCOULDNOTBERESOLVED_S  "hostname: %s could not be resolved\n"


/* 
** rmon/src/rmon_connect.c 
*/
#define MSG_RMON_XBADSERVICE_S                  "%s: Bad service?\n"

/* 
** rmon/src/rmon_daemon.c 
*/
#define MSG_RMON_DAEMONUNABLETOSYSCONFSCOPENMAX "rmon_daemon(): unable to sysconf(_SC_OPEN_MAX)"

/* 
** rmon/src/rmon_get_stat.c
*/
#define MSG_RMON_GOTNOACK                       "Got 'not acknowledge' !\n"


/* 
** rmon/src/rmon_knecht_light.c
*/
#define MSG_RMON_STARTINGJOBX_D                 "starting job "U32CFormat"\n"
#define MSG_RMON_JOBXHASSTOPPED_D               "job "U32CFormat" has stopped\n"

/* 
** rmon/src/rmon_m_c_mconf.c
*/
#define MSG_RMON_CANTESTABLISCONNECTION         "Can't establish connection. \n"
#define MSG_RMON_ERRORINCONFTYPE                "Error in conf-type. \n"

/* 
** rmon/src/rmon_m_c_mdel.c
*/
#define MSG_RMON_UNEXPECTEDREQUESTTYPE          "Unexpected requesttype !!\n"

/* 
** rmon/src/rmon_m_c_mjob.c
*/
#define MSG_RMON_CANTRECEIVEJOBLISTTOADD        "cannot receive job list to add\n"
#define MSG_RMON_CANTRECEIVEJOBLISTTODELETE     "cannot receive job list to delete\n"
#define MSG_RMON_CORRUPTDATASTRUCTURES          "corrupt data structure\n"

/* 
** rmon/src/rmon_m_c_monitoring_level.c
*/
#define MSG_RMON_CANTSENDJOBLIST                "cannot send job list\n"

/* 
** rmon/src/rmon_m_c_mquit.c
*/
#define MSG_RMON_CANTSAVERESTARTFILE            "cannot save restart file\n"
#define MSG_RMON_CANTLOCKRESTARTFILE            "error: cannot lock restart file \n"
#define MSG_RMON_CANTUNLOCKRESTARTFILE          "error: cannot unlock restart file \n"
#define MSG_RMON_CANTSAVECONFFILE               "cannot save conf file\n"
#define MSG_RMON_SHUTDOWNALLREACHABLESPYSQUIT   "Shut down all reachable Spys. Quitting !\n"

/* 
** rmon/src/rmon_m_c_mstat.c
*/
#define MSG_RMON_UNABLETOSENDACKNOWLEDGE        "Unable to send acknowledge !\n"

/* 
** rmon/src/rmon_macros.c
*/
#define MSG_RMON_INVALIDERRNO                   "invalid errno"
#define MSG_RMON_DEBUGLAZERSAREINCONSISTENT     "mpush_layer: debug layers are inconsistent\n"
#define MSG_RMON_TRIEDTOSETTOLARGELAYER         "mpush_layer: tried to set too large layer\n"
#define MSG_RMON_TRIEDTOPOPHIGHESTLAYER         "mpop_layer: tried to pop highest layer\n"
#define MSG_RMON_MKSTDNAMEUNABLETOGETMYHOSTNAME_S  "mkstdname: unable to get my hostname: %s\n"
#define MSG_RMON_MKSTDNAMESTRINGTOOLONG         "mkstdname: string too long\n"
#define MSG_RMON_ILLEGALDBUGLEVELFORMAT         "illegal debug level format\n"
#define MSG_RMON_UNABLETOOPENXFORWRITING_S      "unable to open %s for writing\n"
#define MSG_RMON_ERRNOXY_DS                     "    ERRNO: %d, %s\n"
#define MSG_RMON_XERRORINASHAREDMEMOPERATION_I  "(%d) Error in a Shared Memory Operation !\n"
#define MSG_RMON_XERRORINASEMAPHOREOPERATION_I  "(%d) Error in a Semaphore Operation !\n"
#define MSG_RMON_FILEXLINEY_SI                  "    File: %s, Line: %d\n"


/* 
** rmon/src/rmon_mconf.c
*/
#define MSG_RMON_CONFIGHASBINACCEPTED           "Configuration has been accepted.\n"
#define MSG_RMON_CONFIGHASNOTBENACCEPTED        "Configuration has not been accepted !\n"
#define MSG_RMON_CANTINTERPRETHOSTNAMEINETADDR  "Can`t interpret hostname/inet_addr !\n"
#define MSG_RMON_TOOFEWARGUMENTS                "Too few arguments !\n"
#define MSG_RMON_MCONF_USAGE                    "Usage:  mconf [-r rmond-host] type value\nwhere type is one of:\nm[essages]\n"

/* 
** rmon/src/rmon_mdel.c
*/
#define MSG_RMON_SPYNOTFOUND                    "Spy not found\n"
#define MSG_RMON_SPYDELETEDSUCCESSFULLY         "Spy deleted successfully !\n"
#define MSG_RMON_RMONNOTFOUND                   "rmon not found\n"
#define MSG_RMON_RMONHASBEENSUCCESFULLYDELETED  "rmon has been succesfully deleted !\n"
#define MSG_RMON_ONLYSPYORRMONCANBEDELETED      "Only a spy or a rmon can be delete, but not both !\n"
#define MSG_RMON_MDEL_USAGE                     "usage:  mdel [-r rmond-host] [-s programname] [-n rmon_number]\n"


/* 
** rmon/src/rmon_message_protocol.c
*/
#define MSG_RMON_UNEXPECTEDSTATUSVALUE          "unexpected status value\n"

/* 
** rmon/src/rmon_mjob.c
*/
#define MSG_RMON_OK                             "Ok.\n"
#define MSG_RMON_UNKNOWNOPTION                  "Unknown Option ! \n"
#define MSG_RMON_UNABLETOREADRMONNUMBER         "unable to read rmon-number\n"
#define MSG_RMON_UNABLETOREADJOBNUMB            "unable to read jobnumber\n"
#define MSG_RMON_MJOB_USAGE                     "usage:  mjob [-r rmond_host] rmon-number {-|+} {jobnumber [...] | all }\n"
#define MSG_RMON_UNNOWNCLIENT                   "Unknown Client !\n"
#define MSG_RMON_NOTACCEPTED                    "not accepted\n"
#define MSG_RMON_CANNOTSENDLISTOFJOBSTOADD      "cannot send list of jobs to add\n"
#define MSG_RMON_CANNOTSENDLISTOFJOBSTODEL      "cannot send list of jobs to delete\n"


/* 
** rmon/src/rmon_level.c
*/
#define MSG_RMON_CANTSENDMONITORINGLEVELTORMOND "Can't send monitoring level to rmond.\n"
#define MSG_RMON_UNABLETOREADMONITORINGLEVEL    "unable to read monitoring level\n"
#define MSG_RMON_MLEVEL_USAGE                   "usage:  mlevel [-r rmond-host] rmon-number programname monlev [-w]\n"


/* 
** rmon/src/rmon_mquit.c
*/
#define MSG_RMON_CANTPROPERLYSHUTDOWNMONSYSTEM  "Can't properly shut down the Monitoring-System !\n"
#define MSG_RMON_SHUTDWONMONITORINGSYSTEM       "Shut down Monitoring-System !\n"
#define MSG_RMON_MQUIT_USAGE                    "usage: mquit [-r rmond-host]\n"


/* 
** rmon/src/rmon_mstat.c
*/
#define MSG_RMON_NORMONCONNECTEDTORMOND         "no rmon connected to rmond\n"
#define MSG_RMON_RMONWAITACTIVEJOB              "RMON WAIT ACTIV JOB\n"
#define MSG_RMON_NOSUCHRMON                     "no such rmon\n"
#define MSG_RMON_MONITOREDJOBS                  "\nMonitored jobs:\n"
#define MSG_RMON_NOJOBISCURRENTLYMONITORED      "*** no job is currently monitored ***\n"
#define MSG_RMON_NOPROGISCURRENTLYMONITORED     "*** no program is currently monitored ***\n"
#define MSG_RMON_ALLJOBS                        "all jobs\n"
#define MSG_RMON_MONITOREDPROGS                 "\nMonitored programs:\n"
#define MSG_RMON_STATUSPROGNAME                 "STATUS PROGAMNAME             LVL\n"
#define MSG_RMON_NOSPYCONNECTEDTORMOND          "no spy connected to rmond\n"
#define MSG_RMON_PROGNAMESTATUSRMONJOB          "PROGRAMNAME                    STATUS RMON JOB\n"
#define MSG_RMON_NOSUCHSPY                      "no such spy\n"
#define MSG_RMON_COMBINEDLEVELS                 "combined level(s): "
#define MSG_RMON_PROGNAME_S                     "programname: %s\n"
#define MSG_RMON_LEVEL                          "level: "
#define MSG_RMON_CANTGIVEINFOTOSPYANDRMONBOTH   "Can't give infos to both spy and rmon at the same time !\n"
#define MSG_RMON_MSTAT_USAGE                    "usage:  mstat [-r rmond-host] {-n [rmon-number] | -s [programname] }\n"

/* 
** rmon/src/rmon_msysstat.c
*/
#define MSG_RMON_ALL                            "all\n"
#define MSG_RMON_RMONLIST                       "rmon_list\n"
#define MSG_RMON_WAITLIST                       "wait_list\n"
#define MSG_RMON_SPYLIST                        "spy_list\n"
#define MSG_RMON_TRANSITIONLIST                 "transition_list\n"
#define MSG_RMON_GLOBALJOBLIST                  "global_job_list\n"
#define MSG_RMON_SPYTABLEHEADER                 "PROGRAMNAME            INTERNET ADDR     PORT    UID  CLD     LF LVL FIRST   LAST  \n"
#define MSG_RMON_NOENTRY                        "no entry\n"
#define MSG_RMON_CLIENTTABLEHEADER              "RMON   UID     LF   JOB-ID \n"
#define MSG_RMON_WAITTABLEHEADER                "PROGRAMNAME              RMON"
#define MSG_RMON_TRANSITIONTABLEHEADER          "  RMON PROGRAMNAME            LVL LASTRD\n"
#define MSG_RMON_MSYSSTAT_USAGE                 "Usage: msysstat [-r rmond-host]\n"


/* 
** rmon/src/rmon_restart.c
*/
#define MSG_RMON_CANTOPENXFORCONFIGDATA_S       "error: cannot open %s for configuration data.\n"
#define MSG_RMON_FORMATERRORINXLINYCANNOTREADINAD_SI  "Format error in %s, line %d, cannot read inet_addr\n"
#define MSG_RMON_FORMATERRORINXLINEY_SI         "Format error in %s, line %d\n"



/* 
** rmon/src/rmon_rmon.c
*/
#define MSG_RMON_CANTREGISTERATRMOND            "cannot register at rmond"
#define MSG_RMON_NUMBEROFTHISRMONX_D            "Number of this rmon: "U32CFormat"\n"
#define MSG_RMON_LEVEL_D                        "Level: %ld\n"
#define MSG_RMON_RMON_USAGE                     "usage:  rmon [-hpin] [-r rmond-host] [-f flush-intervall] [-m max_wait_time]\n             [programname monlev [-w]]\n"



/* 
** rmon/src/rmon_rmonid.c
*/
#define MSG_RMON_XISNOTTHERMONDHOST_S           "\n%s is not the rmond-Host !\n"
#define MSG_RMON_RMONDHOSTISX_S                 "rmond-Host is %s\n"
#define MSG_RMON_ACCEPTERRORX_S                 "accept error: %s"
#define MSG_RMON_ERRORREADINGREQUEST            "error reading request"
#define MSG_RMON_SHUTERDOWNUNABLETOSYSCONFSCOPENMAX   "shut_er_down: unable to sysconf(_SC_OPEN_MAX)"


/* 
** rmon/src/rmon_s_c_exit.c
*/
#define MSG_RMON_CANTLOADRESTARTFILEX_S         "can't load restart file: %s\n"
#define MSG_RMON_SPYALREADYDELETEDFROMSPYLIST   "Spy was already deleted from Spy-List (restart file)!\n"
#define MSG_RMON_CANTSAVERESTARTFILEX_S         "can't save restart file: %s\n"

/* 
** rmon/src/rmon_s_c_mconf.c
*/
#define MSG_RMON_MAXMESSAGESARE_D               "max_messages = "U32CFormat"\n"

/* 
** rmon/src/rmon_s_c_sleep.c
*/
#define MSG_RMON_ALREADYSLEEPING                "Hey, I'm already sleeping.\n"
#define MSG_RMON_SEMWAITERROR                   "sem_wait error \n"
#define MSG_RMON_SEMSIGNALERROR                 " sem_signal error \n"

/* 
** rmon/src/rmon_s_c_spy_register.c
*/
#define MSG_RMON_SPYTRIEDTOREGISTERWICE         "spy tried to register twice\n"

/* 
** rmon/src/rmon_s_c_wake_up.c
*/
#define MSG_RMON_NOTSLEEPING                    "Hey, I'm not sleeping.\n"
#define MSG_RMON_CANTREGISTERTOAFTERWAKEUP      "cannot register to after wake up.\n"


/* 
** rmon/src/rmon_semaph.c
*/
#define MSG_RMON_CANTGETVAL                     "can't GETVAL\n"
#define MSG_RMON_CANSETVAL0                     "can SETVAL[0]\n"
#define MSG_RMON_CANSETVAL1                     "can SETVAL[1]\n"
#define MSG_RMON_CANTENDCREATE                  "can't end create\n"
#define MSG_RMON_CANTOPEN                       "can't open\n"
#define MSG_RMON_CANTIPCRMID                    "can't IPC_RMID\n"
#define MSG_RMON_CANTSEMOP                      "can't semop\n"
#define MSG_RMON_SEM1GRTBIGCOUNT                "sem[1] > BIGCOUNT\n"
#define MSG_RMON_CANTUNLOCK                     "can't unlock\n"
#define MSG_RMON_CANTHAVVALUEIS0                "can't have value == 0\n"
#define MSG_RMON_RMONSEMOPIDXPIDYERRORZ_IIS     "rmon_sem_op(id = %d, pid = %d) error: %s\n"

/* 
** rmon/src/rmon_server.c
*/
#define MSG_RMON_SOCKETCREATIONERROR               "socket creation error\n"
#define MSG_RMON_SOCKETOPERATIONFAILUREX_S         "socket operation failure: %s\n"
#define MSG_RMON_BINDERRORPORTXISALREADYINUSE_D    "Bind error: Port "U32CFormat" is already in use !\n"
#define MSG_RMON_BINDERROR                         "Bind error !\n"
#define MSG_RMON_LISTENFAILURE_S                   "listen failure: %s\n"


/* 
** rmon/src/rmon_siginit.c
*/
#define MSG_RMON_ALARMCLOCK                        "===================>>> ALARMCLOCK \n"


/* 
** rmon/src/rmon_spy.c
*/
#define MSG_RMON_UNABLETOGETMYHOSTNAMEX_S       "unable to get my hostname: %s\n"
#define MSG_RMON_UNABLETORESOLVEMYHOSTNAMEXY_SS "unable to resolve my hostname %s: %s\n"
#define MSG_RMON_CANTOPENPIPESX_S               "cannot open pipes: %s\n"
#define MSG_RMON_FCNTLFAILURE_S                 "fcntl() fails: %s\n"
#define MSG_RMON_SHMGETFAILED_S                 "shmget() failed: %s\n"
#define MSG_RMON_SHMATFAILED_S                  "shmat() failed: %s\n"
#define MSG_RMON_NEEDMORESEMAPHORES             "Need more semaphores\n"
#define MSG_RMON_RMONSEMCREATEFAILED_S          "rmon_sem_create() failed: %s\n"
#define MSG_RMON_CANTPUTENV_S                   "cannot putenv: %s\n"
#define MSG_RMON_UNABLETOSYSCONFSCOPENMAX       "unable to sysconf(_SC_OPEN_MAX)"
#define MSG_RMON_RMONSEMCREATEFAILED            "rmon_sem_create() failed\n"
#define MSG_RMON_CANTRMONSEMWAIT1               "Can't rmon_sem_wait(1)\n"
#define MSG_RMON_CANTRMONSEMSIGNAL1             "Can't rmon_sem_signal(1)\n"
#define MSG_RMON_CANTRMONSEMSIGNAL2             "Can't rmon_sem_signal(2)\n"
#define MSG_RMON_CANTRMONSEMCLOSE1              "Can't rmon_sem_close(1)\n"
#define MSG_RMON_CANTEXECUTEPROGXY_SS           "cannot execute the program %s: %s\n."
#define MSG_RMON_ERRORINSHMDT_S                 "ERROR in shmdt(): %s\n"
#define MSG_RMON_ERRORINSHMCTL_S                "ERROR in shmctl(): %s\n"
#define MSG_RMON_SPY_USAGE                      "Usage: spy [-r rmond-host] programname [parameters]\n"


/* 
** rmon/src/rmon_transition_list.c
*/
#define MSG_RMON_INVALIDNEWTRANSITION           "invalid new transition\n"
#define MSG_RMON_INVALIDTRANSITIONLIST          "invalid transition list\n"



#endif /* __MSG_RMON_H   */
