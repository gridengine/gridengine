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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "sgermon.h"
#include "sge_string.h"
#include "sge_exit.h"
#include "sge_log.h"
#include "def.h"
#include "msg_utilib.h"

#define IS_DELIMITOR(c,delimitor) (delimitor?(strchr(delimitor, c)?1:0):isspace(c))

/* compare hosts with FQDN or not */
int fqdn_cmp = 0;
char *default_domain = NULL;

/****** uti/string/sge_basename() **********************************************
*  NAME
*     sge_basename() -- get basename for path
*
*  SYNOPSIS
*     char* sge_basename(const char *name, int delim) 
*
*  FUNCTION
*     Determines the basename for a path like string - the last field of a
*     string where fields are separated by a fixed one character delimiter.
*
*  INPUTS
*     const char *name - contains the input string (path)
*     int delim        - delimiter
*
*  RESULT
*     char* - pointer to base of name after the last delimter
*             NULL if "name" is NULL or zero length string or delimiter is 
*             the last character in "name"
*
*  EXAMPLE
*     sge_basename("/usr/local/bin/flex", '/'); returns "flex"
*
*******************************************************************************/
const char *sge_basename(const char *name, int delim) 
{
   char *cp;

   DENTER(BASIS_LAYER, "sge_basename");

   if (!name)
      return NULL;
   if (name[0] == '\0')
      return NULL;

   cp = strrchr(name, delim);
   if (!cp) {
      DEXIT;
      return name; 
   }
   else {
      cp++;
      if (*cp == '\0') {
         DEXIT;
         return NULL;
      }
      else {
         DEXIT;
         return cp;
      }
   }
}

/*-------------------------------------------------------------------
 * sge_dirname
 *
 * returns: pointer to a malloced string containing the first part
 *          of "name" up to, but not including delimiter
 *          NULL if "name" is NULL or zero length string or delimiter is
 *          first character in "name"
 *
 *          This routine is called "dirname" in opposite to "basename"
 *          but is mostly used to strip off the domainname of a FQDN
 *-------------------------------------------------------------------*/
char *sge_dirname(
const char *name,
int delim 
) {
   char *cp, * cp2;

   DENTER(BASIS_LAYER, "sge_dirname");

   if (!name) {
      DEXIT;
      return NULL;
   }

   if (name[0] == '\0' || name[0] == delim) {
      DEXIT;
      return NULL;
   }

   cp = strchr(name, delim);

   if (!cp) {                   /* no occurence of delim */
      cp2 = strdup(name);
      DEXIT;
      return cp2;
   }
   else {
      if ((cp2 = malloc((cp - name) + 1)) == NULL) {
         DEXIT;
         return 0;
      }
      else {
         strncpy(cp2, name, cp - name);
         cp2[cp - name] = '\0';
         DEXIT;
         return cp2;
      }
   }
}

/*-------------------------------------------------------------------
 * sge_strtok
 *
 * if no delimitor is given isspace is used
 *
 *-------------------------------------------------------------------*/
char *sge_strtok(
const char *str,
const char *delimitor 
) {
   char *cp;
   char *saved_cp;
   static char *static_cp = NULL;
   static char *static_str = NULL;
   static unsigned int alloc_len = 0;
   unsigned int n;

   DENTER(BASIS_LAYER, "sge_strtok");

   if (str) {
      n = strlen(str);
      if (static_str) {
         if (n > alloc_len) {
            /* need more memory */
            free(static_str);
            static_str = malloc(n + 1);
            alloc_len = n;
         }
      }
      else {
         static_str = malloc(n + 1);
         alloc_len = n;
      }
      strcpy(static_str, str);
      saved_cp = static_str;
   }
   else
      saved_cp = static_cp;

   /* seek first character which is no '\0' and no delimitor */
   while (1) {

      /* found end of string */
      if (*saved_cp == '\0') {
         DEXIT;
         return NULL;
      }

      /* eat white spaces */
      if (!IS_DELIMITOR((int) saved_cp[0], delimitor))
         break;

      saved_cp++;
   }

   /* seek end of string given by '\0' or delimitor */
   cp = saved_cp;
   while (1) {
      if (!cp[0]) {
         static_cp = cp;

         DEXIT;
         return saved_cp;
      }

      /* test if we found a delimitor */
      if (IS_DELIMITOR((int) cp[0], delimitor)) {
         cp[0] = '\0';
         cp++;
         static_cp = cp;

         DEXIT;
         return saved_cp;
      }
      cp++;
   }
}

/*-------------------------------------------------------------------
 * sge_strtok_r
 *
 * Reentrant version of sge_strtok. When str is not NULL, *last must
 * be NULL. If str is NULL, *last must not be NULL. The caller is
 * responsible for freeing *last.
 *
 * if no delimitor is given isspace is used
 *
 *-------------------------------------------------------------------*/
char *sge_strtok_r(
const char *str,
const char *delimitor,
struct saved_vars_s **last 
) {
   char *cp;
   char *saved_cp;
   struct saved_vars_s *saved;

   DENTER(BASIS_LAYER, "sge_strtok_r");

   if (str) {
      if (*last) {
         ERROR((SGE_EVENT, MSG_POINTER_INVALIDSTRTOKCALL ));
         DEXIT;
         abort();
      }
      *last = (struct saved_vars_s *)malloc(sizeof(struct saved_vars_s));
      memset(*last, 0, sizeof(struct saved_vars_s));
      saved = *last;

      saved->static_str = malloc(strlen(str) + 1);

      strcpy(saved->static_str, str);
      saved_cp = saved->static_str;
   }
   else {
      if (*last == NULL) {
         ERROR((SGE_EVENT, MSG_POINTER_INVALIDSTRTOKCALL));
         DEXIT;
         abort();
      }
      saved = *last;
      saved_cp = saved->static_cp;
   }

   /* seek first character which is no '\0' and no delimitor */
   while (1) {

      /* found end of string */
      if (*saved_cp == '\0') {
         DEXIT;
         return NULL;
      }

      /* eat white spaces */
      if (!IS_DELIMITOR((int) saved_cp[0], delimitor))
         break;

      saved_cp++;
   }

   /* seek end of string given by '\0' or delimitor */
   cp = saved_cp;
   while (1) {
      if (!cp[0]) {
         saved->static_cp = cp;

         DEXIT;
         return saved_cp;
      }

      /* test if we found a delimitor */
      if (IS_DELIMITOR((int) cp[0], delimitor)) {
         cp[0] = '\0';
         cp++;
         saved->static_cp = cp;

         DEXIT;
         return saved_cp;
      }
      cp++;
   }
}

void free_saved_vars(
struct saved_vars_s *last 
) {
   if (last->static_str)
      free(last->static_str);
   free(last);
}

/*-------------------------------------------------------------------
 * sge_strdup
 *
 *-------------------------------------------------------------------*/
char *sge_strdup(
char *old,
const char *s 
) {
   int n;

   /* free and NULL the old pointer */
   FREE(old);
   old = NULL;

   if (!s)
      return NULL;

   n = strlen(s);
   if (n) {
      old = malloc(n + 1);
      if (old) {
         strcpy(old, s);
      }
   }

   return old;
}

/*-------------------------------------------------------------------
 * strip_blanks
 *  
 *  purpose:
 *     strips all blanks contained in a string. the string is used both
 *      as source and drain for the necessary copy operations. the
 *     string is \0 terminated afterwards.
 *  
 *  input parameters:
 *     str            :  the string to be condensed.
 *-------------------------------------------------------------------*/
void strip_blanks(
char *str 
) {
   char *cp = str;

   DENTER(BASIS_LAYER, "strip_blanks");

   if (!str) {
      DEXIT;
      return;
   }

   while (*str) {
      if (*str != ' ')
         *cp++ = *str;
      str++;
   };
   *cp = '\0';

   DEXIT;
   return;
}

/*************************************************************************

   sge_delim_str

   purpose:
      truncates a string according to a delimiter set

   return values:
      a copy of the input string truncated according to the delimiter set.
      ATTENTION: the user is responsible for freeing the allocated memory
      outside this routine. if not enough space could be allocated, NULL
      is returned.

   input parameters:
      str            :  string to be truncated
      delim_pos      :  a placeholder for the delimiting position in str
                        on exit
      delim          :  string containing delimiter characters

   output parameters:
      delim_pos      :  if set on entry the position of the delimiter in
                        the input string str is returned. if no delimiting
                        character in string was found, the address of the
                        closing \0 in str is returned.

*************************************************************************/
char *sge_delim_str(
char *str,
char **delim_pos,
const char *delim 
) {
   char *cp, *tstr;

   DENTER(BASIS_LAYER, "sge_delim_str");

   /* we want it non-destructive --> we need a copy of str */
   if (!(tstr = strdup(str))) {
      DEXIT;
      return NULL;
   }

   /* walk through str to find a character contained in delim or a
    * closing \0
    */

   cp = tstr;
   while (*cp) {
      if (strchr(delim, (int) *cp))     /* found */
         break;
      cp++;
   }

   /* cp now either points to a closing \0 or to a delim character */
   if (*cp)                     /* if it points to a delim character */
      *cp = '\0';               /* terminate str with a \0 */
   if (delim_pos)               /* give back delimiting position for name */
      *delim_pos = str + strlen(tstr);
   /* delim_pos either points to the delimiter or the closing \0 in str */

   DEXIT;
   return tstr;
}

/************************************************/
char *stradd(const char *cp1, const char *cp2, char *res)
{
   strcpy(res, cp1);
   strcat(res, cp2);
   return res;
}

/*
   ** NAME
   **   sge_strnullcmp
   ** PARAMETER
   **   a     - string parameter 1 to be compared
   **   b     - string parameter 2 to be compared
   ** RETURN
   **   0     - strings are the same or both NULL
   **  -1     - a < b or a is NULL
   **   1     - a > b or b is NULL
   ** EXTERNAL
   **   none
   ** DESCRIPTION
   **   like strcmp, apart from the handling of NULL strings
   **   these are treated as being less than any not-NULL string
   **   important for sorting lists where NULL strings can occur
 */
int sge_strnullcmp(const char *a, const char *b) 
{
   if (!a && b)
      return -1;
   if (a && !b)
      return 1;
   if (!a && !b)
      return 0;
   return strcmp(a, b);
}

/*
   ** NAME
   **   sge_strnullcasecmp
   ** PARAMETER
   **   a     - string parameter 1 to be compared
   **   b     - string parameter 2 to be compared
   ** RETURN
   **   0     - strings are the same minus case or both NULL
   **  -1     - a < b or a is NULL
   **   1     - a > b or b is NULL
   ** EXTERNAL
   **   strcasecmp
   ** DESCRIPTION
   **   like sge_strnullcmp, only insensitive to case
 */
int sge_strnullcasecmp(
const char *a,
const char *b 
) {
   if (!a && b)
      return -1;
   if (a && !b)
      return 1;
   if (!a && !b)
      return 0;
   return SGE_STRCASECMP(a, b);
}


/*********************************************************************** 
   Check for a valid filename.
   Filename can only contain 0-9a-zA-Z._- 
   No / is allowed. 
   
   returns 0 if filename is correct 
 ***********************************************************************/
int check_fname(const char *fname)
{
   const char *cp = fname;

   if (!fname)
      return 1;

   while (*cp) {
      if (!isalnum((int) *cp) && !(strchr("._-", *cp)))
         return 1;
      cp++;
   }
   return 0;
}


/* test whether a string contains a wildcard pattern */
int sge_is_pattern(const char *s) {
   char c;
   while ((c = *s++)) {
      if (strchr("*?[]", c))
         return 1;
   }
   return 0;
}


int hostcmp(const char *h1, const char*h2)
{
   int cmp = -1;
   char h1_cpy[MAXHOSTLEN+1], h2_cpy[MAXHOSTLEN+1];


   DENTER(BASIS_LAYER, "hostcmp");

   hostcpy(h1_cpy,h1);
   hostcpy(h2_cpy,h2);

   if (h1_cpy && h2_cpy) {
     cmp = SGE_STRCASECMP(h1_cpy, h2_cpy);

     DPRINTF(("hostcmp(%s, %s) = %d\n", h1_cpy, h2_cpy));
   }

   DEXIT;
   return cmp;
}

void string_toupper(char *buffer, int max_len ) {
   int i;
   DENTER(BASIS_LAYER, "string_toupper");
   if (buffer != NULL) {
       for ( i = 0 ; (i < max_len) && (i < strlen(buffer)) ; i++) {
           buffer[i] = toupper(buffer[i]); 
       }
   }
   DPRINTF(("string_toupper: string is %s\n", buffer));
   DEXIT;
} 

void hostcpy(
char *dst, 
const char *raw
) {
   char *s;

/*   if ( (dst == NULL) || (raw == NULL ) ) {
      return;
   }  
*/
 
   if (!fqdn_cmp) {              /* standard: simply ignore FQDN */
      strncpy(dst, raw, MAXHOSTLEN);
      if ((s = strchr(dst, '.')))
         *s = '\0';
   } else if (default_domain && /* exotic: honor FQDN but use default_domain */
          SGE_STRCASECMP(default_domain, "none")) {
      if (!strchr(raw, '.')) {
         strncpy(dst, raw, MAXHOSTLEN);
         strncat(dst, ".", MAXHOSTLEN);
         strncat(dst, default_domain, MAXHOSTLEN);
      } else
         strncpy(dst, raw, MAXHOSTLEN);
   } else {                     /* hardcore: honor FQDN, don't use default_domain */
      strncpy(dst, raw, MAXHOSTLEN);
   }
   return;
}
