#ifndef __GDI_SETUP_H
#define __GDI_SETUP_H
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

/* these values should be used to specify params in a sge_gdi_param call */
enum {
   SET_MEWHO,         /* intval */
   SET_LEAVE,         /* strval contains exit_function */
   SET_ISALIVE,       /* intval 1/0 */
   SET_EXIT_ON_ERROR, /* 0/1 default is true */
   LAST_VALUE
};

/* these values are standarized gdi return values */
enum {
   AE_OK = 0,
   AE_ALREADY_SETUP,
   AE_UNKNOWN_PARAM,
   AE_QMASTER_DOWN
};

int sge_gdi_setup(const char *programname, lList **alpp);
int sge_gdi_param(int, int, char *);
int sge_gdi_shutdown(void);

#ifdef  __cplusplus
}
#endif

#endif /* __SGE_GDI_H */
