#ifndef __MSG_SCHEDD_H
#define __MSG_SCHEDD_H
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
** schedd/sge_access_tree.c
*/ 
#define MSG_SCHEDD_NOPRIORITYINACCESSTREEFOUND_UU     _MESSAGE(47000, _("found no priority "U32CFormat" (job "U32CFormat") in access tree\n"))
#define MSG_SCHEDD_INCONSISTENTACCESSTREEDATA         _MESSAGE(47001, _("inconsistent access tree data"))

/* 
** schedd/sge_gdi_attributes.c
*/ 
#define MSG_NET_UNKNOWNHOSTNAME_S                     _MESSAGE(47002, _("unknown hostname "SFQ"\n"))
#define MSG_LIST_FAILEDRETRIEVINGEXECHOSTOBJ_S        _MESSAGE(47003, _("failed retrieving exec host object: "SFN"\n"))
#define MSG_LIST_FAILEDRETRIEVINGEXECHOSTOBJS_S       _MESSAGE(47004, _("failed retrieving exec host objects: "SFN"\n"))
#define MSG_LIST_FAILEDRETRIEVINGCOMPLEXLISTS_S       _MESSAGE(47005, _("failed retrieving complex lists: "SFN"\n"))
#define MSG_LIST_FAILEDRETRIEVINGJOB_US               _MESSAGE(47006, _("failed retrieving job "U32CFormat": "SFN"\n"))
#define MSG_LIST_FAILEDBUILDINGATTRIBUTESFORHOST_S    _MESSAGE(47007, _("failed building attributes for host "SFN"\n"))
#define MSG_ERROR_UNKNOWNREASON                       _MESSAGE(47008, _("unknown reason"))
#define MSG_LIST_NOEXECHOSTOBJECT_S                   _MESSAGE(47009, _("no such exec host object "SFQ"\n"))
#define MSG_LIST_NOATTRIBUTEXFORHOSTY_SS              _MESSAGE(47010, _("no attribute "SFQ" for host "SFQ"\n"))
#define MSG_ERROR_BADARGUMENT                         _MESSAGE(47011, _("bad argument\n"))


/* 
** schedd/sge_complex_schedd.c
*/ 
#define MSG_ATTRIB_ATTRIBUTEWITHOUTNAMEFOUND          _MESSAGE(47012, _("found attribute without name\n"))
#define MSG_ATTRIB_XINATTRIBLISTMISSING_SU            _MESSAGE(47013, _("missing "SFQ" in attribute list (layer = "U32CFormat")\n"))
#define MSG_ATTRIB_ACTUALELEMENTTOATTRIBXMISSING_S    _MESSAGE(47014, _("missing actual element to attrib "SFN"\n"))
#define MSG_POINTER_ASSTRISNULLFORATTRIBX_S           _MESSAGE(47015, _("\"as_str\" is NULL for attrib "SFQ"\n"))
#define MSG_ATTRIB_ATTRIBUTEXALLREADYINLIST_S         _MESSAGE(47016, _("expected attribute "SFQ" being already in attributes list\n"))
#define MSG_MEMORY_UNABLETOALLOCSPACEFORCOMPLEXBUILD  _MESSAGE(47017, _("unable to alloc space for building complexes\n"))
#define MSG_LIST_NOCOMPLEXXATTACHEDTOHOSTY_SS         _MESSAGE(47018, _("no such complex "SFQ" attached to host "SFQ"\n"))
#define MSG_LIST_NOCOMPLEXXATTACHEDTOQUEUEY_SS        _MESSAGE(47019, _("no such complex "SFQ" attached to queue "SFQ"\n"))
#define MSG_SCHEDD_LOADADJUSTMENTSVALUEXNOTNUMERIC_S  _MESSAGE(47020, _("load adjustments value "SFQ" must be of numeric type\n"))

/* 
** schedd/sge_job_schedd.c
*/ 
#define MSG_LOG_JOBSDROPPEDEXECUTIONTIMENOTREACHED    _MESSAGE(47021, _("jobs dropped because execution time not reached: "))
#define MSG_LOG_JOBSDROPPEDERRORSTATEREACHED          _MESSAGE(47022, _("jobs dropped because of error state: "))
#define MSG_LOG_JOBSDROPPEDBECAUSEOFXHOLD             _MESSAGE(47023, _("jobs dropped because of hold: "))
#define MSG_LOG_JOBSDROPPEDBECAUSEDEPENDENCIES        _MESSAGE(47024, _("jobs dropped because of job dependencies: "))
/* #define MSG_LOG_JOBSDROPPEDBECAUSEUSRGRPLIMIT         _message(47025, _("jobs dropped because of user limitations: ")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_EVENT_CKPTOBJXFORJOBYNOTFOUND_SI          _message(47026, _("can't find requested CKPT object "SFQ" for job %d\n")) __TS Removed automatically from testsuite!! TS__*/
#define MSG_ATTRIB_PARSINGATTRIBUTEXFAILED_SS         _MESSAGE(47027, _("failed parsing attribute "SFQ": "SFN"\n"))

/* 
** schedd/sge_schedd_text.c
*/ 
/* #define MSG_SGETEXT_INVALIDHOSTINQUEUE_S        _message(47028, _("invalid hostname "SFQ" associated with queue\n")) __TS Removed automatically from testsuite!! TS__*/
#define MSG_SGETEXT_MODIFIEDINLIST_SSUS         _MESSAGE(47029, _(""SFN"@"SFN" modified \"" U32CFormat "\" in "SFN" list\n"))
/* #define MSG_SGETEXT_CANTRESOLVEUSER_S           _message(47030, _("unknown user name "SFQ"\n") )    __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_SGETEXT_CANTRESOLVEGROUP_S          _message(47031, _("unknown group name "SFQ"\n") )   __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_SGETEXT_NOCOMMD_SS                  _message(47032, _("unable to contact commd at host "SFN" using service "SFN"\n")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_SGETEXT_NOPERM                      _message(47033, _("no permissions for this operation\n")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_SGETEXT_CANTFINDACL_S               _message(47034, _("unable to find referenced access list "SFQ"\n")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_SGETEXT_SHOULD_BE_ROOT_S            _message(47035, _("should be root to start "SFN"\n") )  __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_SGETEXT_STILL_REFERENCED_SS         _message(47036, _("remove reference to "SFQ" in subordinates of queue "SFQ" before deletion\n") )  __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_SGETEXT_NO_SECURITY_LEVEL_FOR_S           _message(47037, _("denied: missing security level for "SFN"\n")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_SGETEXT_MAY_NOT_CHG_QHOST_S               _message(47038, _("may not change host of queue "SFQ"\n")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_SGETEXT_UP_REFERENCED_TWICE_SS            _message(47039, _("denied: share tree contains reference to unknown "SFN" "SFQ"\n") )    __TS Removed automatically from testsuite!! TS__*/
#define MSG_SGETEXT_NO_PROJECT                        _MESSAGE(47040, _("job rejected: no project assigned to job\n") )     
/* #define MSG_SGETEXT_UNABLETORETRIEVE_I                _message(47041, _("unable to retrieve value for system limit (%d)\n") )      __TS Removed automatically from testsuite!! TS__*/


#define MSG_SCHEDD_INFO_CANNOTRUNATHOST_SSS           _MESSAGE(47042, _("(-l"SFN") cannot run at host "SFQ" because "SFN"" ) ) 
#define MSG_SCHEDD_INFO_HASNOPERMISSION_SS            _MESSAGE(47043, _("has no permission for "SFN" "SFQ""))
#define MSG_SCHEDD_INFO_HASINCORRECTPRJ_SSS           _MESSAGE(47044, _("(project "SFN") does not have the correct project to run in "SFN" "SFQ""))
#define MSG_SCHEDD_INFO_HASNOPRJ_S                    _MESSAGE(47045, _("(no project) does not have the correct project to run in "SFN" "SFQ""))
#define MSG_SCHEDD_INFO_EXCLPRJ_SSS                    _MESSAGE(47046, _("(project "SFN") is not allowed to run in "SFN" "SFQ" based on the excluded project list") )   
#define MSG_SCHEDD_INFO_QUEUENOTREQUESTABLE_S         _MESSAGE(47047, _("cannot run in queue "SFQ" because queues are configured to be non requestable"))
#define MSG_SCHEDD_INFO_NOTINHARDQUEUELST_S           _MESSAGE(47048, _("cannot run in queue "SFQ" because it is not contained in its hard queue list (-q)"))
#define MSG_SCHEDD_INFO_NOTPARALLELQUEUE_S            _MESSAGE(47049, _("cannot run in queue "SFQ" because it is not a parallel queue") ) 
#define MSG_SCHEDD_INFO_NOTINQUEUELSTOFPE_SS          _MESSAGE(47050, _("cannot run in queue "SFQ" because it is not in queue list of PE "SFQ"") ) 
#define MSG_SCHEDD_INFO_NOTACKPTQUEUE_SS              _MESSAGE(47051, _("cannot run in queue "SFQ" because it is not a checkpointing queue") ) 
#define MSG_SCHEDD_INFO_NOTINQUEUELSTOFCKPT_SS        _MESSAGE(47052, _("cannot run in queue "SFQ" because not in queue list of ckpt interface defintion "SFQ""))
#define MSG_SCHEDD_INFO_QUEUENOTINTERACTIVE_S         _MESSAGE(47053, _("cannot run in queue "SFQ" because it is not an interactive queue"))
#define MSG_SCHEDD_INFO_NOTASERIALQUEUE_S             _MESSAGE(47054, _("cannot run in queue "SFQ" because it is not a serial (batch or transfer) queue"))
#define MSG_SCHEDD_INFO_NOTPARALLELJOB_S              _MESSAGE(47055, _("cannot run in parallel/ckpt queue "SFQ" because the job is not parallel"))
#define MSG_SCHEDD_INFO_NOTREQFORCEDRES_SS            _MESSAGE(47056, _("does not request 'forced' resource "SFQ" of queue "SFN""))
#define MSG_SCHEDD_INFO_WOULDSETQEUEINALARM_IS        _MESSAGE(47057, _("(%d slots) would set queue "SFQ" in load alarm state") ) 
#define MSG_SCHEDD_INFO_NOSLOTSINQUEUE_S              _MESSAGE(47058, _("cannot run in queue "SFQ" because it has \"0\" slots"))
#define MSG_SCHEDD_INFO_CANNOTRUNINQUEUE_SSS          _MESSAGE(47059, _("(-l"SFN") cannot run in queue "SFQ" because "SFN"") ) 
#define MSG_SCHEDD_INFO_NORESOURCESPE_                _MESSAGE(47060, _("cannot run because resources requested are not available for parallel job"))
#define MSG_SCHEDD_INFO_CANNOTRUNGLOBALLY_SS          _MESSAGE(47061, _("(-l"SFN") cannot run globally because "SFN"\n") ) 
#define MSG_SCHEDD_INFO_NOFORCEDRES_SS                _MESSAGE(47062, _("does not request 'forced' resource "SFQ" of host "SFN"") )  
#define MSG_SCHEDD_INFO_NOGLOBFORCEDRES_SS            _MESSAGE(47063, _("does not request globally 'forced' resource "SFQ"") )    
#define MSG_SCHEDD_INFO_CKPTNOTFOUND_                 _MESSAGE(47064, _("cannot run because requested ckpt object not found"))
#define MSG_SCHEDD_INFO_PESLOTSNOTINRANGE_S           _MESSAGE(47065, _("cannot run because free slots of pe "SFQ" not in range of job"))
#define MSG_SCHEDD_INFO_NOACCESSTOPE_S                _MESSAGE(47066, _("cannot run because no access to pe "SFQ"") ) 
#define MSG_SCHEDD_INFO_QUEUEINALARM_SS               _MESSAGE(47067, _("queue "SFQ" is in suspend alarm: "SN_UNLIMITED) )        
#define MSG_SCHEDD_INFO_QUEUEOVERLOADED_SS            _MESSAGE(47068, _("queue "SFQ" dropped because it is overloaded: "SN_UNLIMITED) ) 
#define MSG_SCHEDD_INFO_ALLALARMOVERLOADED_           _MESSAGE(47069, _("All queues dropped because of overload or full") )  
#define MSG_SCHEDD_INFO_TURNEDOFF_                    _MESSAGE(47070, _("(Collecting of scheduler job information is turned off)") ) 
#define MSG_SCHEDD_INFO_JOBLIST_                      _MESSAGE(47071, _("(Scheduler job information not available for every job)" ) )  
#define MSG_SCHEDD_INFO_EXECTIME_                     _MESSAGE(47072, _("execution time not reached") ) 
#define MSG_SCHEDD_INFO_JOBINERROR_                   _MESSAGE(47073, _("job is in error state") )  
#define MSG_SCHEDD_INFO_JOBHOLD_                     _MESSAGE(47074, _("job dropped because of hold") )  
#define MSG_SCHEDD_INFO_USRGRPLIMIT_                  _MESSAGE(47075, _("job dropped because of user limitations"))
#define MSG_SCHEDD_INFO_JOBDEPEND_                    _MESSAGE(47076, _("job dropped because of job dependencies") )     
#define MSG_SCHEDD_INFO_NOMESSAGE_                    _MESSAGE(47077, _("there are no messages available") ) 
#define MSG_SCHEDD_INFO_QUEUEFULL_                    _MESSAGE(47078, _("queue "SFQ" dropped because it is full") )   
#define MSG_SCHEDD_INFO_QUEUESUSP_                    _MESSAGE(47079, _("queue "SFQ" dropped becasue it is suspended") )   
#define MSG_SCHEDD_INFO_QUEUEDISABLED_                _MESSAGE(47080, _("queue "SFQ" dropped because it is disabled") )    
#define MSG_SCHEDD_INFO_QUEUENOTAVAIL_                _MESSAGE(47081, _("queue "SFQ" dropped because it is temporarily not available") ) 
#define MSG_SCHEDD_INFO_INSUFFICIENTSLOTS_            _MESSAGE(47082, _("parallel job requires more slots than available"))
#define MSG_SCHEDD_INFO_PEALLOCRULE_S                 _MESSAGE(47083, _("pe "SFQ" dropped because allocation rule is not suitable"))
#define MSG_SCHEDD_INFO_NOPEMATCH_                    _MESSAGE(47084, _("no matching pe found"))
#define MSG_SCHEDD_INFO_CLEANUPNECESSARY_S            _MESSAGE(47085, _("cannot run on host "SFQ" until clean up of an previous run has finished"))
#define MSG_SCHEDD_INFO_MAX_AJ_INSTANCES_             _MESSAGE(47086, _("not all array task may be started due to \'max_aj_instances\'"))


#define MSG_SCHEDD_INFO_CANNOTRUNATHOST               _MESSAGE(47087, _("Jobs can not run because no host can satisfy the resource requirements"))
#define MSG_SCHEDD_INFO_HASNOPERMISSION               _MESSAGE(47088, _("There could not be found a queue with suitable access permissions") )  
#define MSG_SCHEDD_INFO_HASINCORRECTPRJ               _MESSAGE(47089, _("Jobs can not run because queue do not provides the jobs assigned project"))
#define MSG_SCHEDD_INFO_HASNOPRJ                      _MESSAGE(47090, _("Jobs are not assigned to a project to get a queue") )    
#define MSG_SCHEDD_INFO_EXCLPRJ                       _MESSAGE(47091, _("Jobs can not run because excluded project list of queue does not allow it"))
#define MSG_SCHEDD_INFO_QUEUENOTREQUESTABLE           _MESSAGE(47092, _("Jobs can not run because queues are configured to be non requestable"))
#define MSG_SCHEDD_INFO_NOTINHARDQUEUELST             _MESSAGE(47093, _("Jobs can not run because queue is not contained in its hard queue list"))
#define MSG_SCHEDD_INFO_NOTPARALLELQUEUE              _MESSAGE(47094, _("Jobs can not run because queue is not a parallel queue") )  
#define MSG_SCHEDD_INFO_NOTINQUEUELSTOFPE             _MESSAGE(47095, _("Jobs can not run because queue is not in queue list of PE") )  
#define MSG_SCHEDD_INFO_NOTACKPTQUEUE                 _MESSAGE(47096, _("Jobs can not run because queue is not a checkpointing queue") )  
#define MSG_SCHEDD_INFO_NOTINQUEUELSTOFCKPT           _MESSAGE(47097, _("Jobs can not run because queue is not in queue list of ckpt interface defintion"))
#define MSG_SCHEDD_INFO_QUEUENOTINTERACTIVE           _MESSAGE(47098, _("Jobs can not run because queue is not an interactive queue") )  
#define MSG_SCHEDD_INFO_NOTASERIALQUEUE               _MESSAGE(47099, _("Jobs can not run because queue is not a serial (batch or transfer) queue"))
#define MSG_SCHEDD_INFO_NOTPARALLELJOB                _MESSAGE(47100, _("Jobs can not run in parallel/ckpt queue because the job is not parallel"))
#define MSG_SCHEDD_INFO_NOTREQFORCEDRES               _MESSAGE(47101, _("Jobs can not run because they do not request 'forced' resource") )   
#define MSG_SCHEDD_INFO_WOULDSETQEUEINALARM           _MESSAGE(47102, _("Jobs would set queue in load alarm state") )     
#define MSG_SCHEDD_INFO_NOSLOTSINQUEUE                _MESSAGE(47103, _("Jobs can not run because queue has 0 slots") )    
#define MSG_SCHEDD_INFO_CANNOTRUNINQUEUE              _MESSAGE(47104, _("Jobs can not run because the resource requirements can not be satified"))
#define MSG_SCHEDD_INFO_NORESOURCESPE                 _MESSAGE(47105, _("Jobs can not run because resources requested are not available for parallel job"))
#define MSG_SCHEDD_INFO_CANNOTRUNGLOBALLY             _MESSAGE(47106, _("Jobs can not run globally because the resource requirements can not be satified"))
#define MSG_SCHEDD_INFO_NOFORCEDRES                   _MESSAGE(47107, _("Jobs can not run because they do not request 'forced' resource"))
#define MSG_SCHEDD_INFO_NOGLOBFORCEDRES               _MESSAGE(47108, _("Jobs can not run globally because they do not request 'forced' resource"))
#define MSG_SCHEDD_INFO_CKPTNOTFOUND                  _MESSAGE(47109, _("Jobs can not run because requested ckpt object not found") )  
#define MSG_SCHEDD_INFO_PESLOTSNOTINRANGE             _MESSAGE(47110, _("Jobs can not run because free slots of pe are not in range of job") ) 
#define MSG_SCHEDD_INFO_NOACCESSTOPE                  _MESSAGE(47111, _("Jobs can not run because they have no access to pe") )     
#define MSG_SCHEDD_INFO_QUEUEINALARM                  _MESSAGE(47112, _("Jobs can not run because queues are in alarm starte") )      
#define MSG_SCHEDD_INFO_QUEUEOVERLOADED               _MESSAGE(47113, _("Jobs can not run because queues are overloaded") ) 
#define MSG_SCHEDD_INFO_ALLALARMOVERLOADED            _MESSAGE(47114, _("Jobs can not run because all queues are overloaded or full") )    
#define MSG_SCHEDD_INFO_TURNEDOFF                     _MESSAGE(47115, _("(Collecting of scheduler job information is turned off)") )         
#define MSG_SCHEDD_INFO_JOBLIST                       _MESSAGE(47116, _("(Scheduler job information not available for every job)") )     
#define MSG_SCHEDD_INFO_EXECTIME                      _MESSAGE(47117, _("Jobs can not run because execution time not reached") )             
#define MSG_SCHEDD_INFO_JOBINERROR                    _MESSAGE(47118, _("Jobs dropped because of error state") )          
#define MSG_SCHEDD_INFO_JOBHOLD                       _MESSAGE(47119, _("Jobs dropped because of hold state") ) 
#define MSG_SCHEDD_INFO_USRGRPLIMIT                   _MESSAGE(47120, _("Job dropped because of user limitations") )       
#define MSG_SCHEDD_INFO_JOBDEPEND                     _MESSAGE(47121, _("Job dropped because of job dependencies") )           
#define MSG_SCHEDD_INFO_NOMESSAGE                     _MESSAGE(47122, _("There are no messages available") )                
#define MSG_SCHEDD_INFO_QUEUEFULL                     _MESSAGE(47123, _("Queues dropped because they are full") )        
#define MSG_SCHEDD_INFO_QUEUESUSP                     _MESSAGE(47124, _("Queues dropped because they are suspended") )       
#define MSG_SCHEDD_INFO_QUEUEDISABLED                 _MESSAGE(47125, _("Queues dropped because they are disabled") )     
#define MSG_SCHEDD_INFO_QUEUENOTAVAIL                 _MESSAGE(47126, _("Queues dropped because they are temporarily not available") )     
#define MSG_SCHEDD_INFO_INSUFFICIENTSLOTS             _MESSAGE(47127, _("Parallel jobs dropped because of insufficient slots"))
#define MSG_SCHEDD_INFO_PEALLOCRULE                   _MESSAGE(47128, _("PE dropped because allocation rule is not suitable"))
#define MSG_SCHEDD_INFO_NOPEMATCH                     _MESSAGE(47129, _("Parallel job dropped because no matching PE found"))
#define MSG_SCHEDD_INFO_CLEANUPNECESSARY              _MESSAGE(47130, _("Jobs can not run because host cleanup has not finished"))
#define MSG_SCHEDD_INFO_MAX_AJ_INSTANCES              _MESSAGE(47131, _("Not all array tasks may be started due to \'max_aj_instances\'"))

#define MSG_SYSTEM_INVALIDERRORNUMBER                 _MESSAGE(47132, _("invalid error number"))
#define MSG_SYSTEM_GOTNULLASERRORTEXT                 _MESSAGE(47133, _("no error text available"))

#define MSG_SCHEDD_INFO_TOTALPESLOTSNOTINRANGE_S      _MESSAGE(47134, _("cannot run because total slots of pe "SFQ" not in range of job"))
#define MSG_SCHEDD_INFO_TOTALPESLOTSNOTINRANGE        _MESSAGE(47135, _("Jobs can not run because total slots of pe are not in range of job") )

/* 
** schedd/sge_pe_schedd.c
*/ 
#define MSG_PE_XFAILEDPARSINGALLOCATIONRULEY_SS       _MESSAGE(47137, _("pe >"SFN"<: failed parsing allocation rule "SFQ"\n"))

/* 
** schedd/sge_process_events.c
*/ 
/* #define MSG_EVENT_GOTSHUTDOWNFROMQMASTER              _message(47138, _("got \"shutdown\" command from qmaster")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_EVENT_GOTFORCESCHEDRUNFROMQMASTER         _message(47139, _("got \"force scheduler run\" command from qmaster")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_EVENT_GOTMASTERGOESDOWNMESSAGEFROMQMASTER _message(47140, _("got \"qmaster goes down\" message from qmaster")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_JOB_CANTFINDJOBXTODELETE_U                _message(47141, _("can't find job \"" U32CFormat "\" to delete")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_JOB_CANTFINDJOBXTODELETEJOBARRAYTASK_U    _message(47142, _("can't find job \"" U32CFormat "\" to delete a job-array task")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_JOB_CANTFINDJOBXTOMODIFY_U                _message(47143, _("can't find job \"" U32CFormat "\" to modify")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_JOB_CANTFINDJOBXTOMODIFYPRIORITY_U        _message(47144, _("can't find job " U32CFormat " to modify priority")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_JOB_CANTFINDJOBXTOMODIFYTASKY_UU          _message(47145, _("can't find job "U32CFormat" to modify task "U32CFormat"")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_JOB_CANTFINDJOBARRAYTASKXYTOMODIFY_UU     _message(47146, _("can't find job-array task "U32CFormat"."U32CFormat" to modify")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_JOB_CANTFINDJOBARRAYTASKXTODELETE_U       _message(47147, _("can't find job-array task "U32CFormat" to delete")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_JOB_CANTFINDJOBXFORUPDATINGUSAGE_U        _message(47148, _("can't find job "U32CFormat" for updating usage")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_JOB_CANTFINDJOBXFORADDINGPETASK_U         _message(47149, _("can't find job "U32CFormat" for adding a pe task")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_JOB_CANTFINDJOBXFORDELETINGPETASK_U       _message(47150, _("can't find job "U32CFormat" for deleting a pe task")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_JOB_CANTFINDJATASKXFORUPDATINGUSAGE_UU    _message(47151, _("can't find job-array task "U32CFormat"."U32CFormat" for updating usage")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_JOB_CANTFINDJATASKXFORADDINGPETASK_UU     _message(47152, _("can't find job-array task "U32CFormat"."U32CFormat" for adding a pe task")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_JOB_CANTFINDJATASKXFORDELETINGPETASK_UU   _message(47153, _("can't find job-array task "U32CFormat"."U32CFormat" for adding a pe task")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_JOB_CANTFINDPETASKXFORUPDATINGUSAGE_UUS   _message(47154, _("can't find pe task "U32CFormat"."U32CFormat":"SFN" for updating usage")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_QUEUE_CANTFINDQUEUEXTODELETE_S            _message(47155, _("can't find queue "SFQ" to delete")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_QUEUE_CANTFINDQUEUEXTOMODIFY_S            _message(47156, _("can't find queue "SFQ" to modify")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_COMPLEX_CANTFINDCOMPLEXXTODELETE_S        _message(47157, _("can't find complex "SFQ" to delete")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_COMPLEX_CANTFINDCOMPLEXXTOMODIFY_S        _message(47158, _("can't find complex "SFQ" to modify")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_USERSET_CANTFINDUSERSETXTODELETE_S        _message(47159, _("can't find userset "SFQ" to delete")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_USERSET_CANTFINDUSERSETXTOMODIFY_S        _message(47160, _("can't find userset "SFQ" to modify")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_EXECHOST_CANTFINDEXECHOSTXTODELETE_S      _message(47161, _("can't find exechost "SFQ" to delete")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_EXECHOST_CANTFINDEXECHOSTXTOMODIFY_S      _message(47162, _("can't find exechost "SFQ" to modify")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_PARENV_CANTFINDPARENVXTODELETE_S          _message(47163, _("can't find parallel environment "SFQ" to delete")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_PARENV_CANTFINDPARENVXTOMODIFY_S          _message(47164, _("can't find parallel environment "SFQ" to modify")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_CHKPNT_CANTFINDCHKPNTINTXTODELETE_S       _message(47165, _("can't find checkpointing interface "SFQ" to delete")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_CHKPNT_CANTFINDCHKPNTINTXTOMODIFY_S       _message(47166, _("can't find checkpointing interface "SFQ" to modify" )) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_SCHEDD_CANTFINDUSERORPROJECTXTODELETE_SS  _message(47167, _("can't find "SFN" "SFQ" to delete")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_SCHEDD_CANTFINDUSERORPROJECTXTOMODIFY_S   _message(47168, _("can't find user/project "SFQ" to modify")) __TS Removed automatically from testsuite!! TS__*/
#define MSG_USER                                      _MESSAGE(47169, _("user"))
#define MSG_PROJECT                                   _MESSAGE(47170, _("project"))
/* #define MSG_QUEUE_CANTFINDQUEUEXTOYONSUBORDINATE_SS   _message(47171, _("can't find queue  "SFQ" to "SFN" on subordinate")) __TS Removed automatically from testsuite!! TS__*/
#define MSG_QUEUE_SUSPEND                             _MESSAGE(47172, _("suspend"))
#define MSG_QUEUE_UNSUSPEND                           _MESSAGE(47173, _("unsuspend"))
/* #define MSG_EVENT_XEVENTADDJOBGOTNONEWJOB_IUU         _message(47174, _("%d. EVENT ADD JOB " U32CFormat "." U32CFormat " - got no new JOB")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_EVENT_XEVENTMODJOBGOTNONEWJOB_UU          _message(47175, _(U32CFormat". EVENT MOD JOB " U32CFormat " - got no new JOB")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_EVENT_XEVENTMODJATASKGOTNONEWJATASK_UUU   _message(47176, _(U32CFormat". EVENT MOD JATASK "U32CFormat"."U32CFormat" - got no new JATASK")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_EVENT_XEVENTMODJOBGOTNONEWJOB_IU          _message(47177, _("%d. EVENT MOD JOB " U32CFormat " - got no new JOB")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_EVENT_XEVENTADDQUEUEXGOTNONEWQUEUE_IS     _message(47178, _("%d. EVENT ADD QUEUE "SFN" - got no new QUEUE")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_EVENT_XEVENTMODQUEUEXGOTNONEWQUEUE_IS     _message(47179, _("%d. EVENT MOD QUEUE "SFN" - got no new QUEUE")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_EVENT_XEVENTADDCOMPLEXXGOTNONEWCOMPLEX_IS _message(47180, _("%d. EVENT ADD COMPLEX "SFN" - got no new COMPLEX")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_EVENT_XEVENTMODCOMPLEXXGOTNONEWCOMPLEX_IS _message(47181, _("%d. EVENT MOD COMPLEX "SFN" - got no new COMPLEX")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_EVENT_XEVENTADDACLXGOTNONEWACL_IS         _message(47182, _("%d. EVENT ADD ACL "SFN" - got no new ACL")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_EVENT_XEVENTMODACLXGOTNONEWACL_IS         _message(47183, _("%d. EVENT MOD ACL "SFN" - got no new ACL")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_EVENT_XEVENTADDUSERORPROJXGOTNONEWONE_ISS _message(47184, _("%d. EVENT ADD "SFN" "SFN" - got no new one")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_EVENT_XEVENTMODUSERORPROJXGOTNONEWONE_ISS _message(47185, _("%d. EVENT MOD "SFN" "SFN" - got no new one")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_EVENT_XEVENTADDEXECHOSTXGOTNONEWEXECHOST_IS     _message(47186, _("%d. EVENT ADD EXECHOST "SFN" - got no new EXECHOST")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_EVENT_XEVENTMODEXECHOSTXGOTNONEWEXECHOST_IS     _message(47187, _("%d. EVENT MOD EXECHOST "SFN" - got no new EXECHOST")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_EVENT_XEVENTADDPETASKXGOTNONEWPETASK_IUUS           _message(47188, _("%d. EVENT ADD PETASK "U32CFormat"."U32CFormat":"SFN" - got no new pe task\n")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_EVENT_XEVENTADDPEXGOTNONEWPE_IS                 _message(47189, _("%d. EVENT ADD PE "SFN" - got no new PE")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_EVENT_XEVENTMODPEXGOTNONEWPE_IS                 _message(47190, _("%d. EVENT MOD PE "SFN" - got no new PE")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_EVENT_XEVENTADDCKPTXGOTNONEWCKPTINT_IS          _message(47191, _("%d. EVENT ADD CKPT "SFN" - got no new CKPT interface")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_EVENT_XEVENTMODCKPTXGOTNONEWCKPTINT_IS          _message(47192, _("%d. EVENT MOD CKPT "SFN" - got no new CKPT interface")) __TS Removed automatically from testsuite!! TS__*/
#define MSG_NO                                              _MESSAGE(47193, _("NO"))
#define MSG_YES                                             _MESSAGE(47194, _("YES"))
/* 
** schedd/sge_schedd.c
*/
#define MSG_FILE_REDIRECTFILEDESCRIPTORFAILED_I             _MESSAGE(47195, _("can't redirect file descriptor #%d\n"))
#define MSG_SCHEDD_STARTSCHEDONMASTERHOST_S                 _MESSAGE(47196, _("please start schedd on same host as qmaster ("SFN")."))
#define MSG_SCHEDD_CANTGOFURTHER                            _MESSAGE(47197, _("can't go on further"))
#define MSG_SCHEDD_USERXMUSTBEMANAGERFORSCHEDDULING_S       _MESSAGE(47198, _("user "SFQ" must be manager for scheduling\n"))
#define MSG_SCHEDD_HOSTXMUSTBEADMINHOSTFORSCHEDDULING_S     _MESSAGE(47199, _("host "SFQ" must be an admin host for scheduling"))
#define MSG_SCHEDD_GOTEMPTYCONFIGFROMMASTERUSINGDEFAULT     _MESSAGE(47200, _("got empty scheduler configuration from qmaster using default"))
#define MSG_SCHEDD_ALRADY_RUNNING                           _MESSAGE(47201, _("scheduler already running"))

#define MSG_SCHEDD_CANTINSTALLALGORITHMXUSINGDEFAULT_S      _MESSAGE(47202, _("can't install scheduling algorithm "SFQ" - using \"default\" algorithm"))
#define MSG_SCHEDD_UNKNOWN                                  _MESSAGE(47203, _("<unknown>"))
#define MSG_SCHEDD_CANTSWITCHTOADMINUSER                    _MESSAGE(47204, _("can't switch to amin_user"))
#define MSG_SCHEDD_CANTSTARTUP                              _MESSAGE(47205, _("can't startup schedd\n"))
#define MSG_QMASTERMOVEDEXITING_SS                          _MESSAGE(47287, _("qmaster moved from "SFQ" to "SFQ": exiting\n"))

/* #define MSG_SCHEDD_REREGISTER_PARAM                         _message(47206, _("schedd parameter changed: reregistering at qmaster\n")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_SCHEDD_REREGISTER_ERROR                         _message(47207, _("problems in schedd event layer: reregistering at qmaster\n")) __TS Removed automatically from testsuite!! TS__*/

/* #define MSG_SCHEDD_CHANGEALGORITHMNOEVENT_S                 _message(47208, _("Switching to scheduler "SFQ". No change with event handler\n")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_SCHEDD_CHANGEALGORITHMEVENT_S                   _message(47209, _("Switching to event handler scheduler "SFQ"\n")) __TS Removed automatically from testsuite!! TS__*/

/* 
** schedd/sge_select_queue.c
*/ 
#define MSG_SCHEDD_FORDEFAULTREQUEST                        _MESSAGE(47210, _("for default request "  ))
#define MSG_SCHEDD_JOBREQUESTSUNKOWNRESOURCE                _MESSAGE(47211, _("job requests unknown resource \""))
#define MSG_SCHEDD_JOBREQUESTSNONREQUESTABLERESOURCE        _MESSAGE(47212, _("job requests non requestable resource \""))
#define MSG_SCHEDD_ITOFFERSONLY                             _MESSAGE(47213, _("it offers only "))
#define MSG_SCHEDD_LOGLIST_QUEUESFULLANDDROPPED             _MESSAGE(47214, _("queues dropped because they are full: "))
#define MSG_SCHEDD_LOGLIST_QUEUESSUSPENDEDANDDROPPED        _MESSAGE(47215, _("queues dropped because they are suspended: "))
#define MSG_SCHEDD_LOGLIST_QUEUESDISABLEDANDDROPPED         _MESSAGE(47216, _("queues dropped because they are disabled: "))
#define MSG_SCHEDD_RETRLAYERDOMINFMASKXOFATTRYFAILED_US     _MESSAGE(47217, _("failed retrieving layer dominance information from mask "U32CFormat" of attribute "SFN"\n" ))
#define MSG_SCHEDD_NOCOMPLEXATTRIBUTEFORTHRESHOLD_S         _MESSAGE(47219, _("\terror: no complex attribute for threshold "SFN"\n"))
#define MSG_SUBORDPOLICYCONFLICT_UUSS                       _MESSAGE(47288, _("Jobs "U32CFormat" & "U32CFormat" dispatched to master/subordinated queues "SFQ"/"SFQ". Suspend on subordinate to occur in same scheduling interval. Policy conflict!"))


/* 
** schedd/sge_update_lists.c
*/ 
#define MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORQUEUEFAILED    _MESSAGE(47221, _("ensure_valid_where(): lWhere() for queue failed\n"))
#define MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORALLQUEUESFAILED  _MESSAGE(47222, _("ensure_valid_where(): lWhere() for all queues failed\n" ))
#define MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORHOSTFAILED     _MESSAGE(47223, _("ensure_valid_where(): lWhere() for host failed\n"))
#define MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORDEPTFAILED     _MESSAGE(47224, _("ensure_valid_where(): lWhere() for dept failed\n"))
#define MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORACLFAILED      _MESSAGE(47225, _("ensure_valid_where(): lWhere() for acl failed\n"))
#define MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORJOBFAILED      _MESSAGE(47226, _("ensure_valid_where(): lWhat() for job failed\n"))

/* 
** schedd/scheduler.c
*/ 
#define MSG_SCHEDD_LOGLIST_QUEUESTEMPORARLYNOTAVAILABLEDROPPED _MESSAGE(47227, _("queues dropped because they are temporarily not available: "))
#define MSG_SCHEDD_MON_NOPENDJOBSTOPERFORMSCHEDULINGON         _MESSAGE(47228, _("no pending jobs to perform scheduling on"))


/* 
** schedd/sge.c
*/ 
/* #define MSG_FILE_OPENSTDOUTASFILEFAILED            _message(47229, _("Could not open stdout as file\n")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_SGE_UNABLETODUMPJOBLIST                _message(47230, _("Unable to dump job list\n")) __TS Removed automatically from testsuite!! TS__*/

/* 
** schedd/sge_share_mon.c
*/ 
#define MSG_USAGE				_MESSAGE(47231, _("usage:"))
#define MSG_SGESHAREMON_NOSHARETREE             _MESSAGE(47232, _("No share tree"))
#define MSG_SGESHAREMON_c_OPT_USAGE             _MESSAGE(47233, _("number of collections (default is infinite)\n"))
#define MSG_SGESHAREMON_d_OPT_USAGE             _MESSAGE(47234, _("delimiter between columns (default is <TAB>)\n"))
#define MSG_SGESHAREMON_f_OPT_USAGE             _MESSAGE(47235, _("list of fields to print\n"))
#define MSG_SGESHAREMON_h_OPT_USAGE             _MESSAGE(47236, _("print a header containing the field names\n"))
#define MSG_SGESHAREMON_i_OPT_USAGE             _MESSAGE(47237, _("collection interval in seconds (default is 15)\n"))
#define MSG_SGESHAREMON_l_OPT_USAGE             _MESSAGE(47238, _("delimiter between nodes (default is <CR>)\n"))
#define MSG_SGESHAREMON_m_OPT_USAGE             _MESSAGE(47239, _("output file fopen mode (default is \"w\")\n"))
#define MSG_SGESHAREMON_n_OPT_USAGE             _MESSAGE(47240, _("use name=value format\n"))
#define MSG_SGESHAREMON_o_OPT_USAGE             _MESSAGE(47241, _("output file\n"))
#define MSG_SGESHAREMON_r_OPT_USAGE             _MESSAGE(47242, _("delimiter between collection records (default is <CR>)\n"))
#define MSG_SGESHAREMON_s_OPT_USAGE             _MESSAGE(47243, _("format of displayed strings (default is %%s)\n"))
#define MSG_SGESHAREMON_t_OPT_USAGE             _MESSAGE(47244, _("show formatted times\n"))
#define MSG_SGESHAREMON_u_OPT_USAGE             _MESSAGE(47245, _("show decayed usage (since timestamp) in nodes\n"))
#define MSG_SGESHAREMON_x_OPT_USAGE             _MESSAGE(47246, _("exclude non-leaf nodes\n"))

#define MSG_ERROR_XISNOTAVALIDINTERVAL_S        _MESSAGE(47247, _(""SFN" is not a valid interval\n"))
#define MSG_ERROR_XISNOTAVALIDCOUNT_S           _MESSAGE(47248, _(""SFN" is not a valid count\n"))
#define MSG_FILE_COULDNOTOPENXFORY_SS           _MESSAGE(47249, _("could not open "SFN" for "SFN"\n"))

/* 
** schedd/schedd_conf.c
*/ 
#define MSG_ATTRIB_ALGORITHMNOVALIDNAME_S       _MESSAGE(47250, _("attribute " SFQ " is not a valid algorithm name.\n"))
#define MSG_ATTRIB_SCHEDDJOBINFONOVALIDPARAM    _MESSAGE(47251, _("attribute \"schedd_job_info \" is not a valid parameter\n"))
#define MSG_ATTRIB_SCHEDDJOBINFONOVALIDJOBLIST  _MESSAGE(47252, _("attribute \"schedd_job_info\" is not a valid job_list\n"))
#define MSG_ATTRIB_USINGXASY_SS                 _MESSAGE(47253, _("using "SFQ" as "SFN"\n"))
#define MSG_ATTRIB_XISNOTAY_SS                  _MESSAGE(47254, _("attribute "SFQ" is not a "SFN"\n"))
#define MSG_ATTRIB_USINGXFORY_SS                _MESSAGE(47255, _("using "SFQ" for "SFN"\n"))
#define MSG_ATTRIB_USINGXFORY_US                _MESSAGE(47256, _("using " U32CFormat " for "SFN"\n"))
#define MSG_ATTRIB_USINGXFORY_6FS               _MESSAGE(47257, _("using %.6g for "SFN"\n"))
#define MSG_TRUE                                _MESSAGE(47258, _("true"))
#define MSG_FALSE                               _MESSAGE(47259, _("false"))


/* 
** schedd/schedd_message.c
*/ 
#define MSG_SCHEDDMESSAGE_CREATEJOBINFOFORMESSAGEFAILED_U      _MESSAGE(47260, _("can not create schedd_job_info for message "U32CFormat"\n"))






/* 
** schedd/slots_used.c
*/ 
#define MSG_SLOTSUSED_SLOTSENTRYINQUEUEMISSING_S      _MESSAGE(47261, _("missing \"slots\" entry in consumable actual list of queue "SFQ"\n"  ) )  

/* 
** schedd/sort_hosts.c
*/   
#define MSG_ATTRIB_NOATTRIBXINCOMPLEXLIST_S           _MESSAGE(47262, _("no attribute "SFQ" in complex list\n"))

/* 
** schedd/valid_queue_user.c
*/ 
#define MSG_VALIDQUEUEUSER_GRPXALLREADYINUSERSETY_SS  _MESSAGE(47263, _("Group "SFQ" already contained in userset "SFQ"\n"))
#define MSG_VALIDQUEUEUSER_USRXALLREADYINUSERSETY_SS  _MESSAGE(47264, _("User "SFQ" already contained in userset "SFQ"\n"))


/* 
** schedd/sge_select_queue.c
*/
#define MSG_SCHEDD_WHYEXCEEDINVALIDLOAD_SS            _MESSAGE(47265, _("invalid load value "SFQ" for theshold "SFN))
#define MSG_SCHEDD_WHYEXCEEDINVALIDTHRESHOLD_SS       _MESSAGE(47266, _("invalid threshold value "SFN"="SFN))
#define MSG_SCHEDD_WHYEXCEEDINVALIDLOADADJUST_SS      _MESSAGE(47267, _("invalid load adjustment value "SFN"="SFN))
#define MSG_SCHEDD_WHYEXCEEDBOOLVALUE_SSSSS           _MESSAGE(47268, _(SFN"="SFN" ("SN_UNLIMITED") "SFN" "SFN))
#define MSG_SCHEDD_WHYEXCEEDFLOATVALUE_SFSSS          _MESSAGE(47269, _(SFN"=%f ("SN_UNLIMITED") "SFN" "SFN))
#define MSG_SCHEDD_WHYEXCEEDSTRINGVALUE_SSSS          _MESSAGE(47270, _(SFN"="SFN" "SFN" "SFN))
#define MSG_SCHEDD_WHYEXCEEDCOMPLEXTYPE_S             _MESSAGE(47271, _("unknown complex attribute type for threshold "SFN))
#define MSG_SCHEDD_WHYEXCEEDNOHOST_S                  _MESSAGE(47272, _("no such host "SFN" for that queue"))
#define MSG_SCHEDD_WHYEXCEEDNOCOMPLEX_S               _MESSAGE(47273, _("no such complex attribute for threshold "SFQ))

#define MSG_SCHEDD_LCDIAGHOSTNP_SFI                   _MESSAGE(47274, _(SFN" * %f with nproc=%d"))
#define MSG_SCHEDD_LCDIAGHOST_SF                      _MESSAGE(47275, _(SFN" * %f"))
#define MSG_SCHEDD_LCDIAGGLOBAL_SF                    _MESSAGE(47276, _(SFN" * %f global"))
#define MSG_SCHEDD_LCDIAGPOSITIVE_SS                  _MESSAGE(47277, _("= "SFN" + "SFN))
#define MSG_SCHEDD_LCDIAGNEGATIVE_SS                  _MESSAGE(47278, _("= "SFN" - "SFN))
#define MSG_SCHEDD_LCDIAGNONE                         _MESSAGE(47279, _("no load adjustment"))
#define MSG_SCHEDD_LCDIAGNOLOAD                       _MESSAGE(47280, _("no load value"))

/* 
 * libs/sched/sge_ssi.c
 */
#define MSG_SSI_ERRORPARSINGJOBIDENTIFIER_S           _MESSAGE(47281, _("error parsing job identifier "SFQ"\n"))
#define MSG_SSI_MISSINGHOSTNAMEINTASKLIST             _MESSAGE(47282, _("missing hostname in task list\n"))
#define MSG_SSI_COULDNOTFINDQUEUEFORHOST_S            _MESSAGE(47283, _("could not find a queue for host "SFQ"\n"))

/*
 * daemons/sched/sge_process_events.c
 */
#define MSG_CANTFINDJOBINMASTERLIST_S                 _MESSAGE(47284, _("could not find job "SFQ" in master list\n")) 
#define MSG_CANTFINDTASKINJOB_UU                      _MESSAGE(47285, _("could not find task "U32CFormat" in job "U32CFormat"\n")) 
#define MSG_NODATAINEVENT                             _MESSAGE(47286, _("event contains no data"))

/* 
 * libs/sched/sge_complex_schedd.c
 */
#define MSG_GDI_NO_ATTRIBUTE_SSS     _MESSAGE(33080, _("denied: attribute "SFQ" is not configured for "SFN" "SFQ"\n"))

#endif /* __MSG_SCHEDD_H */
