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


enum featureset_product_name_id_t {
   FS_SHORT,
   FS_LONG,
   FS_VERSION,
   FS_SHORT_VERSION,
   FS_LONG_VERSION
};

enum featureset_id_t {
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
 
   /* DON'T CHANGE THE ORDER OF THE ENTRIES ABOVE */
 
   FEATURESET_LAST_ENTRY
};
 
 
enum feature_id_t {
   FEATURE_UNINITIALIZED,
   FEATURE_USE_OSJOB_ID,            /* jobs will get a unique job id during startup */
   FEATURE_REPRIORISATION,          /* PTFs auto repriorisation */
   FEATURE_REPORT_USAGE,            /* PDC data collection and
                                       usage reports from execd to master */
   FEATURE_SPOOL_ADD_ATTR,          /* spool additional attributes for sge objects in sg3e-mode */
   FEATURE_SGEEE,                   /* activate the remaining code needed in sg3e-mode */
 
   FEATURE_NO_SECURITY,             /* No security mode active */
   FEATURE_AFS_SECURITY,            /* AFS security */
   FEATURE_DCE_SECURITY,            /* DCE security */
   FEATURE_KERBEROS_SECURITY,       /* KERBEROS security */
   FEATURE_RESERVED_PORT_SECURITY,  /* Use reserved ports for communication */
 
   /* DON'T CHANGE THE ORDER OF THE ENTRIES ABOVE */
 
   FEATURE_LAST_ENTRY
};
 
struct feature_names_t {
   int id;
   char *name;
};            

extern lList *Master_FeatureSet_List;
 
int feature_initialize_from_file(char *filename);
 
int feature_initialize_from_string(char *mode);
 
void feature_activate(enum featureset_id_t id);
 
int feature_is_active(enum featureset_id_t id);
 
char *feature_get_featureset_name(enum featureset_id_t id);
 
enum featureset_id_t feature_get_featureset_id(char *name);
 
char *feature_get_product_name(enum featureset_product_name_id_t style);
 
enum featureset_id_t feature_get_active_featureset_id(void);
 
 
int feature_is_enabled(enum feature_id_t id);
 
char *feature_get_name(enum feature_id_t id);
 
enum feature_id_t feature_get_id(char *name);
 
char *feature_get_last_error_msg(void);

#ifdef  __cplusplus
}
#endif                   

#endif
