#ifndef __SGE_STRING_APPEND_H
#define __SGE_STRING_APPEND_H
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

/****** uti/dstring/DSTRING_INIT **********************************************
*  NAME
*     DSTRING_INIT -- Define to initialize dstring variables 
*
*  SYNOPSIS
*     #define DSTRING_INIT {NULL, 0}
*
*  FUNCTION
*     Define to preinitialize dstring variables 
*
*  EXAMPLE
*     {
*        dstring error_msg = DSTRING_INIT;
*     }
******************************************************************************/
#define DSTRING_INIT {NULL, 0}

typedef struct {
   char *s;
   size_t size;
} dstring;

char* sge_dstring_append(dstring *sb, const char *a);

char* sge_dstring_sprintf(dstring *sb, const char *fmt, ...);

void sge_dstring_free(dstring *sb);

const char *sge_dstring_get_string(const dstring *string);

char* sge_dstring_copy_string(dstring *sb, char* str);

char* sge_dstring_copy_dstring(dstring *sb1, dstring *sb2);

size_t sge_dstring_strlen(const dstring *string);


#endif /* __SGE_STRING_APPEND_H */

