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
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "sgermon.h"
#include "sge_log.h"
#include "sge_get_confval.h"
#include "msg_utilib.h"

/*--------------------------------------------------------------------
 * Name:    get_confval
 * Descr:   get config value for entry "conf_val" from file "fname"
 * Return:  NULL if file cannot be read or entry not found
 *          Pointer to a static buffer containing the value
 *
 *          "conf_val" is case insensitive.
 *          This function uses strtok().
 *          Lines may be up to 1024 characters long.
 *          Up to 1024 characters of the config value are copied to the 
 *          static buffer.
 *--------------------------------------------------------------------*/
char *get_confval(
char *conf_val,
char *fname    
) {
   FILE *fp;
   char buf[1024], *cp;
   static char valbuf[1025];
   
   DENTER(TOP_LAYER, "get_confval");

   if (!(fp = fopen(fname, "r"))) {
      ERROR((SGE_EVENT, MSG_FILE_FOPENFAILED_SS, fname, strerror(errno))); 
      DEXIT;
      return NULL;
   }
   
   while (fgets(buf, sizeof(buf), fp)) {
      /* set chrptr to the first non blank character
       * If line is empty continue with next line   
       */
       if(!(cp = strtok(buf, " \t\n")))
          continue;

       /* allow commentaries */
       if (cp[0] == '#')
          continue;
 
       if (strcasecmp(conf_val, cp))
          continue;
       else {
          fclose(fp);
          if ((cp = strtok(NULL, " \t\n"))) {
              strncpy(valbuf, cp, 512);
              cp = valbuf;
          }
          DEXIT;
          return cp;
       }         
   }

   fclose(fp);
   DEXIT;
   return 0;
}

/*--------------------------------------------------------------------
 * Name:    readpid
 * Descr:   read pid from file
 * Return:  0 if pidfile is not readable or contains no pid
 *          pid if file is readable and first non empty line
 *          can be converted to a number.
 *
 *          The pidfile may be terminated with a '\n'
 *          Empty lines at the beginning of the file are ignored
 *          Whitespaces at the beginning of the line are ignored
 *          Any characters or lines after a valid pid are ignored
 *--------------------------------------------------------------------*/
pid_t readpid(
char *fname 
) {
   FILE *fp;
   char buf[512], *cp;
   pid_t pid;
   
   DENTER(TOP_LAYER, "readpid");

   if (!(fp = fopen(fname, "r"))) {
      DEXIT;
      return 0; 
   }

   pid = 0;
   while (fgets(buf, sizeof(buf), fp)) {
      /* set chrptr to the first non blank character
       * If line is empty continue with next line   
       */
       if(!(cp = strtok(buf, " \t\n")))
          continue;

       /* Check for negative numbers */
       if (!isdigit((int) *cp))
          pid = 0;
       else
          pid = atoi(cp);
       break;
   }

   fclose(fp);

   DEXIT;
   return pid;
}
