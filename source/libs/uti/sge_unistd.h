#ifndef __SGE_UNISTD_H
#define __SGE_UNISTD_H
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

#include <unistd.h>

typedef void (*exit_func_type)(int);

int sge_unlink(const char *prefix, const char *suffix); 

void sge_sleep(int sec, int usec);

int sge_chdir(const char *path, int exit_on_error);  

void sge_exit(int i);

int sge_mkdir(const char *path, int fmode, int exit_on_error);    
 
exit_func_type sge_install_exit_func(exit_func_type);     

int sge_rmdir(const char *cp, char *err_str);
 
int sge_is_directory(const char *name);
 
int sge_is_file(const char *name);

#endif /* __SGE_UNISTD_H */
