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
#include "sge_log.h"
#include "def.h"
#include "msg_utilib.h"

#define IS_DELIMITOR(c,delimitor) \
   (delimitor?(strchr(delimitor, c)?1:0):isspace(c))

/****** uti/string/sge_basename() *********************************************
*  NAME
*     sge_basename() -- get basename for path
*
*  SYNOPSIS
*     char* sge_basename(const char *name, int delim) 
*
*  FUNCTION
*     Determines the basename for a path like string - the last field 
*     of a string where fields are separated by a fixed one character 
*     delimiter.
*
*  INPUTS
*     const char *name - contains the input string (path)
*     int delim        - delimiter
*
*  RESULT
*     char* - pointer to base of name after the last delimter
*             NULL if "name" is NULL or zero length string or 
*             delimiter is the last character in "name"
*
*  EXAMPLE
*     sge_basename("/usr/local/bin/flex", '/'); returns "flex"
******************************************************************************/
const char *sge_basename(const char *name, int delim) 
{
   char *cp;

   DENTER(BASIS_LAYER, "sge_basename");

   if (!name) {
      return NULL;
   }
   if (name[0] == '\0') {
      return NULL;
   }

   cp = strrchr(name, delim);
   if (!cp) {
      DEXIT;
      return name; 
   } else {
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

/****** uti/string/sge_dirname() **********************************************
*  NAME
*     sge_dirname() -- Return first part of string up to deliminator 
*
*  SYNOPSIS
*     char* sge_dirname(const char *name, int delim) 
*
*  FUNCTION
*     The function will return a malloced string containing the first 
*     part of 'name' up to, but not including deliminator. NULL will 
*     be returned if 'name' is NULL or a zero length string or if 
*     'delim' is the first character in 'name' 
*
*  INPUTS
*     const char *name - string 
*     int delim        - deliminator 
*
*  RESULT
*     char* - malloced string
*
*  NOTES
*     This routine is called "dirname" in opposite to "basename"
*     but is mostly used to strip off the domainname of a FQDN     
******************************************************************************/
char *sge_dirname(const char *name, int delim) 
{
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
   } else {
      if ((cp2 = malloc((cp - name) + 1)) == NULL) {
         DEXIT;
         return 0;
      } else {
         strncpy(cp2, name, cp - name);
         cp2[cp - name] = '\0';
         DEXIT;
         return cp2;
      }
   }
}

/****** uti/string/sge_strtok() ***********************************************
*  NAME
*     sge_strtok() -- Replacement for strtok() 
*
*  SYNOPSIS
*     char* sge_strtok(const char *str, const char *delimitor) 
*
*  FUNCTION
*     Replacement for strtok(). If no delimitor is given 
*     isspace() is used.
*
*  INPUTS
*     const char *str       - string which should be tokenized 
*     const char *delimitor - delimitor string 
*
*  RESULT
*     char* - first/next token of str.
*
*  SEE ALSO
*     uti/string/sge_strtok_r()     
******************************************************************************/
char *sge_strtok(const char *str, const char *delimitor) 
{
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
      } else {
         static_str = malloc(n + 1);
         alloc_len = n;
      }
      strcpy(static_str, str);
      saved_cp = static_str;
   } else {
      saved_cp = static_cp;
   }

   /* seek first character which is no '\0' and no delimitor */
   while (1) {

      /* found end of string */
      if (*saved_cp == '\0') {
         DEXIT;
         return NULL;
      }

      /* eat white spaces */
      if (!IS_DELIMITOR((int) saved_cp[0], delimitor)) {
         break;
      }

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

/****** uti/string/sge_strtok_r() *********************************************
*  NAME
*     sge_strtok_r() -- Reentrant version of strtok()
*
*  SYNOPSIS
*     char* sge_strtok_r(const char *str, const char *delimitor, 
*                        struct saved_vars_s **context) 
*
*  FUNCTION
*     Reentrant version of sge_strtok. When 'str' is not NULL, 
*     '*context'has to be NULL. If 'str' is NULL, '*context' 
*     must not be NULL. The caller is responsible for freeing 
*     '*context' with sge_free_saved_vars(). 
*     If no delimitor is given isspace() is used. 
*
*  INPUTS
*     const char *str                - str which should be tokenized 
*     const char *delimitor          - delimitor string 
*     struct saved_vars_s **constext - context
*
*  RESULT
*     char* - first/next token
*
*  SEE ALSO
*     uti/string/sge_strtok()     
*     uti/string/sge_free_saved_vars()
******************************************************************************/
char *sge_strtok_r(const char *str, const char *delimitor, 
                   struct saved_vars_s **context) 
{
   char *cp;
   char *saved_cp;
   struct saved_vars_s *saved;

   DENTER(BASIS_LAYER, "sge_strtok_r");

   if (str) {
      if (*context) {
         ERROR((SGE_EVENT, MSG_POINTER_INVALIDSTRTOKCALL ));
         DEXIT;
         abort();
      }
      *context = (struct saved_vars_s *)malloc(sizeof(struct saved_vars_s));
      memset(*context, 0, sizeof(struct saved_vars_s));
      saved = *context;

      saved->static_str = malloc(strlen(str) + 1);

      strcpy(saved->static_str, str);
      saved_cp = saved->static_str;
   } else {
      if (*context == NULL) {
         ERROR((SGE_EVENT, MSG_POINTER_INVALIDSTRTOKCALL));
         DEXIT;
         abort();
      }
      saved = *context;
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
      if (!IS_DELIMITOR((int) saved_cp[0], delimitor)) {
         break;
      }

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

/****** uti/string/sge_free_saved_vars() **************************************
*  NAME
*     sge_free_saved_vars() -- Free 'context' of sge_strtok_r() 
*
*  SYNOPSIS
*     void sge_free_saved_vars(struct saved_vars_s *context) 
*
*  FUNCTION
*     Free 'context' of sge_strtok_r() 
*
*  INPUTS
*     struct saved_vars_s *context 
*
*  SEE ALSO
*     uti/string/sge_strtok_r() 
******************************************************************************/
void sge_free_saved_vars(struct saved_vars_s *context) 
{
   if (context->static_str) {
      free(context->static_str);
   }
   free(context);
}

/****** uti/string/sge_strdup() ***********************************************
*  NAME
*     sge_strdup() -- Replacement for strdup() 
*
*  SYNOPSIS
*     char* sge_strdup(char *old, const char *s) 
*
*  FUNCTION
*     Duplicate string 's'. "Use" 'old' buffer.
*
*  INPUTS
*     char *old     - buffer (will be freed) 
*     const char *s - string 
*
*  RESULT
*     char* - malloced string
******************************************************************************/
char *sge_strdup(char *old, const char *s) 
{
   int n;

   /* free and NULL the old pointer */
   FREE(old);
   old = NULL;

   if (!s) {
      return NULL;
   }

   n = strlen(s);
   if (n) {
      old = malloc(n + 1);
      if (old) {
         strcpy(old, s);
      }
   }

   return old;
}

/****** uti/string/sge_strip_blanks() *****************************************
*  NAME
*     sge_strip_blanks() -- Strip blanks from string 
*
*  SYNOPSIS
*     void sge_strip_blanks(char *str) 
*
*  FUNCTION
*     Strip all blanks contained in a string. The string is used
*     both as source and drain for the necessary copy operations.
*     The string is '\0' terminated afterwards. 
*
*  INPUTS
*     char *str - pointer to string to be condensed 
******************************************************************************/
void sge_strip_blanks(char *str) 
{
   char *cp = str;

   DENTER(BASIS_LAYER, "sge_strip_blanks");

   if (!str) {
      DEXIT;
      return;
   }

   while (*str) {
      if (*str != ' ') {
         *cp++ = *str;
      }
      str++;
   };
   *cp = '\0';

   DEXIT;
   return;
}

/****** uti/string/sge_delim_str() *******************************************
*  NAME
*     sge_delim_str() -- Trunc. a str according to a delimiter set 
*
*  SYNOPSIS
*     char* sge_delim_str(char *str, char **delim_pos, 
*                         const char *delim) 
*
*  FUNCTION
*     Truncates a string according to a delimiter set. A copy of 
*     the string truncated according to the delimiter set will be 
*     returned.
*
*     ATTENTION: The user is responsible for freeing the allocated
*     memory outside this routine. If not enough space could be 
*     allocated, NULL is returned.
*
*  INPUTS
*     char *str         - string to be truncated 
*     char **delim_pos  - A placeholder for the delimiting position
*                         in str on exit. 
*                         If set on entry the position of the 
*                         delimiter in the input string 'str' is
*                         returned. If no delimiting character in
*                         string was found, the address of the
*                         closing '\0' in 'str' is returned.
*     const char *delim - string containing delimiter characters 
*
*  RESULT
*     char* - Truncated copy of 'str' (Has to be freed by the caller!)
******************************************************************************/
char *sge_delim_str(char *str, char **delim_pos, const char *delim) 
{
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

/****** uti/string/sge_strnullcmp() *******************************************
*  NAME
*     sge_strnullcmp() -- Like strcmp() but honours NULL ptrs.
*
*  SYNOPSIS
*     int sge_strnullcmp(const char *a, const char *b) 
*
*  FUNCTION
*     Like strcmp() apart from the handling of NULL strings.
*     These are treated as being less than any not-NULL strings.
*     Important for sorting lists where NULL strings can occur.  
*
*  INPUTS
*     const char *a - 1st string 
*     const char *b - 2nd string 
*
*  RESULT
*     int - result
*         0 - strings are the same or both NULL 
*        -1 - a < b or a is NULL
*         1 - a > b or b is NULL
******************************************************************************/
int sge_strnullcmp(const char *a, const char *b) 
{
   if (!a && b) {
      return -1;
   }
   if (a && !b) {
      return 1;
   }
   if (!a && !b) {
      return 0;
   }
   return strcmp(a, b);
}

/****** uti/string/sge_strnullcasecmp() ***************************************
*  NAME
*     sge_strnullcasecmp() -- Like strcasecmp() but honours NULL ptrs.
*
*  SYNOPSIS
*     int sge_strnullcasecmp(const char *a, const char *b) 
*
*  FUNCTION
*     Like strcasecmp() apart from the handling of NULL strings.
*     These are treated as being less than any not-NULL strings.
*     Important for sorting lists where NULL strings can occur.  
*
*  INPUTS
*     const char *a - 1st string 
*     const char *b - 2nd string 
*
*  RESULT
*     int - result
*         0 - strings are the same minus case or both NULL 
*        -1 - a < b or a is NULL
*         1 - a > b or b is NULL
******************************************************************************/
int sge_strnullcasecmp(const char *a, const char *b) 
{
   if (!a && b)
      return -1;
   if (a && !b)
      return 1;
   if (!a && !b)
      return 0;
   return SGE_STRCASECMP(a, b);
}

/****** uti/string/sge_is_pattern() *******************************************
*  NAME
*     sge_is_pattern() -- Test if string contains  wildcard pattern 
*
*  SYNOPSIS
*     int sge_is_pattern(const char *s) 
*
*  FUNCTION
*     Check whether string 's' contains a wildcard pattern. 
*
*  INPUTS
*     const char *s - string 
*
*  RESULT
*     int - result
*         0 - no wildcard pattern
*         1 - it is a wildcard pattern  
******************************************************************************/
int sge_is_pattern(const char *s) 
{
   char c;
   while ((c = *s++)) {
      if (strchr("*?[]", c)) {
         return 1;
      }
   }
   return 0;
}

/****** uti/string/sge_strisint() *********************************************
*  NAME
*     sge_strisint() -- Is string a integer value in characters?
*
*  SYNOPSIS
*     int sge_strisint(const char *str) 
*
*  FUNCTION
*     May we convert 'str' to int? 
*
*  INPUTS
*     const char *str - string 
*
*  RESULT
*     int - result
*         0 - It is no integer
*         1 - It is a integer
******************************************************************************/
int sge_strisint(const char *str) 
{
   const char *cp = str;
 
   while (*cp) {
      if (!isdigit((int) *cp++)) {
         return 0;
      }
   }
   return 1;
}            

/****** uti/string/sge_strtoupper() *******************************************
*  NAME
*     sge_strtoupper() -- Convert the first n to upper case 
*
*  SYNOPSIS
*     void sge_strtoupper(char *buffer, int max_len) 
*
*  FUNCTION
*     Convert the first 'max_len' characters to upper case. 
*
*  INPUTS
*     char *buffer - string 
*     int max_len  - number of chars 
******************************************************************************/
void sge_strtoupper(char *buffer, int max_len) 
{
   DENTER(BASIS_LAYER, "sge_strtoupper");

   if (buffer != NULL) {
      int i;

      for (i = 0; (i < max_len) && (i < strlen(buffer)); i++) {
         buffer[i] = toupper(buffer[i]); 
      }
   }
   DEXIT;
} 

/****** uti/string/sge_stradup() **********************************************
*  NAME
*     sge_stradup() -- Duplicate array of strings
*
*  SYNOPSIS
*     char** sge_stradup(char **cpp, int n) 
*
*  FUNCTION
*     Copy list of character pointers including the strings these
*     pointers refer to. If 'n' is 0 strings are '\0'-delimited, if 
*     'n' is not 0 we use n as length of the strings. 
*
*  INPUTS
*     char **cpp - pointer to array of strings 
*     int n      - '\0' terminated? 
*
*  RESULT
*     char** - copy of 'cpp'
******************************************************************************/
char **sge_stradup(char **cpp, int n)
{
   int count = 0, len;
   char **cpp1, **cpp2, **cpp3;
 
   /* first count entries */
   cpp2 = cpp;
   while (*cpp2) {
      cpp2++;
      count++;
   }
 
   /* alloc space */
   cpp1 = (char **) malloc((count + 1) * sizeof(char **));
   if (!cpp1)
      return NULL;
 
   /* copy  */
   cpp2 = cpp;
   cpp3 = cpp1;
   while (*cpp2) {
      /* alloc space */
      if (n)
         len = n;
      else
         len = strlen(*cpp2) + 1;
 
      *cpp3 = (char *) malloc(len);
      if (!(*cpp3)) {
         while ((--cpp3) >= cpp1)
            free(*cpp3);
         free(cpp1);
         return NULL;
      }
 
      /* copy string */
      memcpy(*cpp3, *cpp2, len);
      cpp3++;
      cpp2++;
   }
 
   *cpp3 = NULL;
 
   return cpp1;
}  

/****** uti/string/sge_strafree() *********************************************
*  NAME
*     sge_strafree() -- Free list of character pointers 
*
*  SYNOPSIS
*     void sge_strafree(char **cpp) 
*
*  FUNCTION
*     Free list of character pointers 
*
*  INPUTS
*     char **cpp - Array of string pointers 
******************************************************************************/
void sge_strafree(char **cpp)
{
   char **cpp1 = cpp;
   if (!cpp) {
      return;
   }
 
   while (*cpp1) {
      free(*cpp1++);
   }
   free(cpp);
}                          

/****** uti/string/sge_stramemncpy() ******************************************
*  NAME
*     sge_stramemncpy() -- Find string in string array 
*
*  SYNOPSIS
*     char** sge_stramemncpy(const char *cp, char **cpp, int n) 
*
*  FUNCTION
*     Compare string with string field and return the pointer to
*     the matched character pointer. Compare exactly n chars 
*     case insensitive.
*
*  INPUTS
*     const char *cp - string to be found 
*     char **cpp     - pointer to array of strings 
*     int n          - number of chars 
*
*  RESULT
*     char** - NULL or pointer a string
******************************************************************************/
char **sge_stramemncpy(const char *cp, char **cpp, int n)
{
   while (*cpp) {
      if (!memcmp(*cpp, cp, n)) {
         return cpp;
      }
      cpp++;
   }
   return NULL;
}     

/****** uti/string/sge_stracasecmp() ******************************************
*  NAME
*     sge_stracasecmp() -- Find string in string array 
*
*  SYNOPSIS
*     char** sge_stracasecmp(const char *cp, char **cpp) 
*
*  FUNCTION
*     Compare string with string field and return the pointer to
*     the matched character pointer. Compare case sensitive. 
*
*  INPUTS
*     const char *cp - string 
*     char **cpp     - pointer to array of strings  
*
*  RESULT
*     char** - NULL or pointer a string
******************************************************************************/
char **sge_stracasecmp(const char *cp, char **cpp)
{
   while (*cpp) {
      if (!strcasecmp(*cpp, cp))
         return cpp;
      cpp++;
   }
   return NULL;
}   

/****** uti/string/sge_compress_slashes() *************************************
*  NAME
*     sge_compress_slashes() -- compresses sequences of slashes 
*
*  SYNOPSIS
*     void sge_compress_slashes(char *str) 
*
*  FUNCTION
*     Compresses sequences of slashes in str to one slash 
*
*  INPUTS
*     char *str - string (e.g. path) 
*******************************************************************************/
void sge_compress_slashes(char *str)
{
   char *p;
   int compressed = 0;
   DENTER(BASIS_LAYER, "sge_compress_slashes");

   for (p = str; *p; p++) {
      while (*p == '/' && *(p+1) == '/') {
         compressed = 1;
         *p = '\0';
         p++;
      }
      if (compressed) {
         strcat(str, p);
         compressed = 0;
      }
   }
   DEXIT;
}

/****** uti/string/sge_strip_quotes() *****************************************
*  NAME
*     sge_strip_quotes() -- Strip quotes from string
*
*  SYNOPSIS
*     void sge_strip_quotes(char **pstr) 
*
*  FUNCTION
*     Strip quotes from "pstr". 
*
*  INPUTS
*     char **pstr - string to be modified 
******************************************************************************/
void sge_strip_quotes(char **pstr) 
{
   char *cp, *cp2;

   DENTER(TOP_LAYER, "sge_strip_quotes");
   if (!pstr) {
      DEXIT;
      return;
   }
   for (; *pstr; pstr++) {
      for (cp2 = cp = *pstr; *cp; cp++) {
         if (*cp == '"') {
            *cp2++ = *cp;
         }
      }
   }
   DEXIT;
   return;
}

