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

#include "sge_dstring.h"

/* These files and directories will be found in COMMON_DIR
 * They must be accessed with an absolute path. Do not use these defines!
 */
#define ROOT_DIR                  "/usr/SGE"
#define COMMON_DIR                "common"
#define BOOTSTRAP_FILE            "bootstrap"
#define CONF_FILE                 "configuration"
#define SCHED_CONF_FILE           "sched_configuration"
#define ACCT_FILE                 "accounting"
#define STAT_FILE                 "statistics"
#define LOCAL_CONF_DIR            "local_conf"
#define SHADOW_MASTERS_FILE       "shadow_masters"

#define PATH_SEPARATOR "/"
#define PATH_SEPARATOR_CHAR '/'

void path_mt_init(void);

const char *path_state_get_sge_root(void);
const char *path_state_get_cell_root(void);
const char *path_state_get_bootstrap_file(void);
const char *path_state_get_conf_file(void);
const char *path_state_get_sched_conf_file(void);
const char *path_state_get_act_qmaster_file(void);
const char *path_state_get_acct_file(void);
const char *path_state_get_stat_file(void);
const char *path_state_get_local_conf_dir(void);
const char *path_state_get_shadow_masters_file(void);

void path_state_set_sge_root(const char *path);
void path_state_set_cell_root(const char *path);
void path_state_set_conf_file(const char *path);
void path_state_set_sched_conf_file(const char *path);
void path_state_set_act_qmaster_file(const char *path);
void path_state_set_acct_file(const char *path);
void path_state_set_stat_file(const char *path);
void path_state_set_local_conf_dir(const char *path);
void path_state_set_shadow_masters_file(const char *path);

bool sge_setup_paths(const char *cell, dstring *error_dstring);

#endif /* __SETUP_PATH_H */
