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
#include <sys/types.h>

#ifdef LINUX
    /* needed to define strptime() prototype */
#   define __USE_XOPEN
#endif

#include <time.h>

#ifdef LINUX
    /* needed to define strptime() prototype */
#   undef __USE_XOPEN
#endif

#include <fcntl.h>    
#include <errno.h>    

#include "sge_all_listsL.h"
#include "complex_history.h"
#include "sge_sched.h"
#include "sge_resource.h"
#include "read_write_queue.h"
#include "read_write_complex.h"
#include "path_history.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_stdlib.h"
#include "sge_string.h"
#include "read_write_host.h"
#include "setup_path.h"
#include "sge_language.h"
#include "sge_unistd.h"
#include "sge_hostname.h"
#include "sge_complex.h"

#include "msg_common.h"
#include "msg_history.h"

/*
** local defines
** problem: these values are only for testing
** need to be tested with big accounting files
*/
#define MAX_CACHE_COMPLEX_VERSIONS 50
#define MAX_CACHE_QUEUE_VERSIONS   30

/*
** NAME
**   init_history_subdirs
** PARAMETER
**   dirname    - name of directory to read subdirs from
**                with or without trailing /
**   pplist     - list pointer-pointer, to be set to
**                resulting subdir list, HD_Type
**                this parameter serves as ldir input to some
**                of the functions below
** RETURN
**
** EXTERNAL
**
** DESCRIPTION
**   The usual input to this function will be
**   history/queues, history/complexes and history/exechosts.
**   The function creates a list with the names of the
**   subdirectories, these will be names of queues, complexes
**   or exechosts. In these subdirectories the versions of
**   the queues/complexes/exechosts will be stored.
*/
int init_history_subdirs(
char *dirname,
lList **pplist 
) {
   DIR *dir;
   SGE_STRUCT_DIRENT *dent;
   lListElem *ep;

   DENTER(TOP_LAYER, "init_history_subdirs");

   if (!dirname || !pplist) {
      ERROR((SGE_EVENT, MSG_POINTER_DIRNAMEORPPLISTISNULLININITHISTORYSUBDIRS ));
      DEXIT;
      return -1;
   }
   *pplist  = lCreateList(dirname, HD_Type);

   dir = opendir(dirname);
   if (!dir) {
      ERROR((SGE_EVENT, MSG_FILE_CANTOPENDIRECTORYX_SS, dirname, strerror(errno)));
      DEXIT;
      return -2;
   }

   while ((dent = SGE_READDIR(dir)) != NULL) {
      if (!strcmp(dent->d_name,"..") || !strcmp(dent->d_name,"."))
         continue;

      /*
      ** problem: would like to skip plain files here but dont know how
      ** stat is too much overhead because one would have to assemble
      ** the path first
      */

/*       DPRINTF(("------------->%s\n", dent->d_name)); */
      
      ep = lCreateElem(HD_Type);
      lSetString(ep, HD_name, dent->d_name);
      lAppendElem(*pplist, ep);
   }
   closedir(dir);
   
   DEXIT;
   return 0;
}

/*
** NAME
**   init_history_files
** PARAMETER
**   ldir        -  directory (list) in which version_subdir is a subdirectory
**                  (element), needed to assemble full path, HD_Type
**   lsubdir     -  an element in lsubdir, the subdir to search
**                  for history version files, HD_Type
**   pplist      -  the list of files contained in lsubdir, HF_Type
** RETURN
**
** EXTERNAL
**
** DESCRIPTION
**
*/
int init_history_files(
lList *ldir,
lListElem *lsubdir,
lList **pplist 
) {
   DIR *dir;
   SGE_STRUCT_DIRENT *dent;
   lListElem *ep;
   char dirname[SGE_PATH_MAX + 1];
   unsigned int len;
   char *filename = dirname;
   const char *str;
   int fd;

   DENTER(TOP_LAYER, "init_history_files");

   if (!ldir || !lsubdir || !pplist) {
      ERROR((SGE_EVENT, MSG_POINTER_LDIRORLSUBDIRISNULLININITHISTORYFILES ));
      DEXIT;
      return -1;
   }

   *pplist  = lCreateList(lGetString(lsubdir, HD_name), HF_Type);
   
   *dirname = 0;
   if ((str = lGetListName(ldir))) {
      strcpy(dirname, str);
      if (*dirname && (dirname[strlen(dirname) - 1] != '/')) {
         strcat(dirname, "/");
      }
   }
   else {
      ERROR((SGE_EVENT, MSG_HISTORY_DIRECTORYLISTHASNONAME ));
      DEXIT;
      return -2;
   }
   if ((str = lGetString(lsubdir, HD_name))) {
      strcat(dirname, str);
      if (*dirname && (dirname[strlen(dirname) - 1] != '/')) {
         strcat(dirname, "/");
      }
   }
   else {
      ERROR((SGE_EVENT, MSG_HISTORY_SUBDIRHASNONAME));
      DEXIT;
      return -3;
   }
   len = strlen(dirname);
   if (!len) {
      ERROR((SGE_EVENT, MSG_HISTORY_SUBDIRHASZEROLENGTHNAME));
      DEXIT;
      return -4;
   }
   DPRINTF(("dirname: %s\n", dirname));
   
   dir = opendir(dirname);
   if (!dir) {
      ERROR((SGE_EVENT, MSG_FILE_CANTOPENDIRECTORYX_SS, dirname, strerror(errno)));
      DEXIT;
      return -5;
   }
   
   while ((dent = SGE_READDIR(dir)) != NULL) {
      if (!strcmp(dent->d_name,"..") || !strcmp(dent->d_name,".")) 
         continue;
    
      filename[len] = 0;
      strcat(filename, dent->d_name);
/*       DPRINTF(("------------->%s\n", dent->d_name)); */

      if ((fd = open(filename, O_RDONLY)) < 0) {
         ERROR((SGE_EVENT, MSG_FILE_CANTOPENFILEX_S , filename));
         DEXIT;
         return -6;
      }
      close(fd);
      
      ep = lCreateElem(HD_Type);
      lSetString(ep, HD_name, dent->d_name);
      lAppendElem(*pplist, ep);
   }
 
   lPSortList(*pplist, "%I+", HF_name);

   lSetList(lsubdir, HD_files, *pplist);
   lSetUlong(lsubdir, HD_oldest, version_filename_to_t_time(lGetString(lFirst(*pplist), HF_name)));
   lSetUlong(lsubdir, HD_latest, version_filename_to_t_time(lGetString(lLast(*pplist), HF_name)));
   closedir(dir);
   
   DEXIT;
   return 0;
}


/*
** NAME
**   find_complex_version
** PARAMETER
**   ldir             -  directory (list) in which version_subdir is a subdirectory
**                       (element), HD_Type
**   version_subdir   -  subdirectory (element) which files are searched
**   uldate           -  date to choose version file
**   ppelem           -  pointer to element returned or NULL if element was not found
** RETURN
**   0                -  no error in searching (not found is not an error)
** EXTERNAL
**
** DESCRIPTION
**   retrieves a version of a complex or exechost from the history
*/
int find_complex_version(
lList *ldir,
lListElem *version_subdir,
unsigned long uldate,
lListElem **ppelem 
) {
   lListElem *cmplx_version_file;
   lList *version_file_list, *cmplx_entries;
   lListElem *cmplx;
   char filename[SGE_PATH_MAX + 1];
   int i_ret;
   unsigned long ulfiledate;
   const char *str;
   static int files_cached = 0;
   lList *answer = NULL;

   DENTER(TOP_LAYER, "find_complex_version");

   if (!ldir || !version_subdir || !ppelem) {
      ERROR((SGE_EVENT, MSG_POINTER_LDIRORVERSIONSUBDIRORPPELEMISNULLINFINDCOMPLEXVERSION ));
      DEXIT;
      return -1;
   }

   if (uldate == (unsigned long) 0) {
      ERROR((SGE_EVENT, MSG_HISTORY_INVALIDDATEINFINDCOMPLEXVERSION ));
      DEXIT;
      return -2;
   }

   /*
   ** if the version file list is not yet loaded then load it
   ** still to think about free
   */
   version_file_list = lGetList(version_subdir, HD_files);

   if (!version_file_list) {
      i_ret = init_history_files(ldir, version_subdir, &version_file_list);
      if (i_ret) {
         ERROR((SGE_EVENT, MSG_HISTORY_CANTINITHISTORYFILESX_I , i_ret));
         DEXIT;
         return -3;
      }
   }

   /*
   ** no need to search all files if we know that the file requested
   ** is the latest or oldest one
   */

   if (uldate > lGetUlong(version_subdir, HD_latest)) {

      lListElem *last_file;

      last_file = lLast(version_file_list);
      DPRINTF(("uldate %l > latest %l\n", uldate, lGetUlong(version_subdir, HD_latest)));
      /*
      ** the element to be returned is created here
      ** must be freed somewhere else if no longer needed
      */
      cmplx_entries = lGetList(last_file, HF_entries);
      if (!cmplx_entries) {
         i_ret = make_filename(filename, SGE_PATH_MAX,
                               lGetListName(ldir),
                               lGetString(version_subdir, HD_name),
                               lGetString(last_file, HF_name));
         if (i_ret) {
            ERROR((SGE_EVENT, MSG_HISTORY_CANTASSEMBLEFILENAMEX_I , i_ret));
            DEXIT;
            return -4;
         }
         DPRINTF(("filename: %s\n", filename));
         cmplx = read_cmplx(filename, lGetString(version_subdir, HD_name), 
                                    &answer);
         if (answer) {
            ERROR((SGE_EVENT, lGetString(lFirst(answer), AN_text)));
            answer = lFreeList(answer);
            DEXIT;
            return -4;
         }
         
         lSetList(last_file,
                  HF_entries,
                  lCopyList(lGetString(last_file, HF_name), lGetList(cmplx, CX_entries)));
         files_cached++;
         if (files_cached > MAX_CACHE_COMPLEX_VERSIONS) {
            for_each(cmplx_version_file, version_file_list) {
               lList *lp;

               if (!files_cached) {
                  break;
               }
               if ((lp = lGetList(cmplx_version_file, HF_entries))) {
#if 0
                  lFreeList(lp);
#endif
                  lSetList(cmplx_version_file, HF_entries, NULL);
                  files_cached--;
	       }
	    }
	 }
      }
      else {
         cmplx = lCreateElem(CX_Type);
         /*
         ** the subdir name might end with a slash
         ** the complex name should not
         */
         lSetString(cmplx, CX_name, lGetString(version_subdir, HD_name));
         str = lGetString(cmplx, CX_name);
         if (str && *str && (str[strlen(str) - 1] == '/')) {
            ((char*)str)[strlen(str) - 1] = 0;
         }
         lSetList(cmplx,
                  CX_entries, 
                  lCopyList(lGetString(last_file, HF_name), cmplx_entries));
      }
      *ppelem = cmplx;
      DEXIT;
      return 0;
   }

   /*
   ** problem: this is wrong in fact
   */
   if (uldate < lGetUlong(version_subdir, HD_oldest)) {
      lListElem *first_file;

      first_file = lFirst(version_file_list);
      DPRINTF(("uldate %ld < oldest %ld\n", uldate, lGetUlong(version_subdir, HD_oldest)));

      cmplx_entries = lGetList(first_file, HF_entries);
      if (!cmplx_entries) {
         i_ret = make_filename(filename, SGE_PATH_MAX,
                               lGetListName(ldir),
                               lGetString(version_subdir, HD_name),
                               lGetString(first_file, HF_name));
         if (i_ret) {
            ERROR((SGE_EVENT, MSG_HISTORY_CANTASSEMBLEFILENAMEX_I , i_ret));
            DEXIT;
            return -5;
         }
         DPRINTF(("filename: %s\n", filename));
         cmplx = read_cmplx(filename, lGetString(version_subdir, HD_name), 
                              &answer);
         if (answer) {
            ERROR((SGE_EVENT, lGetString(lFirst(answer), AN_text)));
            answer = lFreeList(answer);
            DEXIT;
            return -5;
         }
         lSetList(first_file, 
                  HF_entries, 
                  lCopyList(lGetString(first_file, HF_name), lGetList(cmplx, CX_entries)));
         files_cached++;
         if (files_cached > MAX_CACHE_COMPLEX_VERSIONS) {
            for_each(cmplx_version_file, version_file_list) {
               lList *lp;

               if (!files_cached) {
                  break;
               }
               if ((lp = lGetList(cmplx_version_file, HF_entries))) {
#if 0
                  lFreeList(lp);
#endif
                  lSetList(cmplx_version_file, HF_entries, NULL);
                  files_cached--;
	       }
	    }            
	 }
      }
      else {
         cmplx = lCreateElem(CX_Type);
         /*
         ** the subdir name might end with a slash
         ** the complex name should not
         */
         lSetString(cmplx, CX_name, lGetString(version_subdir, HD_name));
         str = lGetString(cmplx, CX_name);
         if (str && *str && (str[strlen(str) - 1] == '/')) {
            ((char*)str)[strlen(str) - 1] = 0;
         }
         lSetList(cmplx, 
                  CX_entries, 
                  lCopyList(lGetString(first_file, HF_name), cmplx_entries));
      }
      *ppelem = cmplx;
      DEXIT;
      return 0;
   }

   /*
   ** look at the version file list and see which file matches
   */
   for_each(cmplx_version_file, version_file_list) {
      
      ulfiledate = version_filename_to_t_time(lGetString(cmplx_version_file, HF_name));
      if (ulfiledate > uldate) { 
         cmplx_version_file = lPrev(cmplx_version_file);
         cmplx_entries = lGetList(cmplx_version_file, HF_entries); 
         if (!cmplx_entries) {
            i_ret = make_filename(filename, SGE_PATH_MAX,
                                  lGetListName(ldir),
                                  lGetString(version_subdir, HD_name),
                                  lGetString(cmplx_version_file, HF_name));
            if (i_ret) {
               ERROR((SGE_EVENT, MSG_HISTORY_CANTASSEMBLEFILENAMEX_I , i_ret));
               DEXIT;
               return -6;
            }
            DPRINTF(("filename: %s\n", filename));
            cmplx = read_cmplx(filename, lGetString(version_subdir, HD_name),
                                 &answer);
            if (answer) {
               ERROR((SGE_EVENT, lGetString(lFirst(answer), AN_text)));
               answer = lFreeList(answer);
               DEXIT;
               return -6;
            }

            lSetList(cmplx_version_file, 
                     HF_entries, 
                     lCopyList(lGetString(cmplx_version_file, HF_name), 
                     lGetList(cmplx, CX_entries)));
            files_cached++;
            if (files_cached > MAX_CACHE_COMPLEX_VERSIONS) {
               for_each(cmplx_version_file, version_file_list) {
                  lList *lp;

                  if (!files_cached) {
                     break;
                  }
                  if ((lp = lGetList(cmplx_version_file, HF_entries))) {
#if 0
                     lFreeList(lp);
#endif
                     lSetList(cmplx_version_file, HF_entries, NULL);
                     files_cached--;
	               }
	            }            
	         }
	      } else {
            cmplx = lCreateElem(CX_Type);
            /*
            ** the subdir name might end with a slash
            ** the complex name should not
            */
            lSetString(cmplx, CX_name, lGetString(version_subdir, HD_name));
            str = lGetString(cmplx, CX_name);
            if (str && *str && (str[strlen(str) - 1] == '/')) {
               ((char*)str)[strlen(str) - 1] = 0;
            }
            lSetList(cmplx, 
                     CX_entries, 
                     lCopyList(lGetString(cmplx_version_file, HF_name), cmplx_entries));

	      }
         *ppelem = cmplx;
         DEXIT;
         return 0;
      }
   }
      
   DPRINTF((" no version file found...\n"));
   DEXIT;
   *ppelem = NULL;
   return 0;
}


/*
** NAME
**   find_host_version
** PARAMETER
**   ldir             -  directory (list) in which version_subdir is a subdirectory
**                       (element), HD_Type
**   version_subdir   -  subdirectory (element) which files are searched
**   uldate           -  date to choose version file
**   ppelem           -  pointer to element returned or NULL if element was not found
** RETURN
**   0                -  no error in searching (not found is not an error)
** EXTERNAL
**
** DESCRIPTION
**   retrieves a version of a  exechost from the history
*/
int find_host_version(
lList *ldir,
lListElem *version_subdir,
unsigned long uldate,
lListElem **ppelem 
) {
   lList *version_file_list; 
   int i_ret;
   lListElem *host_version_file;
   unsigned long ulfiledate;
   char filename[SGE_PATH_MAX + 1];
   static int files_cached = 0;
   lList *host_entries = NULL;
   lListElem *host = NULL;
   lList *answer = NULL;
   int tag;
   lList *new_host_list;
/*
   char *str;
*/

   DENTER(TOP_LAYER, "find_host_version");

   if (!ldir || !version_subdir || !ppelem) {
      ERROR((SGE_EVENT, MSG_POINTER_LDIRORVERSIONSUBDIRORPPELEMISNULLINFINDCOMPLEXVERSION));
      DEXIT;
      return -1;
   }

   if (uldate == (unsigned long) 0) {
      ERROR((SGE_EVENT, MSG_HISTORY_INVALIDDATEINFINDHOSTVERSION ));
      DEXIT;
      return -2;
   }

   /*
   ** if the version file list is not yet loaded then load it
   ** still to think about free
   */
   version_file_list = lGetList(version_subdir, HD_files);
   if (!version_file_list) {
      i_ret = init_history_files(ldir, version_subdir, &version_file_list);
      if (i_ret) {
         ERROR((SGE_EVENT, MSG_HISTORY_CANTINITHISTORYFILESX_I , i_ret));
         DEXIT;
         return -3;
      }
   }

   /*
   ** no need to search all files if we know that the file requested
   ** is the latest or oldest one
   */

   if (uldate > lGetUlong(version_subdir, HD_latest)) {
      lListElem *last_file;

      last_file = lLast(version_file_list);
      DPRINTF(("uldate %l > latest %l\n", uldate, lGetUlong(version_subdir, HD_latest)));
      /*
      ** the element to be returned is created here
      ** must be freed somewhere else if no longer needed
      */
      host_entries = lGetList(last_file, HF_entries);
      if (!host_entries) {
         lList *new_host_list;

         i_ret = make_filename(filename, SGE_PATH_MAX,
                               lGetListName(ldir),
                               lGetString(version_subdir, HD_name),
                               lGetString(last_file, HF_name));
         if (i_ret) {
            ERROR((SGE_EVENT, MSG_HISTORY_CANTASSEMBLEFILENAMEX_I , i_ret));
            DEXIT;
            return -4;
         }
         DPRINTF(("filename: %s\n", filename));

         host = cull_read_in_host(filename, NULL, CULL_READ_HISTORY, EH_name, 
            &tag, NULL);

         if (!host) {
            ERROR((SGE_EVENT, MSG_HISTORY_CULLREADINHOSTRETURNEDNOHOST));
            answer = lFreeList(answer);
            DEXIT;
            return -4;
         }
         
         new_host_list = lCreateList(lGetString(last_file, HF_name), EH_Type);
         lAppendElem(new_host_list, lCopyElem(host));
         lSetList(last_file, HF_entries, new_host_list);

         files_cached++;
         if (files_cached > MAX_CACHE_COMPLEX_VERSIONS) {
            for_each(host_version_file, version_file_list) {
               lList *lp;

               if (!files_cached) {
                  break;
               }
               if ((lp = lGetList(host_version_file, HF_entries))) {
#if 0
                  lFreeList(lp);
#endif
                  lSetList(host_version_file, HF_entries, NULL);
                  files_cached--;
	            }
	         }
	      }
      } else {
         host = lCopyElem(lFirst(host_entries));
      }

      *ppelem = host;
      DEXIT;
      return 0;
   }


   /*
   ** problem: this is wrong in fact
   */
   if (uldate < lGetUlong(version_subdir, HD_oldest)) {
      lListElem *first_file;

      first_file = lFirst(version_file_list);
      DPRINTF(("uldate %ld < oldest %ld\n", uldate, lGetUlong(version_subdir, HD_oldest)));

      host_entries = lGetList(first_file, HF_entries);
      if (!host_entries) {
         i_ret = make_filename(filename, SGE_PATH_MAX,
                               lGetListName(ldir),
                               lGetString(version_subdir, HD_name),
                               lGetString(first_file, HF_name));
         if (i_ret) {
            ERROR((SGE_EVENT, MSG_HISTORY_CANTASSEMBLEFILENAMEX_I, i_ret));
            DEXIT;
            return -5;
         }
         DPRINTF(("filename: %s\n", filename));

         host = cull_read_in_host(filename, NULL, CULL_READ_HISTORY, EH_name, &tag, NULL);

         if (!host) {
            ERROR((SGE_EVENT, MSG_HISTORY_CULLREADINHOSTRETURNEDNOHOST));
            answer = lFreeList(answer);
            DEXIT;
            return -5;
         }

         new_host_list = lCreateList(lGetString(first_file, HF_name), EH_Type);
         lAppendElem(new_host_list, lCopyElem(host));
         lSetList(first_file, HF_entries, new_host_list);

         files_cached++;
         if (files_cached > MAX_CACHE_COMPLEX_VERSIONS) {
            for_each(host_version_file, version_file_list) {
               lList *lp;

               if (!files_cached) {
                  break;
               }
               if ((lp = lGetList(host_version_file, HF_entries))) {
#if 0
                  lFreeList(lp);
#endif
                  lSetList(host_version_file, HF_entries, NULL);
                  files_cached--;
	            }
	         }            
	      }
      } else {
         host = lCopyElem(lFirst(host_entries));
      }
      *ppelem = host;
      DEXIT;
      return 0;
   }

   /*
   ** look at the version file list and see which file matches
   */
   for_each(host_version_file, version_file_list) {
      
      ulfiledate = version_filename_to_t_time(lGetString(host_version_file, HF_name));
      if (ulfiledate > uldate)
      { 
         host_version_file = lPrev(host_version_file);
         host_entries = lGetList(host_version_file, HF_entries); 
         if (!host_entries) {
            i_ret = make_filename(filename, SGE_PATH_MAX,
                                  lGetListName(ldir),
                                  lGetString(version_subdir, HD_name),
                                  lGetString(host_version_file, HF_name));
            if (i_ret) {
               ERROR((SGE_EVENT,MSG_HISTORY_CANTASSEMBLEFILENAMEX_I , i_ret));
               DEXIT;
               return -6;
            }
            DPRINTF(("filename: %s\n", filename));
            DPRINTF(("version_subdir: %s\n", lGetString(version_subdir, 
               HD_name)));

            host = cull_read_in_host(filename, NULL, CULL_READ_HISTORY, EH_name, &tag, NULL);
            if (!host) {
               ERROR((SGE_EVENT, MSG_HISTORY_CULLREADINHOSTRETURNEDNOHOST ));
               answer = lFreeList(answer);
               DEXIT;
               return -6;
            }

            new_host_list = lCreateList(lGetString(host_version_file, HF_name), EH_Type);
            lAppendElem(new_host_list, lCopyElem(host));
            lSetList(host_version_file, HF_entries, new_host_list);

            files_cached++;
            if (files_cached > MAX_CACHE_COMPLEX_VERSIONS) {
               for_each(host_version_file, version_file_list) {
                  lList *lp;

                  if (!files_cached) {
                     break;
                  }
                  if ((lp = lGetList(host_version_file, HF_entries))) {
#if 0
                     lFreeList(lp);
#endif
                     lSetList(host_version_file, HF_entries, NULL);
                     files_cached--;
	               }
	            }            
	         }
         } else {
            host = lCopyElem(lFirst(host_entries));
         }

         *ppelem = host;

         DEXIT;
         return 0;
      }
   }
      
   DPRINTF((" no version file found...\n"));
   DEXIT;
   *ppelem = NULL;
   return 0;
}

/*
** NAME
**   find_queue_version
** PARAMETER
**   ldir        -  directory (list) in which version_subdir is a subdirectory
**                  (element), HD_Type
**   version_subdir    -
**   uldate            -
**   ppelem            -
** RETURN
**   0                -  no error in searching (not found is not an error)
** EXTERNAL
**
** DESCRIPTION
**
*/
int find_queue_version(
lList *ldir,
lListElem *version_subdir,
unsigned long uldate,
lListElem **ppelem 
) {
   lListElem *queue_version_file;
   lList *version_file_list, *queue_dummy_list;
   lListElem *queue;
   char filename[SGE_PATH_MAX + 1];
   int i_ret;
   unsigned long ulfiledate;
   static int files_cached = 0;

   DENTER(TOP_LAYER, "find_queue_version");

   if (!ldir || !version_subdir || !ppelem) {
      ERROR((SGE_EVENT, MSG_POINTER_LDIRORVERSIONSUBDIRORPPELEMISNULLINFINDQUEUEVERSION ));
      DEXIT;
      return -1;
   }

   if (uldate == (unsigned long) 0) {
      ERROR((SGE_EVENT, MSG_HISTORY_INVALIDDATINFINDQUEUEVERSION ));
      DEXIT;
      return -2;
   }

   /*
   ** if the version file list is not yet loaded then load it
   ** still to think about free
   */
   version_file_list = lGetList(version_subdir, HD_files);
   if (!version_file_list) {
      i_ret = init_history_files(ldir, version_subdir, &version_file_list);
      if (i_ret) {
         ERROR((SGE_EVENT, MSG_HISTORY_CANTINITHISTORYFILESX_I , i_ret));
         DEXIT;
         return -3;
      }
   }

   /*
   ** no need to search all files if we know that the file requested
   ** is the latest or oldest one
   */
   if (uldate > lGetUlong(version_subdir, HD_latest)) {
      lListElem *last_file = NULL;

      last_file = lLast(version_file_list);
      DPRINTF(("uldate %ld > latest %ld\n", uldate, lGetUlong(version_subdir, HD_latest)));

      queue_dummy_list = lGetList(last_file, HF_entries);
      if (!queue_dummy_list) {
         i_ret = make_filename(filename, SGE_PATH_MAX,
                               lGetListName(ldir), 
                               lGetString(version_subdir, HD_name),
                               lGetString(last_file, HF_name));
         if (i_ret) {
            ERROR((SGE_EVENT, MSG_HISTORY_CANTASSEMBLEFILENAMEX_I , i_ret));
            DEXIT;
            return -4;
         }
         DPRINTF(("filename: %s\n", filename));
         queue = cull_read_in_qconf(NULL, filename, 0, 0, NULL, NULL); 
         if (!queue) {
            ERROR((SGE_EVENT, MSG_HISTORY_CANTFINDQUEUEVERSION ));
            DEXIT;
            return -5;
         }
         queue_dummy_list = lCreateList(lGetString(last_file, HF_name), QU_Type);
         lAppendElem(queue_dummy_list, lCopyElem(queue));

         lSetList(last_file, 
                  HF_entries, 
                  queue_dummy_list);
         files_cached++;
         if (files_cached > MAX_CACHE_QUEUE_VERSIONS) {
            for_each(queue_version_file, version_file_list) {
               if (!files_cached) {
                  break;
               }
               if (lGetList(queue_version_file, HF_entries)) {
#if 0
                  lFreeList(queue_dummy_list);
#endif

                  lSetList(queue_version_file, HF_entries, NULL);
                  files_cached--;
               }
            }            
         }
      }
      else {
         queue = lCopyElem(lFirst(queue_dummy_list));
      }
      *ppelem = queue;
      DEXIT;
      return 0;
   }

   if (uldate < lGetUlong(version_subdir, HD_oldest)) {
      lListElem *first_file = NULL;

      first_file = lFirst(version_file_list);

      DPRINTF(("uldate %l < oldest %l\n", uldate, lGetUlong(version_subdir, HD_oldest)));

      queue_dummy_list = lGetList(first_file, HF_entries);
      if (!queue_dummy_list) {
         i_ret = make_filename(filename, SGE_PATH_MAX,
                               lGetListName(ldir),
                               lGetString(version_subdir, HD_name),
                               lGetString(first_file, HF_name));
         if (i_ret) {
            ERROR((SGE_EVENT, MSG_HISTORY_CANTASSEMBLEFILENAMEX_I , i_ret));
            DEXIT;
            return -6;
         }
         DPRINTF(("filename: %s\n", filename));
         queue = cull_read_in_qconf(NULL, filename, 0, 0, NULL, NULL);
         if (!queue) {
            ERROR((SGE_EVENT, MSG_HISTORY_FINDQUEUEVERSIONERROR ));
            DPRINTF(("cull_read_in_qconf returns NULL\n"));
            DEXIT;
            return -7;
         }
         queue_dummy_list = lCreateList(lGetString(first_file, HF_name), QU_Type);
         lAppendElem(queue_dummy_list, lCopyElem(queue));

         lSetList(first_file, 
                  HF_entries, 
                  queue_dummy_list);
         files_cached++;
         if (files_cached > MAX_CACHE_QUEUE_VERSIONS) {
            for_each(queue_version_file, version_file_list) {
               lList *lp;

               if (!files_cached) {
                  break;
               }
               if ((lp = lGetList(queue_version_file, HF_entries))) {
#if 0
                  lFreeList(lp);
#endif
                                    
                  lSetList(queue_version_file, HF_entries, NULL);
                  files_cached--;
               }
            }            
         }
      }
      else {
         queue = lCopyElem(lFirst(queue_dummy_list));
      }
      *ppelem = queue;
      DEXIT;
      return 0;
   }

   /*
   ** look at the version file list and see which file matches
   */
   for_each(queue_version_file, version_file_list) {
      
      ulfiledate = version_filename_to_t_time(lGetString(queue_version_file, HF_name));
      if (ulfiledate > uldate)
      { 
         queue_version_file = lPrev(queue_version_file);
         queue_dummy_list = lGetList(queue_version_file, HF_entries); 
         if (!queue_dummy_list) {
            i_ret = make_filename(filename, SGE_PATH_MAX,
                                  lGetListName(ldir),
                                  lGetString(version_subdir, HD_name),
                                  lGetString(queue_version_file, HF_name));
            if (i_ret) {
              ERROR((SGE_EVENT, MSG_HISTORY_CANTASSEMBLEFILENAMEX_I , i_ret)); 
              DEXIT;
              return -8;
            }
            DPRINTF(("filename: %s\n", filename));
            queue = cull_read_in_qconf(NULL, filename, 0, 0, NULL, NULL);
            if (!queue) {
               ERROR((SGE_EVENT, MSG_HISTORY_CANTFINDQUEUEVERSION ));
               DEXIT;
               return -9;
            }
            queue_dummy_list = lCreateList(lGetString(queue_version_file, HF_name), QU_Type);
            lAppendElem(queue_dummy_list, lCopyElem(queue));

/* ### */
            lSetList(queue_version_file, 
                     HF_entries, 
                     queue_dummy_list);
            files_cached++;
            if (files_cached > MAX_CACHE_QUEUE_VERSIONS) {
               for_each(queue_version_file, version_file_list) {
                  lList *lp;

                  if (!files_cached) {
                     break;
                  }
                  if ((lp = lGetList(queue_version_file, HF_entries))) {
#if 0
		               lFreeList(lp);
#endif
                     lSetList(queue_version_file, HF_entries, NULL);
                     files_cached--;
                  }
               }
            }
         }
         else {
            queue = lCopyElem(lFirst(queue_dummy_list));
         }
         *ppelem = queue;
         DEXIT;
         return 0;
      }
   }      
   
   DPRINTF((" no queue version file found...\n"));
   DEXIT;
   *ppelem = NULL;
   return 0;
}


/*
** NAME
**   make_complex_list
** PARAMETER
**   ldir  
**   uldate        -
**   pplist        -
** RETURN
**
** EXTERNAL
**
** DESCRIPTION
**
*/
int make_complex_list(
lList *ldir,
unsigned long uldate,
lList **pplist 
) {
   lListElem *subdir, *cmplx;
   int i_ret;

   DENTER(TOP_LAYER, "make_complex_list");
   if (!pplist) {
      ERROR((SGE_EVENT, MSG_POINTER_PPLISTISNULLINMAKECOMPLEXLIST ));
      DEXIT;
      return -1;
   }
   
   *pplist = lCreateList("complex list", CX_Type);

   for_each(subdir, ldir) {
      i_ret = find_complex_version(ldir, subdir, uldate,  &cmplx);
      if (i_ret || !cmplx) {
         lFreeList(*pplist);
         *pplist = NULL;
         ERROR((SGE_EVENT, MSG_HISTORY_CANTFINDCOMPLEXVERSIONX_I , i_ret));
         DEXIT;
         return -2;
      }
      lAppendElem(*pplist, cmplx);
   }

   if (lGetNumberOfElem(*pplist) == 0) {
      lFreeList(*pplist);
      *pplist = NULL;
      ERROR((SGE_EVENT, MSG_HISTORY_NOCOMPLEXESFOUND ));
      DEXIT;
      return -3;
   }

   DEXIT;
   return 0;
}


/*
** NAME
**   make_filename
** PARAMETER
**
** RETURN
**
** EXTERNAL
**
** DESCRIPTION
**
*/
int make_filename(
char *filename,
unsigned int max_len,
const char *dir1,
const char *dir2,
const char *file 
) {
   DENTER(BASIS_LAYER, "make_filename");   

   if (!filename || !dir1 || !dir2 || !file) {
      DPRINTF(("make_filename: NULL pointer received"));
      DEXIT; 
      return -1;
   }
   
   if (strlen(dir1) + strlen(dir2) + strlen(file) + 2 > max_len) {
      DPRINTF(("make_filename: filename too long"));
      DEXIT;
      return -2;
    }
   
   *filename = 0;
   
   strcpy(filename, dir1);
   if (*filename && (filename[strlen(filename) - 1] != '/')) {
      strcat(filename, "/");
   }
   strcat(filename, dir2);
   if (*filename && (filename[strlen(filename) - 1] != '/')) {
      strcat(filename, "/");
   }
   strcat(filename, file);

   DEXIT;
   return 0;
}



/*
** NAME
**   find_queue_version_by_name
** PARAMETER
**   ldir        -  directory (list) in which queue_name is a subdirectory,
**                  HD_Type
**   queue_name     -
**   uldate         -
**   ppelem         -
**   flags          -
** RETURN
**   0                -  no error in searching (not found is not an error)
** EXTERNAL
**
** DESCRIPTION
**
*/
int find_queue_version_by_name(
lList *ldir,
char *queue_name,
unsigned long uldate,
lListElem **ppelem,
unsigned long flags 
) {
   lListElem *queue_subdir;
   const char *dirname;
   char *str;
   int i_ret;
   int not_the_same;
   
   DENTER(TOP_LAYER, "find_queue_version_by_name");
   
   if (!queue_name) {
      ERROR((SGE_EVENT, MSG_POINTER_QUEUENAMEISNULLINFINDQUEUEVERSIONBYNAME ));
      DEXIT;
      return -1;
   }
   if (!*queue_name) {
      ERROR((SGE_EVENT, MSG_HISTORY_QUEUENAMEHASZEROLENGTHINFINDQUEUEVERSIONBYNAME ));
      DEXIT;
      return -2;
   }

   for_each(queue_subdir, ldir) {
      dirname = lGetString(queue_subdir, HD_name);
      if (!dirname) {
         ERROR((SGE_EVENT, MSG_HISTORY_SUBDIRHASNONAME));
         DEXIT;
         return -3;
      }
      if (!*dirname) {
         ERROR((SGE_EVENT, MSG_HISTORY_SUBDIRHASZEROLENGTHNAME));
         DEXIT;
         return -4;
      }
      str = sge_strdup(NULL, dirname);
      if (!str) {
         ERROR((SGE_EVENT, MSG_MEMORY_CANTALLOCATEMEMORY));
         DEXIT;
         return -5;
      }
      if (str[strlen(str) - 1] == '/') {
         str[strlen(str) - 1] = 0;
      }
      not_the_same = ((flags & FLG_BY_NAME_NOCASE) ? 
                      strcmp(str, queue_name) : 
                      strcasecmp(str, queue_name));
      if (!not_the_same) {
         i_ret = find_queue_version(ldir, queue_subdir, uldate, ppelem);
         if (i_ret || !*ppelem) {
            ERROR((SGE_EVENT, MSG_HISTORY_CANTFINDQUEUEVERSIONFORQUEUEXREFTOYZ_SSI,
                   queue_name, ctime((time_t *) &uldate), i_ret));
            free(str);
            DEXIT;
            return -6;
         }
         free(str);
         DEXIT;
         return 0;
      }
      free(str);
   }
   DPRINTF(("find_queue_version_by_name: no history subdir by queue name %s\n",
            queue_name));
   DEXIT;
   *ppelem = NULL;
   return 0;
}


/*
** NAME
**   find_complex_version_by_name
** PARAMETER
**   ldir        -  directory (list) in which complex_name is a subdirectory,
**                  HD_Type
**   complex_name   -
**   uldate         -
**   ppelem         -
**   flags          -
** RETURN
**   0                -  no error in searching (not found is not an error)
** EXTERNAL
**
** DESCRIPTION
**
*/
int find_complex_version_by_name(
lList *ldir,
char *complex_name,
unsigned long uldate,
lListElem **ppelem,
unsigned long flags 
) {
   lListElem *complex_subdir;
   const char *dirname;
   char *str;
   int i_ret;
   int not_the_same;
   
   DENTER(TOP_LAYER, "find_complex_version_by_name");

   if (!complex_name) {
      ERROR((SGE_EVENT, MSG_POINTER_COMPLEXNAMEISNULLINFINDCOMPLEXVERSIONBYNAME ));
      DEXIT;
      return -1;
   }
   if (!*complex_name) {
      ERROR((SGE_EVENT, MSG_HISTORY_NOCOMPLEXNAMEGIVENINFINDCOMPLEXVERSIONBYNAME ));
      DEXIT;
      return -2;
   }

   for_each(complex_subdir, ldir) {
      dirname = lGetString(complex_subdir, HD_name);
      if (!dirname) {
         ERROR((SGE_EVENT, MSG_HISTORY_SUBDIRHASNONAME ));
         DEXIT;
         return -3;
      }
      if (!*dirname) {
         ERROR((SGE_EVENT, MSG_HISTORY_SUBDIRHASZEROLENGTHNAME ));
         DEXIT;
         return -4;
      }
      str = sge_strdup(NULL, dirname);
      if (!str) {
         ERROR((SGE_EVENT, MSG_MEMORY_CANTALLOCATEMEMORY ));
         DEXIT;
         return -5;
      }
      if (str[strlen(str) - 1] == '/') {
         str[strlen(str) - 1] = 0;
      }
      not_the_same = ((flags & FLG_BY_NAME_NOCASE) ? 
                      strcasecmp(str, complex_name) : 
                      strcmp(str, complex_name));
      if (!not_the_same) {
         i_ret = find_complex_version(ldir, complex_subdir, uldate, ppelem);
         if (i_ret || !*ppelem) {
            ERROR((SGE_EVENT, 
	       MSG_HISTORY_FINDCOMPLEXVERSIONBYNAMEERRORXRETRIEVENCOMPLEXVERSIONFORY_IS , 
	       i_ret, complex_name));
            DEXIT;
            free(str);
            return -6;
         }
         free(str);
         DEXIT;
         return 0;
      }
      free(str);
   }
   DPRINTF(("find_complex_version_by_name: no history subdir available for complex %s\n",
      complex_name));
   DEXIT;
   *ppelem = NULL;
   return 0;
}


/*
** NAME
**   find_host_version_by_name
** PARAMETER
**   ldir        -  directory (list) in which host_name is a subdirectory,
**                  HD_Type
**   host_name   -
**   uldate         -
**   ppelem         -
**   flags          -
** RETURN
**   0                -  no error in searching (not found is not an error)
** EXTERNAL
**
** DESCRIPTION
**
*/
int find_host_version_by_name(
lList *ldir,
char *host_name,
unsigned long uldate,
lListElem **ppelem,
unsigned long flags 
) {
   lListElem *host_subdir;
   const char *dirname;
   char *str;
   int i_ret;
   int not_the_same;
   
   DENTER(TOP_LAYER, "find_host_version_by_name");

   if (!host_name) {
      ERROR((SGE_EVENT, MSG_MEMORY_HOSTNAMEISNULLINFINDHOSTVERSIONBYNAME ));
      DEXIT;
      return -1;
   }
   if (!*host_name) {
      ERROR((SGE_EVENT, MSG_HISTORY_NOHOSTNAMEGIVENINFINDHOSTVERSIONBYNAME ));
      DEXIT;
      return -2;
   }

   for_each(host_subdir, ldir) {
      dirname = lGetString(host_subdir, HD_name);
      if (!dirname) {
         ERROR((SGE_EVENT, MSG_HISTORY_SUBDIRHASNONAME));
         DEXIT;
         return -3;
      }
      if (!*dirname) {
         ERROR((SGE_EVENT, MSG_HISTORY_SUBDIRHASZEROLENGTHNAME));
         DEXIT;
         return -4;
      }
      str = sge_strdup(NULL, dirname);
      if (!str) {
         ERROR((SGE_EVENT, MSG_MEMORY_CANTALLOCATEMEMORY));
         DEXIT;
         return -5;
      }
      if (str[strlen(str) - 1] == '/') {
         str[strlen(str) - 1] = 0;
      }
      not_the_same = ((flags & FLG_BY_NAME_NOCASE) ? 
                      sge_hostcmp(str, host_name) : 
                      strcmp(str, host_name));
      if (!not_the_same) {
         i_ret = find_host_version(ldir, host_subdir, uldate, ppelem);
         if (i_ret || !*ppelem) {
            ERROR((SGE_EVENT, 
	            MSG_HISTORY_FINDHOSTVERSIONBYNAMEERRORXRETRIEVINGHOSTVERSIONFORY_IS , i_ret, host_name));
            DEXIT;
            free(str);
            return -6;
         }
         free(str);
         DEXIT;
         return 0;
      }
      free(str);
   }
   DPRINTF(("find_host_version_by_name: no history subdir available for host %s\n",
      host_name));
   DEXIT;
   *ppelem = NULL;
   return 0;
}

time_t version_filename_to_t_time(
const char *timestr 
) {
   struct tm tm_time; 

   strptime(timestr, "%Y%m%d_%H%M%S", &tm_time);
   return mktime(&tm_time);
}

char *t_time_to_version_filename(
char *buf,
size_t buflen,
time_t t_time 
) {
   strftime(buf, buflen,"%Y%m%d_%H%M%S", localtime(&t_time));
   return buf;
}


/*
** NAME
**   create_version_subdir
** PARAMETER
**   ldir        -  directory (list) in which version_subdir will  a subdirectory
**                  (element), needed to assemble full path, HD_Type
**   dirname         -  name of the directory to be created
**   version_subdir  -  pointer-pointer to the newly created element
**                      of type HD_Type representing the new subdir
** RETURN
**
** EXTERNAL
**   me.uid
**   me.gid
** DESCRIPTION
**   creates a subdirectory in the directory given by ldir
**   and the corresponding element in the ldir directory
**   list
*/
int create_version_subdir(
lList *ldir,
const char *dirname,
lListElem **ppelem 
) {
   char *pathname;
   const char *str;
   int i_ret;
   lListElem *ep;

   DENTER(TOP_LAYER, "create_version_subdir");

   if (!ldir || !dirname || !ppelem) {
      ERROR((SGE_EVENT, MSG_POINTER_CREATEVERSIONSUBDIRNULLPOINTERRECEIVED ));
      DEXIT;
      return -1;
   }

   if (!*dirname) {
      ERROR((SGE_EVENT, MSG_HISTORY_CREATEVERSIONSUBDIRDIRHASZERONAME ));
      DEXIT;
      return -2;
   }

   str = lGetListName(ldir);
   if (!str) {
      ERROR((SGE_EVENT, MSG_HISTORY_CREATEVERSIONSUBDIRDIRHASZERONAME ));
      DEXIT;
      return -3;
   }

   pathname = malloc(strlen(str) + strlen(dirname) + 2);
   if (!pathname) {
      ERROR((SGE_EVENT, MSG_HISTORY_CREATEVERSIONSUBDIRMEMORYALLOCFAILED ));
      DEXIT;
      return -4;
   }
   strcpy(pathname, str);
   if (*pathname && (pathname[strlen(pathname) - 1] != '/')) {
      strcat(pathname, "/");
   }
   strcat(pathname, dirname);

   i_ret = sge_mkdir(pathname, 0755, 0);
   if (i_ret) {
      ERROR((SGE_EVENT, MSG_HISTORY_CREATEVERSIONSUBDIRERRORXCREATINGDIR_I , i_ret));
      free(pathname);
      DEXIT;
      return -5;
   }
   free(pathname);

   ep = lCreateElem(HD_Type);
   if (!ep) {
      ERROR((SGE_EVENT, MSG_HISTORY_CREATEVERSIONSUBDIRMEMORYALLOCFAILED ));
      DEXIT;
      return -6;
   }
   lSetString(ep, HD_name, dirname);
   lAppendElem(ldir, ep);
   *ppelem = ep;

   DEXIT;
   return 0;
}


/*
** NAME
**   find_version_subdir
** PARAMETER
**   ldir        -  directory (list) to search for subdirectories
**                  (elements), HD_Type
**   dirname         -  name of directory to look for
**   ppelem          -  pointer-pointer to the element corresponding to
**                      the requested directory or NULL if version
**                      subdir was not found
**   flags           -  FLG_BY_NAME_NOCASE - match case insensitive
** RETURN
**   0                -  no error in searching (not found is not an error)
** EXTERNAL
**
** DESCRIPTION
**   searches a subdirectory element in a list
**   returns the element which HD_name is equal to the dirname given
**   no search in the file system is performed, only in the list
**   this should not be a problem because version subdirs are not
**   created by hand or by other processes, only by qmaster itsself(?)
*/
/*
** problem: different kinds of errors can occur here
** the directory might not exist, which is in fact
** not an error but a result of the search
** it might exist but we have no rights
** or whatever
** read sge_mkdir again to find out what can be found out
** the concept is to cache these basic lists cause its faster
** but does qmaster have enough memory?
*/
int find_version_subdir_by_name(
lList *ldir,
const char *dirname,
lListElem **ppelem,
unsigned long flags 
) {
   const char *a_dirname;
   char *str;
   lListElem *version_subdir;
   int not_the_same;

   DENTER(TOP_LAYER, "find_version_subdir_by_name");

   if (!ldir || !dirname || !ppelem) {
      ERROR((SGE_EVENT, MSG_HISTORY_FINDVERSSUBDIRBYNAMENULLPOINTERRECEIVED ));
      DEXIT;
      return -1;
   }

   if (!*dirname) {
      ERROR((SGE_EVENT, MSG_HISTORY_FINDVERSSUBDIRBYNAMEDIRHASZERONAME ));
      DEXIT;
      return -2;
   }

   for_each(version_subdir, ldir) {
      a_dirname = lGetString(version_subdir, HD_name);
      if (!a_dirname) {
         ERROR((SGE_EVENT, MSG_HISTORY_FINDVERSSUBDIRBYNAMEENCOUNTEREDSUBDIRWITHNONAME ));
         DEXIT;
         return -3;
      }
      if (!*a_dirname) {
         ERROR((SGE_EVENT, MSG_HISTORY_FINDVERSSUBDIRBYNAMEENCOUNTEREDSUBDIRWITHZERONAME));
         DEXIT;
         return -4;
      }
      str = sge_strdup(NULL, a_dirname);
      if (!str) {
         ERROR((SGE_EVENT, MSG_HISTORY_FINDVERSSUBDIRBYNAMEMEMORYALLOCFAILED ));
         DEXIT;
         return -5;
      }
      if (str[strlen(str) - 1] == '/') {
         str[strlen(str) - 1] = 0;
      }
      not_the_same = ((flags & FLG_BY_NAME_NOCASE) ? 
                      strcmp(str, dirname) : 
                      strcasecmp(str, dirname));
      if (!not_the_same) {
         *ppelem = version_subdir;
         free(str);
         DEXIT;
         return 0;
      }
      free(str);
   }

   DPRINTF(("no version subdir found for %s\n", dirname));
   DEXIT;
   *ppelem = NULL;
   return 0;
}


/*
** NAME
**   write_complex_version
** PARAMETER
**   ldir            -  directory list, needed to assemble path
**   version_subdir  -  element in list, directory to be written to
**   uldate          -  date of complex version, used to determine file name
**   cmplx           -  complex structure to be written
**
** RETURN
**
** EXTERNAL
**
** DESCRIPTION
**
*/
/*
** problem: dont forget to create version_file element and
** put it in the right place in the list
** or use an unsorted list here?
** create no version_file element at all?
** we do not read the files in the code where they are written,
** so never cache any attribute lists here
** dont forget to reset latest and oldest, or dont we need these here?
*/
/*
** -1 uldate could mean take current date and time
*/
int write_complex_host_version(
lList *ldir,
lListElem *version_subdir,
unsigned long uldate,
lList *cmplx,
lListElem *host 
) {
   char *pathname;
   char filename[256 + 1];
   int i_ret;
   lList *answer = NULL;

   DENTER(TOP_LAYER, "write_complex_version");

   if (!ldir || !version_subdir || !(cmplx||host)) {
      ERROR((SGE_EVENT, MSG_POINTER_WRITECOMPLEXHOSTVERSIONNULLPOINTERRECEIVED));
      DEXIT;
      return -1;
   }

   pathname = malloc(SGE_PATH_MAX);
   if (!pathname) {
      ERROR((SGE_EVENT, MSG_HISTORY_WRITECOMPLEXHOSTVERSIONMEMORYALLOCFAILED ));
      DEXIT;
      return -2;
   }

   t_time_to_version_filename(filename, sizeof(filename), uldate);

   i_ret = make_filename(pathname, SGE_PATH_MAX,
                         lGetListName(ldir),
                         lGetString(version_subdir, HD_name),
                         filename);
   if (i_ret) {
      ERROR((SGE_EVENT, MSG_HISTORY_WRITECOMPLEXHOSTVERSIONERRORXMAKEINFFILENAME_I , i_ret));
      FREE(pathname);
      DEXIT;
      return -3;
   }
  
   if (cmplx) 
      i_ret = write_cmplx(0, pathname, cmplx, NULL, &answer);
   else
      write_host(0, 3, host, EH_name, pathname);

   if (answer) {
      ERROR((SGE_EVENT, lGetString(lFirst(answer), AN_text)));
      answer = lFreeList(answer);
      FREE(pathname);
      DEXIT;
      return -3;
   }      

   if (cmplx && i_ret) {
      ERROR((SGE_EVENT, MSG_HISTORY_WRITECOMPLEXHOSTVERSIONERRORXWRITEINGCOMPLEX_I ,
             i_ret));
      FREE(pathname);
      DEXIT;
      return -4;
   }
   FREE(pathname);

   /*
   ** if version file existed already we replace the list element
   ** although this is not very likely
   */
   /*
   ** problem: skipped this at the moment
   */


   DEXIT;
   return 0;
}

/*
** NAME
**   write_queue_version
** PARAMETER
**   ldir            -  directory list, needed to assemble path
**   version_subdir  -  element in list, directory to be written to
**   uldate          -  date of queue version, used to determine file name
**   queue           -  queue structure to be written
**
** RETURN
**
** EXTERNAL
**
** DESCRIPTION
**
*/
/*
** problem: dont forget to create version_file element and
** put it in the right place in the list
** or use an unsorted list here?
** create no version_file element at all?
** we do not read the files in the code where they are written,
** so never cache any attribute lists here
** dont forget to reset latest and oldest, or dont we need these here?
** 
** note: at the moment it isn't necessary to create a version_file
** element, but this could be done by an extra lListElem ** parm,
** if not NULL, then all actions described above are performed
*/
/*
** -1 uldate could mean take current date and time
*/
int write_queue_version(
lList *ldir,
lListElem *version_subdir,
unsigned long uldate,
lListElem *queue 
) {
   char *pathname;
   char filename[256 + 1];
   int i_ret;

   DENTER(TOP_LAYER, "write_queue_version");

   if (!ldir || !version_subdir || !queue) {
      ERROR((SGE_EVENT, MSG_POINTER_WRITEQUEUEVERSIONNULLPOINTERRECEIVED));
      DEXIT;
      return -1;
   }

   pathname = malloc(SGE_PATH_MAX);
   if (!pathname) {
      ERROR((SGE_EVENT, MSG_HISTORY_WRITEQUEUEVERSIONMEMORYALLOCFAILED ));
      DEXIT;
      return -2;
   }

   t_time_to_version_filename(filename, sizeof(filename), uldate);

   i_ret = make_filename(pathname, SGE_PATH_MAX,
                         lGetListName(ldir),
                         lGetString(version_subdir, HD_name),
                         filename);
   if (i_ret) {
      ERROR((SGE_EVENT, MSG_HISTORY_WRITEQUEUEVERSIONERRORXMAKINGFILENAME_I ,
             i_ret));
      FREE(pathname);
      DEXIT;
      return -3;
   }
   
   i_ret = cull_write_qconf(0, 0, NULL, pathname, NULL, queue);
   if (i_ret) {
      ERROR((SGE_EVENT, MSG_HISTORY_WRITEQUEUEVERSIONERRORXWRITINGQUEUE_I ,
             i_ret));
      FREE(pathname);
      DEXIT;
      return -4;
   }
   FREE(pathname);

   /*
   ** if version file existed already we replace the list element
   ** although this is not very likely
   */
   /*
   ** problem: skipped this at the moment
   */
   

   DEXIT;
   return 0;
}


/*
** NAME
**   sge_write_queue_version
** PARAMETER
**   ldir            -  directory list, needed to assemble path, HD_Type
**   version_subdir  -  element in list, directory to be written to, HD_Type
**   uldate          -  date of queue version, used to determine file name
**   qep             -  queue structure to be written
**
** RETURN
**
** EXTERNAL
*/
int sge_write_queue_version(
lList *ldir,
lListElem *version_subdir,
unsigned long uldate,
lListElem *qep 
) {
   char *pathname;
   char filename[256 + 1];
   int i_ret;

   DENTER(TOP_LAYER, "sge_write_queue_version");

   if (!ldir || !version_subdir || !qep) {
      ERROR((SGE_EVENT, MSG_HISTORY_SGEWRITEQUEUEVERSIONNULLPOINTER ));
      DEXIT;
      return -1;
   }

   pathname = malloc(SGE_PATH_MAX);
   if (!pathname) {
      ERROR((SGE_EVENT, MSG_HISTORY_SGEWRITEQUEUEVERSIONMEMORY ));
      DEXIT;
      return -2;
   }

   t_time_to_version_filename(filename, sizeof(filename), uldate);

   i_ret = make_filename(pathname, SGE_PATH_MAX,
                         lGetListName(ldir),
                         lGetString(version_subdir, HD_name),
                         filename);
   if (i_ret) {
      ERROR((SGE_EVENT, MSG_HISTORY_SGEWRITEQUEUEVERSIONERRORXMAKEFILENAME_I ,
             i_ret));
      FREE(pathname);
      DEXIT;
      return -3;
   }
   
   /*
   ** no spool data written at the moment
   ** problem: e.g. no version can be requested
   */
   {
      char path[2048];
      char file[2048]; 
      char *pos;

      pos = strrchr(pathname, '/');
      strcpy(file, pos+1);
      pos[0] = '\0';
      strcpy(path, pathname);

      i_ret = cull_write_qconf(0, 0, path, file, NULL, qep);
      if (i_ret) {
         ERROR((SGE_EVENT,MSG_HISTORY_SGEWRITEQUEUEVERSIONERRORXWRITINGQUEUE_I ,
                i_ret));
         FREE(pathname);
         DEXIT;
         return -4;
      }
   }
   FREE(pathname);
   

   DEXIT;
   return 0;
}


/*
** NAME
**   sge_write_queue_history
** PARAMETER
**   queue           -  queue structure to be written
**
** RETURN
**
** EXTERNAL
**
** DESCRIPTION
**   generates a queue version file in the queues subdirectory of the
**   history directory.
*/
int sge_write_queue_history(
lListElem *qep 
) {
   static lList *queue_dirs = NULL;
   lListElem *version_subdir;
   int i_ret;
   unsigned long ultime;

   DENTER(TOP_LAYER, "sge_write_queue_history");

   i_ret = prepare_version_subdir(&queue_dirs, 
      STR_DIR_QUEUES, lGetString(qep, QU_qname), &version_subdir);
   if (i_ret) {
      ERROR((SGE_EVENT,
         MSG_HISTORY_XPREPVERSSUBDIRFORQUEUEY_IS , i_ret, 
         lGetString(qep, QU_qname)));
      lFreeList(queue_dirs);
      queue_dirs = NULL;
      return -1;
   }

   /*
   ** problem: use sge_get_gmt here?
   */
   ultime = (unsigned long) time(NULL);
   i_ret = sge_write_queue_version(queue_dirs, version_subdir, ultime, qep);
   if (i_ret) {
      ERROR((SGE_EVENT,
         MSG_HISTORY_XWRITINGTOVERSSUBDIRTHECONFIGFORQUEUEY_IS , 
         i_ret, lGetString(qep, QU_qname)));
      lFreeList(queue_dirs);
      queue_dirs = NULL;
      return -2;
   }

   DEXIT;
   return 0;
}


/*
** NAME
**   write_host_history
** PARAMETER
**   host           -  host structure to be written
**
** RETURN
**
** EXTERNAL
**   Master_Complex_List
** DESCRIPTION
**   generates a host version file in the exechosts subdirectory of the
**   history directory.
*/
int write_host_history(
lListElem *host 
) {
   static lList *exechost_dirs = NULL;
   lListElem *version_subdir; 
#if 0
   lListElem *hosttemplate;
   lList *hostcomplex = NULL;
#endif
   int i_ret;
   unsigned long ultime;

   DENTER(TOP_LAYER, "write_host_history");

   i_ret = prepare_version_subdir(&exechost_dirs, 
      STR_DIR_EXECHOSTS, lGetHost(host, EH_name), &version_subdir);
   if (i_ret) {
      ERROR((SGE_EVENT, MSG_HISTORY_WRITEHOSTHISTORYERRORXPREPVERSSUBDIRFORHOSTY_IS
          , i_ret, lGetHost(host, EH_name)));
      lFreeList(exechost_dirs);
      exechost_dirs = NULL;
      DEXIT;
      return -1;
   }

   ultime = (unsigned long) time(NULL);
   i_ret = write_complex_host_version(exechost_dirs, version_subdir, ultime, NULL, host);
   if (i_ret) {
      ERROR((SGE_EVENT, MSG_HISTORY_WRITEHOSTHISTORYERRORXWRITINGTOVERSSUBDIRTHECONFIGFORHOSTY_IS
         , i_ret, lGetHost(host, EH_name)));
      lFreeList(exechost_dirs);
      exechost_dirs = NULL;
      DEXIT;
      return -4;
   }

   DEXIT;
   return 0;
}


/*
** NAME
**   write_complex_history
** PARAMETER
**   complex           -  complex structure to be written
**
** RETURN
**
** EXTERNAL
**   Master_Complex_List
** DESCRIPTION
**   generates a complex version file in the complexes subdirectory of the
**   history directory.
*/
int write_complex_history(
lListElem *complex 
) {
   static lList *complex_dirs = NULL;
   lListElem *version_subdir;
   int i_ret;
   unsigned long ultime;

   DENTER(TOP_LAYER, "write_complex_history");

   i_ret = prepare_version_subdir(&complex_dirs, 
      STR_DIR_COMPLEXES, lGetString(complex, CX_name), &version_subdir);
   if (i_ret) {
      ERROR((SGE_EVENT, MSG_HISTORY_WRITECOMPLEXHISTORYERRORXPREPARINGVERSIONSUBIDRFORCOMPLEXY_IS
         , i_ret, lGetString(complex, CX_name)));
      lFreeList(complex_dirs);
      complex_dirs = NULL;
      return -1;
   }

   /*
   ** problem: use sge_get_gmt here?
   */
   ultime = (unsigned long) time(NULL);
   i_ret = write_complex_host_version(complex_dirs, version_subdir, ultime, 
      lGetList(complex, CX_entries), NULL);
   if (i_ret) {
      ERROR((SGE_EVENT,
             MSG_HISTORY_WRITECOMPLEXHISTORYERRORXWRITINGTOVERSIONSUBDIRTHECONFIGFORCOMPLEXY_IS,
               i_ret, lGetString(complex, CX_name) ));
      lFreeList(complex_dirs);
      complex_dirs = NULL;
      return -2;
   }

   DEXIT;
   return 0;
}



/*
** NAME
**   prepare_version_subdir
** PARAMETER
**   pldir           -   list pointer-pointer to list (directory) of
**                       version subdirectories (elements), HD_Type
**                       if list is NULL, then it is created as the
**                       list of subdirectories in path.history_dir/<sname>
**   sname           -   name of subdirectory of the history path
**                       (i.e. queues, exechosts or complexes)
**                       only needed if pldir is NULL
**   name            -   name of the version_subdir to be found or created
**   pversion_subdir -   element pointer-pointer, to be set to found or
**                       newly created version subdirectory, HD_Type
**                       is an element in *pldir           
**
** RETURN
**
** DESCRIPTION
**   finds or creates a version subdirectory
**   creates all necessary directories
**   
*/
int prepare_version_subdir(
lList **pldir,
const char *sname,
const char *name,
lListElem **pversion_subdir 
) {
   int i_ret;
   
   DENTER(TOP_LAYER, "prepare_version_subdir");

   if (!pldir || !sname || !name || !pversion_subdir) {
      ERROR((SGE_EVENT, MSG_HISTORY_PREPAREVERSIONSUBDIRNULLPOINTER));
      DEXIT;
      return -1;
   }

   if (!*pldir) {
      char *str_dir;
      const char *history_dir = path_state_get_history_dir();
      SGE_STRUCT_STAT sbuf;

      if (!history_dir || !*history_dir) {
         ERROR((SGE_EVENT, MSG_HISTORY_PREPAREVERSIONSUBDIRNOHISTSUBDIRCONFIGURED));
	 DEXIT;
         return -2;
      }
      if (SGE_STAT(history_dir, &sbuf)) {
/*         WARNING((SGE_EVENT, "history directory doesn't exist - making\n")); */
         /*
         ** problem: do we want to exit on errors here?
         */
         sge_mkdir(history_dir, 0755, 1);
      }
      str_dir = malloc(strlen(history_dir) + strlen(sname) + 3);
      if (!str_dir) {
         ERROR((SGE_EVENT, MSG_HISTORY_PREPAREVERSIONSUBDIRMEMORYALLOCFAILED ));
	 DEXIT;
         return -3;
      }
      strcpy(str_dir, history_dir);
      if (*str_dir && str_dir[strlen(str_dir) - 1] != '/') {
         strcat(str_dir, "/");
      }
      strcat(str_dir, sname);

      if (SGE_STAT(str_dir, &sbuf)) {
/*          WARNING((SGE_EVENT, "history subdirectory doesn't exist - making\n")); */
         /*
         ** problem: do we want to exit on errors here?
         */
         sge_mkdir(str_dir, 0755, 1);
      }
      i_ret = init_history_subdirs(str_dir, pldir);
      if (i_ret) {
         ERROR((SGE_EVENT, MSG_HISTORY_PREPAREVERSIONSUBDIRERRORXREADINGHISTSUBDIRY_IS , i_ret, sname));
         FREE(str_dir);
	 DEXIT;
         return -4;
      }
      FREE(str_dir);
      str_dir = NULL;
   }

   /*
   ** we do not differentiate between mod and add here
   ** though this could be done
   */
   i_ret = find_version_subdir_by_name(*pldir, 
      name, pversion_subdir, 0);
   if (i_ret) {
      ERROR((SGE_EVENT,
         MSG_HISTORY_PREPAREVERSIONSUBDIRERRORXFINDINGVERSIONSUBDIRFOROBJY_IS , i_ret, name));
      lFreeList(*pldir);
      *pldir = NULL;
      DEXIT;
      return -5;
   }
   if (!*pversion_subdir) {
      DPRINTF(("object %s goes down in history\n", name));
      i_ret = create_version_subdir(*pldir, name, pversion_subdir);
      if (i_ret) {
         ERROR((SGE_EVENT,
            MSG_HISTORY_PREPAREVERSIONSUBDIRERRORXFINDVERSINSUBDIRFOROBJY_IS , i_ret, name));
         lFreeList(*pldir);
         *pldir = NULL;
	 DEXIT;
         return -6;
      }
   }
   
   DEXIT;
   return 0;
}

/*
** NAME
**   is_object_in_history
** PARAMETER
**   sname           -   name of subdirectory of the history path
**                       (i.e. queues, exechosts or complexes)
**                       defines the type of the object
**                       please use the defines in qacct.h,
**                       do not use literal strings
**   name            -   name of object
** RETURN
**
** DESCRIPTION
**   finds out if there is a history entry for the object
**   this function is useful if you want to make sure there
**   is a history entry for an object and you want to
**   create one only in case it isnt there
**   example:
**   is_there = is_object_in_history(STR_DIR_COMPLEXES, "complex1");
*/
int is_object_in_history(
const char *sname,
const char *name 
) {
   char *str_dir;
   const char *history_dir = path_state_get_history_dir();
   SGE_STRUCT_STAT sbuf;

   DENTER(TOP_LAYER, "is_object_in_history");

   if (!sname || !name) {
      ERROR((SGE_EVENT, MSG_HISTORY_ISOBJECTINHISTORYNULLPOINTER ));
      DEXIT;
      return 0;
   }
   if (!history_dir || !*history_dir) {
      ERROR((SGE_EVENT,
        MSG_HISTORY_ISOBJECTINHISTORYNOHISTSUBDIRCONFIGURED ));
      DEXIT;
      return 0;
   }
   /*
   ** test for existence of history path
   */
   if (SGE_STAT(history_dir, &sbuf)) {
      DPRINTF(("%s object %s cannot be in history because "
         "history dir %s does not exist\n", sname, name, history_dir));
      DEXIT;
      return 0;
   }

   str_dir = malloc(strlen(history_dir) + strlen(sname) + strlen(name) + 4);
   if (!str_dir) {
      ERROR((SGE_EVENT, MSG_HISTORY_PREPAREVERSIONSUBDIR ));
      DEXIT;
      return 0;
   }
   
   /*
   ** test for existence of history subdirectories (category)
   */
   strcpy(str_dir, history_dir);
   if (*str_dir && str_dir[strlen(str_dir) - 1] != '/') {
      strcat(str_dir, "/");
   }
   strcat(str_dir, sname);
   if (SGE_STAT(str_dir, &sbuf)) {
      DPRINTF(("object %s cannot be in history because "
         "history subdir %s does not exist\n", name, sname));
      FREE(str_dir);
      DEXIT;
      return 0;
   }

   /*
   ** test for existence of history directory for object
   */
   if (*str_dir && str_dir[strlen(str_dir) - 1] != '/') {
      strcat(str_dir, "/");
   }
   strcat(str_dir, name);
   if (SGE_STAT(str_dir, &sbuf)) {
      DPRINTF(("object %s is not in history\n", name));
      FREE(str_dir);
      DEXIT;
      return 0;
   }
   FREE(str_dir);
   DEXIT;
   return 1;
}


#ifdef TEST
int main(int argc, char *argv[], char *envp[])
{
   lList *L_complex_dirs = NULL;
   lList *L_queue_dirs = NULL;
   lList *L_exechost_dirs = NULL;
   lListElem *ep;
   lList *complex_list, *complex_filled;
   int i_ret;
   
   DENTER_MAIN(TOP_LAYER, "htest");
  
#ifdef __SGE_COMPILE_WITH_GETTEXT__   
   /* init language output for gettext() , it will use the right language */
   sge_init_language_func((gettext_func_type)        gettext,
                         (setlocale_func_type)      setlocale,
                         (bindtextdomain_func_type) bindtextdomain,
                         (textdomain_func_type)     textdomain);
   sge_init_language(NULL,NULL);   
#endif /* __SGE_COMPILE_WITH_GETTEXT__  */
   lInit(nmv);

   i_ret = init_history_subdirs("complexes", &L_complex_dirs);     
   if (i_ret) {
      printf("i_ret 1 = %d\n", i_ret);
      return 1;
   }
   i_ret = init_history_subdirs("queues", &L_queue_dirs);     
   if (i_ret) {
      printf("i_ret 2 = %d\n", i_ret);
      return 2;
   }
   i_ret = init_history_subdirs("exechosts", &L_exechost_dirs);     
   if (i_ret) {
      printf("i_ret 3 = %d\n", i_ret);
      return 3;
   }
  
   /*
   i_ret = init_history_files(L_complex_dirs, lFirst(L_complex_dirs), &a_file_list);
   if (i_ret) {
      printf("i_ret 4 = %d\n", i_ret);
      return 4;
   }
   */
   
   /*
   i_ret = find_complex_version(L_complex_dirs, lFirst(L_complex_dirs), 2, &ep);
   if (i_ret || !ep) {
      printf("i_ret 5 = %d\n", i_ret);
      return 5;
   }
   */

   i_ret = make_complex_list(L_complex_dirs, 2, &complex_list);
   if (i_ret) {
      printf("i_ret 6 = %d\n", i_ret);
      return 6;
   }
   /* lDumpList(stdout, complex_list, 0); */
   /* getchar(); */

   /*
   ep = read_queue(1, "queues/dq/1");
   */

   i_ret = find_queue_version_by_name(L_queue_dirs, "dq", 2, &ep);
   lWriteElemTo(ep, stdout);

   complex_filled = NULL;
   queue_complexes2scheduler(&complex_filled, ep, NULL, complex_list, 0);
   /* lDumpList(stdout, complex_filled, 0); */
   /* getchar(); */
   
   return 0;
}
#endif
