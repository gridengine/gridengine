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
#include "basis_types.h"
#include "sge_spoolmsg.h"
#include "sge_stdio.h"

char *spoolmsg_message[] = {
   "",
   "DO NOT MODIFY THIS FILE MANUALLY!",
   "",
   NULL
};

/****** sge_spoolmsg/sge_spoolmsg_write() *************************************
*  NAME
*     sge_spoolmsg_write() -- add a comment in a file 
*
*  SYNOPSIS
*     int sge_spoolmsg_write(FILE *file, char comment_char) 
*
*  FUNCTION
*     This function writes an additional comment into a file. First
*     character in a comment line is 'comment_char'.  
*
*  INPUTS
*     FILE *file        - file descriptor 
*     char comment_char - first character in a comment line 
*
*  RESULT
*     -1 on error else 0 
*******************************************************************************
*/
int sge_spoolmsg_write(FILE *file, char comment_char, char *version)
{
   int i;

   FPRINTF((file, "%c Version: %s\n", comment_char, version));
   i = 0;
   while(spoolmsg_message[i]) {
      FPRINTF((file, "%c %s\n", comment_char, spoolmsg_message[i]));
      i++;
   }

   return 0;
FPRINTF_ERROR:
   return -1;
}
