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
#define MSG_NET_UNKNOWNHOSTNAME_S                     _MESSAGE(47002, _("unknown hostname \"%s\"\n"))
#define MSG_LIST_FAILEDRETRIEVINGEXECHOSTOBJ_S        _MESSAGE(47003, _("failed retrieving exec host object: %s\n"))
#define MSG_LIST_FAILEDRETRIEVINGEXECHOSTOBJS_S       _MESSAGE(47004, _("failed retrieving exec host objects: %s\n"))
#define MSG_LIST_FAILEDRETRIEVINGCOMPLEXLISTS_S       _MESSAGE(47005, _("failed retrieving complex lists: %s\n"))
#define MSG_LIST_FAILEDRETRIEVINGJOB_US               _MESSAGE(47006, _("failed retrieving job "U32CFormat": %s\n"))
#define MSG_LIST_FAILEDBUILDINGATTRIBUTESFORHOST_S    _MESSAGE(47007, _("failed building attributes for host %s\n"))
#define MSG_ERROR_UNKNOWNREASON                       _MESSAGE(47008, _("unknown reason"))
#define MSG_LIST_NOEXECHOSTOBJECT_S                   _MESSAGE(47009, _("no such exec host object \"%s\"\n"))
#define MSG_LIST_NOATTRIBUTEXFORHOSTY_SS              _MESSAGE(47010, _("no attribute \"%s\" for host \"%s\"\n"))
#define MSG_ERROR_BADARGUMENT                         _MESSAGE(47011, _("bad argument\n"))


/* 
** schedd/sge_c_event.c
*/ 
#define MSG_COMMD_FAILEDTOSENDACKEVENTDELIVERY        _MESSAGE(47012, _("failed to send ACK_EVENT_DELIVERY\n"))
#define MSG_EVENT_HIGHESTEVENTISXWHILEWAITINGFORY_UU  _MESSAGE(47013, _("highest event number is "U32CFormat" while waiting for "U32CFormat"\n"))
#define MSG_EVENT_SMALLESTEVENTXISGRTHYWAITFOR_UU     _MESSAGE(47014, _("smallest event number "U32CFormat" is greater than number "U32CFormat" i'm waiting for\n"))
#define MSG_EVENT_EVENTSWITHNOINCREASINGNUMBERS       _MESSAGE(47015, _("got events with not increasing numbers\n"))
#define MSG_LIST_FAILEDINCULLUNPACKREPORT             _MESSAGE(47016, _("Failed in cull_unpack report\n"))
#define MSG_EVENT_ILLEGAL_ID_OR_NAME_US               _MESSAGE(47017, _("Illegal id "U32CFormat" or name "SFQ" in event client registration\n"))
#define MSG_EVENT_UNINITIALIZED_EC                    _MESSAGE(47018, _("event client not properly initialized (ec_prepare_registration)\n"))
#define MSG_EVENT_ILLEGALEVENTID_I                    _MESSAGE(47019, _("illegal event id %d\n"))
#define MSG_EVENT_NOTREGISTERED                       _MESSAGE(47020, _("event client not registered\n"))
#define MSG_EVENT_HAVETOHANDLEEVENTS                  _MESSAGE(47021, _("you have to handle the events sgeE_QMASTER_GOES_DOWN and sgeE_SHUTDOWN\n"))

/* 
** schedd/sge_category.c
*/ 
#define MSG_CATEGORY_BUILDINGCATEGORYFORJOBXFAILED_U  _MESSAGE(47022, _("failed building category string for job "U32CFormat"\n"))

/* 
** schedd/sge_complex_schedd.c
*/ 
#define MSG_ATTRIB_ATTRIBUTEWITHOUTNAMEFOUND          _MESSAGE(47023, _("found attribute without name\n"))
#define MSG_ATTRIB_XINATTRIBLISTMISSING_SU            _MESSAGE(47024, _("missing \"%s\" in attribute list (layer = "U32CFormat")\n"))
#define MSG_ATTRIB_ACTUALELEMENTTOATTRIBXMISSING_S    _MESSAGE(47025, _("missing actual element to attrib %s\n"))
#define MSG_ATTRIB_MISSINGATTRIBUTEXINCOMPLEXES_S     _MESSAGE(47026, _("missing attribute \"%s\" in complexes\n"))
#define MSG_POINTER_ASSTRISNULLFORATTRIBX_S           _MESSAGE(47027, _("\"as_str\" is NULL for attrib \"%s\"\n"))
#define MSG_ATTRIB_ATTRIBUTEXALLREADYINLIST_S         _MESSAGE(47028, _("expected attribute \"%s\" being already in attributes list\n"))
#define MSG_MEMORY_UNABLETOALLOCSPACEFORCOMPLEXBUILD  _MESSAGE(47029, _("unable to alloc space for building complexes\n"))
#define MSG_LIST_NOCOMPLEXXATTACHEDTOHOSTY_SS         _MESSAGE(47030, _("no such complex \"%s\" attached to host \"%s\"\n"))
#define MSG_LIST_NOCOMPLEXXATTACHEDTOQUEUEY_SS        _MESSAGE(47031, _("no such complex \"%s\" attached to queue \"%s\"\n"))
#define MSG_SCHEDD_LOADADJUSTMENTSVALUEXNOTNUMERIC_S  _MESSAGE(47032, _("load adjustments value \"%s\" must be of numeric type\n"))

/* 
** schedd/sge_job_schedd.c
*/ 
#define MSG_LOG_JOBSDROPPEDEXECUTIONTIMENOTREACHED    _MESSAGE(47033, _("jobs dropped because execution time not reached: "))
#define MSG_LOG_JOBSDROPPEDERRORSTATEREACHED          _MESSAGE(47034, _("jobs dropped because of error state: "))
#define MSG_LOG_JOBSDROPPEDBECAUSEOFXHOLD             _MESSAGE(47035, _("jobs dropped because of hold: "))
#define MSG_LOG_JOBSDROPPEDBECAUSEDEPENDENCIES        _MESSAGE(47036, _("jobs dropped because of job dependencies: "))
#define MSG_LOG_JOBSDROPPEDBECAUSEUSRGRPLIMIT         _MESSAGE(47037, _("jobs dropped because of user limitations: "))
#define MSG_EVENT_CKPTOBJXFORJOBYNOTFOUND_SI          _MESSAGE(47038, _("can't find requested CKPT object \"%s\" for job %d\n"))
#define MSG_ATTRIB_PARSINGATTRIBUTEXFAILED_SS         _MESSAGE(47039, _("failed parsing attribute \"%s\": %s\n"))

/* 
** schedd/sge_schedd_text.c
*/ 
#define MSG_SGETEXT_INVALIDHOSTINQUEUE_S        _MESSAGE(47040, _("invalid hostname "SFQ" associated with queue\n"))
#define MSG_SGETEXT_CANTRESOLVEUSER_S           _MESSAGE(47042, _("unknown user name "SFQ"\n") )   
#define MSG_SGETEXT_CANTRESOLVEGROUP_S          _MESSAGE(47043, _("unknown group name "SFQ"\n") )  
#define MSG_SGETEXT_NOCOMMD_SS                  _MESSAGE(47044, _("unable to contact commd at host "SFN" using service "SFN"\n"))
#define MSG_SGETEXT_NOPERM                      _MESSAGE(47045, _("no permissions for this operation\n"))
#define MSG_SGETEXT_CANTFINDACL_S               _MESSAGE(47046, _("unable to find referenced access list "SFQ"\n"))
/* #define MSG_SGETEXT_SHOULD_BE_ROOT_S            _MESSAGE(47047, _("should be root to start "SFN"\n") ) */
#define MSG_SGETEXT_STILL_REFERENCED_SS         _MESSAGE(47048, _("remove reference to "SFQ" in subordinates of queue "SFQ" before deletion\n") ) 
#define MSG_SGETEXT_NO_SECURITY_LEVEL_FOR_S           _MESSAGE(47049, _("denied: missing security level for "SFN"\n"))
#define MSG_SGETEXT_MAY_NOT_CHG_QHOST_S               _MESSAGE(47050, _("may not change host of queue "SFQ"\n"))
#define MSG_SGETEXT_UP_REFERENCED_TWICE_SS            _MESSAGE(47051, _("denied: share tree contains reference to unknown "SFN" "SFQ"\n") )   
/* #define MSG_SGETEXT_UNABLETORETRIEVE_I                _MESSAGE(47053, _("unable to retrieve value for system limit (%d)\n") )   */  


#define MSG_SCHEDD_INFO_CANNOTRUNATHOST_SSS           _MESSAGE(47054, _("(-l"SFN") cannot run at host "SFQ" because "SFN"" ) ) 
#define MSG_SCHEDD_INFO_HASNOPERMISSION_S             _MESSAGE(47055, _("has no permission for "SFN" "SFQ""))
#define MSG_SCHEDD_INFO_HASINCORRECTPRJ_SSS           _MESSAGE(47056, _("(project "SFN") does not have the correct project to run in "SFN" "SFQ""))
#define MSG_SCHEDD_INFO_HASNOPRJ_S                    _MESSAGE(47057, _("(no project) does not have the correct project to run in "SFN" "SFQ""))
#define MSG_SCHEDD_INFO_EXCLPRJ_SS                    _MESSAGE(47058, _("(project "SFN") is not allowed to run in "SFN" "SFQ" based on the excluded project list") )   
#define MSG_SCHEDD_INFO_QUEUENOTREQUESTABLE_S         _MESSAGE(47059, _("cannot run in queue "SFQ" because queues are configured to be non requestable"))
#define MSG_SCHEDD_INFO_NOTINHARDQUEUELST_S           _MESSAGE(47060, _("cannot run in queue "SFQ" because it is not contained in its hard queue list (-q)"))
#define MSG_SCHEDD_INFO_NOTPARALLELQUEUE_S            _MESSAGE(47061, _("cannot run in queue "SFQ" because it is not a parallel queue") ) 
#define MSG_SCHEDD_INFO_NOTINQUEUELSTOFPE_SS          _MESSAGE(47062, _("cannot run in queue "SFQ" because it is not in queue list of PE "SFQ"") ) 
#define MSG_SCHEDD_INFO_NOTACKPTQUEUE_SS              _MESSAGE(47063, _("cannot run in queue "SFQ" because it is not a checkpointing queue") ) 
#define MSG_SCHEDD_INFO_NOTINQUEUELSTOFCKPT_SS        _MESSAGE(47064, _("cannot run in queue "SFQ" because not in queue list of ckpt interface defintion "SFQ""))
#define MSG_SCHEDD_INFO_QUEUENOTINTERACTIVE_S         _MESSAGE(47065, _("cannot run in queue "SFQ" because it is not an interactive queue"))
#define MSG_SCHEDD_INFO_NOTASERIALQUEUE_S             _MESSAGE(47066, _("cannot run in queue "SFQ" because it is not a serial (batch or transfer) queue"))
#define MSG_SCHEDD_INFO_NOTPARALLELJOB_S              _MESSAGE(47067, _("cannot run in parallel/ckpt queue "SFQ" because the job is not parallel"))
#define MSG_SCHEDD_INFO_NOTREQFORCEDRES_SS            _MESSAGE(47068, _("does not request 'forced' resource "SFQ" of queue "SFN""))
#define MSG_SCHEDD_INFO_WOULDSETQEUEINALARM_IS        _MESSAGE(47069, _("(%d slots) would set queue "SFQ" in load alarm state") ) 
#define MSG_SCHEDD_INFO_NOSLOTSINQUEUE_S              _MESSAGE(47070, _("cannot run in queue "SFQ" because it has \"0\" slots"))
#define MSG_SCHEDD_INFO_CANNOTRUNINQUEUE_SSS          _MESSAGE(47071, _("(-l"SFN") cannot run in queue "SFQ" because "SFN"") ) 
#define MSG_SCHEDD_INFO_NOSLOTSUPPORTBYPE_S           _MESSAGE(47072, _("cannot run because requested amount of slots is not supported by pe "SFQ""))
#define MSG_SCHEDD_INFO_CANNOTRUNGLOBALLY_SS          _MESSAGE(47073, _("(-l"SFN") cannot run globally because "SFN"\n") ) 
#define MSG_SCHEDD_INFO_NOFORCEDRES_SS                _MESSAGE(47074, _("does not request 'forced' resource "SFQ" of host "SFN"") )  
#define MSG_SCHEDD_INFO_NOGLOBFORCEDRES_SS            _MESSAGE(47075, _("does not request globally 'forced' resource "SFQ"") )    
#define MSG_SCHEDD_INFO_CKPTNOTFOUND_                 _MESSAGE(47076, _("cannot run because requested ckpt object not found"))
#define MSG_SCHEDD_INFO_PESLOTSNOTINRANGE_S           _MESSAGE(47077, _("cannot run because free slots of pe "SFQ" not in range of job"))
#define MSG_SCHEDD_INFO_NOACCESSTOPE_S                _MESSAGE(47078, _("cannot run because no access to pe "SFQ"") ) 
#define MSG_SCHEDD_INFO_QUEUEINALARM_SS               _MESSAGE(47079, _("queue "SFQ" is in suspend alarm: "SN_UNLIMITED) )        
#define MSG_SCHEDD_INFO_QUEUEOVERLOADED_SS            _MESSAGE(47080, _("queue "SFQ" dropped because it is overloaded: "SN_UNLIMITED) ) 
#define MSG_SCHEDD_INFO_ALLALARMOVERLOADED_           _MESSAGE(47081, _("All queues dropped because of overload or full") )  
#define MSG_SCHEDD_INFO_TURNEDOFF_                    _MESSAGE(47082, _("(Collecting of scheduler job information is turned off)") ) 
#define MSG_SCHEDD_INFO_JOBLIST_                      _MESSAGE(47083, _("(Scheduler job information not available for every job)" ) )  
#define MSG_SCHEDD_INFO_EXECTIME_                     _MESSAGE(47084, _("execution time not reached") ) 
#define MSG_SCHEDD_INFO_JOBINERROR_                   _MESSAGE(47085, _("job is in error state") )  
#define MSG_SCHEDD_INFO_JOBHOLD_                     _MESSAGE(47086, _("job dropped because of hold") )  
#define MSG_SCHEDD_INFO_USRGRPLIMIT_                  _MESSAGE(47087, _("job dropped because of user limitations"))
#define MSG_SCHEDD_INFO_JOBDEPEND_                    _MESSAGE(47088, _("job dropped because of job dependencies") )     
#define MSG_SCHEDD_INFO_NOMESSAGE_                    _MESSAGE(47089, _("there are no messages available") ) 
#define MSG_SCHEDD_INFO_QUEUEFULL_                    _MESSAGE(47090, _("queue "SFQ" dropped because it is full") )   
#define MSG_SCHEDD_INFO_QUEUESUSP_                    _MESSAGE(47091, _("queue "SFQ" dropped becasue it is suspended") )   
#define MSG_SCHEDD_INFO_QUEUEDISABLED_                _MESSAGE(47092, _("queue "SFQ" dropped because it is disabled") )    
#define MSG_SCHEDD_INFO_QUEUENOTAVAIL_                _MESSAGE(47093, _("queue "SFQ" dropped because it is temporarily not available") ) 
#define MSG_SCHEDD_INFO_INSUFFICIENTSLOTS_            _MESSAGE(47094, _("parallel job requires more slots than available"))
#define MSG_SCHEDD_INFO_PEALLOCRULE_S                 _MESSAGE(47095, _("pe "SFQ" dropped because allocation rule is not suitable"))
#define MSG_SCHEDD_INFO_NOPEMATCH_                    _MESSAGE(47096, _("no matching pe found"))
#define MSG_SCHEDD_INFO_CLEANUPNECESSARY_S            _MESSAGE(47097, _("cannot run on host "SFQ" until clean up of an previous run has finished"))
#define MSG_SCHEDD_INFO_MAX_AJ_INSTANCES_             _MESSAGE(47098, _("not all array task may be started due to \'max_aj_instances\'"))


#define MSG_SCHEDD_INFO_CANNOTRUNATHOST               _MESSAGE(47099, _("Jobs can not run because no host can satisfy the resource requirements"))
#define MSG_SCHEDD_INFO_HASNOPERMISSION               _MESSAGE(47100, _("There could not be found a queue with suitable access permissions") )  
#define MSG_SCHEDD_INFO_HASINCORRECTPRJ               _MESSAGE(47101, _("Jobs can not run because queue do not provides the jobs assigned project"))
#define MSG_SCHEDD_INFO_HASNOPRJ                      _MESSAGE(47102, _("Jobs are not assigned to a project to get a queue") )    
#define MSG_SCHEDD_INFO_EXCLPRJ                       _MESSAGE(47103, _("Jobs can not run because excluded project list of queue does not allow it"))
#define MSG_SCHEDD_INFO_QUEUENOTREQUESTABLE           _MESSAGE(47104, _("Jobs can not run because queues are configured to be non requestable"))
#define MSG_SCHEDD_INFO_NOTINHARDQUEUELST             _MESSAGE(47105, _("Jobs can not run because queue is not contained in its hard queue list"))
#define MSG_SCHEDD_INFO_NOTPARALLELQUEUE              _MESSAGE(47106, _("Jobs can not run because queue is not a parallel queue") )  
#define MSG_SCHEDD_INFO_NOTINQUEUELSTOFPE             _MESSAGE(47107, _("Jobs can not run because queue is not in queue list of PE") )  
#define MSG_SCHEDD_INFO_NOTACKPTQUEUE                 _MESSAGE(47108, _("Jobs can not run because queue is not a checkpointing queue") )  
#define MSG_SCHEDD_INFO_NOTINQUEUELSTOFCKPT           _MESSAGE(47109, _("Jobs can not run because queue is not in queue list of ckpt interface defintion"))
#define MSG_SCHEDD_INFO_QUEUENOTINTERACTIVE           _MESSAGE(47110, _("Jobs can not run because queue is not an interactive queue") )  
#define MSG_SCHEDD_INFO_NOTASERIALQUEUE               _MESSAGE(47111, _("Jobs can not run because queue is not a serial (batch or transfer) queue"))
#define MSG_SCHEDD_INFO_NOTPARALLELJOB                _MESSAGE(47112, _("Jobs can not run in parallel/ckpt queue because the job is not parallel"))
#define MSG_SCHEDD_INFO_NOTREQFORCEDRES               _MESSAGE(47113, _("Jobs can not run because they do not request 'forced' resource") )   
#define MSG_SCHEDD_INFO_WOULDSETQEUEINALARM           _MESSAGE(47114, _("Jobs would set queue in load alarm state") )     
#define MSG_SCHEDD_INFO_NOSLOTSINQUEUE                _MESSAGE(47115, _("Jobs can not run because queue has 0 slots") )    
#define MSG_SCHEDD_INFO_CANNOTRUNINQUEUE              _MESSAGE(47116, _("Jobs can not run because the resource requirements can not be satified"))
#define MSG_SCHEDD_INFO_NOSLOTSUPPORTBYPE             _MESSAGE(47117, _("Jobs can not run because requested amount of slots is not supported by pe"))
#define MSG_SCHEDD_INFO_CANNOTRUNGLOBALLY             _MESSAGE(47118, _("Jobs can not run globally because the resource requirements can not be satified"))
#define MSG_SCHEDD_INFO_NOFORCEDRES                   _MESSAGE(47119, _("Jobs can not run because they do not request 'forced' resource"))
#define MSG_SCHEDD_INFO_NOGLOBFORCEDRES               _MESSAGE(47120, _("Jobs can not run globally because they do not request 'forced' resource"))
#define MSG_SCHEDD_INFO_CKPTNOTFOUND                  _MESSAGE(47121, _("Jobs can not run because requested ckpt object not found") )  
#define MSG_SCHEDD_INFO_PESLOTSNOTINRANGE             _MESSAGE(47122, _("Jobs can not run because free slots of pe are not in range of job") ) 
#define MSG_SCHEDD_INFO_NOACCESSTOPE                  _MESSAGE(47123, _("Jobs can not run because they have no access to pe") )     
#define MSG_SCHEDD_INFO_QUEUEINALARM                  _MESSAGE(47124, _("Jobs can not run because queues are in alarm starte") )      
#define MSG_SCHEDD_INFO_QUEUEOVERLOADED               _MESSAGE(47125, _("Jobs can not run because queues are overloaded") ) 
#define MSG_SCHEDD_INFO_ALLALARMOVERLOADED            _MESSAGE(47126, _("Jobs can not run because all queues are overloaded or full") )    
#define MSG_SCHEDD_INFO_TURNEDOFF                     _MESSAGE(47127, _("(Collecting of scheduler job information is turned off)") )         
#define MSG_SCHEDD_INFO_JOBLIST                       _MESSAGE(47128, _("(Scheduler job information not available for every job)") )     
#define MSG_SCHEDD_INFO_EXECTIME                      _MESSAGE(47129, _("Jobs can not run because execution time not reached") )             
#define MSG_SCHEDD_INFO_JOBINERROR                    _MESSAGE(47130, _("Jobs dropped because of error state") )          
#define MSG_SCHEDD_INFO_JOBHOLD                       _MESSAGE(47131, _("Jobs dropped because of hold state") ) 
#define MSG_SCHEDD_INFO_USRGRPLIMIT                   _MESSAGE(47132, _("Job dropped because of user limitations") )       
#define MSG_SCHEDD_INFO_JOBDEPEND                     _MESSAGE(47133, _("Job dropped because of job dependencies") )           
#define MSG_SCHEDD_INFO_NOMESSAGE                     _MESSAGE(47134, _("There are no messages available") )                
#define MSG_SCHEDD_INFO_QUEUEFULL                     _MESSAGE(47135, _("Queues dropped because they are full") )        
#define MSG_SCHEDD_INFO_QUEUESUSP                     _MESSAGE(47136, _("Queues dropped because they are suspended") )       
#define MSG_SCHEDD_INFO_QUEUEDISABLED                 _MESSAGE(47137, _("Queues dropped because they are disabled") )     
#define MSG_SCHEDD_INFO_QUEUENOTAVAIL                 _MESSAGE(47138, _("Queues dropped because they are temporarily not available") )     
#define MSG_SCHEDD_INFO_INSUFFICIENTSLOTS             _MESSAGE(47139, _("Parallel jobs dropped because of insufficient slots"))
#define MSG_SCHEDD_INFO_PEALLOCRULE                   _MESSAGE(47140, _("PE dropped because allocation rule is not suitable"))
#define MSG_SCHEDD_INFO_NOPEMATCH                     _MESSAGE(47141, _("Parallel job dropped because no matching PE found"))
#define MSG_SCHEDD_INFO_CLEANUPNECESSARY              _MESSAGE(47142, _("Jobs can not run because host cleanup has not finished"))
#define MSG_SCHEDD_INFO_MAX_AJ_INSTANCES              _MESSAGE(47143, _("Not all array tasks may be started due to \'max_aj_instances\'"))

#define MSG_SYSTEM_INVALIDERRORNUMBER                 _MESSAGE(47144, _("invalid error number"))
#define MSG_SYSTEM_GOTNULLASERRORTEXT                 _MESSAGE(47145, _("no error text available"))


/* 
** schedd/sge_orders.c
*/ 
#define MSG_LIST_NOANSWERLISTSENDORDERFAILED          _MESSAGE(47146, _("no answer-list - failed sending order"))
#define MSG_LIST_NOELEMENTINANSWERLISTSENDORDERFAILED _MESSAGE(47147, _("no element in answer-list - failed sending order"))
#define MSG_LIST_SENDORDERXFAILED_S                   _MESSAGE(47148, _("failed sending order: %s"))

/* 
** schedd/sge_pe_schedd.c
*/ 
#define MSG_PE_XFAILEDPARSINGALLOCATIONRULEY_SS       _MESSAGE(47149, _("pe >%s<: failed parsing allocation rule \"%s\"\n"))

/* 
** schedd/sge_process_events.c
*/ 
#define MSG_EVENT_GOTSHUTDOWNFROMQMASTER              _MESSAGE(47150, _("got \"shutdown\" command from qmaster"))
#define MSG_EVENT_GOTFORCESCHEDRUNFROMQMASTER         _MESSAGE(47151, _("got \"force scheduler run\" command from qmaster"))
#define MSG_EVENT_GOTMASTERGOESDOWNMESSAGEFROMQMASTER _MESSAGE(47152, _("got \"qmaster goes down\" message from qmaster"))
#define MSG_JOB_CANTFINDJOBXTODELETE_U                _MESSAGE(47153, _("can't find job \"" U32CFormat "\" to delete"))
#define MSG_JOB_CANTFINDJOBXTODELETEJOBARRAYTASK_U    _MESSAGE(47154, _("can't find job \"" U32CFormat "\" to delete a job-array task"))
#define MSG_JOB_CANTFINDJOBXTOMODIFY_U                _MESSAGE(47155, _("can't find job \"" U32CFormat "\" to modify"))
#define MSG_JOB_CANTFINDJOBXTOMODIFYPRIORITY_U        _MESSAGE(47156, _("can't find job " U32CFormat " to modify priority"))
#define MSG_JOB_CANTFINDJOBXTOMODIFYTASKY_UU          _MESSAGE(47157, _("can't find job "U32CFormat" to modify task "U32CFormat""))
#define MSG_JOB_CANTFINDJOBARRAYTASKXYTOMODIFY_UU     _MESSAGE(47158, _("can't find job-array task "U32CFormat"."U32CFormat" to modify"))
#define MSG_JOB_CANTFINDJOBARRAYTASKXTODELETE_U       _MESSAGE(47159, _("can't find job-array task "U32CFormat" to delete"))
#define MSG_JOB_CANTFINDJOBXFORUPDATINGUSAGE_U        _MESSAGE(47160, _("can't find job \"" U32CFormat "\" for updating usage"))
#define MSG_QUEUE_CANTFINDQUEUEXTODELETE_S            _MESSAGE(47161, _("can't find queue \"%s\" to delete"))
#define MSG_QUEUE_CANTFINDQUEUEXTOMODIFY_S            _MESSAGE(47162, _("can't find queue \"%s\" to modify"))
#define MSG_COMPLEX_CANTFINDCOMPLEXXTODELETE_S        _MESSAGE(47163, _("can't find complex \"%s\" to delete"))
#define MSG_COMPLEX_CANTFINDCOMPLEXXTOMODIFY_S        _MESSAGE(47164, _("can't find complex \"%s\" to modify"))
#define MSG_USERSET_CANTFINDUSERSETXTODELETE_S        _MESSAGE(47165, _("can't find userset \"%s\" to delete"))
#define MSG_USERSET_CANTFINDUSERSETXTOMODIFY_S        _MESSAGE(47166, _("can't find userset \"%s\" to modify"))
#define MSG_EXECHOST_CANTFINDEXECHOSTXTODELETE_S      _MESSAGE(47167, _("can't find exechost \"%s\" to delete"))
#define MSG_EXECHOST_CANTFINDEXECHOSTXTOMODIFY_S      _MESSAGE(47168, _("can't find exechost \"%s\" to modify"))
#define MSG_PARENV_CANTFINDPARENVXTODELETE_S          _MESSAGE(47169, _("can't find parallel environment \"%s\" to delete"))
#define MSG_PARENV_CANTFINDPARENVXTOMODIFY_S          _MESSAGE(47170, _("can't find parallel environment \"%s\" to modify"))
#define MSG_CHKPNT_CANTFINDCHKPNTINTXTODELETE_S       _MESSAGE(47171, _("can't find checkpointing interface \"%s\" to delete"))
#define MSG_CHKPNT_CANTFINDCHKPNTINTXTOMODIFY_S       _MESSAGE(47172, _("can't find checkpointing interface \"%s\" to modify" ))
#define MSG_SCHEDD_CANTFINDUSERORPROJECTXTODELETE_SS  _MESSAGE(47173, _("can't find %s \"%s\" to delete"))
#define MSG_SCHEDD_CANTFINDUSERORPROJECTXTOMODIFY_S   _MESSAGE(47174, _("can't find user/project \"%s\" to modify"))
#define MSG_USER                                      _MESSAGE(47175, _("user"))
#define MSG_PROJECT                                   _MESSAGE(47176, _("project"))
#define MSG_QUEUE_CANTFINDQUEUEXTOYONSUBORDINATE_SS   _MESSAGE(47177, _("can't find queue  \"%s\" to %s on subordinate"))
#define MSG_QUEUE_SUSPEND                             _MESSAGE(47178, _("suspend"))
#define MSG_QUEUE_UNSUSPEND                           _MESSAGE(47179, _("unsuspend"))
#define MSG_EVENT_XEVENTADDJOBGOTNONEWJOB_IUU         _MESSAGE(47180, _("%d. EVENT ADD JOB " U32CFormat "." U32CFormat " - got no new JOB"))
#define MSG_EVENT_XEVENTMODJOBGOTNONEWJOB_UU          _MESSAGE(47181, _(U32CFormat". EVENT MOD JOB " U32CFormat " - got no new JOB"))
#define MSG_EVENT_XEVENTMODJATASKGOTNONEWJATASK_UUU   _MESSAGE(47182, _(U32CFormat". EVENT MOD JATASK "U32CFormat"."U32CFormat" - got no new JATASK"))
#define MSG_EVENT_XEVENTMODJOBGOTNONEWJOB_IU          _MESSAGE(47183, _("%d. EVENT MOD JOB " U32CFormat " - got no new JOB"))
#define MSG_EVENT_XEVENTADDQUEUEXGOTNONEWQUEUE_IS     _MESSAGE(47184, _("%d. EVENT ADD QUEUE %s - got no new QUEUE"))
#define MSG_EVENT_XEVENTMODQUEUEXGOTNONEWQUEUE_IS     _MESSAGE(47185, _("%d. EVENT MOD QUEUE %s - got no new QUEUE"))
#define MSG_EVENT_XEVENTADDCOMPLEXXGOTNONEWCOMPLEX_IS _MESSAGE(47186, _("%d. EVENT ADD COMPLEX %s - got no new COMPLEX"))
#define MSG_EVENT_XEVENTMODCOMPLEXXGOTNONEWCOMPLEX_IS _MESSAGE(47187, _("%d. EVENT MOD COMPLEX %s - got no new COMPLEX"))
#define MSG_EVENT_XEVENTADDACLXGOTNONEWACL_IS         _MESSAGE(47188, _("%d. EVENT ADD ACL %s - got no new ACL"))
#define MSG_EVENT_XEVENTMODACLXGOTNONEWACL_IS         _MESSAGE(47189, _("%d. EVENT MOD ACL %s - got no new ACL"))
#define MSG_EVENT_XEVENTADDUSERORPROJXGOTNONEWONE_ISS _MESSAGE(47190, _("%d. EVENT ADD %s %s - got no new one"))
#define MSG_EVENT_XEVENTMODUSERORPROJXGOTNONEWONE_ISS _MESSAGE(47191, _("%d. EVENT MOD %s %s - got no new one"))
#define MSG_EVENT_XEVENTADDEXECHOSTXGOTNONEWEXECHOST_IS     _MESSAGE(47192, _("%d. EVENT ADD EXECHOST %s - got no new EXECHOST"))
#define MSG_EVENT_XEVENTMODEXECHOSTXGOTNONEWEXECHOST_IS     _MESSAGE(47193, _("%d. EVENT MOD EXECHOST %s - got no new EXECHOST"))
#define MSG_EVENT_XEVENTADDPEXGOTNONEWPE_IS                 _MESSAGE(47194, _("%d. EVENT ADD PE %s - got no new PE"))
#define MSG_EVENT_XEVENTMODPEXGOTNONEWPE_IS                 _MESSAGE(47195, _("%d. EVENT MOD PE %s - got no new PE"))
#define MSG_EVENT_XEVENTADDCKPTXGOTNONEWCKPTINT_IS          _MESSAGE(47196, _("%d. EVENT ADD CKPT %s - got no new CKPT interface"))
#define MSG_EVENT_XEVENTMODCKPTXGOTNONEWCKPTINT_IS          _MESSAGE(47197, _("%d. EVENT MOD CKPT %s - got no new CKPT interface"))
#define MSG_NO                                              _MESSAGE(47198, _("NO"))
#define MSG_YES                                             _MESSAGE(47199, _("YES"))
/* 
** schedd/sge_schedd.c
*/
#define MSG_FILE_REDIRECTFILEDESCRIPTORFAILED_I             _MESSAGE(47200, _("can't redirect file descriptor #%d\n"))
#define MSG_SCHEDD_STARTSCHEDONMASTERHOST_S                 _MESSAGE(47201, _("please start schedd on same host as qmaster (%s)."))
#define MSG_SCHEDD_CANTGOFURTHER                            _MESSAGE(47202, _("can't go on further"))
#define MSG_SCHEDD_USERXMUSTBEMANAGERFORSCHEDDULING_S       _MESSAGE(47203, _("user \"%s\" must be manager for scheduling\n"))
#define MSG_SCHEDD_HOSTXMUSTBEADMINHOSTFORSCHEDDULING_S     _MESSAGE(47204, _("host \"%s\" must be an admin host for scheduling"))
#define MSG_SCHEDD_GOTEMPTYCONFIGFROMMASTERUSINGDEFAULT     _MESSAGE(47205, _("got empty scheduler configuration from qmaster using default"))
#define MSG_SCHEDD_ALRADY_RUNNING                           _MESSAGE(47206, _("scheduler already running"))

#define MSG_SCHEDD_CANTINSTALLALGORITHMXUSINGDEFAULT_S      _MESSAGE(47207, _("can't install scheduling algorithm \"%s\" - using \"default\" algorithm"))
#define MSG_SCHEDD_UNKNOWN                                  _MESSAGE(47208, _("<unknown>"))
#define MSG_SCHEDD_CANTSWITCHTOADMINUSER                    _MESSAGE(47209, _("can't switch to amin_user"))
#define MSG_SCHEDD_CANTSTARTUP                              _MESSAGE(47210, _("can't startup schedd\n"))

#define MSG_SCHEDD_REREGISTER_PARAM                         _MESSAGE(47211, _("schedd parameter changed: reregistering at qmaster\n"))
#define MSG_SCHEDD_REREGISTER_ERROR                         _MESSAGE(47212, _("problems in schedd event layer: reregistering at qmaster\n"))

#define MSG_SCHEDD_CHANGEALGORITHMNOEVENT_S                 _MESSAGE(47213, _("Switching to scheduler \"%s\". No change with event handler\n"))
#define MSG_SCHEDD_CHANGEALGORITHMEVENT_S                   _MESSAGE(47214, _("Switching to event handler scheduler \"%s\"\n"))

/* 
** schedd/sge_select_queue.c
*/ 
#define MSG_SCHEDD_FORDEFAULTREQUEST                        _MESSAGE(47215, _("for default request "  ))
#define MSG_SCHEDD_JOBREQUESTSUNKOWNRESOURCE                _MESSAGE(47216, _("job requests unknown resource \""))
#define MSG_SCHEDD_JOBREQUESTSNONREQUESTABLERESOURCE        _MESSAGE(47217, _("job requests non requestable resource \""))
#define MSG_SCHEDD_ITOFFERSONLY                             _MESSAGE(47218, _("it offers only "))
#define MSG_SCHEDD_LOGLIST_QUEUESFULLANDDROPPED             _MESSAGE(47219, _("queues dropped because they are full: "))
#define MSG_SCHEDD_LOGLIST_QUEUESSUSPENDEDANDDROPPED        _MESSAGE(47220, _("queues dropped because they are suspended: "))
#define MSG_SCHEDD_LOGLIST_QUEUESDISABLEDANDDROPPED         _MESSAGE(47221, _("queues dropped because they are disabled: "))
#define MSG_SCHEDD_RETRLAYERDOMINFMASKXOFATTRYFAILED_US     _MESSAGE(47222, _("failed retrieving layer dominance information from mask "U32CFormat" of attribute %s\n" ))
#define MSG_SCHEDD_NOHOSTFORQUEUE                           _MESSAGE(47223, _("\terror: no host defined for queue\n"))
#define MSG_SCHEDD_NOCOMPLEXATTRIBUTEFORTHRESHOLD_S         _MESSAGE(47224, _("\terror: no complex attribute for threshold %s\n"))
#define MSG_SCHEDD_NOLOADVALUEFORTHRESHOLD_S                _MESSAGE(47225, _("\terror: no load value for threshold %s\n"))

#define MSG_SCHEDD_WHYEXCEEDINVALIDLOAD_SS                  _MESSAGE(47226, _("invalid load value "SFQ" for theshold "SFN))
#define MSG_SCHEDD_WHYEXCEEDINVALIDTHRESHOLD_SS             _MESSAGE(47227, _("invalid threshold value "SFN"="SFN))
#define MSG_SCHEDD_WHYEXCEEDINVALIDLOADADJUST_SS            _MESSAGE(47228, _("invalid load adjustment value "SFN"="SFN))
#define MSG_SCHEDD_WHYEXCEEDBOOLVALUE_SSSSS                 _MESSAGE(47229, _(SFN"="SFN" ("SN_UNLIMITED") "SFN" "SFN))
#define MSG_SCHEDD_WHYEXCEEDFLOATVALUE_SFSSS                _MESSAGE(47230, _(SFN"=%f ("SN_UNLIMITED") "SFN" "SFN))
#define MSG_SCHEDD_WHYEXCEEDSTRINGVALUE_SSSS                _MESSAGE(47231, _(SFN"="SFN" "SFN" "SFN))
#define MSG_SCHEDD_WHYEXCEEDCOMPLEXTYPE_S                   _MESSAGE(47232, _("unknown complex attribute type for theshold "SFN))
#define MSG_SCHEDD_WHYEXCEEDNOHOST_S                        _MESSAGE(47233, _("no such host "SFN" for that queue"))
#define MSG_SCHEDD_WHYEXCEEDNOCOMPLEX_S                     _MESSAGE(47234, _("no such complex attribute for threshold "SFQ))

#define MSG_SCHEDD_LCDIAGHOSTNP_SFI                         _MESSAGE(47235, _("%s * %f with nproc=%d"))
#define MSG_SCHEDD_LCDIAGHOST_SF                            _MESSAGE(47236, _("%s * %f"))
#define MSG_SCHEDD_LCDIAGGLOBAL_SF                          _MESSAGE(47237, _("%s * %f global"))
#define MSG_SCHEDD_LCDIAGPOSITIVE_SS                        _MESSAGE(47238, _("= "SFN" + "SFN))
#define MSG_SCHEDD_LCDIAGNEGATIVE_SS                        _MESSAGE(47239, _("= "SFN" - "SFN))
#define MSG_SCHEDD_LCDIAGNONE                               _MESSAGE(47240, _("no load adjustment"))




/* 
** schedd/sge_update_lists.c
*/ 
#define MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORQUEUEFAILED    _MESSAGE(47241, _("ensure_valid_where(): lWhere() for queue failed\n"))
#define MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORALLQUEUESFAILED  _MESSAGE(47242, _("ensure_valid_where(): lWhere() for all queues failed\n" ))
#define MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORHOSTFAILED     _MESSAGE(47243, _("ensure_valid_where(): lWhere() for host failed\n"))
#define MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORDEPTFAILED     _MESSAGE(47244, _("ensure_valid_where(): lWhere() for dept failed\n"))
#define MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORACLFAILED      _MESSAGE(47245, _("ensure_valid_where(): lWhere() for acl failed\n"))
#define MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORJOBFAILED      _MESSAGE(47246, _("ensure_valid_where(): lWhat() for job failed\n"))

/* 
** schedd/scheduler.c
*/ 
#define MSG_SCHEDD_LOGLIST_QUEUESTEMPORARLYNOTAVAILABLEDROPPED _MESSAGE(47247, _("queues dropped because they are temporarily not available: "))
#define MSG_SCHEDD_MON_NOPENDJOBSTOPERFORMSCHEDULINGON         _MESSAGE(47248, _("no pending jobs to perform scheduling on"))


/* 
** schedd/event.c
*/ 
#define MSG_EVENT_DELJOB_III                       _MESSAGE(47249, _("%d. EVENT DEL JOB %d.%d\n"))
#define MSG_EVENT_ADDJOB_III                       _MESSAGE(47250, _("%d. EVENT ADD JOB %d.%d\n"))
#define MSG_EVENT_MODJOB_III                       _MESSAGE(47251, _("%d. EVENT MOD JOB %d.%d\n"))
#define MSG_EVENT_JOBLISTXELEMENTS_II              _MESSAGE(47252, _("%d. EVENT JOB LIST %d Elements\n"))
#define MSG_EVENT_DELJOB_SCHEDD_INFO_III                       _MESSAGE(47253, _("%d. EVENT DEL JOB_SCHEDD_INFO %d.%d\n"))
#define MSG_EVENT_ADDJOB_SCHEDD_INFO_III                       _MESSAGE(47254, _("%d. EVENT ADD JOB_SCHEDD_INFO %d.%d\n"))
#define MSG_EVENT_MODJOB_SCHEDD_INFO_III                       _MESSAGE(47255, _("%d. EVENT MOD JOB_SCHEDD_INFO %d.%d\n"))
#define MSG_EVENT_JOB_SCHEDD_INFOLISTXELEMENTS_II              _MESSAGE(47256, _("%d. EVENT JOB_SCHEDD_INFO LIST %d Elements\n"))
#define MSG_EVENT_DELZOMBIE_III                       _MESSAGE(47257, _("%d. EVENT DEL ZOMBIE %d.%d\n"))
#define MSG_EVENT_ADDZOMBIE_III                       _MESSAGE(47258, _("%d. EVENT ADD ZOMBIE %d.%d\n"))
#define MSG_EVENT_MODZOMBIE_III                       _MESSAGE(47259, _("%d. EVENT MOD ZOMBIE %d.%d\n"))
#define MSG_EVENT_ZOMBIELISTXELEMENTS_II              _MESSAGE(47260, _("%d. EVENT ZOMBIE LIST %d Elements\n"))
#define MSG_EVENT_MODSCHEDDPRIOOFJOBXTOY_IDI       _MESSAGE(47261, _("%d. EVENT MODIFY SCHEDULING PRIORITY OF JOB "U32CFormat" TO %d\n"))
#define MSG_EVENT_JOBXUSAGE_II                     _MESSAGE(47262, _("%d. EVENT JOB %d USAGE\n"))
#define MSG_EVENT_JOBXFINALUSAGE_II                _MESSAGE(47263, _("%d. EVENT JOB %d FINAL USAGE\n"))
#define MSG_EVENT_DELQUEUEX_IS                     _MESSAGE(47264, _("%d. EVENT DEL QUEUE %s\n"))
#define MSG_EVENT_ADDQUEUEX_IS                     _MESSAGE(47265, _("%d. EVENT ADD QUEUE %s\n"))
#define MSG_EVENT_MODQUEUEX_IS                     _MESSAGE(47266, _("%d. EVENT MOD QUEUE %s\n"))
#define MSG_EVENT_QUEUELISTXELEMENTS_II            _MESSAGE(47267, _("%d. EVENT QUEUE LIST %d Elements\n"))
#define MSG_EVENT_UNSUSPENDQUEUEXONSUBORDINATE_IS  _MESSAGE(47268, _("%d. EVENT UNSUSPEND QUEUE %s ON SUBORDINATE\n"))
#define MSG_EVENT_SUSPENDQUEUEXONSUBORDINATE_IS    _MESSAGE(47269, _("%d. EVENT SUSPEND QUEUE %s ON SUBORDINATE\n"))
#define MSG_EVENT_DELCOMPLEXX_IS                   _MESSAGE(47270, _("%d. EVENT DEL COMPLEX %s\n"))
#define MSG_EVENT_ADDCOMPLEXX_IS                   _MESSAGE(47271, _("%d. EVENT ADD COMPLEX %s\n"))
#define MSG_EVENT_MODCOMPLEXX_IS                   _MESSAGE(47272, _("%d. EVENT MOD COMPLEX %s\n"))
#define MSG_EVENT_COMPLEXLISTXELEMENTS_II          _MESSAGE(47273, _("%d. EVENT COMPLEX LIST %d Elements\n"))
#define MSG_EVENT_DELCONFIGX_IS                   _MESSAGE(47274, _("%d. EVENT DEL CONFIG %s\n"))
#define MSG_EVENT_ADDCONFIGX_IS                   _MESSAGE(47275, _("%d. EVENT ADD CONFIG %s\n"))
#define MSG_EVENT_MODCONFIGX_IS                   _MESSAGE(47276, _("%d. EVENT MOD CONFIG %s\n"))
#define MSG_EVENT_CONFIGLISTXELEMENTS_II          _MESSAGE(47277, _("%d. EVENT CONFIG LIST %d Elements\n"))
#define MSG_EVENT_DELCALENDARX_IS                  _MESSAGE(47278, _("%d. EVENT DEL CALENDAR %s\n"))
#define MSG_EVENT_ADDCALENDARX_IS                  _MESSAGE(47279, _("%d. EVENT ADD CALENDAR %s\n"))
#define MSG_EVENT_MODCALENDARX_IS                  _MESSAGE(47280, _("%d. EVENT MOD CALENDAR %s\n"))
#define MSG_EVENT_CALENDARLISTXELEMENTS_II         _MESSAGE(47281, _("%d. EVENT CALENDAR LIST %d Elements\n"))
#define MSG_EVENT_DELADMINHOSTX_IS                  _MESSAGE(47282, _("%d. EVENT DEL ADMINHOST %s\n"))
#define MSG_EVENT_ADDADMINHOSTX_IS                  _MESSAGE(47283, _("%d. EVENT ADD ADMINHOST %s\n"))
#define MSG_EVENT_MODADMINHOSTX_IS                  _MESSAGE(47284, _("%d. EVENT MOD ADMINHOST %s\n"))
#define MSG_EVENT_ADMINHOSTLISTXELEMENTS_II         _MESSAGE(47285, _("%d. EVENT ADMINHOST LIST %d Elements\n"))
#define MSG_EVENT_DELEXECHOSTX_IS                  _MESSAGE(47286, _("%d. EVENT DEL EXECHOST %s\n"))
#define MSG_EVENT_ADDEXECHOSTX_IS                  _MESSAGE(47287, _("%d. EVENT ADD EXECHOST %s\n"))
#define MSG_EVENT_MODEXECHOSTX_IS                  _MESSAGE(47288, _("%d. EVENT MOD EXECHOST %s\n"))
#define MSG_EVENT_EXECHOSTLISTXELEMENTS_II         _MESSAGE(47289, _("%d. EVENT EXECHOST LIST %d Elements\n"))
#define MSG_EVENT_DELFEATURE_SETX_IS                  _MESSAGE(47290, _("%d. EVENT DEL FEATURE_SET %s\n"))
#define MSG_EVENT_ADDFEATURE_SETX_IS                  _MESSAGE(47291, _("%d. EVENT ADD FEATURE_SET %s\n"))
#define MSG_EVENT_MODFEATURE_SETX_IS                  _MESSAGE(47292, _("%d. EVENT MOD FEATURE_SET %s\n"))
#define MSG_EVENT_FEATURE_SETLISTXELEMENTS_II         _MESSAGE(47293, _("%d. EVENT FEATURE_SET LIST %d Elements\n"))
#define MSG_EVENT_DELMANAGERX_IS                  _MESSAGE(47294, _("%d. EVENT DEL MANAGER %s\n"))
#define MSG_EVENT_ADDMANAGERX_IS                  _MESSAGE(47295, _("%d. EVENT ADD MANAGER %s\n"))
#define MSG_EVENT_MODMANAGERX_IS                  _MESSAGE(47296, _("%d. EVENT MOD MANAGER %s\n"))
#define MSG_EVENT_MANAGERLISTXELEMENTS_II         _MESSAGE(47297, _("%d. EVENT MANAGER LIST %d Elements\n"))
#define MSG_EVENT_DELOPERATORX_IS                  _MESSAGE(47298, _("%d. EVENT DEL OPERATOR %s\n"))
#define MSG_EVENT_ADDOPERATORX_IS                  _MESSAGE(47299, _("%d. EVENT ADD OPERATOR %s\n"))
#define MSG_EVENT_MODOPERATORX_IS                  _MESSAGE(47300, _("%d. EVENT MOD OPERATOR %s\n"))
#define MSG_EVENT_OPERATORLISTXELEMENTS_II         _MESSAGE(47301, _("%d. EVENT OPERATOR LIST %d Elements\n"))
#define MSG_EVENT_DELSUBMITHOSTX_IS                  _MESSAGE(47302, _("%d. EVENT DEL SUBMITHOST %s\n"))
#define MSG_EVENT_ADDSUBMITHOSTX_IS                  _MESSAGE(47303, _("%d. EVENT ADD SUBMITHOST %s\n"))
#define MSG_EVENT_MODSUBMITHOSTX_IS                  _MESSAGE(47304, _("%d. EVENT MOD SUBMITHOST %s\n"))
#define MSG_EVENT_SUBMITHOSTLISTXELEMENTS_II         _MESSAGE(47305, _("%d. EVENT SUBMITHOST LIST %d Elements\n"))
#define MSG_EVENT_DELUSERSETX_IS                   _MESSAGE(47306, _("%d. EVENT DEL USER SET %s\n"))
#define MSG_EVENT_ADDUSERSETX_IS                   _MESSAGE(47307, _("%d. EVENT ADD USER SET %s\n"))
#define MSG_EVENT_MODUSERSETX_IS                   _MESSAGE(47308, _("%d. EVENT MOD USER SET %s\n"))
#define MSG_EVENT_USERSETLISTXELEMENTS_II          _MESSAGE(47309, _("%d. EVENT USER SET LIST %d Elements\n"))
#define MSG_EVENT_DELUSERX_IS                      _MESSAGE(47310, _("%d. EVENT DEL USER %s\n"))
#define MSG_EVENT_ADDUSERX_IS                      _MESSAGE(47311, _("%d. EVENT ADD USER %s\n"))
#define MSG_EVENT_MODUSERX_IS                      _MESSAGE(47312, _("%d. EVENT MOD USER %s\n"))
#define MSG_EVENT_USERLISTXELEMENTS_II             _MESSAGE(47313, _("%d. EVENT USER LIST %d Elements\n"))
#define MSG_EVENT_DELPROJECTX_IS                   _MESSAGE(47314, _("%d. EVENT DEL PROJECT %s\n"))
#define MSG_EVENT_ADDPROJECTX_IS                   _MESSAGE(47315, _("%d. EVENT ADD PROJECT %s\n"))
#define MSG_EVENT_MODPROJECTX_IS                   _MESSAGE(47316, _("%d. EVENT MOD PROJECT %s\n"))
#define MSG_EVENT_PROJECTLISTXELEMENTS_II          _MESSAGE(47317, _("%d. EVENT PROJECT LIST %d Elements\n"))
#define MSG_EVENT_DELPEX_IS                        _MESSAGE(47318, _("%d. EVENT DEL PE %s\n"))
#define MSG_EVENT_ADDPEX_IS                        _MESSAGE(47319, _("%d. EVENT ADD PE %s\n"))
#define MSG_EVENT_MODPEX_IS                        _MESSAGE(47320, _("%d. EVENT MOD PE %s\n"))
#define MSG_EVENT_PELISTXELEMENTS_II               _MESSAGE(47321, _("%d. EVENT PE LIST %d Elements\n"))
#define MSG_EVENT_SHUTDOWN_I                       _MESSAGE(47322, _("%d. EVENT SHUTDOWN\n"))
#define MSG_EVENT_QMASTERGOESDOWN_I                _MESSAGE(47323, _("%d. EVENT QMASTER GOES DOWN\n"))
#define MSG_EVENT_TRIGGERSCHEDULERMONITORING_I     _MESSAGE(47324, _("%d. EVENT TRIGGER SCHEDULER MONITORING\n"))
#define MSG_EVENT_SHARETREEXNODESYLEAFS_III        _MESSAGE(47325, _("%d. EVENT SHARETREE %d nodes %d leafs\n"))
#define MSG_EVENT_SCHEDULERCONFIG_I                _MESSAGE(47326, _("%d. EVENT SCHEDULER CONFIG \n"))
#define MSG_EVENT_GLOBAL_CONFIG_I                  _MESSAGE(47327, _("%d. EVENT NEW GLOBAL CONFIG\n"))
#define MSG_EVENT_DELCKPT_IS                       _MESSAGE(47328, _("%d. EVENT DEL CKPT %s\n"))
#define MSG_EVENT_ADDCKPT_IS                       _MESSAGE(47329, _("%d. EVENT ADD CKPT %s\n"))
#define MSG_EVENT_MODCKPT_IS                       _MESSAGE(47330, _("%d. EVENT MOD CKPT %s\n"))
#define MSG_EVENT_CKPTLISTXELEMENTS_II             _MESSAGE(47331, _("%d. EVENT CKPT LIST %d Elements\n"))
#define MSG_EVENT_DELJATASK_UUU                    _MESSAGE(47332, _(U32CFormat". EVENT DEL JATASK "U32CFormat"."U32CFormat"\n"))
#define MSG_EVENT_MODJATASK_UUU                    _MESSAGE(47333, _(U32CFormat". EVENT MOD JATASK "U32CFormat"."U32CFormat"\n"))
#define MSG_EVENT_NOTKNOWN_I                       _MESSAGE(47334, _("%d. EVENT ????????\n"))


/* 
** schedd/sge.c
*/ 
#define MSG_FILE_OPENSTDOUTASFILEFAILED            _MESSAGE(47335, _("Could not open stdout as file\n"))
#define MSG_SGE_UNABLETODUMPJOBLIST                _MESSAGE(47336, _("Unable to dump job list\n"))

/* 
** schedd/sge_share_mon.c
*/ 
#define MSG_USAGE				_MESSAGE(47337, _("usage:"))
#define MSG_SGESHAREMON_NOSHARETREE             _MESSAGE(47338, _("No share tree"))
#define MSG_SGESHAREMON_c_OPT_USAGE             _MESSAGE(47339, _("number of collections (default is infinite)\n"))
#define MSG_SGESHAREMON_d_OPT_USAGE             _MESSAGE(47340, _("delimiter between columns (default is <TAB>)\n"))
#define MSG_SGESHAREMON_f_OPT_USAGE             _MESSAGE(47341, _("list of fields to print\n"))
#define MSG_SGESHAREMON_h_OPT_USAGE             _MESSAGE(47342, _("print a header containing the field names\n"))
#define MSG_SGESHAREMON_i_OPT_USAGE             _MESSAGE(47343, _("collection interval in seconds (default is 15)\n"))
#define MSG_SGESHAREMON_l_OPT_USAGE             _MESSAGE(47344, _("delimiter between nodes (default is <CR>)\n"))
#define MSG_SGESHAREMON_m_OPT_USAGE             _MESSAGE(47345, _("output file fopen mode (default is \"w\")\n"))
#define MSG_SGESHAREMON_n_OPT_USAGE             _MESSAGE(47346, _("use name=value format\n"))
#define MSG_SGESHAREMON_o_OPT_USAGE             _MESSAGE(47347, _("output file\n"))
#define MSG_SGESHAREMON_r_OPT_USAGE             _MESSAGE(47348, _("delimiter between collection records (default is <CR>)\n"))
#define MSG_SGESHAREMON_s_OPT_USAGE             _MESSAGE(47349, _("format of displayed strings (default is %%s)\n"))
#define MSG_SGESHAREMON_t_OPT_USAGE             _MESSAGE(47350, _("show formatted times\n"))
#define MSG_SGESHAREMON_u_OPT_USAGE             _MESSAGE(47351, _("show decayed usage (since timestamp) in nodes\n"))
#define MSG_SGESHAREMON_x_OPT_USAGE             _MESSAGE(47352, _("exclude non-leaf nodes\n"))

#define MSG_ERROR_XISNOTAVALIDINTERVAL_S        _MESSAGE(47353, _("%s is not a valid interval\n"))
#define MSG_ERROR_XISNOTAVALIDCOUNT_S           _MESSAGE(47354, _("%s is not a valid count\n"))
#define MSG_FILE_COULDNOTOPENXFORY_SS           _MESSAGE(47355, _("could not open %s for %s\n"))
/* 
** schedd/schedd_conf.c
*/ 
#define MSG_ATTRIB_ALGORITHMNOVALIDNAME         _MESSAGE(47356, _("attribute \"algorithm\" is not a valid algorithm name.\n"))
#define MSG_ATTRIB_SCHEDDJOBINFONOVALIDPARAM    _MESSAGE(47357, _("attribute \"schedd_job_info \" is not a valid parameter\n"))
#define MSG_ATTRIB_SCHEDDJOBINFONOVALIDJOBLIST  _MESSAGE(47358, _("attribute \"schedd_job_info\" is not a valid job_list\n"))
#define MSG_ATTRIB_USINGXASY_SS                 _MESSAGE(47359, _("using \"%s\" as %s\n"))
#define MSG_ATTRIB_XISNOTAY_SS                  _MESSAGE(47360, _("attribute \"%s\" is not a %s\n"))
#define MSG_ATTRIB_USINGXFORY_SS                _MESSAGE(47361, _("using \"%s\" for %s\n"))
#define MSG_ATTRIB_USINGXFORY_US                _MESSAGE(47362, _("using " U32CFormat " for %s\n"))
#define MSG_ATTRIB_USINGXFORY_6FS               _MESSAGE(47363, _("using %.6g for %s\n"))
#define MSG_TRUE                                _MESSAGE(47364, _("true"))
#define MSG_FALSE                               _MESSAGE(47365, _("false"))
#define MSG_NONE_NOT_ALLOWED                    _MESSAGE(47366, _("The keyword \"none\" is not allowed in \"load_formula\"\n"))
#define MSG_NOTEXISTING_ATTRIBUTE_S             _MESSAGE(47367, _("\"load_formula\" references not existing complex attribute "SFQ"\n"))
#define MSG_WRONGTYPE_ATTRIBUTE_S               _MESSAGE(47368, _("String, CString or Host attributes are not allowed in \"load_formula\": " SFQ "\n"))

/* 
** schedd/schedd_message.c
*/ 
#define MSG_SCHEDDMESSAGE_CREATEJOBINFOFORMESSAGEFAILED_U      _MESSAGE(47369, _("can not create schedd_job_info for message "U32CFormat"\n"))






/* 
** schedd/slots_used.c
*/ 
#define MSG_SLOTSUSED_SLOTSENTRYINQUEUEMISSING_S     _MESSAGE(47370, _("missing \"slots\" entry in consumable actual list of queue \"%s\"\n"  ) )  

/* 
** schedd/sort_hosts.c
*/   
#define MSG_ATTRIB_NOATTRIBXINCOMPLEXLIST_S           _MESSAGE(47371, _("no attribute \"%s\" in complex list\n"))

/* 
** schedd/valid_queue_user.c
*/ 
#define MSG_VALIDQUEUEUSER_GRPXALLREADYINUSERSETY_SS  _MESSAGE(47372, _("Group \"%s\" already contained in userset \"%s\"\n"))
#define MSG_VALIDQUEUEUSER_USRXALLREADYINUSERSETY_SS  _MESSAGE(47373, _("User \"%s\" already contained in userset \"%s\"\n"))

#endif /* __MSG_SCHEDD_H */
