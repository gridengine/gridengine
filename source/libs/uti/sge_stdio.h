#ifndef __SGE_STDIO_H
#define __SGE_STDIO_H
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

#include <stdio.h>
#include <sys/time.h>
#include <time.h>  

/****** uti/stdio/FPRINTF() ***************************************************
*  NAME
*     FPRINTF() -- fprintf() macro 
*
*  SYNOPSIS
*     #define FPRINTF(arguments)
*     void fprintf(FILE *stream, const char *format, ...)
*
*  FUNCTION
*     This FPRINTF macro has to be used similar to the fprintf 
*     function. It is not necessary to check the return value. 
*     In case of an error the macro will jump to a defined label.
*     The label name is 'FPRINTF_ERROR'.
*
*  INPUTS
*     FILE *stream       - output stream
*     const char *format - format string
*     ...
*
*  NOTES
*     Don't forget to define the 'FPRINTF_ERROR'-label
******************************************************************************/
#define FPRINTF(x) \
   __fprintf_ret = fprintf x; \
   if (__fprintf_ret == -1) { \
      goto FPRINTF_ERROR; \
   }

extern int __fprintf_ret;

pid_t sge_peopen(const char *shell, int login_shell, const char *command, 
                 const char *user, char **env, FILE **fp_in, FILE **fp_out, 
                 FILE **fp_err);
 
int sge_peclose(pid_t pid, FILE *fp_in, FILE *fp_out, FILE *fp_err, 
                struct timeval *timeout); 

#endif /* __SGE_STDIO_H */
