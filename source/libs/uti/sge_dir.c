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
 *  License at http://www.gridengine.sunsource.net/license.html
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
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include "basis_types.h"
#include "sge_dir.h"
#include "msg_utilib.h"
#include "sge_stat.h" 
#include "sge_dirent.h"

#ifdef TEST

int main(int argc, char **argv)
{
   char err_str[1024];

   if (argc!=2) {
      fprintf(stderr, "usage: rmdir <dir>\n");
      exit(1);
   }
   if (recursive_rmdir(argv[1], err_str)) {
      fprintf(stderr, "%s", err_str);
      return 1;
   }
   return 0;
}

#endif

/******************************************************/
int recursive_rmdir(
char *cp,
char *err_str 
) {
   SGE_STRUCT_STAT statbuf;
   SGE_STRUCT_DIRENT *dent;
   DIR *dir;
   char fname[SGE_PATH_MAX];

   if (!cp) {
      sprintf(err_str, MSG_POINTER_NULLPARAMETER);
      return -1;
   }

   if (!(dir = opendir(cp))) {
      sprintf(err_str, MSG_FILE_OPENDIRFAILED_SS , cp, strerror(errno));
      return -1;
   }

   while ((dent = SGE_READDIR(dir))) {
      if (strcmp(dent->d_name, ".") && strcmp(dent->d_name, "..")) {

         sprintf(fname, "%s/%s", cp, dent->d_name);

#ifndef WIN32 /* lstat not called */
         if (SGE_LSTAT(fname, &statbuf)) {
            sprintf(err_str,MSG_FILE_STATFAILED_SS , fname,
                    strerror(errno));
            closedir(dir);
            return -1;
         }
#else
         /* so symbolic links under Windows */
         if (SGE_STAT(fname, &statbuf)) {
            sprintf(err_str, MSG_FILE_STATFAILED_SS , fname,
                    strerror(errno));
            closedir(dir);
            return -1;
         }
#endif /* WIN32 */

#if defined(NECSX4) || defined(NECSX5)
	 if (S_ISDIR(statbuf.st_mode)) {
#else
	 if (S_ISDIR(statbuf.st_mode) && !S_ISLNK(statbuf.st_mode)) {
#endif
            if (recursive_rmdir(fname, err_str)) {
               fprintf(stderr, MSG_FILE_RECURSIVERMDIRFAILED );
               closedir(dir);
               return -1;
            }
         }
         else {
#ifdef TEST
            printf("unlink %s\n", fname);
#else
            if (unlink(fname)) {
               sprintf(err_str, MSG_FILE_UNLINKFAILED_SS ,
                      fname, strerror(errno));
               closedir(dir);
               return -1;
            }
#endif
         }
      }
   }

   closedir(dir);

#ifdef TEST
   printf("rmdir %s\n", cp);
#else
   if (rmdir(cp)) {
      sprintf(err_str, MSG_FILE_RMDIRFAILED_SS , cp, strerror(errno));
      return -1;
   }
#endif

   return 0;
}
