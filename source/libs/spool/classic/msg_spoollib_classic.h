#ifndef __MSG_SPOOLLIB_CLASSIC_H
#define __MSG_SPOOLLIB_CLASSIC_H
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
 * libs/spool/read_list.c
 */
#define MSG_CONFIG_READINGINEXECUTIONHOSTS      _MESSAGE(61000, _("Reading in execution hosts."))
#define MSG_CONFIG_READINGINADMINHOSTS          _MESSAGE(61002, _("Reading in administrative hosts."))
#define MSG_CONFIG_READINGINSUBMITHOSTS         _MESSAGE(61003, _("Reading in submit hosts."))
#define MSG_CONFIG_READINGINGPARALLELENV        _MESSAGE(61004, _("Reading in parallel environments:"))
#define MSG_SETUP_PE_S                          _MESSAGE(61005, _("\tPE "SFQ"."))
#define MSG_CONFIG_READINGINCALENDARS           _MESSAGE(61006, _("Reading in calendars:"))
#define MSG_SETUP_CALENDAR_S                    _MESSAGE(61007, _("\tCalendar "SFQ"."))
#define MSG_CONFIG_FAILEDPARSINGYEARENTRYINCALENDAR_SS _MESSAGE(61008, _("failed parsing year entry in calendar "SFQ": "SFN))
#define MSG_CONFIG_READINGINCKPTINTERFACEDEFINITIONS   _MESSAGE(61009, _("Reading in ckpt interface definitions:"))
#define MSG_SETUP_CKPT_S                        _MESSAGE(61010, _("\tCKPT "SFQ"."))
#define MSG_CONFIG_READINGINQUEUES              _MESSAGE(61011, _("Reading in queues:"))
#define MSG_SETUP_QUEUE_S                       _MESSAGE(61012, _("\tQueue "SFQ"."))
#define MSG_CONFIG_READINGFILE_SS               _MESSAGE(61013, _("reading file "SFN"/"SFN))
#define MSG_CONFIG_OBSOLETEQUEUETEMPLATEFILEDELETED _MESSAGE(61015, _("obsolete queue template file deleted"))
#define MSG_CONFIG_FOUNDQUEUETEMPLATEBUTNOTINFILETEMPLATEIGNORINGIT    _MESSAGE(61016, _("found queue 'template', but not in file 'template'; ignoring it!"))
#define MSG_CONFIG_CANTRECREATEQEUEUE_SS        _MESSAGE(61017, _("cannot recreate queue "SFN" from disk because of unknown host "SFN))
#define MSG_CONFIG_READINGINPROJECTS            _MESSAGE(61019, _("Reading in projects:"))
#define MSG_SETUP_PROJECT_S                     _MESSAGE(61020, _("\tProject "SFQ"."))
#define MSG_CONFIG_READINGINUSERSETS            _MESSAGE(61021, _("Reading in usersets:"))
#define MSG_CONFIG_READINGINUSERS               _MESSAGE(61022, _("Reading in users:"))
#define MSG_SETUP_USER_S                        _MESSAGE(61023, _("\tUser "SFQ"."))
#define MSG_SETUP_USERSET_S                     _MESSAGE(61024, _("\tUserset "SFQ"."))
#define MSG_CONFIG_CANTRESOLVEHOSTNAMEX_SSS     _MESSAGE(61025, _("cannot resolve "SFN" name "SFQ": "SFN))
#define MSG_CONFIG_CANTRESOLVEHOSTNAMEX_SS      _MESSAGE(61026, _("cannot resolve "SFN" name "SFQ))
#define MSG_SGETEXT_CANTSPOOL_SS                _MESSAGE(61027, _("qmaster is unable to spool "SFN" "SFQ))
#define MSG_FILE_NOOPENDIR_S                    _MESSAGE(61028, _("can't open directory "SFQ))

/*
 * libs/spool/read_write_manop.c
 */
#define MSG_FILE_ERROROPENINGX_S    _MESSAGE(61032, _("error opening "SFN))

/*
 * libs/spool/read_write_job.c
 */
#define MSG_CONFIG_FAILEDREMOVINGSCRIPT_SS            _MESSAGE(61033, _("failed removing script of bad jobfile (reason: "SFN"): please delete "SFQ" manually"))
#define MSG_CONFIG_REMOVEDSCRIPTOFBADJOBFILEX_S       _MESSAGE(61034, _("removed script of bad jobfile "SFQ))
#define MSG_CONFIG_READINGINX_S                       _MESSAGE(61035, _("Reading in "SFN"."))
#define MSG_CONFIG_NODIRECTORY_S                      _MESSAGE(61036, _(SFQ" is no directory - skipping the entry"))
#define MSG_CONFIG_CANTFINDSCRIPTFILE_U               _MESSAGE(61037, _("can't find script file for job " sge_U32CFormat " - deleting"))
#define MSG_CONFIG_JOBFILEXHASWRONGFILENAMEDELETING_U _MESSAGE(61038, _("job file \""sge_U32CFormat"\" has wrong file name - deleting"))
#define MSG_CONFIG_JOBSPOOLINGLONGDELAY_UUI           _MESSAGE(61135, _("spooling job "sge_U32CFormat"."sge_U32CFormat" took %d seconds")) 

/*
 * libs/spool/sge_spooling_classic.c
 * libs/spool/sge_spooling_flatfile.c
 */ 

#define MSG_SPOOL_CHANGINGTOSPOOLDIRECTORY_S _MESSAGE(61039, _("changing to spool directory "SFN)) 
#define MSG_SPOOL_INCORRECTPATHSFORCOMMONANDSPOOLDIR _MESSAGE(61040, _("incorrect paths given for common and/or spool directory"))
#define MSG_SPOOL_COMMONDIRDOESNOTEXIST_S  _MESSAGE(61047, _("common directory "SFQ" does not exist"))
#define MSG_SPOOL_SPOOLDIRDOESNOTEXIST_S  _MESSAGE(61041, _("spool directory "SFQ" does not exist"))
#define MSG_SPOOL_NOTSUPPORTEDREADINGJOB _MESSAGE(61042, _("reading a single job from disk not supported in classic spooling context"))
#define MSG_SPOOL_NOTSUPPORTEDREADINGMANAGER _MESSAGE(61043, _("reading a single manager from disk not supported in classic spooling context"))
#define MSG_SPOOL_NOTSUPPORTEDREADINGOPERATOR _MESSAGE(61044, _("reading a single operator from disk not supported in classic spooling context"))
#define MSG_SPOOL_GLOBALCONFIGNOTDELETED _MESSAGE(61045, _("the global configuration must not be deleted"))
#define MSG_SPOOL_SCHEDDCONFIGNOTDELETED _MESSAGE(61046, _("the scheduler configuration must not be deleted"))

/* 
 * libs/spool/read_list.c continued
 */
#define MSG_QMASTER_PRJINCORRECT_S       _MESSAGE(61048, _("Spoolfile for project "SFQ" containes invalid name"))
#define MSG_SETUP_COMPLEX_ATTR_S         _MESSAGE(61049, _("\tComplex attribute "SFQ"."))
#define MSG_CONFIG_READINGINCOMPLEXATTRS _MESSAGE(61050, _("Reading in complex attributes."))
#define MSG_HGROUP_INCFILE_S             _MESSAGE(61051, _("Incorrect spoolfile for hostgroup "SFQ))
#define MSG_CONFIG_READINGHOSTGROUPENTRYS  _MESSAGE(61052, _("Reading in host group entries:"))
#define MSG_SETUP_HOSTGROUPENTRIES_S               _MESSAGE(61053, _("\tHost group entries for group "SFQ"."))
#define MSG_CONFIG_READINGUSERMAPPINGENTRY _MESSAGE(61054, _("Reading in user mapping entries:"))
#define MSG_SETUP_MAPPINGETRIES_S                  _MESSAGE(61055, _("\tMapping entries for "SFQ"."))

/*
 * read_object.c
 */
#define MSG_MEMORY_CANTMALLOCBUFFERFORXOFFILEY_SS    _MESSAGE(61100, _("can't malloc buffer for "SFN" of file "SFQ))
#define MSG_FILE_CANTDETERMINESIZEFORXOFFILEY_SS    _MESSAGE(61101, _("can't determine size for "SFN" of file "SFQ))
#define MSG_SGETEXT_UNKNOWN_CONFIG_VALUE_SSS          _MESSAGE(61102, _("unknown attribute "SFQ" in "SFN" configuration in file "SFN) ) 

/*
 * read_write_host.c
 */
#define MSG_FILE_ERRORWRITINGHOSTNAME    _MESSAGE(61110, _("error writing hostname"))

/*
 * read_write_queue.c
 */
#define MSG_ERROR_CLASSIC_SPOOLING           _MESSAGE(61120, _("error: error message not spooled! (no classic spooling support)"))
#define MSG_ANSWER_GETUNIQUEHNFAILEDRESX_SS  _MESSAGE(61121, _("getuniquehostname() failed resolving "SFN": "SFN))

/*
 * read_write_userprj.c
 */
#define MSG_RWUSERPRJ_EPISNULLNOUSERORPROJECTELEMENT    _MESSAGE(61130, _("ep == NULL - no user or project element"))
#define MSG_PROJECT_FOUNDPROJECTXTWICE_S    _MESSAGE(61131, _("found project "SFN" twice"))
#define MSG_PROJECT_INVALIDPROJECTX_S    _MESSAGE(61132, _("invalid project "SFN))
#define MSG_JOB_FOUNDJOBWITHWRONGKEY_S    _MESSAGE(61133, _("found job with wrong key "SFQ""))
#define MSG_JOB_FOUNDJOBXTWICE_U    _MESSAGE(61134, _("found job "sge_U32CFormat" twice"))

/*
 * read_write_userset.c
 */
#define MSG_USERSET_NOUSERETELEMENT    _MESSAGE(61140, _("no userset element"))














/*
 * all classic spooling code
 */
#define MSG_TMPNAM_GENERATINGTMPNAM    _MESSAGE(61900, _("generating tmpnam()"))

#endif /* __MSG_SPOOLLIB_CLASSIC_H */
