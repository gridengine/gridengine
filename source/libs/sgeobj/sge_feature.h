#ifndef __SGE_FEATURE_H
#define __SGE_FEATURE_H
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

#include "sge_featureL.h"

#ifdef  __cplusplus
extern "C" {
#endif  

typedef enum {
   FS_SHORT,
   FS_LONG,
   FS_VERSION,
   FS_SHORT_VERSION,
   FS_LONG_VERSION
} featureset_product_name_id_t;

typedef enum {
   FEATURESET_UNINITIALIZED,
   FEATURESET_SGE,
   FEATURESET_SGEEE,
   FEATURESET_SGE_AFS,
   FEATURESET_SGEEE_AFS,
   FEATURESET_SGE_DCE,
   FEATURESET_SGEEE_DCE,
   FEATURESET_SGE_KERBEROS,
   FEATURESET_SGEEE_KERBEROS,
   FEATURESET_SGE_RESERVED_PORT,
   FEATURESET_SGEEE_RESERVED_PORT,
   FEATURESET_SGE_CSP,
   FEATURESET_SGEEE_CSP,
 
   /* DON'T CHANGE THE ORDER OF THE ENTRIES ABOVE */
 
   FEATURESET_LAST_ENTRY
} featureset_id_t;
 

typedef enum {
   FEATURE_UNINITIALIZED,
   FEATURE_REPRIORITIZATION,        /* PTFs auto reprioritization */
   FEATURE_REPORT_USAGE,            /* PDC data collection and
                                       usage reports from execd to master */
   FEATURE_SPOOL_ADD_ATTR,          /* spool additional attributes for sge 
                                       objects in sg3e-mode */
   FEATURE_SGEEE,                   /* activate the remaining code needed in 
                                       sg3e-mode */
   FEATURE_NO_SECURITY,             /* No security mode active */
   FEATURE_AFS_SECURITY,            /* AFS security */
   FEATURE_DCE_SECURITY,            /* DCE security */
   FEATURE_KERBEROS_SECURITY,       /* KERBEROS security */
   FEATURE_RESERVED_PORT_SECURITY,  /* Use reserved ports for communication */
   FEATURE_CSP_SECURITY,            /* CSP security */
 
   /* DON'T CHANGE THE ORDER OF THE ENTRIES ABOVE */
 
   FEATURE_LAST_ENTRY
} feature_id_t;
 
typedef struct {
   feature_id_t id;
   const char *name;
} feature_names_t;            

typedef struct {
   featureset_id_t id;
   char *name;
} featureset_names_t;            

#if defined(SGE_MT)
void feature_init_mt(void);
#endif

void feature_initialize(void);
 
int feature_initialize_from_file(const char *filename, lList **alpp);
 
int feature_initialize_from_string(const char *mode);
 
void feature_activate(featureset_id_t id);

bool feature_is_active(featureset_id_t id);
 
const char *feature_get_featureset_name(featureset_id_t id);
 
featureset_id_t feature_get_featureset_id(const char *name);
 
featureset_id_t feature_get_active_featureset_id(void);
 
bool feature_is_enabled(feature_id_t id);
 
const char *feature_get_name(feature_id_t id);
 
feature_id_t feature_get_id(const char *name);

const char *feature_get_product_name(featureset_product_name_id_t style, dstring *buffer);
 
#ifdef  __cplusplus
}
#endif                   

#endif
