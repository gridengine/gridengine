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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h> 

#if defined(LINUX)
#  include <limits.h>
#endif  

#include "sge_unistd.h"
#include "sgermon.h"
#include "sge_log.h"
#include "basis_types.h"
#include "msg_utilib.h"

typedef enum {
   FILE_TYPE_NOT_EXISTING,
   FILE_TYPE_FILE,
   FILE_TYPE_DIRECTORY
} file_type_t; 

static int sge_domkdir(const char *, int, int);

static file_type_t sge_get_file_type(const char *name);  

static sge_exit_func_t exit_func = NULL;

static file_type_t sge_get_file_type(const char *name)
{
   SGE_STRUCT_STAT stat_buffer;
   int ret = FILE_TYPE_NOT_EXISTING;
 
   if (SGE_STAT(name, &stat_buffer)) {
      ret = FILE_TYPE_NOT_EXISTING;
   } else {
      if (S_ISDIR(stat_buffer.st_mode)) {
         ret = FILE_TYPE_DIRECTORY;
      } else if (S_ISREG(stat_buffer.st_mode)) {
         ret = FILE_TYPE_FILE;
      } else {
         ret = FILE_TYPE_NOT_EXISTING;
      }
   }
   return ret;
}             

static int sge_domkdir(const char *path_, int fmode, int exit_on_error) 
{
   SGE_STRUCT_STAT statbuf;
 
   DENTER(BASIS_LAYER, "sge_domkdir");
 
   if (mkdir(path_, (mode_t) fmode)) {
      if (errno == EEXIST) {
         DPRINTF(("directory \"%s\" already exists\n", path_));
         DEXIT;
         return 0;
      }
 
      if (!SGE_STAT(path_, &statbuf) && S_ISDIR(statbuf.st_mode)) {
         /*
          * may be we do not have permission, 
          * but directory already exists 
          */
         DEXIT;
         return 0;
      }
 
      if (exit_on_error) {
         CRITICAL((SGE_EVENT, MSG_FILE_CREATEDIRFAILED_SS, path_, 
                   strerror(errno)));
         SGE_EXIT(1);
      } else {
         ERROR((SGE_EVENT, MSG_FILE_CREATEDIRFAILED_SS, path_, 
                strerror(errno)));
         DEXIT;
         return -1;
      }
   }
 
   DEXIT;
   return 0;
}         

/****** uti/unistd/sge_unlink() ***********************************************
*  NAME
*     sge_unlink() -- delete a name and possibly the file it refers to
*
*  SYNOPSIS
*     int sge_unlink(const char *prefix, const char *suffix) 
*
*  FUNCTION
*     Replacement for unlink(). 'prefix' and 'suffix' will be combined
*     to a filename. This file will be deleted. 'prefix' may be NULL.
*
*  INPUTS
*     const char *prefix - pathname or NULL
*     const char *suffix - filename 
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - Error
******************************************************************************/
int sge_unlink(const char *prefix, const char *suffix) 
{
   int status;
   stringT str;
 
   DENTER(TOP_LAYER, "sge_unlink");
 
   if (!suffix) {
      ERROR((SGE_EVENT, MSG_POINTER_SUFFIXISNULLINSGEUNLINK ));
      DEXIT;
      return -1;
   }
 
   if (prefix) {
      sprintf(str, "%s/%s", prefix, suffix);
   } else {
      sprintf(str, "%s", suffix);
   }
 
   DPRINTF(("file to unlink: \"%s\"\n", str));
   status = unlink(str);
 
   if (status) {
      ERROR((SGE_EVENT, "ERROR: unlinking "SFQ": "SFN"\n", str, strerror(errno)));
      DEXIT;
      return -1;
   } else {
      DEXIT;
      return 0;
   }
}  

/****** uti/unistd/sge_sleep() ************************************************
*  NAME
*     sge_sleep() -- sleep for x microseconds 
*
*  SYNOPSIS
*     void sge_sleep(int sec, int usec) 
*
*  FUNCTION
*     Delays the calling application for 'sec' seconds and 'usec'
*     microseconds 
*
*  INPUTS
*     int sec  - seconds 
*     int usec - microseconds 
******************************************************************************/
void sge_sleep(int sec, int usec) 
{
   static struct timeval timeout;
 
   timeout.tv_sec = sec;
   timeout.tv_usec = usec;
 
#if !(defined(HPUX) || defined(HP10_01) || defined(HPCONVEX))
   select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &timeout);
#else
   select(0, (int *) 0, (int *) 0, (int *) 0, &timeout);
#endif
}       

/****** uti/unistd/sge_chdir_exit() *******************************************
*  NAME
*     sge_chdir_exit() -- Replacement for chdir() 
*
*  SYNOPSIS
*     int sge_chdir_exit(const char *path, int exit_on_error) 
*
*  FUNCTION
*     Change working directory 
*
*  INPUTS
*     const char *path  - pathname 
*     int exit_on_error - exit in case of errors 
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - ERROR ('exit_on_error'==1 the function may not return)
*
*  SEE ALSO
*     uti/unistd/sge_chdir()
******************************************************************************/
int sge_chdir_exit(const char *path, int exit_on_error) 
{
   DENTER(BASIS_LAYER, "sge_chdir");
 
   if (chdir(path)) {
      if (exit_on_error) {
         CRITICAL((SGE_EVENT, MSG_FILE_NOCDTODIRECTORY_S , path));
         SGE_EXIT(1);
      } else {
         ERROR((SGE_EVENT, MSG_FILE_NOCDTODIRECTORY_S , path));
         return -1;
      }
   }
 
   DEXIT;
   return 0;
}           

/****** sge/unistd/sge_chdir() ************************************************
*  NAME
*     sge_chdir() --  Replacement for chdir()
*
*  SYNOPSIS
*     int sge_chdir(const char *dir) 
*
*  FUNCTION
*     Change working directory 
*
*  INPUTS
*     const char *dir - pathname 
*
*  RESULT
*     int - error state
*        0 - success
*        != 0 - error 
*
*  NOTE
*     Might be used in shepherd because it does not use CRITICAL/ERROR.
*
*  SEE ALSO
*     uti/unistd/sge_chdir_exit()
******************************************************************************/
int sge_chdir(const char *dir)
{
   SGE_STRUCT_STAT statbuf;

   /*
    * force automount
    */
   SGE_STAT(dir, &statbuf);
   return chdir(dir);
}

/****** uti/unistd/sge_exit() *************************************************
*  NAME
*     sge_exit() -- Wrapped exit Function 
*
*  SYNOPSIS
*     void sge_exit(int i) 
*
*  FUNCTION
*     Calls 'exit_func' if installed. Stops monitoring with DCLOSE 
*
*  INPUTS
*     int i - exit state 
*
*  SEE ALSO
*     uti/unistd/sge_install_exit_func()
******************************************************************************/
void sge_exit(int i) 
{
   if (exit_func) {
      exit_func(i);
   }
 
   DCLOSE;
   exit(i);
}
 
/****** uti/unistd/sge_install_exit_func() ************************************
*  NAME
*     sge_install_exit_func() -- Installs a new exit handler 
*
*  SYNOPSIS
*     sge_exit_func_t sge_install_exit_func(sge_exit_func_t new) 
*
*  FUNCTION
*     Installs a new exit handler and returns the old one. Exit function
*     will be called be sge_exit()
*
*  INPUTS
*     sge_exit_func_t new - new function pointer 
*
*  RESULT
*     sge_exit_func_t - old function pointer 
*
*  SEE ALSO
*     uti/unistd/sge_exit() 
******************************************************************************/
sge_exit_func_t sge_install_exit_func(sge_exit_func_t new) 
{
   sge_exit_func_t old;
 
   old = exit_func;
   exit_func = new;
 
   return old;
}          

/****** uti/unistd/sge_mkdir() ************************************************
*  NAME
*     sge_mkdir() -- Create a directory (and subdirectorys)  
*
*  SYNOPSIS
*     int sge_mkdir(const char *path, int fmode, int exit_on_error) 
*
*  FUNCTION
*     Create a directory 
*
*  INPUTS
*     const char *path  - path 
*     int fmode         - file mode 
*     int exit_on_error - as it says 
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - Error (The function may never return)
******************************************************************************/
int sge_mkdir(const char *path, int fmode, int exit_on_error) 
{
   int i = 0, res=0;
   stringT path_;
 
   DENTER(TOP_LAYER, "sge_mkdir");
   if (!path) {
      if (exit_on_error) {
         CRITICAL((SGE_EVENT,MSG_VAR_PATHISNULLINSGEMKDIR ));
         DCLOSE;
         SGE_EXIT(1);
      } else {
         ERROR((SGE_EVENT, MSG_VAR_PATHISNULLINSGEMKDIR ));
         DEXIT;
         return -1;
      }
   }
 
   memset(path_, 0, sizeof(path_));
   while ((unsigned char) path[i]) {
      path_[i] = path[i];
      if ((path[i] == '/') && (i != 0)) {
         path_[i] = (unsigned char) 0;
         res = sge_domkdir(path_, fmode, exit_on_error);
         if (res) {
            DEXIT;
            return res;
         }
      }
      path_[i] = path[i];
      i++;
   }
 
   i = sge_domkdir(path_, fmode, exit_on_error);
 
   DEXIT;
   return i;
}   

/****** uti/unistd/sge_rmdir() ************************************************
*  NAME
*     sge_rmdir() -- Recursive rmdir
*
*  SYNOPSIS
*     int sge_rmdir(const char *cp, char *err_str)  
*
*  FUNCTION
*     Remove a directory tree. In case of errors a message may be found
*     in 'err_str' afterwards.
*
*  INPUTS
*     const char *cp  - path 
*     char *cp        - err_str 
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - Error 
******************************************************************************/
int sge_rmdir(const char *cp, char *err_str) 
{
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
            if (sge_rmdir(fname, err_str)) {
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
 
/****** uti/unistd/sge_is_directory() *****************************************
*  NAME
*     sge_is_directory() -- Does 'name' exist and is it a directory? 
*
*  SYNOPSIS
*     int sge_is_directory(const char *name) 
*
*  FUNCTION
*     Does 'name' exist and is it a directory?  
*
*  INPUTS
*     const char *name  - directory name 
*
*  RESULT
*     int - error state
*         0 - No
*         1 - Yes 
******************************************************************************/
int sge_is_directory(const char *name)
{
   return (sge_get_file_type(name) == FILE_TYPE_DIRECTORY);
}
 
/****** uti/unistd/sge_is_file() **********************************************
*  NAME
*     sge_is_file() -- Does 'name' exist and is it a file? 
*
*  SYNOPSIS
*     int sge_is_file(const char *name) 
*
*  FUNCTION
*     Does 'name' exist and is it a file?  
*
*  INPUTS
*     const char *name  - filename 
*
*  RESULT
*     int - error state
*         0 - No
*         1 - Yes 
******************************************************************************/
int sge_is_file(const char *name)
{
   return (sge_get_file_type(name) == FILE_TYPE_FILE);
}                         

/****** uti/unistd/sge_sysconf() **********************************************
*  NAME
*     sge_sysconf() -- Replacement for sysconf 
*
*  SYNOPSIS
*     u_long32 sge_sysconf(sge_sysconf_t id)
*
*  FUNCTION
*     Replacement for sysconf  
*
*  INPUTS
*     sge_sysconf_t id - value 
*
*  RESULT
*     u_long32 - meaning depends on 'id' 
*
*  SEE ALSO
*     uti/unistd/sge_sysconf_t
******************************************************************************/
u_long32 sge_sysconf(sge_sysconf_t id) 
{
   u_long32 ret = 0;
 
   DENTER(BASIS_LAYER, "sge_sysconf");
   switch (id) {
      case SGE_SYSCONF_NGROUPS_MAX:
#if defined(AIX42)
         ret = NGROUPS;
#else
         ret = sysconf(_SC_NGROUPS_MAX);
#endif
      break;
      default:
         CRITICAL((SGE_EVENT, MSG_SYSCONF_UNABLETORETRIEVE_I, (int) id));
      break;
   }
   DEXIT;
   return ret;
}      
 
#ifdef TEST
int main(int argc, char **argv)
{
   char err_str[1024];
 
   if (argc!=2) {
      fprintf(stderr, "usage: rmdir <dir>\n");
      exit(1);
   }
   if (sge_rmdir(argv[1], err_str)) {
      fprintf(stderr, "%s", err_str);
      return 1;
   }
   return 0;
}
#endif   
