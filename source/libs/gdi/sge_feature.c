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
#include <string.h>

#include "sge.h"
#include "utility.h"
#include "def.h"
#include "sge_answerL.h"
#include "job_log.h"
#include "sge_queue_qmaster.h"
#include "sge_host_qmaster.h"
#include "sge_m_event.h"
#include "config_file.h"
#include "sge_me.h"
#include "sge_prognames.h"
#include "sgermon.h"
#include "sge_log.h"
#include "gdi_utility_qmaster.h"
#include "msg_common.h"
#include "msg_utilib.h"
#include "msg_qmaster.h"
#include "sge_feature.h"         
#include "msg_gdilib.h"
#include "version.h"

#define FEATURESET_DEFAULT FEATURESET_SGE

static char product_name_and_version[256] = "";

/* *INDENT-OFF* */

static int enabled_features_mask[FEATURESET_LAST_ENTRY][FEATURE_LAST_ENTRY] = { 
/*  FEATURE_UNINITIALIZED                                          */
/*  |  FEATURE_REPRIORISATION                                   */
/*  |  |  FEATURE_REPORT_USAGE                                  */
/*  |  |  |  FEATURE_SPOOL_ADD_ATTR                             */
/*  |  |  |  |  FEATURE_SGEEE                                   */
/*  |  |  |  |  |                                               */
/*  |  |  |  |  |    FEATURE_NO_SECURITY                        */
/*  |  |  |  |  |    |  FEATURE_AFS_SECUIRITY                   */
/*  |  |  |  |  |    |  |  FEATURE_DCE_SECURITY                 */
/*  |  |  |  |  |    |  |  |  FEATURE_KERBEROS_SECURITY         */   
/*  |  |  |  |  |    |  |  |  |  FEATURE_RESERVED_PORT_SECURITY */ 
/*  |  |  |  |  |    |  |  |  |  |  FEATURE_CSP_SECURITY        */
/*  v  v  v  v  v    v  v  v  v  v  v                           */
                                       
   {0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0},   /* FEATURESET_UNINITIALIZED       */
   {0, 0, 1, 0, 0,   1, 0, 0, 0, 0, 0},   /* FEATURESET_SGE                 */
   {0, 1, 1, 1, 1,   1, 0, 0, 0, 0, 0},   /* FEATURESET_SGEEE               */
   {0, 0, 1, 0, 0,   0, 1, 0, 0, 0, 0},   /* FEATURESET_SGE_AFS             */
   {0, 1, 1, 1, 1,   0, 1, 0, 0, 0, 0},   /* FEATURESET_SGEEE_AFS           */
   {0, 0, 1, 0, 0,   0, 0, 1, 0, 0, 0},   /* FEATURESET_SGE_DCE             */
   {0, 1, 1, 1, 1,   0, 0, 1, 0, 0, 0},   /* FEATURESET_SGEEE_DCE           */
   {0, 0, 1, 0, 0,   0, 0, 0, 1, 0, 0},   /* FEATURESET_SGE_KERBEROS        */
   {0, 1, 1, 1, 1,   0, 0, 0, 1, 0, 0},   /* FEATURESET_SGEEE_KERBEROS      */
   {0, 0, 1, 0, 0,   0, 0, 0, 0, 1, 0},   /* FEATURESET_SGE_RESERVED_PORT   */
   {0, 1, 1, 1, 1,   0, 0, 0, 0, 1, 0},   /* FEATURESET_SGEEE_RESERVED_PORT */
   {0, 0, 1, 0, 0,   0, 0, 0, 0, 0, 1},   /* FEATURESET_SGE_CSP   */
   {0, 1, 1, 1, 1,   0, 0, 0, 0, 0, 1}    /* FEATURESET_SGEEE_CSP */
};

static feature_names_t feature_list[] = {
   {FEATURE_REPRIORISATION,         "repriorisation"},
   {FEATURE_REPORT_USAGE,           "report_usage"},
   {FEATURE_NO_SECURITY,            "no_security"},
   {FEATURE_AFS_SECURITY,           "afs_security"},
   {FEATURE_DCE_SECURITY,           "dce_security"},
   {FEATURE_KERBEROS_SECURITY,      "kerberos_security"},
   {FEATURE_RESERVED_PORT_SECURITY, "reserved_port_security"},
   {FEATURE_CSP_SECURITY,           "csp_security"},
   {0, NULL}
};  

static feature_names_t featureset_list[] = {
   {FEATURESET_SGE,                 "sge"},
   {FEATURESET_SGEEE,               "sgeee"},
   {FEATURESET_SGE_AFS,             "sge-afs"},
   {FEATURESET_SGEEE_AFS,           "sgeee-afs"},
   {FEATURESET_SGE_DCE,             "sge-dce"},
   {FEATURESET_SGEEE_DCE,           "sgeee-dce"},
   {FEATURESET_SGE_KERBEROS,        "sge-kerberos"},
   {FEATURESET_SGEEE_KERBEROS,      "sgeee-kerberos"},
/*
 * if changed, please update setup_commd_path.c 
 * function use_reserved_port() 
 */
   {FEATURESET_SGE_RESERVED_PORT,   "sge-reserved_port"},  
   {FEATURESET_SGEEE_RESERVED_PORT, "sgeee-reserved_port"},
   {FEATURESET_SGE_CSP,             "sge-csp"},
   {FEATURESET_SGEEE_CSP,           "sgeee-csp"},
   {0, NULL}
};

/* *INDENT-ON* */

static int already_read_from_file = 0;
lList *Master_FeatureSet_List = NULL;

static void feature_initialize(void);  

/****** gdi/feature/feature_initialize() **************************************
*  NAME
*     feature_initialize() -- initialize this module 
*
*  SYNOPSIS
*     static void feature_initialize(void) 
*
*  FUNCTION
*     build up the CULL list "Master_FeatureSet_List" (FES_Type) with 
*     information found in the array "enabled_features_mask"
*
*  INPUTS
*     static array enabled_features_mask[][] 
*
*  RESULT
*     initialized Master_FeatureSet_List
******************************************************************************/
static void feature_initialize(void)
{
   if (!Master_FeatureSet_List) {
      lListElem *featureset;
      lListElem *feature;
      int featureset_id;
      int feature_id;
 
      for(featureset_id = 0;
          featureset_id < FEATURESET_LAST_ENTRY;
          featureset_id++) {
         featureset = lAddElemUlong(&Master_FeatureSet_List, FES_id,
                                  featureset_id, FES_Type);
         lSetUlong(featureset, FES_active, 0);
         for(feature_id = 0;
             feature_id < FEATURE_LAST_ENTRY;
             feature_id++) {
            feature = lAddSubUlong(featureset, FE_id,
                                  feature_id, FES_features, FE_Type);
            lSetUlong(feature, FE_enabled,
                            enabled_features_mask[featureset_id][feature_id]);
         }
      }
   }
}                 

/****** gdi/feature/feature_initialize_from_file() ****************************
*  NAME
*     feature_initialize_from_file() -- tag one featureset as active 
*
*  SYNOPSIS
*     int feature_initialize_from_file(char *filename) 
*
*  FUNCTION
*     This function reads the product mode string from file and
*     tags the corresponding featureset enty within the Master_FeatureSet_List
*     as active.
*
*  INPUTS
*     char *filename - product mode filename 
*
*  RESULT
*        0 OK
*       -1 invalid filename
*       -2 file doesn't exist
*       -3 unknown mode-string in file
******************************************************************************/
int feature_initialize_from_file(const char *filename) 
{
   int ret;

   DENTER(TOP_LAYER, "featureset_initialize_from_file");

   if (!already_read_from_file) {
      FILE *fp;

      if (!filename) {
         ret = -1;
      } else {
         fp = fopen(filename, "r");
         if (!fp) {
            ERROR((SGE_EVENT, MSG_GDI_PRODUCTMODENOTSETFORFILE_S, 
               filename));
            ret = -2;
         } else {
            char mode[128];
            char buf[128];
            fgets(buf, 127, fp);
            fclose(fp); 
            sscanf(buf,"%s", mode);
            ret = feature_initialize_from_string(mode);

            if (ret == -3) {
               ERROR((SGE_EVENT, MSG_GDI_CORRUPTPRODMODFILE_S, filename));  
            } else if (ret == 0) {
               already_read_from_file = 1;
            }
         }
      }
   } else{
      ret = 0;
   }
   DEXIT;
   return ret;
}

/****** gdi/feature/feature_initialize_from_string() **************************
*  NAME
*     feature_initialize_from_string() -- tag one featureset as active 
*
*  SYNOPSIS
*     int feature_initialize_from_string(char *mode) 
*
*  FUNCTION
*     This function interprets the mode string and tags the corresponding 
*     featureset enty within the Master_FeatureSet_List as active.  
*
*  INPUTS
*     char *mode - product mode string (valid strings are defined in
*                  the arry featureset_list[])
*
*  RESULT
*     0 OK
*    -3 unknown mode-string
******************************************************************************/
int feature_initialize_from_string(const char *mode) {
   featureset_id_t id;
   int ret;

   DENTER(TOP_LAYER, "featureset_initialize_from_string");
   id = feature_get_featureset_id(mode);
   if (id == FEATURESET_UNINITIALIZED) {
      ERROR((SGE_EVENT, MSG_GDI_INVALIDPRODUCTMODESTRING_S, mode));
      ret = -3;
   } else {
      feature_activate(id);
      ret = 0;
   }
   DEXIT;
   return ret;
}

/****** gdi/feature/feature_activate() ****************************************
*  NAME
*     feature_activate() -- switches the active featureset 
*
*  SYNOPSIS
*     void feature_activate(featureset_ id) 
*
*  FUNCTION
*     Marks the current active featureset within the Master_FeatureSet_List
*     as inactive and flags the featureset given as parameter as active.
*      
*
*  INPUTS
*     id - feature set constant 
*
*  RESULT
*     modifies the Master_FeatureSet_List  
******************************************************************************/
void feature_activate(
featureset_id_t id 
) {
   lListElem *active_set;
   lListElem *inactive_set;

   DENTER(TOP_LAYER, "featureset_activate");  
   if (!Master_FeatureSet_List) {
      feature_initialize();
   }

   inactive_set = lGetElemUlong(Master_FeatureSet_List, FES_id, id);
   active_set = lGetElemUlong(Master_FeatureSet_List, FES_active, 1);
   if (inactive_set && active_set) {
      lSetUlong(active_set, FES_active, 0);
      lSetUlong(inactive_set, FES_active, 1);
      if (lGetUlong(active_set, FES_id) != id) {
         WARNING((SGE_EVENT, MSG_GDI_SWITCHFROMTO_SS, 
            feature_get_featureset_name(lGetUlong(active_set, FES_id)),
            feature_get_featureset_name(id)));
      }
   } else if (inactive_set) {
      lSetUlong(inactive_set, FES_active, 1);
   }
   DEXIT;
}
 
/****** gdi/feature/feature_is_active() ***************************************
*  NAME
*     feature_is_active() -- is featureset active? 
*
*  SYNOPSIS
*     int feature_is_active(featureset_id_t id) 
*
*  FUNCTION
*     returns true or false whether the given featureset whithin the 
*     Master_FeatureSet_List is marked as active or not. 
*
*  INPUTS
*     id - feature set constant 
*
*  RESULT
*     0 (false)
*     1 (true) 
******************************************************************************/
int feature_is_active(
featureset_id_t id 
) {
   lListElem *feature;
   int ret = 0;

   DENTER(TOP_LAYER, "featureset_is_active");
   feature = lGetElemUlong(Master_FeatureSet_List, FES_id, id);
   if (feature) {
      ret = lGetUlong(feature, FES_active);
   }
   DEXIT;
   return ret;
}

/****** gdi/feature/feature_get_active_featureset_id() ************************
*  NAME
*     feature_get_active_featureset_id() -- current active featureset 
*
*  SYNOPSIS
*     featureset_id_t feature_get_active_featureset_id() 
*
*  FUNCTION
*     return an id of the current active featureset 
*
*  RESULT
*     featureset_id_t - (find the definition in the .h file)
******************************************************************************/
featureset_id_t feature_get_active_featureset_id(void) 
{
   lListElem *feature;
   int ret = FEATURESET_UNINITIALIZED;

   DENTER(TOP_LAYER, "feature_get_active_featureset_id");
   for_each(feature, Master_FeatureSet_List) {
      if (lGetUlong(feature, FES_active)) {
         ret = lGetUlong(feature, FES_id);
         break;
      }
   }
   DEXIT;
   return ret;  
}

/****** gdi/feature/feature_get_featureset_name() *****************************
*  NAME
*     feature_get_featureset_name() -- return the product mode string
*
*  SYNOPSIS
*     char* feature_get_featureset_name(featureset_id_t id) 
*
*  FUNCTION
*     returns the corresponding modestring for a featureset constant 
*
*  INPUTS
*     featureset_id_t id - constant
*
*  RESULT
*     mode string 
******************************************************************************/
const char *feature_get_featureset_name(
featureset_id_t id 
) {
   int i = 0;
   char *ret = "<<unknown>>";

   DENTER(TOP_LAYER, "feature_get_featureset_name");
   while (featureset_list[i].name && featureset_list[i].id != id) {
      i++;
   }
   if (featureset_list[i].name) {
      ret = featureset_list[i].name;
   } 
   DEXIT;
   return ret; 
}

/****** gdi/feature/feature_get_featureset_id() *******************************
*  NAME
*     feature_get_featureset_id() -- returns a constant for a featureset string 
*
*  SYNOPSIS
*     featureset_id_t feature_get_featureset_id(char* name) 
*
*  FUNCTION
*     This function returns the corresponding enum value for
*     a given featureset string 
*
*  INPUTS
*     char* name - feature set name earlier known as product mode string 
*
*  RESULT
*     featureset_id_t 
******************************************************************************/
featureset_id_t feature_get_featureset_id(
const char *name 
) {
   int i = 0;
   featureset_id_t ret = FEATURESET_UNINITIALIZED;

   DENTER(TOP_LAYER, "featureset_get_id");
   if (!name) {
      DEXIT;
      return ret;
   }
   while (featureset_list[i].name && strcmp(featureset_list[i].name, name)) {
      i++;
   }
   if (featureset_list[i].name) {
      ret = featureset_list[i].id;
   } 
   DEXIT;
   return ret;
}

/****** gdi/feature/feature_get_product_name() *******************************
*  NAME
*     feature_get_product_name() -- get product name string 
*
*  SYNOPSIS
*     char* feature_get_product_name(featureset_product_name_id_t style) 
*
*  FUNCTION
*     This function will return a text string containing the 
*     the product name. The return value depends on the style
*     parameter. An invalid style value will automatically be 
*     interpreted as FS_SHORT.
*
*  INPUTS
*     style     - FS_SHORT         = return short name
*                 FS_LONG          = return long name
*                 FS_VERSION       = return version
*                 FS_SHORT_VERSION = return short name and version
*                 FS_LONG_VERSION  = return long name and version
*
*  RESULT
*     char* - static string
******************************************************************************/
const char *feature_get_product_name(
featureset_product_name_id_t style 
) {
   const char *long_name  = "";
   const char *short_name = "";
   const char *version    = "";
   const char *ret = NULL;  
   DENTER(TOP_LAYER, "feature_get_product_name");

   if (feature_get_active_featureset_id() != FEATURESET_UNINITIALIZED ) {
      if (feature_is_enabled(FEATURE_SGEEE)) {
         short_name = "SGEEE";
#ifndef ADD_SUN_COPYRIGHT         
         long_name  = "Grid Engine Enterprise Edition";
#else
         long_name  = "Sun Grid Engine, Enterprise Edition";
#endif         
      } else {
         short_name = "SGE";
#ifndef ADD_SUN_COPYRIGHT
         long_name  = "Grid Engine";
#else
         long_name  = "Sun Grid Engine";
#endif                  
      }
   }
   version = GDI_VERSION;  /* set in version.c ( in gdi libary ) */

   switch (style) {
      case FS_SHORT:
         ret = short_name;
         break;
         
      case FS_LONG:
         ret = long_name;
         break;

      case FS_VERSION:
         ret = version;
         break;

      case FS_SHORT_VERSION:
         sprintf(product_name_and_version, ""SFN" "SFN"", short_name, version); 
         ret = product_name_and_version;
         break;

      case FS_LONG_VERSION:
         sprintf(product_name_and_version, ""SFN" "SFN"", long_name, version); 
         ret = product_name_and_version;
         break;

      default:
         ret = short_name;
         break;
   }
   DEXIT;
   return ret;
}
 
/****** gdi/feature/feature_is_enabled() **************************************
*  NAME
*     feature_is_enabled() -- 0/1 whether the feature is enabled 
*
*  SYNOPSIS
*     int feature_is_enabled(feature_id_t id) 
*
*  FUNCTION
*     return true or false whether the given feature is enabled or disabled
*     in the current active featureset 
*
*  INPUTS
*     feature_id_t id 
*
*  RESULT
*     0 (false)
*     1 (true)
******************************************************************************/
int feature_is_enabled(
feature_id_t id 
) {
   lListElem *active_set;
   lListElem *feature = NULL;
   int ret = 0;

   DENTER(BASIS_LAYER, "feature_is_enabled");
   active_set = lGetElemUlong(Master_FeatureSet_List, FES_active, 1);
   if (active_set) {
      feature = lGetSubUlong(active_set, FE_id, id, FES_features);
   }
   if (feature) {
      ret = lGetUlong(feature, FE_enabled);
   }
   DEXIT;
   return ret;
}  
 
/****** gdi/feature/feature_get_name() ****************************************
*  NAME
*     feature_get_name() -- returns the feature as string 
*
*  SYNOPSIS
*     char* feature_get_name(feature_id_t id) 
*
*  FUNCTION
*     return the corresponding feature name 
*
*  INPUTS
*     feature_ id 
*
*  RESULT
*     char* - name of the given feature constant 
*             (or "<<unknown>>" when the id isn't a valid feature constant) 
******************************************************************************/
const char *feature_get_name(
feature_id_t id 
) {
   int i = 0;
   char *ret = "<<unknown>>";

   DENTER(TOP_LAYER, "feature_get_name");
   while (feature_list[i].name && feature_list[i].id != id) {
      i++;
   }
   if (feature_list[i].name) {
      ret = feature_list[i].name;
   } 
   DEXIT;
   return ret; 
}
 
/****** gdi/feature/feature_get_id() ******************************************
*  NAME
*     feature_get_id() -- translates a feature string into the constant 
*
*  SYNOPSIS
*     feature_id_t feature_get_id(char* name) 
*
*  FUNCTION
*     returns the corresponding constant for a given feature string 
*
*  INPUTS
*     char* name - valid strings are mentioned above (feature_list[]) 
*
*  RESULT
*     feature_id_t 
******************************************************************************/
feature_id_t feature_get_id(
const char *name 
) {
   int i = 0;
   feature_id_t ret = FEATURESET_UNINITIALIZED;

   DENTER(TOP_LAYER, "feature_get_id");
   while (feature_list[i].name && strcmp(feature_list[i].name, name)) {
      i++;
   }
   if (feature_list[i].name) {
      ret = feature_list[i].id;
   }
   DEXIT;
   return ret;
}
