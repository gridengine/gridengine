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
#include "sge_object.h"
#include "sge_edit.h"
#include "sge_hgroup.h"
#include "sge_hgroup_qconf.h"
#include "sge_href.h"

#include "msg_common.h"
#include "msg_clients_common.h"

#ifndef QCONF_FLATFILE
#include "spool/classic/read_write_host_group.h"
#else
#include "spool/flatfile/sge_flatfile.h"
#include "spool/flatfile/sge_flatfile_obj.h"
#include "sgeobj/sge_hgroupL.h"

static const spool_flatfile_instr hgqconf_sub_name_value_space_sfi = 
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
   &hgqconf_sub_name_value_space_sfi,
   { NoName, NoName, NoName }
};

static const spool_flatfile_instr hgqconf_sfi = 
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
   &hgqconf_sub_name_value_space_sfi,
   { NoName, NoName, NoName }
};
#endif

static void 
hgroup_list_show_elem(lList *hgroup_list, const char *name, int indent);

static void 
hgroup_list_show_elem(lList *hgroup_list, const char *name, int indent)
{
   const char *const indent_string = "   ";
   lListElem *hgroup = NULL;
   int i;

   DENTER(TOP_LAYER, "hgroup_list_show_elem");

   for (i = 0; i < indent; i++) {
      printf("%s", indent_string);
   }
   printf("%s\n", name);

   hgroup = lGetElemHost(hgroup_list, HGRP_name, name);   
   if (hgroup != NULL) {
      lList *sub_list = lGetList(hgroup, HGRP_host_list);
      lListElem *href = NULL;

      for_each(href, sub_list) {
         const char *href_name = lGetHost(href, HR_name);

         hgroup_list_show_elem(hgroup_list, href_name, indent + 1); 
      } 
   } 
   DEXIT;
}

bool 
hgroup_add_del_mod_via_gdi(lListElem *this_elem, lList **answer_list,
                           u_long32 gdi_command)
{
   bool ret = false;

   DENTER(TOP_LAYER, "hgroup_add_del_mod_via_gdi");
   if (this_elem != NULL) {
      lList *hgroup_list = NULL;
      lList *gdi_answer_list = NULL;

      hgroup_list = lCreateList("", HGRP_Type);
      lAppendElem(hgroup_list, this_elem);
      gdi_answer_list = sge_gdi(SGE_HGROUP_LIST, gdi_command,
                                &hgroup_list, NULL, NULL);
      answer_list_replace(answer_list, &gdi_answer_list);
   }
   DEXIT;
   return ret;
}

lListElem *hgroup_get_via_gdi(lList **answer_list, const char *name) 
{
   lListElem *ret = NULL;

   DENTER(TOP_LAYER, "hgroup_get_via_gdi");
   if (name != NULL) {
      lList *gdi_answer_list = NULL;
      lEnumeration *what = NULL;
      lCondition *where = NULL;
      lList *houstgroup_list = NULL;

      what = lWhat("%T(ALL)", HGRP_Type);
      where = lWhere("%T(%I==%s)", HGRP_Type, HGRP_name, 
                     name);
      gdi_answer_list = sge_gdi(SGE_HGROUP_LIST, SGE_GDI_GET, 
                                &houstgroup_list, where, what);
      what = lFreeWhat(what);
      where = lFreeWhere(where);

      if (!answer_list_has_error(&gdi_answer_list)) {
         ret = lFirst(houstgroup_list);
      } else {
         answer_list_replace(answer_list, &gdi_answer_list);
      }
   } 
   DEXIT;
   return ret;
}

bool hgroup_provide_modify_context(lListElem **this_elem, lList **answer_list,
                                   bool ignore_unchanged_message)
{
   bool ret = false;
   int status = 0;
#ifdef QCONF_FLATFILE
   int fields_out[MAX_NUM_FIELDS];
   int missing_field = NoName;
#endif 
   
   DENTER(TOP_LAYER, "hgroup_provide_modify_context");
   if (this_elem != NULL && *this_elem) {
      char *filename = NULL;
#ifdef QCONF_FLATFILE
      filename = (char *)spool_flatfile_write_object(answer_list, *this_elem,
                                                     false, HGRP_fields,
                                                     &hgqconf_sfi,
                                                     SP_DEST_TMP, SP_FORM_ASCII,
                                                     filename, false);
      if (answer_list_output(answer_list)) {
         DEXIT;
         SGE_EXIT (1);
      }
#else
      filename = write_host_group(2, 1, *this_elem);
#endif 
      status = sge_edit(filename);
      
      if (status >= 0) {
         lListElem *hgroup;

#ifdef QCONF_FLATFILE
         fields_out[0] = NoName;
         hgroup = spool_flatfile_read_object(answer_list, HGRP_Type, NULL,
                                         HGRP_fields, fields_out, true, &hgqconf_sfi,
                                         SP_FORM_ASCII, NULL, filename);
            
         if (answer_list_output (answer_list)) {
            hgroup = lFreeElem (hgroup);
         }

         if (hgroup != NULL) {
            missing_field = spool_get_unprocessed_field (HGRP_fields, fields_out, answer_list);
         }

         if (missing_field != NoName) {
            hgroup = lFreeElem (hgroup);
            answer_list_output (answer_list);
         }      
#else
         hgroup = cull_read_in_host_group(NULL, filename, 1, 0, 0, NULL);
#endif

         if (hgroup != NULL) {
            if (object_has_differences(*this_elem, answer_list,
                                       hgroup, false) || 
                ignore_unchanged_message) {
               *this_elem = lFreeElem(*this_elem);
               *this_elem = hgroup; 
               ret = true;
            } else {
               answer_list_add(answer_list, MSG_FILE_NOTCHANGED,
                               STATUS_ERROR1, ANSWER_QUALITY_ERROR);
            }
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

bool hgroup_add(lList **answer_list, const char *name) 
{
   bool ret = true;

   DENTER(TOP_LAYER, "hgroup_add");
   if (name != NULL) {
      lListElem *hgroup = hgroup_create(answer_list, name, NULL);

      if (hgroup == NULL) {
         ret = false;
      }
      if (ret) {
         ret &= hgroup_provide_modify_context(&hgroup, answer_list, true);
      }
      if (ret) {
         ret &= hgroup_add_del_mod_via_gdi(hgroup, answer_list, 
                                              SGE_GDI_ADD); 
      } 
   }  
  
   DEXIT;
   return ret; 
}

bool hgroup_add_from_file(lList **answer_list, const char *filename) 
{
   bool ret = true;
#ifdef QCONF_FLATFILE
   int fields_out[MAX_NUM_FIELDS];
   int missing_field = NoName;
#endif 

   DENTER(TOP_LAYER, "hgroup_add");
   if (filename != NULL) {
      lListElem *hgroup;

#ifdef QCONF_FLATFILE
      fields_out[0] = NoName;
      hgroup = spool_flatfile_read_object(answer_list, HGRP_Type, NULL,
                                      HGRP_fields, fields_out, true, &hgqconf_sfi,
                                      SP_FORM_ASCII, NULL, filename);
            
      if (answer_list_output (answer_list)) {
         hgroup = lFreeElem (hgroup);
      }

      if (hgroup != NULL) {
         missing_field = spool_get_unprocessed_field (HGRP_fields, fields_out, answer_list);
      }

      if (missing_field != NoName) {
         hgroup = lFreeElem (hgroup);
         answer_list_output (answer_list);
      }      
#else
      hgroup = cull_read_in_host_group(NULL, filename, 1, 0, 0, NULL);
#endif
      if (hgroup == NULL) {
         ret = false;
      }
      if (ret) {
         ret &= hgroup_add_del_mod_via_gdi(hgroup, answer_list, SGE_GDI_ADD); 
      } 
   }  
  
   DEXIT;
   return ret; 
}

bool hgroup_modify(lList **answer_list, const char *name)
{
   bool ret = true;

   DENTER(TOP_LAYER, "hgroup_modify");
   if (name != NULL) {
      lListElem *hgroup = hgroup_get_via_gdi(answer_list, name);

      if (hgroup == NULL) {
         sprintf(SGE_EVENT, MSG_HGROUP_NOTEXIST_S, name);
         answer_list_add(answer_list, SGE_EVENT,
                         STATUS_ERROR1, ANSWER_QUALITY_ERROR);
         ret = false;
      }
      if (ret) {
         ret &= hgroup_provide_modify_context(&hgroup, answer_list, false);
      }
      if (ret) {
         ret &= hgroup_add_del_mod_via_gdi(hgroup, answer_list, SGE_GDI_MOD);
      }
      if (hgroup) {
         hgroup = lFreeElem(hgroup);
      }
   }

   DEXIT;
   return ret;
}

bool hgroup_modify_from_file(lList **answer_list, const char *filename)
{
   bool ret = true;
#ifdef QCONF_FLATFILE
   int fields_out[MAX_NUM_FIELDS];
   int missing_field = NoName;
#endif 

   DENTER(TOP_LAYER, "hgroup_modify");
   if (filename != NULL) {
      lListElem *hgroup;

#ifdef QCONF_FLATFILE
      fields_out[0] = NoName;
      hgroup = spool_flatfile_read_object(answer_list, HGRP_Type, NULL,
                                      HGRP_fields, fields_out, true, &hgqconf_sfi,
                                      SP_FORM_ASCII, NULL, filename);
            
      if (answer_list_output (answer_list)) {
         hgroup = lFreeElem (hgroup);
      }

      if (hgroup != NULL) {
         missing_field = spool_get_unprocessed_field (HGRP_fields, fields_out, answer_list);
      }

      if (missing_field != NoName) {
         hgroup = lFreeElem (hgroup);
         answer_list_output (answer_list);
      }      
#else
      hgroup = cull_read_in_host_group(NULL, filename, 1, 0, 0, NULL);
#endif
      if (hgroup == NULL) {
         sprintf(SGE_EVENT, MSG_HGROUP_FILEINCORRECT_S, filename);
         answer_list_add(answer_list, SGE_EVENT,
                         STATUS_ERROR1, ANSWER_QUALITY_ERROR);
         ret = false;
      }
      if (ret) {
         ret &= hgroup_add_del_mod_via_gdi(hgroup, answer_list, SGE_GDI_MOD);
      }
      if (hgroup) {
         hgroup = lFreeElem(hgroup);
      }
   }

   DEXIT;
   return ret;
}

bool hgroup_delete(lList **answer_list, const char *name)
{
   bool ret = true;

   DENTER(TOP_LAYER, "hgroup_delete");
   if (name != NULL) {
      lListElem *hgroup = hgroup_create(answer_list, name, NULL); 
   
      if (hgroup != NULL) {
         ret &= hgroup_add_del_mod_via_gdi(hgroup, answer_list, SGE_GDI_DEL); 
      }
   }
   DEXIT;
   return ret;
}

bool hgroup_show(lList **answer_list, const char *name)
{
   bool ret = true;

   DENTER(TOP_LAYER, "hgroup_show");
   if (name != NULL) {
      lListElem *hgroup = hgroup_get_via_gdi(answer_list, name); 
   
      if (hgroup != NULL) {
#ifdef QCONF_FLATFILE
         spool_flatfile_write_object(answer_list, hgroup, false, HGRP_fields,
                                     &hgqconf_sfi, SP_DEST_STDOUT,
                                     SP_FORM_ASCII, NULL, false);
      
      if (answer_list_output(answer_list)) {
         DEXIT;
         SGE_EXIT (1);
      }
#else
         write_host_group(0, 0, hgroup);
#endif 
         hgroup = lFreeElem(hgroup);
      } else {
         sprintf(SGE_EVENT, MSG_HGROUP_NOTEXIST_S, name);
         answer_list_add(answer_list, SGE_EVENT,
                         STATUS_ERROR1, ANSWER_QUALITY_ERROR); 
         ret = false;
      }
   }
   DEXIT;
   return ret;
}

bool hgroup_show_structure(lList **answer_list, const char *name, 
                           bool show_tree)
{
   bool ret = true;

   DENTER(TOP_LAYER, "hgroup_show_tree");
   if (name != NULL) {
      lList *hgroup_list = NULL;
      lListElem *hgroup = NULL;
      lEnumeration *what = NULL;
      lList *alp = NULL;
      lListElem *alep = NULL;

      what = lWhat("%T(ALL)", HGRP_Type);
      alp = sge_gdi(SGE_HGROUP_LIST, SGE_GDI_GET, &hgroup_list, NULL, what);
      what = lFreeWhat(what);

      alep = lFirst(alp);
      answer_exit_if_not_recoverable(alep);
      if (answer_get_status(alep) != STATUS_OK) {
         fprintf(stderr, "%s\n", lGetString(alep, AN_text));
         return 0;
      }

      hgroup = lGetElemHost(hgroup_list, HGRP_name, name); 
      if (hgroup != NULL) {
         if (show_tree) {
            hgroup_list_show_elem(hgroup_list, name, 0);
         } else {
            dstring string = DSTRING_INIT;
            lList *sub_host_list = NULL;
            lList *sub_hgroup_list = NULL;

            hgroup_find_all_references(hgroup, answer_list, hgroup_list, 
                                       &sub_host_list, &sub_hgroup_list);
            href_list_make_uniq(sub_host_list, answer_list);
            href_list_append_to_dstring(sub_host_list, &string);
            if (sge_dstring_get_string(&string)) {
               printf("%s\n", sge_dstring_get_string(&string));
            }
            sge_dstring_free(&string);
         }
      } else {
         sprintf(SGE_EVENT, MSG_HGROUP_NOTEXIST_S, name);
         answer_list_add(answer_list, SGE_EVENT,
                         STATUS_ERROR1, ANSWER_QUALITY_ERROR); 
         ret = false;
      }
   }
   DEXIT;
   return ret;
}
