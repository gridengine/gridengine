#ifndef __SETUP_PATH_H
#define __SETUP_PATH_H
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
#include "cull_list.h"
#include "sge.h"

/* These files and directories will be found in COMMON_DIR
 * They must be accessed with an absolute path. Do not use these defines!
 */
#define ROOT_DIR                  "/usr/SGE"
#define COMMON_DIR                "common"
#define CONF_FILE                 "configuration"
#define SCHED_CONF_FILE           "sched_configuration"
#define KEY_FILE                  "key"
#define ACCT_FILE                 "accounting"
#define STAT_FILE                 "statistics"
#define LOCAL_CONF_DIR            "local_conf"
#define HISTORY_DIR               "history"
#define SHADOW_MASTERS_FILE       "shadow_masters"
#define LICENSE_FILE              "license"
#define MASTER_IOR_FILE           "master.ior"

#ifndef WIN32NATIVE
#  define PATH_SEPARATOR "/"
#  define PATH_SEPARATOR_CHAR '/'
#else
#  define PATH_SEPARATOR "\\"
#  define PATH_SEPARATOR_CHAR '\\'
#endif      

typedef struct _sge_path_type {
    char       *sge_root;
    char       *cell_root;
    char       *conf_file;
    char       *sched_conf_file;
    char       *act_qmaster_file;
    char       *acct_file;
    char       *stat_file;
    char       *local_conf_dir;
    char       *key_file;
    char       *history_dir;
    char       *execd_args;
    char       *shadow_masters_file;
    char       *license_file;
    char       *product_mode_file;
    char       *get_token_cmd;
    char       *master_ior_file;
} sge_path_type;

extern sge_path_type path;

void sge_setup_paths(const char *cell, sge_path_type *p, lList **alpp);

#ifdef WIN32NATIVE
void sge_delete_paths ();
#endif /* WIN32NATIVE */

#endif /* __SETUP_PATH_H */
