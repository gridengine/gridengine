#ifndef _SGE_REPORTING_QMASTER_H_
#define _SGE_REPORTING_QMASTER_H_
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

#include <time.h>

#include "cull.h"

#include "sge_dstring.h"

#include "sge_object.h"

bool
sge_initialize_reporting(lList **answer_list);

bool
sge_shutdown_reporting(lList **answer_list);

void
deliver_reporting_trigger(u_long32 type, u_long32 when, 
                          u_long32 uval0, u_long32 uval1, const char *key);

bool
sge_create_acct_record(lList **answer_list, 
                       lListElem *job_report, 
                       lListElem *job, lListElem *ja_task);

bool 
sge_create_reporting_record(lList **answer_list, 
                            sge_object_type object_type,
                            const char *data);

bool 
sge_flush_accounting_data(lList **answer_list);

bool 
sge_flush_reporting_data(lList **answer_list);

bool 
sge_flush_report_file(lList **answer_list, dstring *contents, 
                      const char *filename);

bool 
sge_flush_reporting(lList **answer_list, time_t flush, time_t *next_flush);


#endif /* _SGE_REPORTING_QMASTER_H_ */

