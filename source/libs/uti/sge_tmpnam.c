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
 *   Copyright: 2003 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/

#include "sge_tmpnam.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "basis_types.h"
#include "sge_dstring.h"
#include "sge_string.h"
#include "sgermon.h"
#include "sge_unistd.h"
#include "msg_utilib.h"

static int elect_path(dstring *aBuffer);
static int spawn_file(dstring *aBuffer, dstring *error_message);


/****** uti/sge_tmpnam/sge_tmpnam() *******************************************
*  NAME
*     sge_tmpnam() -- Secure replacement for tmpnam() 
*
*  SYNOPSIS
*     char* sge_tmpnam(char *aBuffer) 
*
*  FUNCTION
*     Generate a string that is a unique valid filename within a given
*     directory. The corresponding file is created as soon as the filename
*     has been generated, thus avoiding any delay between filename generation
*     and actual file usage. The file will have read and write access for the
*     user only. 
*
*     The 'aBuffer' argument points to an array of at least SGE_PATH_MAX length.
*     'aBuffer' will contain the generated filename upon successful completion.
*     In addition, 'aBuffer' will be returned. If the function fails, NULL will
*     be returned and 'errno' set to indicate the error.
*
*     If the environment variable TMPDIR is defined, it's value will be used
*     as the path prefix for the file. If TMPDIR is not set or it does not
*     refer to a valid directory, the value of P_tmpdir will be used.
*     P_tmpdir shall be defined in <stdio.h>. If P_tmpdir is not defined or
*     it does not refer to a valid directory, /tmp will be used.
*
*     NOTE: Since the file already exists, the O_EXCL flag must not be used if
*     the returned filename is opened for usage within an application. It is,
*     however, the duty of the application calling this function to delete the
*     file denoted by the generated filename after it is no longer needed.
*
*  INPUTS
*     char *aBuffer - Array to hold filename
*
*  RESULT
*     char* - Points to 'aBuffer' if successful, NULL otherwise
*
*  NOTE
*     MT-NOTE: sge_tmpnam() is MT safe.
******************************************************************************/
char *sge_tmpnam(char *aBuffer, dstring *error_message)
{
   dstring s = DSTRING_INIT;

   DENTER(TOP_LAYER, "sge_tmpnam");

   if (aBuffer == NULL) {
      sge_dstring_sprintf(error_message, MSG_TMPNAM_GOT_NULL_PARAMETER);
      DEXIT;
      return NULL;
   }

   if (elect_path(&s) < 0) {
      sge_dstring_sprintf(error_message, MSG_TMPNAM_CANNOT_GET_TMP_PATH);
      sge_dstring_free(&s);
      DEXIT;
      return NULL;
   }

   if ((sge_dstring_get_string(&s))[sge_dstring_strlen(&s)-1] != '/') {
      sge_dstring_append_char(&s, '/'); 
   }

   if (spawn_file(&s, error_message) < 0) {
      sge_dstring_free(&s);
      DEXIT;
      return NULL;
   }

   sge_strlcpy(aBuffer, sge_dstring_get_string(&s), SGE_PATH_MAX);
   sge_dstring_free(&s);

   DPRINTF(("sge_tmpnam: returning %s\n", aBuffer));
   DEXIT;
   return aBuffer;
}


static int elect_path(dstring *aBuffer)
{
   const char *d;

   d = getenv("TMPDIR");
   if ((d != NULL) && sge_is_directory(d)) {
      sge_dstring_append(aBuffer, d);
      return 0;
   } else if (sge_is_directory(P_tmpdir)) {
      sge_dstring_append(aBuffer, P_tmpdir);
      return 0;
   } else if (sge_is_directory("/tmp")) {
      sge_dstring_append(aBuffer, "/tmp/");
      return 0;
   }
   return -1;
}


static int spawn_file(dstring *aBuffer, dstring *error_message) {
   int my_errno;
   char* mktemp_return = NULL;
   char tmp_file_string[256];
   char tmp_string[SGE_PATH_MAX];

   /*
    * generate template filename for mktemp()
    */   
   snprintf(tmp_file_string, 256, "pid-%u-XXXXXX", (unsigned int)getpid()); 

   /*
    * check final length of path
    */
   if (sge_dstring_strlen(aBuffer) + strlen(tmp_file_string) >= SGE_PATH_MAX) {
      sge_dstring_append(aBuffer, tmp_file_string);
      sge_dstring_sprintf(error_message, MSG_TMPNAM_SGE_MAX_PATH_LENGTH_US,
                          sge_u32c(SGE_PATH_MAX), sge_dstring_get_string(aBuffer));
      return -1;
   }

   /*
    * now build full path string for mktemp()
    */
   snprintf(tmp_string, SGE_PATH_MAX, "%s%s", sge_dstring_get_string(aBuffer), tmp_file_string);

   /*
    * generate temp file by call to mktemp()
    */
   errno = 0;
   mktemp_return = mktemp(tmp_string);
   my_errno = errno;
   if (mktemp_return[0] == '\0') {
      sge_dstring_sprintf(error_message, MSG_TMPNAM_GOT_SYSTEM_ERROR_SS, 
                                strerror(my_errno), sge_dstring_get_string(aBuffer));
      return -1;
   }

   /*
    * finally copy the resulting path to aBuffer
    */
   sge_dstring_sprintf(aBuffer, tmp_string);
   return 0;
}
