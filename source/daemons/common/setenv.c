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
#include <stdlib.h>
#include <string.h>

#include "setenv.h"

extern char **environ;

/************************************************************
 Use with care!! Assumes it is called with a new environment
 of at leat 100 elements
 Never use with the original **environ

 returns:
 - 0 OK
 - 1 malloc error
 ************************************************************/

int sge_setenv(
char *name,
char *value 
) {
   int i;
   int varlen;
   int vallen;
   char tmpchar;
   static int envsize = 0;              /* allocated environment size */


   if (!envsize) {
      environ = (char **) malloc(sizeof(char *) * (100));
      environ[0] = NULL;
      envsize = 100;
   }

   varlen = strlen(name);

   if (value == NULL)
      vallen = 0;
   else
      vallen = strlen(value);

   for (i = 0; environ[i] != NULL; i++) {
      if (strlen(environ[i]) < varlen)
         continue;
      tmpchar = environ[i][varlen];
      if ((!strncmp(environ[i], name, varlen))
          && ((tmpchar == '\0') || (tmpchar == '='))) {

         /* Allocate mem for new variable entry. An entry looks like:
            "envirvar=trulla" or just "envirvar". In the second case we waste
            a byte. But who cares. */
         if (!(environ[i] = realloc(environ[i], (varlen + vallen + 2))))
            return 1;
         strcpy(environ[i], name);
         if (value) {
            strcat(environ[i], "=");
            strcat(environ[i], value);
         }
         return 0;
      }
   }

   /* if allocated environment is eaten up add 10 more entries */
   if (i == envsize-1) {
      if (!(environ = (char **)realloc(environ, 
                                       sizeof(char *)*(envsize + 10)))) {
         return 1;
      }
      envsize += 10;
   }

   if (!(environ[i] = malloc(varlen + vallen + 2)))
      return 1;

   strcpy(environ[i], name);
   if (value) {
      strcat(environ[i], "=");
      strcat(environ[i], value);
   }
   environ[++i] = NULL;

   return 0;
}
