#ifndef __MSG_CLIENTS_COMMON_H
#define __MSG_CLIENTS_COMMON_H
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

#define MSG_OPTION_OPTUANDOPTUALLARENOTALLOWDTOGETHER   _MESSAGE(1000, _("-u and -uall are not allowed together\n"))
#define MSG_JOB_UNASSIGNED       _MESSAGE(1001, _("unassigned"))
#define MSG_FILE_CANTREADCURRENTWORKINGDIR    _MESSAGE(1002, _("cannot read current working directory\n"))
#define MSG_SRC_USAGE                                    _MESSAGE(1003, _("usage:"))
#define MSG_FORCED                                      _MESSAGE(1004, _("forced "))
#define MSG_QDEL_not_available_OPT_USAGE_S _MESSAGE(1005, _("no usage for "SFQ" available\n"))
#define MSG_WARNING                                     _MESSAGE(1006, _("warning: "))
#define MSG_SEC_SETJOBCRED    _MESSAGE(1007, _("\nCannot set job credentials.\n"))
#define MSG_GDI_QUEUESGEGDIFAILED              _MESSAGE(1008, _("queue: sge_gdi failed\n"))
#define MSG_GDI_JOBSGEGDIFAILED              _MESSAGE(1009, _("job: sge_gdi failed\n"))
#define MSG_GDI_EXECHOSTSGEGDIFAILED              _MESSAGE(1010, _("exec host: sge_gdi failed\n"))
#define MSG_GDI_COMPLEXSGEGDIFAILED              _MESSAGE(1011, _("complex: sge_gdi failed\n"))
#define MSG_GDI_SCHEDDCONFIGSGEGDIFAILED    _MESSAGE(1012, _("scheduler configuration: sge_gdi failed\n"))

/*
 * qstat_printing.c
 */
#define MSG_QSTAT_PRT_QUEUENAME    "queuename"
#define MSG_QSTAT_PRT_QTYPE        "qtype"
#define MSG_QSTAT_PRT_USEDTOT      "used/tot."
#define MSG_QSTAT_PRT_STATES       "states"
#define MSG_QSTAT_PRT_PEDINGJOBS    _MESSAGE(1014, _(" - PENDING JOBS - PENDING JOBS - PENDING JOBS - PENDING JOBS - PENDING JOBS\n"))
#define MSG_QSTAT_PRT_JOBSWAITINGFORACCOUNTING    _MESSAGE(1015, _(" -----   JOBS WAITING FOR ACCOUNTING  -  JOBS WAITING FOR ACCOUNTING   ----- \n"))
#define MSG_QSTAT_PRT_ERRORJOBS    _MESSAGE(1016, _("  -  ERROR JOBS  -  ERROR JOBS  -  ERROR JOBS  -  ERROR JOBS  -  ERROR JOBS  -\n"))
#define MSG_QSTAT_PRT_FINISHEDJOBS    _MESSAGE(1017, _(" --  FINISHED JOBS  -  FINISHED JOBS  -  FINISHED JOBS  -  FINISHED JOBS  --  \n"))

#define MSG_CENTRY_NOTCHANGED         _MESSAGE(1018, _("Complex attribute configuartion has not been changed\n"))
#define MSG_CENTRY_DOESNOTEXIST_S     _MESSAGE(1019, _("Complex attribute "SFQ" does not exist\n"))
#define MSG_CENTRY_FILENOTCORRECT_S   _MESSAGE(1020, _("Complex attribute file "SFQ" is not correct\n"))

#endif /* __MSG_CLIENTS_COMMON_H */

