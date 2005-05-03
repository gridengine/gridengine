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
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

#include "sge_gdi_request.h"
#include "config.h"
#include "read_object.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_unistd.h"
#include "sge_answer.h"
#include "sge_conf.h"

#include "msg_common.h"
#include "spool/classic/msg_spoollib_classic.h"

/****
 **** read_object

 ARGUMENTS 
   tag
      'tag' can be used to return an additional state from 
      work function to the caller of read_object().

   fields 
      If non NULL the caller will also accept a reduced element
      In this case read_object() the cull name of all fields 
      are returned in 'fields'. 'fields' must be large enough
      for incorporating all possible cull names.
         
 ****/
lListElem* read_object(
const char *dirname,
const char *filename,
int spool,
int flag,
int read_config_list_flag,
struct read_object_args *args,
int *tag,
int fields[]  
) {
   int ret;
   stringT fullname;
   FILE *fp;
   lListElem *ep, *unused;
   lList *alp = NULL, *clp = NULL;
   SGE_STRUCT_STAT sb;
   size_t size;
   char *buf;

   DENTER(TOP_LAYER, "read_object");

   /* build full filename */
   if(dirname && filename)
      sprintf(fullname, "%s/%s", dirname, filename);
   else if(dirname)
      sprintf(fullname, "%s", dirname);
   else
      sprintf(fullname, "%s", filename);
      
   /* open file */
   if(!(fp = fopen(fullname, "r"))) {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANT_OPEN_SS, fullname, strerror(errno)));
      DEXIT;
      return NULL;
   }

   if (!SGE_STAT(fullname, &sb)) {
      size = MAX(sb.st_size, 10000);
      if (((SGE_OFF_T)size != MAX(sb.st_size, 10000))
          || (buf = (char *) malloc(size)) == NULL) {
         fclose(fp);
         ERROR((SGE_EVENT, MSG_MEMORY_CANTMALLOCBUFFERFORXOFFILEY_SS, 
               args->objname, fullname));
         DEXIT;
         return NULL;
      }
   }   
   else {
      ERROR((SGE_EVENT, MSG_FILE_CANTDETERMINESIZEFORXOFFILEY_SS, 
             args->objname, fullname));
      fclose(fp);
      DEXIT;
      return NULL;
   }


   /* create List Element */
   if (!(ep = lCreateElem(args->objtype))) {
      fclose(fp);
      free(buf);
      ERROR((SGE_EVENT, MSG_SGETEXT_NOMEM));
      DEXIT;
      return NULL;
   }

   /* read in config file */
   if (read_config_list(fp, &clp, &alp, CF_Type, CF_name, CF_value,
                        CF_sublist, NULL, read_config_list_flag, buf, size)) {
      ERROR((SGE_EVENT, lGetString(lFirst(alp), AN_text)));
      alp = lFreeList(alp);
      fclose(fp);
      free(buf);
      DEXIT;
      return NULL;
   }

   free(buf);
   fclose(fp);

   /* well, let's do the work... */
   ret = args->work_func(&alp, &clp, fields, ep, spool, flag, tag, 0);
   if (ret) {
      if (alp) 
         ERROR((SGE_EVENT, lGetString(lFirst(alp), AN_text)));
      alp = lFreeList(alp);
      clp = lFreeList(clp);
      ep = lFreeElem(ep);
      DEXIT;
      return NULL;
   }

   /* complain about unused configuration elements */
   if ((unused = lFirst(clp))) {
      ERROR((SGE_EVENT, MSG_SGETEXT_UNKNOWN_CONFIG_VALUE_SSS,
         lGetString(unused, CF_name), args->objname, fullname));
      lFreeList(clp);
      lFreeElem(ep);
      DEXIT;
      return NULL;
   }

   /* remove warnings in alp */
   alp = lFreeList(alp);

   DEXIT;
   return ep;
}


