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
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "basis_types.h"
#include "sge_afsutil.h"
#include "sge_peopen.h"
#include "sgermon.h"
#include "sge_copy_append.h"
#include "msg_utilib.h"
#include "sge_stat.h"

                     
/*------------------------------------------------------
 * read_token
 * read token from file, malloc buffer, add '\0' byte and
 * return pointer to buffer
 *------------------------------------------------------*/
char *read_token(
const char *file 
) {
   SGE_STRUCT_STAT sb;
   int fd;
   char *tokenbuf;
   size_t size;

   DENTER(TOP_LAYER, "read_token");

   if (SGE_STAT(file, &sb)) {
      DTRACE;
      return NULL;
   }

   size = sb.st_size + 1;
   if (((SGE_OFF_T)size != sb.st_size + 1)
       || (tokenbuf = (char *) malloc(size)) == NULL) {
      DTRACE;
      return NULL;
   }

   if ((fd = open(file, O_RDONLY)) == -1) {
      DTRACE;
      return NULL;
   }

   if (read(fd, tokenbuf, sb.st_size) != sb.st_size) {
      DTRACE;
      close(fd);
      return NULL;
   }

   tokenbuf[sb.st_size] = '\0';

   close(fd);
   DEXIT;
   return tokenbuf;
}

/*-------------------------------------------------------------------
 * extend_afs_token
 * call command, pipe content of tokenbuf to command, using
 * user as arg1 and token_extend_time as arg2
 * return 0  in case of sucess, 
 *        -1 in case of failure
 *-------------------------------------------------------------------*/
int extend_afs_token(
const char *command,
char *tokenbuf,
const char *user,
int token_extend_time,
char *err_str 
) {
   pid_t command_pid;
   FILE *fp_in, *fp_out, *fp_err;
   int ret;
   char cmdbuf[SGE_PATH_MAX+128];

   DENTER(TOP_LAYER, "extend_afs_token");

   sprintf(cmdbuf, "%s %s %d", command, user, token_extend_time);
   if (err_str)
      strcpy(err_str, cmdbuf);

   command_pid = peopen("/bin/sh", 0, cmdbuf, NULL, NULL, &fp_in, &fp_out, &fp_err);
   if (command_pid == -1) {
      if (err_str)
         sprintf(err_str, MSG_TOKEN_NOSTART_S , cmdbuf);
      DEXIT;
      return -1;
   }
    
   if (sge_string2bin(fp_in, tokenbuf) == -1) {
      if (err_str)
         sprintf(err_str, MSG_TOKEN_NOWRITEAFS_S , cmdbuf);
      DEXIT;
      return -1;
   }

   if ((ret = peclose(command_pid, fp_in, fp_out, fp_err, NULL)) != 0) {
      if (err_str)
         sprintf(err_str, MSG_TOKEN_NOSETAFS_SI , cmdbuf, ret);
      DEXIT;
      return -1;
   }
    
   return 0;
}


/*-------------------------------------------------------------------
 * get_token_cmd
 * check if "tokencmdname" file exist and is executable
 * log error messages to stderr if buf = NULL, else log message in buf
 * return 0 if sucess
 *        1 in case of error
 *-------------------------------------------------------------------*/
int get_token_cmd(
const char *tokencmdname,
char *buf 
) {
    SGE_STRUCT_STAT sb;

    if (!tokencmdname || !strlen(tokencmdname)) {
       if (!buf)
          fprintf(stderr, MSG_COMMAND_NOPATHFORTOKEN);
       else   
          strcpy(buf, MSG_COMMAND_NOPATHFORTOKEN);
       return 1;
    }   
    
    if (SGE_STAT(tokencmdname, &sb) == -1) {
       if (!buf) 
          fprintf(stderr, MSG_COMMAND_NOFILESTATUS_S , tokencmdname);
       else
          sprintf(buf, MSG_COMMAND_NOFILESTATUS_S , tokencmdname);
       return 1;
    }   
    
    if (!(sb.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH))) {
       if (!buf) 
          fprintf(stderr, MSG_COMMAND_NOTEXECUTABLE_S , tokencmdname);
       else
          sprintf(buf, MSG_COMMAND_NOTEXECUTABLE_S , tokencmdname);
       return 1;
    }
    
    return 0;
}
