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
#define MSG_CONFIG_READINGINEXECUTIONHOSTS      _MESSAGE(61000, _("Reading in execution hosts.\n"))
#define MSG_CONFIG_CANTWRITEHISTORYFORHOSTX_S   _MESSAGE(61001, _("cannot write history for host "SFQ"\n"))
#define MSG_CONFIG_READINGINADMINHOSTS          _MESSAGE(61002, _("Reading in administrative hosts.\n"))
#define MSG_CONFIG_READINGINSUBMITHOSTS         _MESSAGE(61003, _("Reading in submit hosts.\n"))
#define MSG_CONFIG_READINGINGPARALLELENV        _MESSAGE(61004, _("Reading in parallel environments:\n"))
#define MSG_SETUP_PE_S                          _MESSAGE(61005, _("\tPE "SFQ".\n"))
#define MSG_CONFIG_READINGINCALENDARS           _MESSAGE(61006, _("Reading in calendars:\n"))
#define MSG_SETUP_CALENDAR_S                    _MESSAGE(61007, _("\tCalendar "SFQ".\n"))
#define MSG_CONFIG_FAILEDPARSINGYEARENTRYINCALENDAR_SS _MESSAGE(61008, _("failed parsing year entry in calendar "SFQ": "SFN"\n"))
#define MSG_CONFIG_READINGINCKPTINTERFACEDEFINITIONS   _MESSAGE(61009, _("Reading in ckpt interface definitions:\n"))
#define MSG_SETUP_CKPT_S                        _MESSAGE(61010, _("\tCKPT "SFQ".\n"))
#define MSG_CONFIG_READINGINQUEUES              _MESSAGE(61011, _("Reading in queues:\n"))
#define MSG_SETUP_QUEUE_S                       _MESSAGE(61012, _("\tQueue "SFQ".\n"))
#define MSG_CONFIG_READINGFILE_SS               _MESSAGE(61013, _("reading file "SFN"/"SFN"\n"))
#define MSG_CONFIG_QUEUEXUPDATED_S              _MESSAGE(61014, _("Queue "SFN" updated\n"))
#define MSG_CONFIG_OBSOLETEQUEUETEMPLATEFILEDELETED _MESSAGE(61015, _("obsolete queue template file deleted\n"))
#define MSG_CONFIG_FOUNDQUEUETEMPLATEBUTNOTINFILETEMPLATEIGNORINGIT    _MESSAGE(61016, _("found queue 'template', but not in file 'template'; ignoring it!\n"))
#define MSG_CONFIG_CANTRECREATEQEUEUEXFROMDISKBECAUSEOFUNKNOWNHOSTY_SS _MESSAGE(61017, _("cannot recreate queue "SFN" from disk because of unknown host "SFN"\n"))
#define MSG_CONFIG_CANTWRITEHISTORYFORQUEUEX_S  _MESSAGE(61018, _("can't write history for queue "SFQ"\n"))
#define MSG_CONFIG_READINGINPROJECTS            _MESSAGE(61019, _("Reading in projects:\n"))
#define MSG_SETUP_PROJECT_S                     _MESSAGE(61020, _("\tProject "SFQ".\n"))
#define MSG_CONFIG_READINGINUSERSETS            _MESSAGE(61021, _("Reading in usersets:\n"))
#define MSG_CONFIG_READINGINUSERS               _MESSAGE(61022, _("Reading in users:\n"))
#define MSG_SETUP_USER_S                        _MESSAGE(61023, _("\tUser "SFQ".\n"))
#define MSG_SETUP_USERSET_S                     _MESSAGE(61024, _("\tUserset "SFQ".\n"))
#define MSG_CONFIG_CANTRESOLVEHOSTNAMEX_SSS     _MESSAGE(61025, _("cannot resolve "SFN" name "SFQ": "SFN))
#define MSG_CONFIG_CANTRESOLVEHOSTNAMEX_SS      _MESSAGE(61026, _("cannot resolve "SFN" name "SFQ))
#define MSG_SGETEXT_CANTSPOOL_SS                _MESSAGE(61027, _("qmaster is unable to spool "SFN" "SFQ"\n"))
#define MSG_FILE_NOOPENDIR_S                    _MESSAGE(61028, _("can't open directory "SFQ))
#define MSG_CONFIG_READINGINCOMPLEXES           _MESSAGE(61029, _("Reading in complexes:\n"))
#define MSG_SETUP_COMPLEX_S                     _MESSAGE(61030, _("\tComplex "SFQ".\n"))
#define MSG_FILE_NOWRITEHIST_S                  _MESSAGE(61031, _("can't write history for complex "SFQ))

/*
 * libs/spool/read_write_manop.c
 */
#define MSG_FILE_ERROROPENINGX_S    _MESSAGE(61032, _("error opening "SFN"\n"))

/*
 * libs/spool/read_write_job.c
 */
#define MSG_CONFIG_FAILEDREMOVINGSCRIPT_SS            _MESSAGE(61033, _("failed removing script of bad jobfile (reason: "SFN"): please delete "SFQ" manually\n"))
#define MSG_CONFIG_REMOVEDSCRIPTOFBADJOBFILEX_S       _MESSAGE(61034, _("removed script of bad jobfile "SFQ"\n"))
#define MSG_CONFIG_READINGINX_S                       _MESSAGE(61035, _("Reading in "SFN".\n"))
#define MSG_CONFIG_NODIRECTORY_S                      _MESSAGE(61036, _(SFQ" is no directory - skipping the entry\n"))
#define MSG_CONFIG_CANTFINDSCRIPTFILE_U               _MESSAGE(61037, _("can't find script file for job " U32CFormat " - deleting\n"))
#define MSG_CONFIG_JOBFILEXHASWRONGFILENAMEDELETING_U _MESSAGE(61038, _("job file \""U32CFormat"\" has wrong file name - deleting\n"))

/*
 * libs/spool/sge_spooling_classic.c
 */ 

#define MSG_SPOOL_STARTEDINWRONGDIRECTORY_SS _MESSAGE(61039, _("process should start in the spool directory "SFQ", but we are in "SFQ"; changing to spool directory\n")) 
#define MSG_SPOOL_INCORRECTPATHSFORCOMMONANDSPOOLDIR _MESSAGE(61040, _("incorrect paths given for common and/or spool directory\n"))
#define MSG_SPOOL_COMMONDIRDOESNOTEXIST_S  _MESSAGE(61041, _("common directory "SFQ" does not exist\n"))
#define MSG_SPOOL_NOTSUPPORTEDREADINGJOB _MESSAGE(61042, _("reading a single job from disk not supported in classic spooling context\n"))
#define MSG_SPOOL_NOTSUPPORTEDREADINGMANAGER _MESSAGE(61043, _("reading a single manager from disk not supported in classic spooling context\n"))
#define MSG_SPOOL_NOTSUPPORTEDREADINGOPERATOR _MESSAGE(61044, _("reading a single operator from disk not supported in classic spooling context\n"))
#define MSG_SPOOL_GLOBALCONFIGNOTDELETED _MESSAGE(61045, _("the global configuration must not be deleted\n"))
#define MSG_SPOOL_SCHEDDCONFIGNOTDELETED _MESSAGE(61045, _("the scheduler configuration must not be deleted\n"))
#endif /* __MSG_SPOOLLIB_CLASSIC_H */
