#ifndef __SGE_GDI_ATTRIBUTES_H
#define __SGE_GDI_ATTRIBUTES_H
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

#ifdef  __cplusplus
extern "C" {
#endif

/* gdi setup, get attribute value for this host, gdi shutdown */
const char *sge_gdi_myhost_attribute(const char *attr_name);

/* gdi setup, get scaling value for this host, gdi shutdown */
double sge_gdi_myhost_scaling(const char *attr_name);

/* gdi setup, get # of slots for job at this host, gdi shutdown 
   jobid may be 0 -> getenv("JOB_ID") is used */
int sge_gdi_myhost_nslots(u_long32 jobid);

/* get attribute value for given host */
double sge_gdi_host_scaling(const char *hostname, const char *attr_name);

/* get scaling value for this host */
const char *sge_gdi_host_attribute(const char *hostname, const char *attr_name);

/* get # of slots for job at this host */
int sge_gdi_host_nslots(const char *hostname, u_long32 jobid);

#ifdef  __cplusplus
}
#endif

#endif /* __SGE_GDI_ATTRIBUTES_H */
