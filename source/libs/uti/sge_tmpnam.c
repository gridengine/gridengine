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
#include "sgermon.h"
#include "sge_unistd.h"


static int elect_path(dstring *aBuffer);
static int spawn_file(dstring *aBuffer);


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
char *sge_tmpnam(char *aBuffer)
{
   dstring s = DSTRING_INIT;

   DENTER(BASIS_LAYER, "sge_tmpnam");

   if (aBuffer == NULL) {
      errno = EINVAL;
      DEXIT;
      return NULL;
   }

   if (elect_path(&s) < 0) {
      errno = ENOENT;
      sge_dstring_free(&s);
      DEXIT;
      return NULL;
   }

   if ((sge_dstring_get_string(&s))[sge_dstring_strlen(&s)-1] != '/') {
      sge_dstring_append_char(&s, '/'); 
   }

   if (spawn_file(&s) < 0) {
      sge_dstring_free(&s);
      DEXIT;
      return NULL;
   }

   if (sge_dstring_strlen(&s) > (SGE_PATH_MAX - 1)) {
      sge_dstring_free(&s);
      errno = ENAMETOOLONG;
      DEXIT;
      return NULL;
   }

   strncpy(aBuffer, sge_dstring_get_string(&s), SGE_PATH_MAX);
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


/*
 * NOTE: If the size of SUFFIX_LEN is increased on 32-bit architectures, the
 *       generated filename will contain the same characters after the current
 *       SUFFIX_LEN position (pid-123456XX - XX will be the same for a suffix 
 *       length of 8).
 *
 *       The reason for this is, that the value of 'v' in the inner loop will
 *       become 0 after the sixth iteration and reamin constant on a 32-bit
 *       architectures. Thus, the same element of POOL_SIZE will be referenced
 *       for the following iterations.
 */
static int spawn_file(dstring *aBuffer)
{
   static const char pool[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
   static const int POOL_SIZE = 62;
   static const int DELTA = 7575;
   static const int SUFFIX_LEN = 6;
   static unsigned long val; /* no need to synchronize */
   struct timeval tv;
   int i, j, fd;
   unsigned long rand;
   dstring s = DSTRING_INIT;
   unsigned int trails = POOL_SIZE * POOL_SIZE * POOL_SIZE * POOL_SIZE;  /* try POOL_SIZE**4 times */

   gettimeofday(&tv, NULL);
   rand = ((unsigned long)tv.tv_usec << 16) ^ tv.tv_sec;
   val += rand ^ getpid();

   sge_dstring_sprintf(&s, "%u-", (unsigned int)getpid());

   for (i = 0; i < trails; val += DELTA, i++) {
      unsigned long v = val;

      for (j = 0; j < SUFFIX_LEN; v /= POOL_SIZE, j++) {
         sge_dstring_append_char(&s, pool[v % POOL_SIZE]);
      }

      sge_dstring_append_dstring(aBuffer, &s);
      fd = open(sge_dstring_get_string(aBuffer), O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
      if (fd >= 0) {
         close(fd);
         break;
      }
      sge_dstring_clear(&s); /* no luck, need another name */
   }
   
   sge_dstring_free(&s);
   return (fd >= 0) ? 0 : -1 ;
}

