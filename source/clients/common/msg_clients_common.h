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

#define MSG_OPTION_OPTUANDOPTUALLARENOTALLOWDTOGETHER   _("-u and -uall are not allowed together\n")
#define MSG_JOB_UNASSIGNED       _("unassigned")
#define MSG_FILE_CANTREADCURRENTWORKINGDIR    _("cannot read current working directory\n")
#define MSG_SRC_USAGE                                    _("usage:")
#define MSG_FORCED                                      _("forced ")
#define MSG_QDEL_not_available_OPT_USAGE_S _("no usage for \"%s\" available\n")
#define MSG_WARNING_S                                   _("warning: %s")
#define MSG_SEC_SETJOBCRED    _("\nCannot set job credentials.\n")
#define MSG_GDI_QUEUESGEGDIFAILED              _("queue: sge_gdi failed\n")
#define MSG_GDI_JOBSGEGDIFAILED              _("job: sge_gdi failed\n")
#define MSG_GDI_EXECHOSTSGEGDIFAILED              _("exec host: sge_gdi failed\n")
#define MSG_GDI_COMPLEXSGEGDIFAILED              _("complex: sge_gdi failed\n")
#define MSG_GDI_SCHEDDCONFIGSGEGDIFAILED    _("scheduler configuration: sge_gdi failed\n")
#define MSG_GDI_SGE_SETUP_FAILED_S    _("sge_gdi_setup failed: %s\n")




/*
** qstat_printing.c
*/
#define MSG_QSTAT_PRT_QUEUENAME    "queuename"
#define MSG_QSTAT_PRT_QTYPE        "qtype"
#define MSG_QSTAT_PRT_USEDTOT      "used/tot."
#define MSG_QSTAT_PRT_STATES       "states"
#define MSG_QSTAT_PRT_PEDINGJOBS    _(" - PENDING JOBS - PENDING JOBS - PENDING JOBS - PENDING JOBS - PENDING JOBS\n")
#define MSG_QSTAT_PRT_JOBSWAITINGFORACCOUNTING    _(" -----   JOBS WAITING FOR ACCOUNTING  -  JOBS WAITING FOR ACCOUNTING   ----- \n")
#define MSG_QSTAT_PRT_ERRORJOBS    _("  -  ERROR JOBS  -  ERROR JOBS  -  ERROR JOBS  -  ERROR JOBS  -  ERROR JOBS  -\n")
#define MSG_QSTAT_PRT_FINISHEDJOBS    _(" --  FINISHED JOBS  -  FINISHED JOBS  -  FINISHED JOBS  -  FINISHED JOBS  --  \n")



#endif /* __MSG_CLIENTS_COMMON_H */

