
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

#include <string.h>

#include "sge.h"
#include "sgermon.h"
#include "sge_conf.h"
#include "sge_log.h"
#include "sge_gdi.h"
#include "sge_unistd.h"

#include "sge_answer.h"
#include "sge_centry_qconf.h"
#include "sge_centry.h"
#include "sge_object.h"
#include "sge_io.h"
#include "sge_edit.h"

#include "msg_common.h"
#include "msg_clients_common.h"

#ifndef QCONF_FLATFILE
#include "spool/classic/read_write_host_group.h"
#include "spool/classic/read_write_complex.h"
#include "spool/classic/read_write_centry.h"
#else
#include "spool/flatfile/sge_flatfile.h"
#include "spool/flatfile/sge_flatfile_obj.h"

static const spool_flatfile_instr ceqconf_ce_sfi = 
{
   NULL,
   false,
   true,
   true,
   false,
   true,
   '\0',
   ' ',
   '\0',
   '\0',
   '\n',
   NULL,
   { NoName, NoName, NoName }
};

static const spool_flatfile_instr ceqconf_sub_name_value_space_sfi = 
{
   NULL,
   false,
   false,
   false,
   false,
   false,
   '\0',
   '=',
   ' ',
   '\0',
   '\0',
   &ceqconf_sub_name_value_space_sfi,
   { NoName, NoName, NoName }
};

static const spool_flatfile_instr ceqconf_sfi = 
{
   NULL,
   true,
   false,
   false,
   true,
   false,
   ' ',
   '\n',
   '\0',
   '\0',
   '\0',
   &ceqconf_sub_name_value_space_sfi,
   { NoName, NoName, NoName }
};
#endif

bool 
centry_add_del_mod_via_gdi(lListElem *this_elem, lList **answer_list,
                           u_long32 gdi_command)
{
   bool ret = false;

   DENTER(TOP_LAYER, "centry_add_del_mod_via_gdi");
   if (this_elem != NULL) {
      lList *centry_list = NULL;
      lList *gdi_answer_list = NULL;

      centry_list = lCreateList("", CE_Type);
      lAppendElem(centry_list, this_elem);
      gdi_answer_list = sge_gdi(SGE_CENTRY_LIST, gdi_command,
                                &centry_list, NULL, NULL);
      answer_list_replace(answer_list, &gdi_answer_list);
   }
   DEXIT;
   return ret;
}

lListElem *
centry_get_via_gdi(lList **answer_list, const char *name) 
{
   lListElem *ret = NULL;

   DENTER(TOP_LAYER, "centry_get_via_gdi");
   if (name != NULL) {
      lList *gdi_answer_list = NULL;
      lEnumeration *what = NULL;
      lCondition *where = NULL;
      lList *centry_list = NULL;

      what = lWhat("%T(ALL)", CE_Type);
      where = lWhere("%T(%I==%s)", CE_Type, CE_name, name);
      gdi_answer_list = sge_gdi(SGE_CENTRY_LIST, SGE_GDI_GET, 
                                &centry_list, where, what);
      what = lFreeWhat(what);
      where = lFreeWhere(where);

      if (!answer_list_has_error(&gdi_answer_list)) {
         ret = lFirst(centry_list);
      } else {
         answer_list_replace(answer_list, &gdi_answer_list);
      }
   } 
   DEXIT;
   return ret;
}

bool 
centry_provide_modify_context(lListElem **this_elem, lList **answer_list)
{
   bool ret = false;
   int status = 0;
   lList *alp;
#ifdef QCONF_FLATFILE
   int fields_out[MAX_NUM_FIELDS];
   int missing_field = NoName;
#endif
   
   DENTER(TOP_LAYER, "centry_provide_modify_context");
   if (this_elem != NULL && *this_elem) {
      char *filename = NULL;

#ifdef QCONF_FLATFILE
      filename = (char *)spool_flatfile_write_object(&alp, *this_elem, false,
                                             CE_fields, &ceqconf_sfi, SP_DEST_TMP,
                                             SP_FORM_ASCII, filename, false);
      
      if (answer_list_output(&alp)) {
         DEXIT;
         SGE_EXIT (1);
      }
#else
      filename = write_centry(2, 1, *this_elem); 
#endif
 
      status = sge_edit(filename);
      if (status >= 0) {
         lListElem *centry;

#ifdef QCONF_FLATFILE
         fields_out[0] = NoName;
         centry = spool_flatfile_read_object(&alp, CE_Type, NULL,
                                             CE_fields, fields_out, true, &ceqconf_sfi,
                                             SP_FORM_ASCII, NULL, filename);
            
         if (answer_list_output (&alp)) {
            centry = lFreeElem (centry);
         }

         if (centry != NULL) {
            missing_field = spool_get_unprocessed_field (CE_fields, fields_out, &alp);
         }

         if (missing_field != NoName) {
            centry = lFreeElem (centry);
            answer_list_output (&alp);
         }      
#else
         centry = cull_read_in_centry(NULL, filename, 1, 0, NULL);
#endif
         if (centry != NULL) {
            *this_elem = lFreeElem(*this_elem);
            *this_elem = centry; 
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
   
   alp = lFreeList (alp);
   DEXIT;
   return ret;
}

bool 
centry_add(lList **answer_list, const char *name) 
{
   bool ret = true;

   DENTER(TOP_LAYER, "centry_add");
   if (name != NULL) {
      lListElem *centry = centry_create(answer_list, name);

      if (centry == NULL) {
         ret = false;
      }
      if (ret) {
         ret &= centry_provide_modify_context(&centry, answer_list);
      }
      if (ret) {
         ret &= centry_add_del_mod_via_gdi(centry, answer_list, SGE_GDI_ADD); 
      } 
   }  
  
   DEXIT;
   return ret; 
}

bool 
centry_add_from_file(lList **answer_list, const char *filename) 
{
   bool ret = true;
#ifdef QCONF_FLATFILE
   int fields_out[MAX_NUM_FIELDS];
   int missing_field = NoName;
#endif

   DENTER(TOP_LAYER, "centry_add_from_file");
   if (filename != NULL) {
      lListElem *centry;

#ifdef QCONF_FLATFILE
      fields_out[0] = NoName;
      centry = spool_flatfile_read_object(answer_list, CE_Type, NULL,
                                          CE_fields, fields_out, true, &ceqconf_sfi,
                                          SP_FORM_ASCII, NULL, filename);
            
      if (answer_list_output (answer_list)) {
         centry = lFreeElem (centry);
      }

      if (centry != NULL) {
         missing_field = spool_get_unprocessed_field (CE_fields, fields_out, answer_list);
      }

      if (missing_field != NoName) {
         centry = lFreeElem (centry);
         answer_list_output (answer_list);
      }      
#else
      centry = cull_read_in_centry(NULL, filename, 1, 0, NULL); 
#endif

      if (centry == NULL) {
         ret = false;
      }
      if (ret) {
         ret &= centry_add_del_mod_via_gdi(centry, answer_list, SGE_GDI_ADD); 
      } 
   }  
   
   DEXIT;
   return ret; 
}

bool 
centry_modify(lList **answer_list, const char *name)
{
   bool ret = true;

   DENTER(TOP_LAYER, "centry_modify");
   if (name != NULL) {
      lListElem *centry = centry_get_via_gdi(answer_list, name);

      if (centry == NULL) {
         sprintf(SGE_EVENT, MSG_CENTRY_DOESNOTEXIST_S, name);
         answer_list_add(answer_list, SGE_EVENT,
                         STATUS_ERROR1, ANSWER_QUALITY_ERROR);
         ret = false;
      }
      if (ret) {
         ret &= centry_provide_modify_context(&centry, answer_list);
      }
      if (ret) {
         ret &= centry_add_del_mod_via_gdi(centry, answer_list, SGE_GDI_MOD);
      }
      if (centry) {
         centry = lFreeElem(centry);
      }
   }

   DEXIT;
   return ret;
}

bool 
centry_modify_from_file(lList **answer_list, const char *filename)
{
   bool ret = true;
#ifdef QCONF_FLATFILE
   int fields_out[MAX_NUM_FIELDS];
   int missing_field = NoName;
#endif

   DENTER(TOP_LAYER, "centry_modify_from_file");
   if (filename != NULL) {
      lListElem *centry;

#ifdef QCONF_FLATFILE
      fields_out[0] = NoName;
      centry = spool_flatfile_read_object(answer_list, CE_Type, NULL,
                                          CE_fields, fields_out, true, &ceqconf_sfi,
                                          SP_FORM_ASCII, NULL, filename);
            
      if (answer_list_output (answer_list)) {
         centry = lFreeElem (centry);
      }

      if (centry != NULL) {
         missing_field = spool_get_unprocessed_field (CE_fields, fields_out, answer_list);
      }

      if (missing_field != NoName) {
         centry = lFreeElem (centry);
         answer_list_output (answer_list);
      }      
#else
      centry = cull_read_in_centry(NULL, filename, 1, 0, NULL); 
#endif

      if (centry == NULL) {
         sprintf(SGE_EVENT, MSG_CENTRY_FILENOTCORRECT_S, filename);
         answer_list_add(answer_list, SGE_EVENT,
                         STATUS_ERROR1, ANSWER_QUALITY_ERROR);
         ret = false;
      }
      if (ret) {
         ret &= centry_add_del_mod_via_gdi(centry, answer_list, SGE_GDI_MOD);
      }
      if (centry) {
         centry = lFreeElem(centry);
      }
   }
   
   DEXIT;
   return ret;
}

bool 
centry_delete(lList **answer_list, const char *name)
{
   bool ret = true;

   DENTER(TOP_LAYER, "centry_delete");
   if (name != NULL) {
      lListElem *centry = centry_create(answer_list, name); 
   
      if (centry != NULL) {
         ret &= centry_add_del_mod_via_gdi(centry, answer_list, SGE_GDI_DEL); 
      }
   }
   DEXIT;
   return ret;
}

bool 
centry_show(lList **answer_list, const char *name)
{
   bool ret = true;

   DENTER(TOP_LAYER, "centry_show");
   if (name != NULL) {
      lListElem *centry = centry_get_via_gdi(answer_list, name);
   
      if (centry != NULL) {
#ifdef QCONF_FLATFILE
         spool_flatfile_write_object(answer_list, centry, false, CE_fields,
                                     &ceqconf_sfi, SP_DEST_STDOUT, SP_FORM_ASCII,
                                     NULL, false);
      
      if (answer_list_output(answer_list)) {
         DEXIT;
         SGE_EXIT (1);
      }
#else
         write_centry(0, 0, centry);
#endif
         centry = lFreeElem(centry);
      } else {
         sprintf(SGE_EVENT, MSG_CENTRY_DOESNOTEXIST_S, name);
         answer_list_add(answer_list, SGE_EVENT,
                         STATUS_ERROR1, ANSWER_QUALITY_ERROR); 
         ret = false;
      }
   }
   DEXIT;
   return ret;
}

bool
centry_list_show(lList **answer_list) 
{
   bool ret = true;
   lList *centry_list = NULL;

   DENTER(TOP_LAYER, "centry_list_show");
   centry_list = centry_list_get_via_gdi(answer_list);
   if (centry_list != NULL) {
#ifdef QCONF_FLATFILE
      spool_flatfile_align_list(answer_list, (const lList *)centry_list,
                                CE_fields, 3);
      spool_flatfile_write_list(answer_list, centry_list, CE_fields,
                                &ceqconf_ce_sfi, SP_DEST_STDOUT, SP_FORM_ASCII, 
                                NULL, false);
      
      if (answer_list_output(answer_list)) {
         DEXIT;
         SGE_EXIT (1);
      }
#else
      write_cmplx(0, NULL, centry_list, stdout, NULL);
#endif
   }
   DEXIT;
   return ret;
}

lList *
centry_list_get_via_gdi(lList **answer_list)
{
   lList *ret = NULL;
   lList *gdi_answer_list = NULL;
   lEnumeration *what = NULL;

   DENTER(TOP_LAYER, "centry_list_get_via_gdi");
   what = lWhat("%T(ALL)", CE_Type);
   gdi_answer_list = sge_gdi(SGE_CENTRY_LIST, SGE_GDI_GET,
                             &ret, NULL, what);
   what = lFreeWhat(what);

   if (answer_list_has_error(&gdi_answer_list)) {
      answer_list_replace(answer_list, &gdi_answer_list);
   }

   centry_list_sort(ret);

   DEXIT;
   return ret;
}

bool
centry_list_add_del_mod_via_gdi(lList **this_list, lList **answer_list,
                                lList **old_list) 
{
   bool ret = true;

   DENTER(TOP_LAYER, "centry_list_add_del_mod_via_gdi");
   if (!this_list || !old_list) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_INAVLID_PARAMETER_IN_S, SGE_FUNC));
      answer_list_add(answer_list, SGE_EVENT, 
                      STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return false;
   }

   
   if (*this_list != NULL) {
      lList *modify_list = NULL;
      lList *add_list = NULL;
      lListElem *centry_elem = NULL;
      lListElem *next_centry_elem = NULL;
      bool cont = true;

      /* check for duplicate names */
      next_centry_elem = lFirst(*this_list);
      while ((centry_elem = next_centry_elem)) {
         lListElem *cmp_elem = lFirst(*this_list);
            
         while((centry_elem != cmp_elem)){
            const char *name1 = NULL;
            const char *name2 = NULL;
            const char *shortcut1 = NULL;
            const char *shortcut2 = NULL;

            /* Bugfix: Issuezilla 1161
             * Previously it was assumed that name and shortcut would never be
             * NULL.  In the course of testing for duplicate names, each name
             * would potentially be accessed twice.  Now, in order to check for
             * NULL without making a mess, I access each name exactly once.  In
             * some cases, that may be 50% more than the previous code, but in
             * what I expect is the usual case, it will be 50% less. */
            name1 = lGetString(centry_elem, CE_name);
            name2 = lGetString(cmp_elem, CE_name);
            shortcut1 = lGetString(centry_elem, CE_shortcut);
            shortcut2 = lGetString(cmp_elem, CE_shortcut);
            
            if (name1 == NULL) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                       ANSWER_QUALITY_ERROR,
                                       MSG_CENTRY_NULL_NAME);
               DEXIT;
               return false;
            }                  
            else if (name2 == NULL) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                       ANSWER_QUALITY_ERROR,
                                       MSG_CENTRY_NULL_NAME);
               DEXIT;
               return false;
            }                  
            else if (shortcut1 == NULL) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                       ANSWER_QUALITY_ERROR,
                                       MSG_CENTRY_NULL_SHORTCUT_S,
                                       name1);
               DEXIT;
               return false;
            }                  
            else if (shortcut2 == NULL) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                       ANSWER_QUALITY_ERROR,
                                       MSG_CENTRY_NULL_SHORTCUT_S,
                                       name2);
               DEXIT;
               return false;
            }                  
            else if ((strcmp (name1, name2) == 0) ||
                     (strcmp(name1, shortcut2) == 0) ||
                     (strcmp(shortcut1, name2) == 0) ||
                     (strcmp(shortcut1, shortcut2) == 0)) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                       ANSWER_QUALITY_ERROR,
                                       MSG_ANSWER_COMPLEXXALREADYEXISTS_S,
                                       name1);
               cont = false;
            } 
            
            cmp_elem = lNext(cmp_elem);
         }
         
         if (!centry_elem_validate(centry_elem, NULL, answer_list)){
            cont = false;
         }

         next_centry_elem = lNext(centry_elem);
      }
     
      if(!cont){
         DEXIT;
         return false;
      }

      modify_list = lCreateList("", CE_Type);
      add_list = lCreateList("", CE_Type);
      next_centry_elem = lFirst(*this_list);
      while ((centry_elem = next_centry_elem)) {
         const char *name = lGetString(centry_elem, CE_name);
         lListElem *tmp_elem = centry_list_locate(*old_list, name);

         next_centry_elem = lNext(centry_elem);
         if (tmp_elem != NULL) {
            lDechainElem(*this_list, centry_elem);
            if (object_has_differences(centry_elem, NULL, tmp_elem, false)) {
               lAppendElem(modify_list, centry_elem);
            }
            lRemoveElem(*old_list, tmp_elem);
         } else {
            lDechainElem(*this_list, centry_elem);
            lAppendElem(add_list, centry_elem);
         }
      }
      *this_list = lFreeList(*this_list);
      
      {
         lList *gdi_answer_list = NULL;
         lList *mal_answer_list = NULL;
         state_gdi_multi state = STATE_GDI_MULTI_INIT;
         int del_id = -1;
         int mod_id = -1;
         int add_id = -1; 
         int number_req = 0;
         bool do_del = false;
         bool do_mod = false;
         bool do_add = false;

         /*
          * How many requests are summarized to one multi request? 
          */
         if (lGetNumberOfElem(*old_list) > 0) {
            number_req++;
            do_del = true;
         }
         if (lGetNumberOfElem(modify_list) > 0) {
            number_req++;
            do_mod = true;
         }
         if (lGetNumberOfElem(add_list) > 0) {
            number_req++;
            do_add = true;
         }

         /*
          * Do the multi request
          */
         if (ret && do_del) {
            int mode = (--number_req > 0) ? SGE_GDI_RECORD : SGE_GDI_SEND;

            del_id = sge_gdi_multi(&gdi_answer_list, mode, 
                                   SGE_CENTRY_LIST, SGE_GDI_DEL, old_list,
                                   NULL, NULL, &mal_answer_list, &state, false);
            if (answer_list_has_error(&gdi_answer_list)) {
               DTRACE;
               ret = false;
            }
            if (*old_list != NULL) {
               *old_list = lFreeList(*old_list);
            }
         }
         if (ret && do_mod) {
            int mode = (--number_req > 0) ? SGE_GDI_RECORD : SGE_GDI_SEND;

            mod_id = sge_gdi_multi(&gdi_answer_list, mode, 
                                   SGE_CENTRY_LIST, SGE_GDI_MOD, &modify_list,
                                   NULL, NULL, &mal_answer_list, &state, false);
            if (answer_list_has_error(&gdi_answer_list)) {
               DTRACE;
               ret = false;
            }
            if (modify_list) {
               modify_list = lFreeList(modify_list);
            }
         }
         if (ret && do_add) {
            int mode = (--number_req > 0) ? SGE_GDI_RECORD : SGE_GDI_SEND;

            add_id = sge_gdi_multi(&gdi_answer_list, mode, 
                                   SGE_CENTRY_LIST, SGE_GDI_ADD, &add_list,
                                   NULL, NULL, &mal_answer_list, &state, false);
            if (answer_list_has_error(&gdi_answer_list)) {
               DTRACE;
               ret = false;
            }
            if (add_list != NULL){
               add_list = lFreeList(add_list);
            }
         }

         /*
          * Verify that the parts of the multi request are successfull
          */
         if (do_del) {
            gdi_answer_list = sge_gdi_extract_answer(SGE_GDI_DEL, 
                                                     SGE_CENTRY_LIST, del_id,
                                                     mal_answer_list, NULL);
            answer_list_append_list(answer_list, &gdi_answer_list);
         }
         if (do_mod) {
            gdi_answer_list = sge_gdi_extract_answer(SGE_GDI_MOD, 
                                                     SGE_CENTRY_LIST, mod_id,
                                                     mal_answer_list, NULL);
            answer_list_append_list(answer_list, &gdi_answer_list);
         }
         if (do_add) {
            gdi_answer_list = sge_gdi_extract_answer(SGE_GDI_ADD, 
                                                     SGE_CENTRY_LIST, add_id,
                                                     mal_answer_list, NULL);
            answer_list_append_list(answer_list, &gdi_answer_list);
         }

         /*
          * Provide an overall summary for the callee
          */
         if (lGetNumberOfElem(*answer_list) == 0) {
            answer_list_add_sprintf(answer_list, STATUS_OK, 
                                    ANSWER_QUALITY_INFO, 
                                    MSG_CENTRY_NOTCHANGED);
         }
      }
   }
   DEXIT;
   return ret;    
}

bool 
centry_list_modify(lList **answer_list)
{
   bool ret = true;

   DENTER(TOP_LAYER, "centry_list_modify");
   if (ret) {
      lList *centry_list = centry_list_get_via_gdi(answer_list);
      lList *old_centry_list = lCopyList("", centry_list);

      ret &= centry_list_provide_modify_context(&centry_list, answer_list);
      if (ret) {
         ret &= centry_list_add_del_mod_via_gdi(&centry_list, answer_list, &old_centry_list);  
      }
   }
   DEXIT;
   return ret;
}

bool 
centry_list_modify_from_file(lList **answer_list, const char *filename)
{
   bool ret = true;

   DENTER(TOP_LAYER, "centry_list_modify_from_file");
   if (ret) {
      lList *old_centry_list = NULL; 
#ifdef QCONF_FLATFILE
      lList *centry_list = NULL; 
      
      centry_list = spool_flatfile_read_list(answer_list, CE_Type, CE_fields,
                                             NULL, true, &ceqconf_ce_sfi,
                                             SP_FORM_ASCII, NULL, filename);
            
      if (answer_list_output (answer_list)) {
         centry_list = lFreeList (centry_list);
      }
#else
      lList *centry_list = read_cmplx(filename, "", answer_list); 
#endif

      old_centry_list = centry_list_get_via_gdi(answer_list); 

      if (centry_list == NULL) {
         answer_list_add_sprintf(answer_list, STATUS_ERROR1, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_CENTRY_FILENOTCORRECT_S, filename);
         ret = false;
      } 
      if (ret) { 
         ret &= centry_list_add_del_mod_via_gdi(&centry_list, answer_list, 
                                                &old_centry_list);  
      }
   }
   DEXIT;
   return ret;
}

bool 
centry_list_provide_modify_context(lList **this_list, 
                                   lList **answer_list)
{
   bool ret = false;
   int status = 0;
   
   DENTER(TOP_LAYER, "centry_list_provide_modify_context");
   if (this_list != NULL) {
      char filename[SGE_PATH_MAX] = "complex";

      sge_tmpnam(filename);
#ifdef QCONF_FLATFILE
      spool_flatfile_align_list(answer_list, (const lList *)*this_list,
                                CE_fields, 3);
      spool_flatfile_write_list(answer_list, *this_list, CE_fields,
                                &ceqconf_ce_sfi, SP_DEST_SPOOL,
                                SP_FORM_ASCII, filename, false);
      
      if (answer_list_output(answer_list)) {
         DEXIT;
         SGE_EXIT (1);
      }
#else
      write_cmplx(0, filename, *this_list, NULL, NULL); 
#endif
      status = sge_edit(filename);
      if (status >= 0) {
         lList *centry_list;

#ifdef QCONF_FLATFILE
         centry_list = spool_flatfile_read_list(answer_list, CE_Type, CE_fields,
                                                NULL, true, &ceqconf_ce_sfi,
                                                SP_FORM_ASCII, NULL, filename);
            
      if (answer_list_output (answer_list)) {
         centry_list = lFreeList (centry_list);
      }
#else
         centry_list = read_cmplx(filename, "", answer_list);
#endif
         if (centry_list != NULL) {
            *this_list = lFreeList(*this_list);
            *this_list = centry_list; 
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
