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
#include <malloc.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "def.h"
#include "rw_args.h"
#include "sgermon.h"
#include "sge_string.h"
#include "sge_log.h"
#include "sge_stat.h" 
#include "msg_qmaster.h"

static int write_args_2_file(int, char **, char *);
static int read_args_from_file(int *, char ***, char *);

/*------------------------------------------------------------------------
 * prepare_arglist
 * Assemble commandline arguments
 * Rules:
 * if "file" exists, read args file
 * if "-noread-argfile" appears in commandline, don't read "file"
 * if arguments are in cmdline append them to cmdline
 * argv[0] is always stripped from new arguments
 * this routine always expects the original command line, since it strips 
 * argv[0]
 *------------------------------------------------------------------------*/
int prepare_arglist(
int *myargc,
char ***myargv,
int argc,
char **argv,
char *file 
) {
 char **cpp, **fcpp, **newargv, **ptr;
 int newargc, readfile, writefile, cmdargc, isInCmd, len;

 DENTER(TOP_LAYER, "prepare_arglist");
 
 /* Important: need a solution for admin_user before activating this code */
 readfile = FALSE;
 writefile = FALSE;
 
 /* Write commandline if !-nowrite-argfile */
 for (cpp = &argv[1]; cpp && *cpp; cpp++)
    if (!strcmp("-nowrite-argfile", *cpp))
       writefile = FALSE;
 
 if(writefile) {
   if(write_args_2_file(argc, argv, file)==0) /* success */
      readfile = FALSE;
 }

 /* First read commandline */ 
 for (cpp = &argv[1]; cpp && *cpp; cpp++)
    if (!strcmp("-noread-argfile", *cpp))
       readfile = FALSE;

 newargc = 0;
 newargv = NULL;
 if (readfile == TRUE)
    read_args_from_file(&newargc, &newargv, file);

 /* Now count cmdline arguments which are different to arguments in file
  * Set arguments in cmdline found in file to 0
  */
 cmdargc = 0;
 for (cpp = &argv[1]; cpp && *cpp; cpp++) {
    /* Check if cmdline arg is also in file. If so, skip argument */
    isInCmd = FALSE;
    for (fcpp = newargv; fcpp && *fcpp; fcpp++) {
       if (!strcmp(*cpp, *fcpp)) {
          (*cpp)[0] = '\0';
          isInCmd = TRUE;
          break;
       }
    }
    if (!isInCmd)
       cmdargc++;    
 }

 if (newargc > 0) {
    newargv = (char **) realloc(newargv, sizeof(char *) * (cmdargc + newargc + 1));
    
    /* Skip zero length strings */
    cmdargc = 0;
    ptr = &newargv[newargc];
    for (cpp = &argv[1]; cpp && *cpp; cpp++) {
       if ((len = strlen(*cpp))) {
          *ptr = malloc(len + 1);
          strcpy(*ptr, *cpp);
          ptr++;
          cmdargc++;
       }
    }
    *ptr = NULL;

    *myargc = newargc + cmdargc;
    *myargv = newargv;
 }
 else {
    *myargc = argc - 1;
    *myargv = &argv[1];
 }   

 DEXIT;
 return 0;
}


/*------------------------------------------------------------------------
 * write_args_2_file
 *------------------------------------------------------------------------*/
static int write_args_2_file(
int argc,
char **argv,
char *fname 
) {
   int fd, flags;
   int i;
   char** cpp;

   /* create the file ONLY if it does not exist */
   flags = O_WRONLY | O_CREAT | O_EXCL;

   /* except when -truncate-argfile, then truncate the file */
   for (cpp = &argv[1]; cpp && *cpp; cpp++)
      if (!strcmp("-truncate-argfile", *cpp)) {
         flags ^= O_EXCL;
         flags |= O_TRUNC;
      }

   fd = open(fname, flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
   if (fd == -1)
      return -1;

   for (i = 1; i < argc; i++)
      if(strcmp(argv[i], "-help") &&
         strcmp(argv[i], "-noread-argfile") &&
         strcmp(argv[i], "-nowrite-argfile") &&
         strcmp(argv[i], "-truncate-argfile")) {
            write(fd, argv[i], strlen(argv[i]));
            write(fd, " ", 1);
      }

   close(fd);
   return 0;
}

/*------------------------------------------------------------------------
 * read_args_from_file
 * return 
 *------------------------------------------------------------------------*/
static int read_args_from_file(
int *argc,
char ***argv,
char *file 
) {
   int i, len, fd;
   char **ptr, *buf;
   SGE_STRUCT_STAT sb;
   const char *cp;
   size_t size;
   
   DENTER(TOP_LAYER, "read_args_from_file");
   
   if (SGE_STAT(file, &sb) || S_ISREG(sb.st_mode) == 0 || sb.st_size == 0) {
      DEXIT;
      return -2;
   }
   
   if ((fd = open(file, O_RDONLY)) == -1) {
      ERROR((SGE_EVENT, MSG_RWARGS_CANTOPENCOMMANDLINEARGUMENTFILEX_S, file));
      return -2;
   }   

   size = sb.st_size + 1;
   if(((SGE_OFF_T)size != sb.st_size + 1)
      || !(buf = malloc(size))) {
      ERROR((SGE_EVENT, MSG_RWARGS_CANTALLOCATEMEMORYFORREADINGARGSFILEX_S, file));
      return -2;
   }


   if (read(fd, buf, sb.st_size) != sb.st_size) {
      ERROR((SGE_EVENT, MSG_RWARGS_CANTREADCOMMANDLINEARGUMENTFILEX_S, file));
      return -2;
   }   
   
   buf[sb.st_size] = '\0';

   close(fd);
          
   /* Count arguments */
   i = 0;
   for (cp = sge_strtok(buf, NULL); cp; cp = sge_strtok(NULL, NULL))
      i++;

   DPRINTF(("Items in file %s: %d\n", file, i));

   *argc = i;
   *argv = (char **) malloc(sizeof(char *) * (i + 1));

   ptr = *argv;
   for (cp = sge_strtok(buf, NULL); cp; cp = sge_strtok(NULL, NULL)) {
      len = strlen(cp);
      *ptr = malloc(len + 1);
      strcpy(*ptr, cp);
      DPRINTF(("Item: %s\n", cp));
      ptr++;
   }
   *ptr = NULL;
   
   free(buf);

   DEXIT;       
   return 0;
}
