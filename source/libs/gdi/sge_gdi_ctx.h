#ifndef __SGE_GDI_CTX_H
#define __SGE_GDI_CTX_H
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

#include "sge_env.h"
#include "sge_prog.h"
#include "setup_path.h"
#include "sge_bootstrap.h"
#include "sge_error_class.h"

typedef struct sge_gdi_ctx_class_str sge_gdi_ctx_class_t; 

struct sge_gdi_ctx_class_str {
   void *sge_gdi_ctx_handle;
   
   sge_error_class_t *eh;

   sge_env_state_class_t* (*get_sge_env_state)(sge_gdi_ctx_class_t *thiz);
   sge_prog_state_class_t* (*get_sge_prog_state)(sge_gdi_ctx_class_t *thiz);
   sge_path_state_class_t* (*get_sge_path_state)(sge_gdi_ctx_class_t *thiz);
   sge_bootstrap_state_class_t* (*get_sge_bootstrap_state)(sge_gdi_ctx_class_t *thiz);
   
   bool (*connect)(sge_gdi_ctx_class_t *thiz);
   bool (*is_alive)(sge_gdi_ctx_class_t *thiz);
   lList* (*gdi)(sge_gdi_ctx_class_t *thiz, int target, int cmd, lList **lp, lCondition *where, lEnumeration *what);
   int (*gdi_multi)(sge_gdi_ctx_class_t* ctx, lList **alpp, int mode, u_long32 target, u_long32 cmd,
                  lList **lp, lCondition *cp, lEnumeration *enp, lList **malpp, state_gdi_multi *state, bool do_copy);
   
   const char* (*get_master)(sge_gdi_ctx_class_t *thiz);
   u_long32 (*get_sge_qmaster_port)(sge_gdi_ctx_class_t *thiz);
   const char* (*get_progname)(sge_gdi_ctx_class_t *thiz);
   const char* (*get_username)(sge_gdi_ctx_class_t *thiz);
   const char* (*get_cell_root)(sge_gdi_ctx_class_t *thiz);
   const char* (*get_groupname)(sge_gdi_ctx_class_t *thiz);
   uid_t (*get_uid)(sge_gdi_ctx_class_t *thiz);
   gid_t (*get_gid)(sge_gdi_ctx_class_t *thiz);
   
   /* commlib */
   cl_com_handle_t* (*get_com_handle)(sge_gdi_ctx_class_t *thiz);
   void (*set_com_handle)(sge_gdi_ctx_class_t *thiz, cl_com_handle_t*com_handle);
   
   /* dump current settings */
   void (*dprintf)(sge_gdi_ctx_class_t *thiz);
};

sge_gdi_ctx_class_t *sge_gdi_ctx_class_create(int prog_number, 
                                              const char* username, 
                                              const char *sge_root, 
                                              const char *sge_cell, 
                                              int sge_qmaster_port, 
                                              int sge_execd_port,
                                              sge_error_class_t *error_handler);



/*
   create an instance of sge_gdi_ctx_class_t and initialize it
   with the information of the bootstrap file

   @param prog_number  number of the program which uses the context
   @param url          url to the bootstrap file
   @param username     name of the user which uses the context
   @param credentials  credentials of the user
   @param alpp         answer list reference
   @return the gdi context

   TODO we need a answer list to transport error messages to the caller
*/
sge_gdi_ctx_class_t *sge_gdi_ctx_class_create_from_bootstrap(int prog_number,
                                                             const char* url,
                                                             const char* username,
                                                             const char* credentials,
                                                             sge_error_class_t *error_handler);

void sge_gdi_ctx_class_destroy(sge_gdi_ctx_class_t **pst);

sge_gdi_ctx_class_t* sge_gdi_get_thread_local_ctx(void);
void sge_gdi_set_thread_local_ctx(sge_gdi_ctx_class_t* ctx);

#endif /* __SGE_GDI_CTX_H */


