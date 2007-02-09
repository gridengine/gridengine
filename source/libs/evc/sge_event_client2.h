#ifndef __SGE_C_EVENT2_H
#define __SGE_C_EVENT2_H
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

#ifdef TEST_GDI2

#include "sge_gdi.h"
#include "sge_gdi_ctx.h"
#include "sge_eventL.h"
#include "uti/sge_monitor.h"
#include "gdi/sge_gdi_ctx.h"

#define DEFAULT_EVENT_DELIVERY_INTERVAL (10)

typedef struct sge_evc_class_str sge_evc_class_t; 

struct sge_evc_class_str {
   void *sge_evc_handle;

   sge_gdi_ctx_class_t* (*get_gdi_ctx)(sge_evc_class_t *thiz);
   bool (*ec_register)(sge_evc_class_t *thiz, bool exit_on_qmaster_down, lList **alpp);
   bool (*ec_deregister)(sge_evc_class_t *thiz);
   bool (*ec_is_initialized)(sge_evc_class_t *thiz);
   lListElem* (*ec_get_event_client)(sge_evc_class_t *thiz);

   bool (*ec_subscribe)(sge_evc_class_t *thiz, ev_event event);
   bool (*ec_subscribe_all)(sge_evc_class_t *thiz);

   bool (*ec_unsubscribe)(sge_evc_class_t *thiz, ev_event event);
   bool (*ec_unsubscribe_all)(sge_evc_class_t *thiz);

   int (*ec_get_flush)(sge_evc_class_t *thiz, ev_event event);
   bool (*ec_set_flush)(sge_evc_class_t *thiz, ev_event event, bool flush, int interval);
   bool (*ec_unset_flush)(sge_evc_class_t *thiz, ev_event event);

   bool (*ec_subscribe_flush)(sge_evc_class_t *thiz, ev_event event, int flush);

   bool (*ec_mod_subscription_where)(sge_evc_class_t *thiz, ev_event event, const lListElem *what, const lListElem *where);

   int (*ec_set_edtime)(sge_evc_class_t *thiz, int intval);
   int (*ec_get_edtime)(sge_evc_class_t *thiz);

   bool (*ec_set_busy_handling)(sge_evc_class_t *thiz, ev_busy_handling handling);
   ev_busy_handling (*ec_get_busy_handling)(sge_evc_class_t *thiz);

   bool (*ec_set_flush_delay)(sge_evc_class_t *thiz, int flush_delay);
   int (*ec_get_flush_delay)(sge_evc_class_t *thiz);

   bool (*ec_set_busy)(sge_evc_class_t *thiz, int busy);
   bool (*ec_get_busy)(sge_evc_class_t *thiz);

   bool (*ec_set_session)(sge_evc_class_t *thiz, const char *session);
   const char *(*ec_get_session)(sge_evc_class_t *thiz);

   ev_registration_id (*ec_get_id)(sge_evc_class_t *thiz);

   bool (*ec_commit)(sge_evc_class_t *thiz, lList **alpp);
   bool (*ec_commit_multi)(sge_evc_class_t *thiz, lList **malp, state_gdi_multi *state);

   bool (*ec_get)(sge_evc_class_t *thiz, lList **event_list, bool exit_on_qmaster_down);

   void (*ec_mark4registration)(sge_evc_class_t *thiz);
   bool (*ec_need_new_registration)(sge_evc_class_t *thiz);
      
   bool (*ec_register_local)(sge_evc_class_t *thiz, bool exit_on_qmaster_down, lList **alpp);
   bool (*ec_deregister_local)(sge_evc_class_t *thiz);
   bool (*ec_commit_local)(sge_evc_class_t *thiz, event_client_update_func_t update_func);
   /* dump current settings */
   void (*dprintf)(sge_evc_class_t *thiz);
};

sge_evc_class_t *sge_evc_class_create(sge_gdi_ctx_class_t *sge_gdi_ctx,
                                      ev_registration_id id,
                                      lList **alpp);

void sge_evc_class_destroy(sge_evc_class_t **pst);

bool sge_gdi2_evc_setup(sge_evc_class_t **evc_ref, sge_gdi_ctx_class_t *sge_gdi_ctx, ev_registration_id reg_id, lList **alpp);

#endif /* TEST_GDI2 */

#endif /* __SGE_C_EVENT2_H */

