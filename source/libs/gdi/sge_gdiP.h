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

u_long32 gdi_state_get_request_id(void);
u_long32 gdi_state_get_next_request_id(void);
int gdi_state_get_daemon_first(void);
int gdi_state_get_first_time(void);
int gdi_state_get_commd_state(void);
int gdi_state_get_program_id(void);
int gdi_state_get_isalive(void);
int gdi_state_get_sec_initialized(void);
char *gdi_state_get_cached_master_name(void);


void gdi_state_set_request_id(u_long32 id);
void gdi_state_set_daemon_first(int i);
void gdi_state_set_first_time(int i);
void gdi_state_set_commd_state(int i);
void gdi_state_set_program_id(int i);
void gdi_state_set_isalive(int i);
void gdi_state_set_sec_initialized(int i);

gdi_send_t* gdi_state_get_last_gdi_request(void); 
void        gdi_state_clear_last_gdi_request(void);
void        gdi_free_request(gdi_send_t **async_gdi); 
bool        gdi_set_request(const char* rhost, const char* commproc, u_short id, 
                            state_gdi_multi *out, u_long32 gdi_request_mid); 

#ifdef  __cplusplus
}
#endif

#endif /* __SGE_GDI_INTERN_H */

