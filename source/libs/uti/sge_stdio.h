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

#define FPRINTF(x) \
   __fprintf_ret = fprintf x; \
   if (__fprintf_ret < 0) { \
      goto FPRINTF_ERROR; \
   }

#define SGE_FOPEN(fp, filename, mode) \
   fp = fopen(filename, mode); \
   __fclose_done = 0;

#define FCLOSE(x) \
   if(fclose(x) != 0) { \
      __fclose_done = 1; \
      goto FPRINTF_ERROR; \
   }

#define FCLEANUP(fp, filename) \
   if(__fclose_done == 0) { \
      fclose(fp); \
   } \
   unlink(filename);


extern int __fprintf_ret;
extern int __fclose_done;

#endif /* __SGE_STDIO_H */
