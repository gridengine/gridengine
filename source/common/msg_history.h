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
 *  License at http://www.gridengine.sunsource.net/license.html
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
#define MSG_POINTER_DIRNAMEORPPLISTISNULLININITHISTORYSUBDIRS  _("dirname or pplist == NULL in init_history_subdirs()\n")
#define MSG_POINTER_LDIRORLSUBDIRISNULLININITHISTORYFILES      _("ldir or lsubdir or pplist == NULL in init_history_files()")
#define MSG_HISTORY_DIRECTORYLISTHASNONAME                     _("directory list has no name")
#define MSG_HISTORY_SUBDIRHASNONAME                            _("subdirectory has no name")
#define MSG_HISTORY_SUBDIRHASZEROLENGTHNAME                    _("subdirectory has zero length name")
#define MSG_FILE_CANTOPENFILEX_S                               _("can't open file \"%s\"\n")
#define MSG_POINTER_LDIRORVERSIONSUBDIRORPPELEMISNULLINFINDCOMPLEXVERSION  _("ldir or version_subdir or ppelem == NULL in find_complex_version()")
#define MSG_HISTORY_INVALIDDATEINFINDCOMPLEXVERSION            _("invalid date in find_complex_version()\n")
#define MSG_HISTORY_CANTINITHISTORYFILESX_I                    _("can't initialize history files: %d")
#define MSG_HISTORY_CANTASSEMBLEFILENAMEX_I                    _("can't assemble file name: %d")
#define MSG_HISTORY_INVALIDDATEINFINDHOSTVERSION               _("invalid date in find_host_version()\n")
#define MSG_HISTORY_CULLREADINHOSTRETURNEDNOHOST               _("cull_read_in_host returned no host")
#define MSG_POINTER_LDIRORVERSIONSUBDIRORPPELEMISNULLINFINDQUEUEVERSION _("ldir or version_subdir or ppelem == NULL in find_queue_version()")
#define MSG_HISTORY_INVALIDDATINFINDQUEUEVERSION               _("invalid date in find_queue_version()")
#define MSG_HISTORY_CANTFINDQUEUEVERSION                       _("can't find queue version")
#define MSG_HISTORY_FINDQUEUEVERSIONERROR                      _("find_queue_version: error finding queue version\n")
#define MSG_POINTER_PPLISTISNULLINMAKECOMPLEXLIST              _("pplist == NULL in make_complex_list()")
#define MSG_HISTORY_CANTFINDCOMPLEXVERSIONX_I                  _("can't find complex version: %d")
#define MSG_HISTORY_NOCOMPLEXESFOUND                           _("no complexes found")
#define MSG_POINTER_QUEUENAMEISNULLINFINDQUEUEVERSIONBYNAME    _("queue_name == NULL in find_queue_version_by_name()")
#define MSG_HISTORY_QUEUENAMEHASZEROLENGTHINFINDQUEUEVERSIONBYNAME _("queue_name has zero length in find_queue_version_by_name()")
#define MSG_MEMORY_CANTALLOCATEMEMORY                          _("can't allocate memory" )
#define MSG_HISTORY_CANTFINDQUEUEVERSIONFORQUEUEXREFTOYZ_SSI    _("can't find queue version for queue \"%s\" referring to \"%s\": %d"               )
#define MSG_POINTER_COMPLEXNAMEISNULLINFINDCOMPLEXVERSIONBYNAME   _("complex_name == NULL in find_complex_version_by_name()")
#define MSG_HISTORY_NOCOMPLEXNAMEGIVENINFINDCOMPLEXVERSIONBYNAME  _("no complex name given in find_complex_version_by_name()")
#define MSG_HISTORY_SUBDIRHASNONAME                               _("subdirectory has no name")
#define MSG_HISTORY_FINDCOMPLEXVERSIONBYNAMEERRORXRETRIEVENCOMPLEXVERSIONFORY_IS _("find_complex_version_by_name: error %d retrieving complex version for %s\n")
#define MSG_MEMORY_HOSTNAMEISNULLINFINDHOSTVERSIONBYNAME    _("host_name == NULL in find_host_version_by_name()")
#define MSG_HISTORY_NOHOSTNAMEGIVENINFINDHOSTVERSIONBYNAME  _("no host name given in find_host_version_by_name()")
#define MSG_HISTORY_FINDHOSTVERSIONBYNAMEERRORXRETRIEVINGHOSTVERSIONFORY_IS   _("find_host_version_by_name: error %d retrieving host version for %s\n")
#define MSG_POINTER_CREATEVERSIONSUBDIRNULLPOINTERRECEIVED  _("create_version_subdir: NULL pointer received\n")
#define MSG_HISTORY_CREATEVERSIONSUBDIRDIRHASZERONAME       _("create_version_subdir: directory has zero name\n")
#define MSG_HISTORY_CREATEVERSIONSUBDIRMEMORYALLOCFAILED    _("create_version_subdir: memory allocation fault\n")
#define MSG_HISTORY_CREATEVERSIONSUBDIRERRORXCREATINGDIR_I  _("create_version_subdir: error %d creating directory\n" )
#define MSG_HISTORY_FINDVERSSUBDIRBYNAMENULLPOINTERRECEIVED _("find_version_subdir_by_name: NULL pointer received\n")
#define MSG_HISTORY_FINDVERSSUBDIRBYNAMEDIRHASZERONAME      _("find_version_subdir_by_name: directory has zero name\n")
#define MSG_HISTORY_FINDVERSSUBDIRBYNAMEENCOUNTEREDSUBDIRWITHNONAME  _("find_version_subdir_by_name: encountered subdir with no name\n")
#define MSG_HISTORY_FINDVERSSUBDIRBYNAMEENCOUNTEREDSUBDIRWITHZERONAME _("find_version_subdir_by_name: encountered subdir with zero name\n")
#define MSG_HISTORY_FINDVERSSUBDIRBYNAMEMEMORYALLOCFAILED   _("find_version_subdir_by_name: memory allocation failed\n")
#define MSG_POINTER_WRITECOMPLEXHOSTVERSIONNULLPOINTERRECEIVED _("write_complex_host_version: NULL pointer received\n")
#define MSG_HISTORY_WRITECOMPLEXHOSTVERSIONMEMORYALLOCFAILED          _("write_complex_host_version: memory allocation fault\n")
#define MSG_HISTORY_WRITECOMPLEXHOSTVERSIONERRORXMAKEINFFILENAME_I   _("write_complex_host_version: error %d making file name\n")
#define MSG_HISTORY_WRITECOMPLEXHOSTVERSIONERRORXWRITEINGCOMPLEX_I   _("write_complex_host_version: error %d writing complex\n")
#define MSG_POINTER_WRITEQUEUEVERSIONNULLPOINTERRECEIVED             _("write_queue_version: NULL pointer received\n")
#define MSG_HISTORY_WRITEQUEUEVERSIONMEMORYALLOCFAILED               _("write_queue_version: memory allocation fault\n")
#define MSG_HISTORY_WRITEQUEUEVERSIONERRORXMAKINGFILENAME_I          _("write_queue_version: error %d making file name\n")
#define MSG_HISTORY_WRITEQUEUEVERSIONERRORXWRITINGQUEUE_I            _("write_queue_version: error %d writing queue\n")
#define MSG_HISTORY_SGEWRITEQUEUEVERSIONNULLPOINTER                  _("sge_write_queue_version: NULL pointer received\n")
#define MSG_HISTORY_SGEWRITEQUEUEVERSIONMEMORY                       _("sge_write_queue_version: memory allocation fault\n")
#define MSG_HISTORY_SGEWRITEQUEUEVERSIONERRORXMAKEFILENAME_I         _("sge_write_queue_version: error %d making file name\n")
#define MSG_HISTORY_SGEWRITEQUEUEVERSIONERRORXWRITINGQUEUE_I         _("sge_write_queue_version: error %d writing queue\n")
#define MSG_HISTORY_XPREPVERSSUBDIRFORQUEUEY_IS                      _("%d preparing version subdir for queue %s\n")
#define MSG_HISTORY_XWRITINGTOVERSSUBDIRTHECONFIGFORQUEUEY_IS        _("%d writing to version subdir the configuration for queue %s\n")
#define MSG_HISTORY_WRITEHOSTHISTORYERRORXPREPVERSSUBDIRFORHOSTY_IS  _("write_host_history: error %d preparing version subdir for host %s\n")
#define MSG_HISTORY_WRITEHOSTHISTORYERRORXWRITINGTOVERSSUBDIRTHECONFIGFORHOSTY_IS  _("write_host_history: error %d writing to version subdir the configuration for host %s\n" )
#define MSG_HISTORY_WRITECOMPLEXHISTORYERRORXPREPARINGVERSIONSUBIDRFORCOMPLEXY_IS  _("write_complex_history: error %d preparing version subdir for complex %s\n" )
#define MSG_HISTORY_WRITECOMPLEXHISTORYERRORXWRITINGTOVERSIONSUBDIRTHECONFIGFORCOMPLEXY_IS  _("write_complex_history: error %d writing to version subdir the configuration for complex %s\n" )
#define MSG_HISTORY_PREPAREVERSIONSUBDIRNULLPOINTER               _("prepare_version_subdir: NULL pointer received\n")
#define MSG_HISTORY_PREPAREVERSIONSUBDIRNOHISTSUBDIRCONFIGURED    _("prepare_version_subdir: no history subdir configured\n")
#define MSG_HISTORY_PREPAREVERSIONSUBDIRMEMORYALLOCFAILED         _("prepare_version_subdir: memory allocation fault\n")
#define MSG_HISTORY_PREPAREVERSIONSUBDIRERRORXREADINGHISTSUBDIRY_IS  _("prepare_version_subdir: error %d reading history subdirectory %s\n")
#define MSG_HISTORY_PREPAREVERSIONSUBDIRERRORXFINDINGVERSIONSUBDIRFOROBJY_IS  _("prepare_version_subdir: error %d finding version subdir for object %s\n")
#define MSG_HISTORY_PREPAREVERSIONSUBDIRERRORXFINDVERSINSUBDIRFOROBJY_IS   _("prepare_version_subdir: error %d finding version subdir for object %s\n")
#define MSG_HISTORY_ISOBJECTINHISTORYNULLPOINTER            _("is_object_in_history: NULL pointer received\n")
#define MSG_HISTORY_ISOBJECTINHISTORYNOHISTSUBDIRCONFIGURED _("is_object_in_history: no history subdir configured\n")
#define MSG_HISTORY_PREPAREVERSIONSUBDIR                    _("prepare_version_subdir: memory allocation fault\n")

/* 
** history/qacct.c
*/ 
#define MSG_HISTORY_NOJOBSRUNNINGSINCESTARTUP      _("no jobs running since startup\n")
#define MSG_HISTORY_FAILEDRESOLVINGHOSTNAME_SS     _("failed resolving hostname \"%s\": %s\n")
#define MSG_HISTORY_TOPTIONMASTHAVELISTOFTASKIDRANGES  _("ERROR! -t option must have a list of task id ranges\n")
#define MSG_HISTORY_INVALIDLISTOFTASKIDRANGES_S       _("ERROR! invalid list of task id ranges: %s\n")
#define MSG_HISTORY_QMASTERISNOTALIVE                 _("qmaster is not alive")
#define MSG_HISTORY_QACCTERRORXREADHISTORYSUBDIRFORCOMPLEXES_I   _("qacct: error %d reading history subdirectories for complexes\n")
#define MSG_HISTORY_QACCTERRORXREADHISTORYSUBDIRFORQUEUES_I      _("qacct: error %d reading history subdirectories for queues\n")
#define MSG_HISTORY_QACCTERRORXREADHISTORYSUBDIRFOREXECHOSTS_I   _("qacct: error %d reading history subdirectories for exechosts\n")
#define MSG_HISTORY_ERRORUNABLETOOPENX_S     _("error unable to open %s\n")
#define MSG_HISTORY_NOTENOUGTHMEMORYTOCREATELIST   _("not enough memory to create list\n")
#define MSG_HISTORY_IGNORINGINVALIDENTRYINLINEX_U  _("ignoring invalid entry in line " U32CFormat "\n")
#define MSG_HISTORY_COMPLEXSTRUCTUREREFTOXCANTBEASSEMBLEDY_SI  _("complex structure referring to %s can't be assembled(%d)\n")
#define MSG_HISTORY_USENOHISTTOREFTOCURCOMPLEXCONFIG           _("Use -nohist to refer to current complex configuration.\n")
#define MSG_HISTORY_QUEUECONFIGFORXREFTOYCANTBEASSEMBLEDY_SSI  _("queue configuration for queue %s referring to %s can't be assembled(%d)\n")
#define MSG_HISTORY_HOSTCONFIGFORHOSTXREFTOYCANTBEASSEMBLEDYFORJOBZ_SSIU   _("host configuration for host %s referring to %s can't be assembled(%d) for job " U32CFormat"\n")
#define MSG_HISTORY_GLOBALHOSTCONFIGFORGLOBALHOSTXREFTOYCANTBEASSEMBLEDYFORJOBZ_SSID   _("global_host configuration for global_host %s referring to %s can't be assembled(%d) for job " U32CFormat"\n")
#define MSG_HISTORY_IGNORINGJOBXFORACCOUNTINGMASTERQUEUEYNOTEXISTS_IS   _("ignoring job %d for accounting: jobs master queue %s does not longer exist\n")
#define MSG_HISTORY_COMPLEXTEMPLATESCANTBEFILLEDCORRECTLYFORJOBX_D   _("complex templates can't be filled correctly for job " U32CFormat"\n")
#define MSG_HISTORY_JOBARRAYTASKSWXYZNOTFOUND_DDDD   _("Job-array tasks "U32CFormat"."U32CFormat"-"U32CFormat":"U32CFormat" not found\n" )
#define MSG_HISTORY_JOBIDXNOTFOUND_D          _("job id " U32CFormat  " not found\n"  )
#define MSG_HISTORY_JOBARRAYTASKSWXYZNOTFOUND_SDDD    _("Job-array tasks \"%s\"."U32CFormat"-"U32CFormat":"U32CFormat" not found\n")
#define MSG_HISTORY_JOBNAMEXNOTFOUND_S       _("job name %s not found\n")
#define MSG_HISTORY_TOPTIONREQUIRESJOPTION   _("-t option requires -j option\n")
#define MSG_HISTORY_HOST            "HOST"
#define MSG_HISTORY_QUEUE           "QUEUE"
#define MSG_HISTORY_GROUP           "GROUP"
#define MSG_HISTORY_OWNER           "OWNER"
#define MSG_HISTORY_PROJECT         "PROJECT"
#define MSG_HISTORY_DEPARTMENT      "DEPARTMENT"
#define MSG_HISTORY_PE              "PE"
#define MSG_HISTORY_SLOTS           "SLOTS"
#define MSG_HISTORY_TOTSYSTEMUSAGE  _("Total System Usage\n")
#define MSG_HISTORY_REALOWNERSYSTEMCPUMEMORYIO  _("         REAL         OWNER        SYSTEM           CPU             MEMORY                 IO")
#define MSG_HISTORY_USAGE           _("usage:"    )
#define MSG_HISTORY_A_OPT_USAGE     _("jobs accounted to the given account\n"      )
#define MSG_HISTORY_help_OPT_USAGE  _("display this message\n"  )
#define MSG_HISTORY_h_OPT_USAGE     _("list [matching] host\n")
#define MSG_HISTORY_q_OPT_USAGE     _("list [matching] queue\n")
#define MSG_HISTORY_g_OPT_USAGE     _("list [matching] group\n")
#define MSG_HISTORY_o_OPT_USAGE     _("list [matching] owner\n")
#define MSG_HISTORY_P_OPT_USAGE     _("list [matching] project\n")
#define MSG_HISTORY_D_OPT_USAGE     _("list [matching] department\n")
#define MSG_HISTORY_pe_OPT_USAGE    _("list [matching] parallel environment\n")
#define MSG_HISTORY_slots_OPT_USAGE _("list [matching] job slots\n"   )
#define MSG_HISTORY_l_OPT_USAGE     _("request given complex attributes\n" )
#define MSG_HISTORY_b_OPT_USAGE     _("jobs started after\n")
#define MSG_HISTORY_e_OPT_USAGE     _("jobs started before\n")
#define MSG_HISTORY_d_OPT_USAGE     _("jobs started during the last d days\n")
#define MSG_HISTORY_j_OPT_USAGE     _("list all [matching] jobs\n")
#define MSG_HISTORY_t_OPT_USAGE     _("list all [matching] tasks (requires -j option)\n")
#define MSG_HISTORY_history_OPT_USAGE    _("read history for complex attributes from given directory\n")
#define MSG_HISTORY_nohist_OPT_USAGE      _("do not use historical data for complex attribute matching\n")
#define MSG_HISTORY_f_OPT_USAGE           _("use alternate accounting file\n")
#define MSG_HISTORY_beginend_OPT_USAGE    _("[[CC]YYMMDDhhmm[.SS]\n")
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
#define MSG_HISTORY_GETALLLISTSGETCOMPLEXLISTFAILED      _("get_all_lists: failed to get complex list\n")
#define MSG_HISTORY_GETALLLISTSGETEXECHOSTLISTFAILED     _("get_all_lists: failed to get exechost list\n")
#define MSG_HISTORY_GETALLLISTSGETQUEUELISTFAILED        _("get_all_lists: failed to get queue list\n")




#endif /* __MSG_HISTORY_H   */
