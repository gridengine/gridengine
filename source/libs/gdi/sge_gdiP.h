#ifndef __SGE_GDIP_H
#define __SGE_GDIP_H
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


#ifdef WIN32NATIVE
#	include "win32nativetypes.h"
#endif

#include "cull.h"
#include "sge_gdi.h"

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef NEW_GDI_STATE
u_long32 gdi_state_get_next_request_id(void);
gdi_send_t* gdi_state_get_last_gdi_request(void); 
char *gdi_state_get_cached_master_name(void);
void        gdi_state_clear_last_gdi_request(void);
bool        gdi_set_request(const char* rhost, 
                            const char* commproc, 
                            u_short id, 
                            state_gdi_multi *out, 
                            u_long32 gdi_request_mid); 

#else

#include "sge_gdi_ctx.h"

u_long32 gdi_state_get_next_request_id(sge_gdi_ctx_class_t *ctx);
gdi_send_t* gdi_state_get_last_gdi_request(sge_gdi_ctx_class_t *ctx); 
void        gdi_state_clear_last_gdi_request(sge_gdi_ctx_class_t *ctx);
bool        gdi_set_request(sge_gdi_ctx_class_t *ctx,
                            const char* rhost, 
                            const char* commproc, 
                            u_short id, 
                            state_gdi_multi *out, 
                            u_long32 gdi_request_mid); 

#endif /* NEW_GDI_STATE */

#ifdef  __cplusplus
}
#endif


#endif /* __SGE_GDI_INTERN_H */

