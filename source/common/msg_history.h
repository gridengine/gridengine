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
** history/complex_history.c
*/ 
#define MSG_POINTER_DIRNAMEORPPLISTISNULLININITHISTORYSUBDIRS  _MESSAGE(25000, _("dirname or pplist == NULL in init_history_subdirs()\n"))
#define MSG_POINTER_LDIRORLSUBDIRISNULLININITHISTORYFILES      _MESSAGE(25001, _("ldir or lsubdir or pplist == NULL in init_history_files()"))
#define MSG_HISTORY_DIRECTORYLISTHASNONAME                     _MESSAGE(25002, _("directory list has no name"))
#define MSG_HISTORY_SUBDIRHASZEROLENGTHNAME                    _MESSAGE(25004, _("subdirectory has zero length name"))
#define MSG_FILE_CANTOPENFILEX_S                               _MESSAGE(25005, _("can't open file \"%s\"\n"))
#define MSG_POINTER_LDIRORVERSIONSUBDIRORPPELEMISNULLINFINDCOMPLEXVERSION  _MESSAGE(25006, _("ldir or version_subdir or ppelem == NULL in find_complex_version()"))
#define MSG_HISTORY_INVALIDDATEINFINDCOMPLEXVERSION            _MESSAGE(25007, _("invalid date in find_complex_version()\n"))
#define MSG_HISTORY_CANTINITHISTORYFILESX_I                    _MESSAGE(25008, _("can't initialize history files: %d"))
#define MSG_HISTORY_CANTASSEMBLEFILENAMEX_I                    _MESSAGE(25009, _("can't assemble file name: %d"))
#define MSG_HISTORY_INVALIDDATEINFINDHOSTVERSION               _MESSAGE(25010, _("invalid date in find_host_version()\n"))
#define MSG_HISTORY_CULLREADINHOSTRETURNEDNOHOST               _MESSAGE(25011, _("cull_read_in_host returned no host"))
#define MSG_POINTER_LDIRORVERSIONSUBDIRORPPELEMISNULLINFINDQUEUEVERSION _MESSAGE(25012, _("ldir or version_subdir or ppelem == NULL in find_queue_version()"))
#define MSG_HISTORY_INVALIDDATINFINDQUEUEVERSION               _MESSAGE(25013, _("invalid date in find_queue_version()"))
#define MSG_HISTORY_CANTFINDQUEUEVERSION                       _MESSAGE(25014, _("can't find queue version"))
#define MSG_HISTORY_FINDQUEUEVERSIONERROR                      _MESSAGE(25015, _("find_queue_version: error finding queue version\n"))
#define MSG_POINTER_PPLISTISNULLINMAKECOMPLEXLIST              _MESSAGE(25016, _("pplist == NULL in make_complex_list()"))
#define MSG_HISTORY_CANTFINDCOMPLEXVERSIONX_I                  _MESSAGE(25017, _("can't find complex version: %d"))
#define MSG_HISTORY_NOCOMPLEXESFOUND                           _MESSAGE(25018, _("no complexes found"))
#define MSG_POINTER_QUEUENAMEISNULLINFINDQUEUEVERSIONBYNAME    _MESSAGE(25019, _("queue_name == NULL in find_queue_version_by_name()"))
#define MSG_HISTORY_QUEUENAMEHASZEROLENGTHINFINDQUEUEVERSIONBYNAME _MESSAGE(25020, _("queue_name has zero length in find_queue_version_by_name()"))
#define MSG_MEMORY_CANTALLOCATEMEMORY                          _MESSAGE(25021, _("can't allocate memory" ))
#define MSG_HISTORY_CANTFINDQUEUEVERSIONFORQUEUEXREFTOYZ_SSI    _MESSAGE(25022, _("can't find queue version for queue \"%s\" referring to \"%s\": %d"               ))
#define MSG_POINTER_COMPLEXNAMEISNULLINFINDCOMPLEXVERSIONBYNAME   _MESSAGE(25023, _("complex_name == NULL in find_complex_version_by_name()"))
#define MSG_HISTORY_NOCOMPLEXNAMEGIVENINFINDCOMPLEXVERSIONBYNAME  _MESSAGE(25024, _("no complex name given in find_complex_version_by_name()"))
#define MSG_HISTORY_SUBDIRHASNONAME                               _MESSAGE(25025, _("subdirectory has no name"))
#define MSG_HISTORY_FINDCOMPLEXVERSIONBYNAMEERRORXRETRIEVENCOMPLEXVERSIONFORY_IS _MESSAGE(25026, _("find_complex_version_by_name: error %d retrieving complex version for %s\n"))
#define MSG_MEMORY_HOSTNAMEISNULLINFINDHOSTVERSIONBYNAME    _MESSAGE(25027, _("host_name == NULL in find_host_version_by_name()"))
#define MSG_HISTORY_NOHOSTNAMEGIVENINFINDHOSTVERSIONBYNAME  _MESSAGE(25028, _("no host name given in find_host_version_by_name()"))
#define MSG_HISTORY_FINDHOSTVERSIONBYNAMEERRORXRETRIEVINGHOSTVERSIONFORY_IS   _MESSAGE(25029, _("find_host_version_by_name: error %d retrieving host version for %s\n"))
#define MSG_POINTER_CREATEVERSIONSUBDIRNULLPOINTERRECEIVED  _MESSAGE(25030, _("create_version_subdir: NULL pointer received\n"))
#define MSG_HISTORY_CREATEVERSIONSUBDIRDIRHASZERONAME       _MESSAGE(25031, _("create_version_subdir: directory has zero name\n"))
#define MSG_HISTORY_CREATEVERSIONSUBDIRMEMORYALLOCFAILED    _MESSAGE(25032, _("create_version_subdir: memory allocation fault\n"))
#define MSG_HISTORY_CREATEVERSIONSUBDIRERRORXCREATINGDIR_I  _MESSAGE(25033, _("create_version_subdir: error %d creating directory\n" ))
#define MSG_HISTORY_FINDVERSSUBDIRBYNAMENULLPOINTERRECEIVED _MESSAGE(25034, _("find_version_subdir_by_name: NULL pointer received\n"))
#define MSG_HISTORY_FINDVERSSUBDIRBYNAMEDIRHASZERONAME      _MESSAGE(25035, _("find_version_subdir_by_name: directory has zero name\n"))
#define MSG_HISTORY_FINDVERSSUBDIRBYNAMEENCOUNTEREDSUBDIRWITHNONAME  _MESSAGE(25036, _("find_version_subdir_by_name: encountered subdir with no name\n"))
#define MSG_HISTORY_FINDVERSSUBDIRBYNAMEENCOUNTEREDSUBDIRWITHZERONAME _MESSAGE(25037, _("find_version_subdir_by_name: encountered subdir with zero name\n"))
#define MSG_HISTORY_FINDVERSSUBDIRBYNAMEMEMORYALLOCFAILED   _MESSAGE(25038, _("find_version_subdir_by_name: memory allocation failed\n"))
#define MSG_POINTER_WRITECOMPLEXHOSTVERSIONNULLPOINTERRECEIVED _MESSAGE(25039, _("write_complex_host_version: NULL pointer received\n"))
#define MSG_HISTORY_WRITECOMPLEXHOSTVERSIONMEMORYALLOCFAILED          _MESSAGE(25040, _("write_complex_host_version: memory allocation fault\n"))
#define MSG_HISTORY_WRITECOMPLEXHOSTVERSIONERRORXMAKEINFFILENAME_I   _MESSAGE(25041, _("write_complex_host_version: error %d making file name\n"))
#define MSG_HISTORY_WRITECOMPLEXHOSTVERSIONERRORXWRITEINGCOMPLEX_I   _MESSAGE(25042, _("write_complex_host_version: error %d writing complex\n"))
#define MSG_POINTER_WRITEQUEUEVERSIONNULLPOINTERRECEIVED             _MESSAGE(25043, _("write_queue_version: NULL pointer received\n"))
#define MSG_HISTORY_WRITEQUEUEVERSIONMEMORYALLOCFAILED               _MESSAGE(25044, _("write_queue_version: memory allocation fault\n"))
#define MSG_HISTORY_WRITEQUEUEVERSIONERRORXMAKINGFILENAME_I          _MESSAGE(25045, _("write_queue_version: error %d making file name\n"))
#define MSG_HISTORY_WRITEQUEUEVERSIONERRORXWRITINGQUEUE_I            _MESSAGE(25046, _("write_queue_version: error %d writing queue\n"))
#define MSG_HISTORY_SGEWRITEQUEUEVERSIONNULLPOINTER                  _MESSAGE(25047, _("sge_write_queue_version: NULL pointer received\n"))
#define MSG_HISTORY_SGEWRITEQUEUEVERSIONMEMORY                       _MESSAGE(25048, _("sge_write_queue_version: memory allocation fault\n"))
#define MSG_HISTORY_SGEWRITEQUEUEVERSIONERRORXMAKEFILENAME_I         _MESSAGE(25049, _("sge_write_queue_version: error %d making file name\n"))
#define MSG_HISTORY_SGEWRITEQUEUEVERSIONERRORXWRITINGQUEUE_I         _MESSAGE(25050, _("sge_write_queue_version: error %d writing queue\n"))
#define MSG_HISTORY_XPREPVERSSUBDIRFORQUEUEY_IS                      _MESSAGE(25051, _("%d preparing version subdir for queue %s\n"))
#define MSG_HISTORY_XWRITINGTOVERSSUBDIRTHECONFIGFORQUEUEY_IS        _MESSAGE(25052, _("%d writing to version subdir the configuration for queue %s\n"))
#define MSG_HISTORY_WRITEHOSTHISTORYERRORXPREPVERSSUBDIRFORHOSTY_IS  _MESSAGE(25053, _("write_host_history: error %d preparing version subdir for host %s\n"))
#define MSG_HISTORY_WRITEHOSTHISTORYERRORXWRITINGTOVERSSUBDIRTHECONFIGFORHOSTY_IS  _MESSAGE(25054, _("write_host_history: error %d writing to version subdir the configuration for host %s\n" ))
#define MSG_HISTORY_WRITECOMPLEXHISTORYERRORXPREPARINGVERSIONSUBIDRFORCOMPLEXY_IS  _MESSAGE(25055, _("write_complex_history: error %d preparing version subdir for complex %s\n" ))
#define MSG_HISTORY_WRITECOMPLEXHISTORYERRORXWRITINGTOVERSIONSUBDIRTHECONFIGFORCOMPLEXY_IS  _MESSAGE(25056, _("write_complex_history: error %d writing to version subdir the configuration for complex %s\n" ))
#define MSG_HISTORY_PREPAREVERSIONSUBDIRNULLPOINTER               _MESSAGE(25057, _("prepare_version_subdir: NULL pointer received\n"))
#define MSG_HISTORY_PREPAREVERSIONSUBDIRNOHISTSUBDIRCONFIGURED    _MESSAGE(25058, _("prepare_version_subdir: no history subdir configured\n"))
#define MSG_HISTORY_PREPAREVERSIONSUBDIRMEMORYALLOCFAILED         _MESSAGE(25059, _("prepare_version_subdir: memory allocation fault\n"))
#define MSG_HISTORY_PREPAREVERSIONSUBDIRERRORXREADINGHISTSUBDIRY_IS  _MESSAGE(25060, _("prepare_version_subdir: error %d reading history subdirectory %s\n"))
#define MSG_HISTORY_PREPAREVERSIONSUBDIRERRORXFINDINGVERSIONSUBDIRFOROBJY_IS  _MESSAGE(25061, _("prepare_version_subdir: error %d finding version subdir for object %s\n"))
#define MSG_HISTORY_PREPAREVERSIONSUBDIRERRORXFINDVERSINSUBDIRFOROBJY_IS   _MESSAGE(25062, _("prepare_version_subdir: error %d finding version subdir for object %s\n"))
#define MSG_HISTORY_ISOBJECTINHISTORYNULLPOINTER            _MESSAGE(25063, _("is_object_in_history: NULL pointer received\n"))
#define MSG_HISTORY_ISOBJECTINHISTORYNOHISTSUBDIRCONFIGURED _MESSAGE(25064, _("is_object_in_history: no history subdir configured\n"))
#define MSG_HISTORY_PREPAREVERSIONSUBDIR                    _MESSAGE(25065, _("prepare_version_subdir: memory allocation fault\n"))

/* 
** history/qacct.c
*/ 
#define MSG_HISTORY_NOJOBSRUNNINGSINCESTARTUP      _MESSAGE(25066, _("no jobs running since startup\n"))
#define MSG_HISTORY_FAILEDRESOLVINGHOSTNAME_SS     _MESSAGE(25067, _("failed resolving hostname \"%s\": %s\n"))
#define MSG_HISTORY_TOPTIONMASTHAVELISTOFTASKIDRANGES  _MESSAGE(25068, _("ERROR! -t option must have a list of task id ranges\n"))
#define MSG_HISTORY_INVALIDLISTOFTASKIDRANGES_S       _MESSAGE(25069, _("ERROR! invalid list of task id ranges: %s\n"))
#define MSG_HISTORY_QMASTERISNOTALIVE                 _MESSAGE(25070, _("qmaster is not alive"))
#define MSG_HISTORY_QACCTERRORXREADHISTORYSUBDIRFORCOMPLEXES_I   _MESSAGE(25071, _("qacct: error %d reading history subdirectories for complexes\n"))
#define MSG_HISTORY_QACCTERRORXREADHISTORYSUBDIRFORQUEUES_I      _MESSAGE(25072, _("qacct: error %d reading history subdirectories for queues\n"))
#define MSG_HISTORY_QACCTERRORXREADHISTORYSUBDIRFOREXECHOSTS_I   _MESSAGE(25073, _("qacct: error %d reading history subdirectories for exechosts\n"))
#define MSG_HISTORY_ERRORUNABLETOOPENX_S     _MESSAGE(25074, _("error unable to open %s\n"))
#define MSG_HISTORY_NOTENOUGTHMEMORYTOCREATELIST   _MESSAGE(25075, _("not enough memory to create list\n"))
#define MSG_HISTORY_IGNORINGINVALIDENTRYINLINEX_U  _MESSAGE(25076, _("ignoring invalid entry in line " U32CFormat "\n"))
#define MSG_HISTORY_COMPLEXSTRUCTUREREFTOXCANTBEASSEMBLEDY_SI  _MESSAGE(25077, _("complex structure referring to %s can't be assembled(%d)\n"))
#define MSG_HISTORY_USENOHISTTOREFTOCURCOMPLEXCONFIG           _MESSAGE(25078, _("Use -nohist to refer to current complex configuration.\n"))
#define MSG_HISTORY_QUEUECONFIGFORXREFTOYCANTBEASSEMBLEDY_SSI  _MESSAGE(25079, _("queue configuration for queue %s referring to %s can't be assembled(%d)\n"))
#define MSG_HISTORY_HOSTCONFIGFORHOSTXREFTOYCANTBEASSEMBLEDYFORJOBZ_SSIU   _MESSAGE(25080, _("host configuration for host %s referring to %s can't be assembled(%d) for job " U32CFormat"\n"))
#define MSG_HISTORY_GLOBALHOSTCONFIGFORGLOBALHOSTXREFTOYCANTBEASSEMBLEDYFORJOBZ_SSID   _MESSAGE(25081, _("global_host configuration for global_host %s referring to %s can't be assembled(%d) for job " U32CFormat"\n"))
#define MSG_HISTORY_IGNORINGJOBXFORACCOUNTINGMASTERQUEUEYNOTEXISTS_IS   _MESSAGE(25082, _("ignoring job %d for accounting: jobs master queue %s does not longer exist\n"))
#define MSG_HISTORY_COMPLEXTEMPLATESCANTBEFILLEDCORRECTLYFORJOBX_D   _MESSAGE(25083, _("complex templates can't be filled correctly for job " U32CFormat"\n"))
#define MSG_HISTORY_JOBARRAYTASKSWXYZNOTFOUND_DDDD   _MESSAGE(25084, _("Job-array tasks "U32CFormat"."U32CFormat"-"U32CFormat":"U32CFormat" not found\n" ))
#define MSG_HISTORY_JOBIDXNOTFOUND_D          _MESSAGE(25085, _("job id " U32CFormat  " not found\n"  ))
#define MSG_HISTORY_JOBARRAYTASKSWXYZNOTFOUND_SDDD    _MESSAGE(25086, _("Job-array tasks \"%s\"."U32CFormat"-"U32CFormat":"U32CFormat" not found\n"))
#define MSG_HISTORY_JOBNAMEXNOTFOUND_S       _MESSAGE(25087, _("job name %s not found\n"))
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
#define MSG_HISTORY_history_OPT_USAGE    _MESSAGE(25107, _("read history for complex attributes from given directory\n"))
#define MSG_HISTORY_nohist_OPT_USAGE      _MESSAGE(25108, _("do not use historical data for complex attribute matching\n"))
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
