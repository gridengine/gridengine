#ifndef __UTILITY_H
#define __UTILITY_H
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

void sge_show_hold_list(int hold, int how);

int sge_atoi(const char *int_str);

char *sge_malloc(int size);

char *sge_realloc(char *ptr, int size);

char *sge_free(char *cp);

void sge_show_checkpoint(int how, int op);

void sge_show_y_n(int op, int how);

void sge_show_mail_options(int op, int how);

void sge_get_states(int nm, char *str, u_long32 op);

void sge_show_states(int nm, u_long32 how, u_long32 states);

int sge_unlink(const char *prefix, const char *suffix);

const char *sge_getenv(const char *env_str);

void sge_sleep(int sec, int usec);

int check_isalive(const char *);

int verify_filename(const char *fname);

#endif /* __UTILITY_H */

