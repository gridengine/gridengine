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
#include <string.h>
#include <stdlib.h>
#include <stdlib.h>
#include <pwd.h>
#include <errno.h>
#include <sys/types.h>
#include <errno.h>
       
#include "sgermon.h"
#include "sge_gdi_intern.h"
#include "sge_answerL.h"
#include "sge_paL.h"
#include "sge_getpwnam.h"
#include "path_aliases.h"
#include "sge_log.h"
#include "resolve_host.h"
#include "setup_path.h"
#include "msg_common.h"
#include "msg_utilib.h"
#include "sge_stat.h" 
#include "msg_daemons_common.h"
#include "sge_string.h" 


static void compress_slashes(char *str);
static int read_path_aliases(char *file_name, lList **path_alias_list, lList **alpp);

/*-------------------------------------------------------------------------*/
/* compresses sequences of slashes in str to one slash                     */
/*-------------------------------------------------------------------------*/
static void compress_slashes(
char *str 
) {
   char *p;
   int compressed = 0;

   DENTER(BASIS_LAYER, "compress_slashes");

   DPRINTF(("---> start compress_slashes: %s\n", str));

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

   DPRINTF(("---> end compress_slashes: %s\n", str));

   DEXIT;
}


/*-------------------------------------------------------------------------*/
/* read aliases from path_aliases file                                     */ 
/*-------------------------------------------------------------------------*/
static int read_path_aliases(
char *file_name,
lList **path_alias_list,
lList **alpp 
) {
   FILE *fd;
   char buf[10000];
   char err[BUFSIZ];
   char origin[SGE_PATH_MAX];
   char submit_host[SGE_PATH_MAX];
   char exec_host[SGE_PATH_MAX];
   char translation[SGE_PATH_MAX];
   lListElem *pal;
   SGE_STRUCT_STAT sb;
   int ret = 0;

   DENTER(TOP_LAYER, "read_path_aliases");

   if (!path_alias_list || !file_name) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return -1;
   }

   if (SGE_STAT(file_name, &sb) && errno == ENOENT) {
      DEXIT;
      return 0;
   }    

   if (!(fd=(fopen(file_name, "r")))) {
      DEXIT;
      return -1;
   }

   while (fgets(buf, sizeof(buf), fd)) {

      /* strip \n */
      *(strchr(buf, (int) '\n')) = '\0';

      DPRINTF(("Path Alias: >%s<\n",buf));

      /* skip empty lines and comments */
      if (!strlen(buf) || (*buf == '#' ))
         continue;

      /*
      ** reset
      */
      strcpy(origin, "");
      strcpy(submit_host, "");
      strcpy(exec_host, "");
      strcpy(translation, "");

      sscanf(buf, "%s %s %s %s", origin, submit_host, exec_host, translation);

      /*
      ** check for correctness of path alias file
      */
      if (*origin == '\0' || *submit_host == '\0' || *exec_host == '\0' ||
            *translation == '\0') {
         sprintf(err, MSG_ALIAS_INVALIDSYNTAXOFPATHALIASFILEX_S, file_name);
         sge_add_answer(alpp, err, STATUS_ESYNTAX, 0);
         ret = -1;
         break;
      }

      /*
      ** compress multiple slashes to one slash
      */
      compress_slashes(origin);
      compress_slashes(translation);
            
      
      pal = lAddElemStr(path_alias_list, PA_origin, origin, PA_Type);
      
      if (!pal) {
         sge_add_answer(alpp, MSG_SGETEXT_NOMEM, STATUS_EMALLOC, 0);
         ret = -1;
         break;
      }

      /*
      ** set the values of the element
      */
      lSetString(pal, PA_submit_host, submit_host);
      if (strcmp(submit_host, "*") && sge_resolve_host(pal, PA_submit_host)) {
         sprintf(SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_S, submit_host);
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         ret = -1;
         break;
      }
      lSetString(pal, PA_exec_host, exec_host);
      lSetString(pal, PA_translation, translation);

   } /* while (fgets) */

   fclose(fd);

   DEXIT;
   return ret;
}


/*-------------------------------------------------------------------------*/
/* build entire path aliases list                                          */
/*-------------------------------------------------------------------------*/
int build_path_aliases(
lList **path_alias_list,
char *user,
char *host,
lList **alpp 
) {
   char filename[2][SGE_PATH_MAX];
   char err[BUFSIZ];
   lCondition *where = NULL;
   struct passwd *pwd;
   int i;
   
   DENTER(TOP_LAYER, "build_path_aliases");

   /* read the different sge_path_alias files */
   sprintf(filename[0], "%s/%s", path.cell_root, SGE_COMMON_PATH_ALIASES_FILE);

   /* the path_alias file in the user's home directory */
   pwd = sge_getpwnam(user);
   if (!pwd) {
      sprintf(err, MSG_USER_INVALIDNAMEX_S, user);
      sge_add_answer(alpp, err, STATUS_ENOSUCHUSER, 0);
      DEXIT;
      return -1;
   }
   if (!pwd->pw_dir) {
      sprintf(err, MSG_USER_NOHOMEDIRFORUSERX_S, user);
      sge_add_answer(alpp, err, STATUS_EDISK, 0);
      DEXIT;
      return -1;
   }
   
   sprintf(filename[1], "%s/%s", pwd->pw_dir, SGE_HOME_PATH_ALIASES_FILE);

   for (i=0; i<2; i++) {
      sprintf(err, "can't read path aliasing file \"%s\": %s\n", filename[i
], strerror(errno));
      if (read_path_aliases(filename[i], path_alias_list, alpp) != 0) {
         sprintf(err, MSG_ALIAS_CANTREAD_SS, filename[i], strerror(errno));
         sge_add_answer(alpp, err, STATUS_EDISK, 0);
         DEXIT;
         return -1;
      }
   }

   /*
   ** remove the unnecessary hosts from the list
   */
   where = lWhere("%T(%I == %s || %I == %s)", PA_Type, PA_submit_host, "*",
                     PA_submit_host, host);
   *path_alias_list = lSelectDestroy(*path_alias_list, where);
   where = lFreeWhere(where);

   DEXIT;
   return 0;
}



/*-------------------------------------------------------------------------*/
int get_path_alias(
const char *inpath,                /* in/out path */
char *outpath,
int outmax,
lList *path_aliases,
const char *myhost,
lList **alpp 
) {
   lListElem *pap;
   const char *origin;
   const char *translation;
   const char *exec_host;
   char the_path[SGE_PATH_MAX];
 
   DENTER(TOP_LAYER, "get_path_alias");
 
   strncpy(outpath, inpath, outmax);
   strncpy(the_path, outpath, SGE_PATH_MAX); 

   DPRINTF(("############ ORIGINAL PATH >>%s<<\n", outpath));
 
   if (path_aliases) { /* use aliases */
      for_each(pap, path_aliases) {
         origin = lGetString(pap, PA_origin);
         exec_host = lGetString(pap, PA_exec_host);
         translation = lGetString(pap, PA_translation);

         if (strncmp(origin, the_path, strlen(origin))) {
            /* path leaders aren't the same ==> no match */
            continue;
         }
 
         /* the paths are ok, what about the exec hosts ? */
         /* if exec_host is a '*' we have a match */
         if (*exec_host != '*') {
            /* no '*', so we have to look closer   */
            /* resolv the exec host from the alias */
            if (sge_resolve_host(pap, PA_exec_host)) {
               ERROR((SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_S, exec_host)); 
               continue;
            }
            exec_host = lGetString(pap, PA_exec_host);

            /* and compare it to the executing host */
            if (hostcmp(exec_host, myhost))
               continue;

         }
 
         /* copy the alias as leading part of cwd */
         strcpy(outpath, translation);
 
         /* now append the trailer of the original cwd */
         strcat(outpath, the_path + strlen(origin));
 
         DPRINTF(("############ PATH ALIASED TO >>%s<<\n", outpath));
 
         /* and we have to start all over again for subsequent aliases */
         strncpy(the_path, outpath, SGE_PATH_MAX);
      }
   } 
 

   DEXIT;
   return 0;

}
