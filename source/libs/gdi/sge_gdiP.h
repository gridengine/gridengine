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

/* v5.0:        0x10000000 */
/* v5.1:        0x10000001 */
/* v5.2:        0x10000002 */
/* v5.2.3:      0x10000003 */
/* v5.3 alpha1  0x100000F0 */
/* before hash  0x100000F1 */
/* v5.3beta1    0x100000F2 */
/* v5.3beta2    0x100000F3 */
/* v5.3         0x100000F4 */
/* v6.0, v6.0u1 0x10000FFF */
#define GRM_GDI_VERSION 0x10001000

u_long32 gdi_state_get_request_id(void);
int gdi_state_get_daemon_first(void);
int gdi_state_get_first_time(void);
int gdi_state_get_commd_state(void);
int gdi_state_get_program_id(void);
int gdi_state_get_isalive(void);
int gdi_state_get_reread_qmaster_file(void);
int gdi_state_get_sec_initialized(void);
char *gdi_state_get_cached_master_name(void);


void gdi_state_set_request_id(u_long32 id);
void gdi_state_set_daemon_first(int i);
void gdi_state_set_first_time(int i);
void gdi_state_set_commd_state(int i);
void gdi_state_set_program_id(int i);
void gdi_state_set_isalive(int i);
void gdi_state_set_reread_qmaster_file(int i);
void gdi_state_set_sec_initialized(int i);

#ifdef  __cplusplus
}
#endif

#endif /* __SGE_GDI_INTERN_H */

