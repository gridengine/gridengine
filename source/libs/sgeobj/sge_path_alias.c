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
#include <pwd.h>
#include <errno.h>
#include <sys/types.h>
       
#include "sgermon.h"
#include "sge_gdi_intern.h"
#include "sge_path_alias.h"
#include "sge_log.h"
#include "sge_host.h"
#include "setup_path.h"
#include "sge_string.h" 
#include "sge_uidgid.h"
#include "sge_unistd.h"
#include "sge_hostname.h"
#include "sge_answer.h"

#include "msg_common.h"
#include "msg_daemons_common.h"

/****** sgeobj/path_alias/-PathAlias *******************************************
*  NAME
*     PathAlias - Path aliasing mechanism for SGE/EE
*
*  FUNCTION
*     Sometimes the directory structure on the different submit,
*     execution hosts is not exactly the same. This makes it 
*     necessary to map certain paths for specific hosts.
*
*     The administrators/users have the possibility to
*     activate path aliasing by creating one or more of following
*     files:
*
*        $SGE_ROOT/$CELL/common/sge_aliases
*        $HOME/.sge_aliases 
*
*     The file format is documentied in the ADOC commet for the 
*     function path_alias_read_from_file().
*
*     The files are interpreted as follows:
*        - after a submit client (qsub, qmon, ...) has retrieved 
*          the current working directory, the cluster global
*          path aliasing file is read, if present. The user
*          path aliasing file is read afterwards, as if it were 
*          appended to the global file.
*        - as soon as both files are read, the path aliasing
*          information is passed along with the submitted job.
*        - On the execution host the aliasing information will be
*          evaluated. The leading part of the current working 
*          directory will be replaced if the execution host entry of
*          the path alias matches the executing host.
*
*  SEE ALSO
*     sgeobj/path_alias/path_alias_read_from_file()
*     sgeobj/path_alias/path_alias_list_initialize()
*     sgeobj/path_alias/path_alias_list_get_path()
******************************************************************************/

static int path_alias_read_from_file(lList **path_alias_list, lList **alpp, 
                                     char *file_name);

/****** sgeobj/path_alias/path_alias_read_from_file() *************************
*  NAME
*     path_alias_read_from_file() -- read file content to list
*
*  SYNOPSIS
*     #include <sgeobj/sge_path_alias.h>
*  
*     static int path_alias_read_from_file(lList **path_alias_list, 
*                                          lList **alpp, 
*                                          char *file_name) 
*
*  FUNCTION
*     Read and parse the file with the name "file_name" and append
*     entries into "path_alias_list". Errors will be logged in "alpp". 
*
*     File format:
*     - Blank lines and lines beginning with a # sign in the first
*       column are skipped.
*     - Each line - other than a blank line or a line preceded by # - 
*       must contain four strings separated by any number of blanks 
*       or tabs.
*     - The first string specifies a source path, the second a submit
*       host, the third an execution host, and the fourth the source 
*       path replacement.
*     - Both the submit and the execution host entries may consist
*       of only a * sign, which matches any host.
*
*  INPUTS
*     lList **path_alias_list - PA_Type list pointer
*     lList **alpp            - AN_Type list pointer 
*     char *file_name         - name of an alias file 
*
*  RESULT
*     static int - error state
*        -1 - Error
*         0 - OK 
******************************************************************************/
static int path_alias_read_from_file(lList **path_alias_list, lList **alpp,
                                     char *file_name)
{
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

   DENTER(TOP_LAYER, "path_alias_read_from_file");

   if (!path_alias_list || !file_name) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
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
      char *crp;

      /* strip \n */
      if ((crp = strchr(buf, (int)'\n')))
         *crp = '\0';

      DPRINTF(("Path Alias: >%s<\n",buf));

      /* skip empty lines and comments */
      if (!strlen(buf) || (*buf == '#' )) {
         continue;
      }

      /*
       * reset
       */
      strcpy(origin, "");
      strcpy(submit_host, "");
      strcpy(exec_host, "");
      strcpy(translation, "");

      sscanf(buf, "%s %s %s %s", origin, submit_host, exec_host, translation);

      /*
       * check for correctness of path alias file
       */
      if (*origin == '\0' || *submit_host == '\0' || *exec_host == '\0' ||
            *translation == '\0') {
         sprintf(err, MSG_ALIAS_INVALIDSYNTAXOFPATHALIASFILEX_S, file_name);
         answer_list_add(alpp, err, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         ret = -1;
         break;
      }

      /*
       * compress multiple slashes to one slash
       */
      sge_compress_slashes(origin);
      sge_compress_slashes(translation);
            
      
      pal = lAddElemStr(path_alias_list, PA_origin, origin, PA_Type);
      
      if (!pal) {
         answer_list_add(alpp, MSG_SGETEXT_NOMEM, STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
         ret = -1;
         break;
      }

      /*
       * set the values of the element
       */
      lSetHost(pal, PA_submit_host, submit_host);
      if (strcmp(submit_host, "*") && sge_resolve_host(pal, PA_submit_host)) {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_S, submit_host));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         ret = -1;
         break;
      }
      lSetHost(pal, PA_exec_host, exec_host);
      lSetString(pal, PA_translation, translation);

   } /* while (fgets) */

   fclose(fd);

   DEXIT;
   return ret;
}

/****** sgeobj/path_alias/path_alias_list_initialize() ************************
*  NAME
*     path_alias_list_initialize() -- initialize path_alias_list 
*
*  SYNOPSIS
*     int path_alias_list_initialize(lList **path_alias_list, 
*                                    lList **alpp, 
*                                    const char *user, 
*                                    const char *host) 
*
*  FUNCTION
*     Intitialize "path_alias_list" according to the different
*     path aliasing files. 
*
*     Following files will be used if available:
*
*        $SGE_ROOT/$CELL/common/sge_aliases
*        $HOME/.sge_aliases 
*
*  INPUTS
*     lList **path_alias_list - PA_Type list pointer
*     lList **alpp            - AN_Type list pointer 
*     const char *user        - username
*     const char *host        - hostname 
*
*  RESULT
*     int - return state
*        -1 - error
*         0 - OK
******************************************************************************/
int path_alias_list_initialize(lList **path_alias_list, 
                               lList **alpp,
                               const char *user,
                               const char *host) 
{
   char filename[2][SGE_PATH_MAX];
   char err[BUFSIZ];
   DENTER(TOP_LAYER, "path_alias_list_initialize");

   /* 
    * find names of different sge_path_alias files:
    *    global
    *    home directory
    */
   {
      struct passwd *pwd;

      pwd = sge_getpwnam(user);
      if (!pwd) {
         sprintf(err, MSG_USER_INVALIDNAMEX_S, user);
         answer_list_add(alpp, err, STATUS_ENOSUCHUSER, ANSWER_QUALITY_ERROR);
         DEXIT;
         return -1;
      }
      if (!pwd->pw_dir) {
         sprintf(err, MSG_USER_NOHOMEDIRFORUSERX_S, user);
         answer_list_add(alpp, err, STATUS_EDISK, ANSWER_QUALITY_ERROR);
         DEXIT;
         return -1;
      }
      sprintf(filename[0], "%s/%s", path.cell_root, PATH_ALIAS_COMMON_FILE);
      sprintf(filename[1], "%s/%s", pwd->pw_dir, PATH_ALIAS_HOME_FILE);
   }

   /*
    * read files
    */
   {
      int i;

      for (i=0; i<2; i++) {
         sprintf(err, "can't read path aliasing file \"%s\": %s\n", 
                 filename[i], strerror(errno));
         if (path_alias_read_from_file(path_alias_list, 
                                       alpp, filename[i]) != 0) {
            sprintf(err, MSG_ALIAS_CANTREAD_SS, filename[i], strerror(errno));
            answer_list_add(alpp, err, STATUS_EDISK, ANSWER_QUALITY_ERROR);
            DEXIT;
            return -1;
         }
      }
   }

   /*
    * remove the unnecessary hosts from the list
    */
   {
      lCondition *where = NULL;

      where = lWhere("%T(%I == %s || %I == %s)", PA_Type, 
                     PA_submit_host, "*", PA_submit_host, host);
      *path_alias_list = lSelectDestroy(*path_alias_list, where);
      where = lFreeWhere(where);
   }

   DEXIT;
   return 0;
}

/****** sgeobj/path_alias/path_alias_list_get_path() **************************
*  NAME
*     path_alias_list_get_path() -- map path according alias table 
*
*  SYNOPSIS
*     int path_alias_list_get_path(const lList *path_aliases, 
*                                  lList **alpp, 
*                                  const char *inpath, 
*                                  const char *myhost,
*                                  char *outpath, 
*                                  int outmax)
*
*  FUNCTION
*     "path_aliases" is used to map "inpath" for the host "myhost"
*     into its alias path which will be written into the buffer 
*     "outpath" of size "outmax". 
*
*  INPUTS
*     const lList *path_aliases - alias table (PA_Type) 
*     lList **alpp              - AN_Type list pointer 
*     const char *inpath        - input path 
*     const char *myhost        - hostname 
*     char *outpath             - result path 
*     int outmax                - size of "outpath" 
*
*  RESULT
*     int - return state
*        0 - OK
*******************************************************************************/
int path_alias_list_get_path(const lList *path_aliases, lList **alpp,
                             const char *inpath, const char *myhost,
                             dstring *outpath)
{
   lListElem *pap;
   const char *origin;
   const char *translation;
   const char *exec_host;
   dstring the_path = DSTRING_INIT;
 
   DENTER(TOP_LAYER, "path_alias_list_get_path");

   sge_dstring_copy_string(outpath, inpath);
   sge_dstring_copy_dstring(&the_path, outpath); 

   if (path_aliases && lGetNumberOfElem(path_aliases) > 0) { 
      for_each(pap, path_aliases) {
         size_t orign_str_len = 0; 
         origin = lGetString(pap, PA_origin);
         orign_str_len = strlen(origin);
         exec_host = lGetHost(pap, PA_exec_host);
         translation = lGetString(pap, PA_translation);

         if (strncmp(origin, sge_dstring_get_string(&the_path), 
             orign_str_len )) {
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
            exec_host = lGetHost(pap, PA_exec_host);

            /* and compare it to the executing host */
            if (sge_hostcmp(exec_host, myhost))
               continue;

         }
 
         /* copy the alias as leading part of cwd */
         sge_dstring_copy_string(outpath, translation);
 
         /* now append the trailer of the original cwd */
         {  
            const char *path = sge_dstring_get_string(&the_path);
            sge_dstring_append(outpath, path + orign_str_len );
         }

         DPRINTF(("Path "SFQ" has been aliased to "SFQ"\n", inpath, sge_dstring_get_string(outpath))); 
 
         /* and we have to start all over again for subsequent aliases */
         sge_dstring_copy_dstring(&the_path, outpath);
      }
   } else {
      DPRINTF(("\"path_aliases\" containes no elements\n"));
   }

   sge_dstring_free(&the_path);

   DEXIT;
   return 0;

}
