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

#include "sge.h"
#include "sgermon.h"
#include "sge_conf.h"
#include "sge_log.h"
#include "sge_gdi.h"
#include "sge_unistd.h"

#include "sge_answer.h"
#include "sge_cuser.h"
#include "sge_cuser_qconf.h"
#include "parse_qconf.h"
#include "spool/classic/read_write_ume.h"

#include "msg_common.h"

#ifndef __SGE_NO_USERMAPPING__

bool 
cuser_add_del_mod_via_gdi(lListElem *this_elem, lList **answer_list,
                          u_long32 gdi_command)
{
   bool ret = false;

   DENTER(TOP_LAYER, "cuser_add_del_mod_via_gdi");
   if (this_elem != NULL) {
      lList *cuser_list = NULL;
      lList *gdi_answer_list = NULL;

      cuser_list = lCreateList("", CU_Type);
      lAppendElem(cuser_list, this_elem);
      gdi_answer_list = sge_gdi(SGE_USER_MAPPING_LIST, gdi_command,
                                &cuser_list, NULL, NULL);
      answer_list_replace(answer_list, &gdi_answer_list);
   }
   DEXIT;
   return ret;
}

lListElem *cuser_get_via_gdi(lList **answer_list, const char *name) 
{
   lListElem *ret = NULL;

   DENTER(TOP_LAYER, "cuser_get_via_gdi");
   if (name != NULL) {
      lList *gdi_answer_list = NULL;
      lEnumeration *what = NULL;
      lCondition *where = NULL;
      lList *cuser_list = NULL;

      what = lWhat("%T(ALL)", CU_Type);
      where = lWhere("%T(%I==%s)", CU_Type, CU_name, name);
      gdi_answer_list = sge_gdi(SGE_USER_MAPPING_LIST, SGE_GDI_GET, 
                                &cuser_list, where, what);
      what = lFreeWhat(what);
      where = lFreeWhere(where);

      if (!answer_list_has_error(&gdi_answer_list)) {
         ret = lFirst(cuser_list);
      } else {
         answer_list_replace(answer_list, &gdi_answer_list);
      }
   } 
   DEXIT;
   return ret;
}

bool cuser_provide_modify_context(lListElem **this_elem, lList **answer_list)
{
   bool ret = false;
   int status = 0;
   
   DENTER(TOP_LAYER, "cuser_provide_modify_context");
   if (this_elem != NULL && *this_elem) {
      char *filename = write_ume(2, 1, *this_elem); 
 
      status = sge_edit(filename);
      if (status >= 0) {
         lListElem *cuser;

         cuser = cull_read_in_ume(NULL, filename, 1, 0, 0, NULL);
         if (cuser != NULL) {
            *this_elem = lFreeElem(*this_elem);
            *this_elem = cuser; 
            ret = true;
         } else {
            answer_list_add(answer_list, MSG_FILE_ERRORREADINGINFILE,
                            STATUS_ERROR1, ANSWER_QUALITY_ERROR);
         }
      } else {
         answer_list_add(answer_list, MSG_PARSE_EDITFAILED,
                         STATUS_ERROR1, ANSWER_QUALITY_ERROR);
      }
      unlink(filename);
   } 
   DEXIT;
   return ret;
}

bool cuser_add(lList **answer_list, const char *name) 
{
   bool ret = true;

   DENTER(TOP_LAYER, "cuser_add");
   if (name != NULL) {
      lListElem *cuser = cuser_create(answer_list, name, NULL);

      if (cuser == NULL) {
         ret = false;
      }
      if (ret) {
         ret &= cuser_provide_modify_context(&cuser, answer_list);
      }
      if (ret) {
         ret &= cuser_add_del_mod_via_gdi(cuser, answer_list, SGE_GDI_ADD); 
      } 
   }  
  
   DEXIT;
   return ret; 
}

bool cuser_add_from_file(lList **answer_list, const char *filename) 
{
   bool ret = true;

   DENTER(TOP_LAYER, "cuser_add_from_file");
   if (filename != NULL) {
      lListElem *cuser;

      cuser = cull_read_in_ume(NULL, filename, 1, 0, 0, NULL); 
      if (cuser == NULL) {
         ret = false;
      }
      if (ret) {
         ret &= cuser_add_del_mod_via_gdi(cuser, answer_list, SGE_GDI_ADD); 
      } 
   }  
  
   DEXIT;
   return ret; 
}

bool cuser_modify(lList **answer_list, const char *name)
{
   bool ret = true;

   DENTER(TOP_LAYER, "cuser_modify");
   if (name != NULL) {
      lListElem *cuser = cuser_get_via_gdi(answer_list, name);

      if (cuser == NULL) {
         /* EB: TODO move to msg file */
         sprintf(SGE_EVENT, "User mapping entry "SFQ" does not exist\n", name);
         answer_list_add(answer_list, SGE_EVENT,
                         STATUS_ERROR1, ANSWER_QUALITY_ERROR);
         ret = false;
      }
      if (ret) {
         ret &= cuser_provide_modify_context(&cuser, answer_list);
      }
      if (ret) {
         ret &= cuser_add_del_mod_via_gdi(cuser, answer_list, SGE_GDI_MOD);
      }
      if (cuser) {
         cuser = lFreeElem(cuser);
      }
   }

   DEXIT;
   return ret;
}

bool cuser_modify_from_file(lList **answer_list, const char *filename)
{
   bool ret = true;

   DENTER(TOP_LAYER, "cuser_modify_from_file");
   if (filename != NULL) {
      lListElem *cuser;

      cuser = cull_read_in_ume(NULL, filename, 1, 0, 0, NULL); 
      if (cuser == NULL) {
         /* EB: TODO move to msg file */
         sprintf(SGE_EVENT, "User mapping file "SFQ" is not correct\n", filename);
         answer_list_add(answer_list, SGE_EVENT,
                         STATUS_ERROR1, ANSWER_QUALITY_ERROR);
         ret = false;
      }
      if (ret) {
         ret &= cuser_add_del_mod_via_gdi(cuser, answer_list, SGE_GDI_MOD);
      }
      if (cuser != NULL) {
         cuser = lFreeElem(cuser);
      }
   }

   DEXIT;
   return ret;
}

bool cuser_delete(lList **answer_list, const char *name)
{
   bool ret = true;

   DENTER(TOP_LAYER, "cuser_delete");
   if (name != NULL) {
      lListElem *cuser = cuser_create(answer_list, name, NULL); 
   
      if (cuser != NULL) {
         ret &= cuser_add_del_mod_via_gdi(cuser, answer_list, SGE_GDI_DEL); 
      }
   }
   DEXIT;
   return ret;
}

bool cuser_show(lList **answer_list, const char *name)
{
   bool ret = true;

   DENTER(TOP_LAYER, "cuser_show");
   if (name != NULL) {
      lListElem *cuser = cuser_get_via_gdi(answer_list, name); 
   
      if (cuser != NULL) {
         write_ume(0, 0, cuser);
         cuser = lFreeElem(cuser);
      } else {
         /* EB: TODO move to msg file */
         sprintf(SGE_EVENT, "Cluster user "SFQ" does not exist\n", name);
         answer_list_add(answer_list, SGE_EVENT,
                         STATUS_ERROR1, ANSWER_QUALITY_ERROR); 
         ret = false;
      }
   }
   DEXIT;
   return ret;
}

#endif
