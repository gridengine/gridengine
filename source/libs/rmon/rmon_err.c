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
#define DEBUG

#include <stdarg.h>

#include "rmon_h.h"
#include "rmon_def.h"
#include "rmon_rmon.h"
#include "rmon_err.h"

/* You may not include err.h in this file !!! */
static int loggit(char *ptr, unsigned int size);

/************************************************************************/
/* use like rmon_errf( int type, char *fmt, ... )  */

int rmon_errf(int type, const char *fmt,...)
{
   va_list args;

   time_t now;
   string err_str;
   string complete_err_str, dummy_str, date;
   string tmp;
   string hostname;

#undef FUNC
#define FUNC "rmon_errf"
   DENTER;

   va_start(args, fmt);
   vsprintf(err_str, fmt, args);

   now = time((time_t *) NULL);
   sprintf(dummy_str, "%s", ctime(&now));
   bzero(date, sizeof(date));
   sscanf(dummy_str, "%[^\n]", date);
   bzero((char *) complete_err_str, sizeof(complete_err_str));

   if (type == TERMINAL)
      strcpy(tmp, "TERMINAL");

   if (type == TRIVIAL)
      strcpy(tmp, "TRIVIAL");

   gethostname(hostname, 80);

   /* sprintf(complete_err_str,"%s:%s:%s:%s:%s\n",date,me.prog_name, me.unqualified_hostname,tmp,err_str); */
   sprintf(complete_err_str, "%s:%d:%s:%s:%s\n", hostname, getpid(), date, tmp, err_str);

   DPRINTF(("%s", complete_err_str));

   loggit(complete_err_str, strlen(complete_err_str));
   printf(complete_err_str);

   if (type == TERMINAL)
      exit(0);

   va_end(args);
   DEXIT;
   return (0);
}                               /* rmon_errf() */

/************************************************************************/

static int loggit(
char *ptr,
unsigned int size 

) {
   static char err_file[] = "../err-log-file";

#undef FUNC
#define FUNC "loggit"

   int i = 0, fd;
   SGE_STRUCT_STAT buf;
   mode_t mode = 0644;

   DENTER;

   if (SGE_STAT(err_file, &buf))
      close(open(err_file, O_WRONLY | O_CREAT | O_TRUNC, mode));

   if ((fd = open(err_file, O_WRONLY | O_APPEND, mode)) >= 0) {
      i = write(fd, ptr, size);
      close(fd);
   }

   DEXIT;
   return (i);
}
