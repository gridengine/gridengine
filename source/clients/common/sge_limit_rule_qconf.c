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
 
#include "sge_limit_rule_qconf.h"
#include "msg_common.h"
#include "msg_clients_common.h"

#include "uti/sge_log.h"
#include "uti/sge_edit.h"
#include "rmon/sgermon.h"
#include "gdi/sge_gdi.h"

#include "sgeobj/sge_limit_rule.h"
#include "sgeobj/sge_answer.h"
#include "spool/flatfile/sge_flatfile.h"
#include "spool/flatfile/sge_flatfile_obj.h"

/****** sge_limit_rule_qconf/limit_rule_show() *********************************
*  NAME
*     limit_rule_show() -- show limitation rule sets
*
*  SYNOPSIS
*     bool limit_rule_show(lList **answer_list, const char *name) 
*
*  FUNCTION
*     This funtion gets the selected limitation rule sets from GDI and
*     writes they out on stdout
*
*  INPUTS
*     lList **answer_list - answer list
*     const char *name    - comma separated list of limitation rule set names
*
*  RESULT
*     bool - true  on success
*            false on error
*
*  NOTES
*     MT-NOTE: limit_rule_show() is MT safe 
*
*******************************************************************************/
bool limit_rule_show(lList **answer_list, const char *name)
{
   lList *lirs_list = NULL;
   const char* filename = NULL;
   bool ret = false;

   DENTER(TOP_LAYER, "limit_rule_show");

   if (name != NULL) {
      lList *lirsref_list = NULL;

      lString2List(name, &lirsref_list, LIRS_Type, LIRS_name, ", ");
      ret = limit_rule_get_via_gdi(answer_list, lirsref_list, &lirs_list);
      lFreeList(&lirsref_list);
   } else {
      ret = limit_rule_get_all_via_gdi(answer_list, &lirs_list);
   }

   if (ret && lGetNumberOfElem(lirs_list)) {
      spooling_field *fields = sge_build_LIRS_field_list(false, true);
      filename = spool_flatfile_write_list(answer_list, lirs_list, fields, 
                                        &qconf_limit_rule_set_sfi,
                                        SP_DEST_STDOUT, SP_FORM_ASCII, NULL,
                                        false);
      FREE(filename);
      FREE(fields);
   }
   if (lGetNumberOfElem(lirs_list) == 0) {
      answer_list_add(answer_list, MSG_NOLIRSFOUND, STATUS_EEXIST, ANSWER_QUALITY_WARNING);
      ret = false;
   }

   lFreeList(&lirs_list);

   DRETURN(ret);
}

/****** sge_limit_rule_qconf/limit_rule_get_via_gdi() **************************
*  NAME
*     limit_rule_get_via_gdi() -- get limitation rule sets from GDI
*
*  SYNOPSIS
*     bool limit_rule_get_via_gdi(lList **answer_list, const lList 
*     *lirsref_list, lList **lirs_list) 
*
*  FUNCTION
*     This function gets the selected limitation rule sets from qmaster. The selection
*     is done in the string list lirsref_list.
*
*  INPUTS
*     lList **answer_list       - answer list
*     const lList *lirsref_list - limitation rule sets selection
*     lList **lirs_list         - copy of the selected rule sets
*
*  RESULT
*     bool - true  on success
*            false on error
*
*  NOTES
*     MT-NOTE: limit_rule_get_via_gdi() is MT safe 
*
*******************************************************************************/
bool limit_rule_get_via_gdi(lList **answer_list, const lList *lirsref_list,
                            lList **lirs_list)
{
   bool ret = false;

   DENTER(TOP_LAYER, "limit_rule_get_via_gdi");
   if (lirsref_list != NULL) {
      lListElem *lirsref = NULL;
      lCondition *where = NULL;
      lEnumeration *what = NULL;

      what = lWhat("%T(ALL)", LIRS_Type);

      for_each(lirsref, lirsref_list) {
         lCondition *add_where = NULL;
         add_where = lWhere("%T(%I p= %s)", LIRS_Type, LIRS_name, lGetString(lirsref, LIRS_name));
         if (where == NULL) {
            where = add_where;
         } else {
            where = lOrWhere(where, add_where);
         }
      }

      *answer_list = sge_gdi(SGE_LIRS_LIST, SGE_GDI_GET, lirs_list, where, what);
      if (!answer_list_has_error(answer_list)) {
         ret = true;
      }

      lFreeWhat(&what);
      lFreeWhere(&where);
   }

   DRETURN(ret);
}

/****** sge_limit_rule_qconf/limit_rule_get_all_via_gdi() **********************
*  NAME
*     limit_rule_get_all_via_gdi() -- get all limiation rule sets from GDI 
*
*  SYNOPSIS
*     bool limit_rule_get_all_via_gdi(lList **answer_list, lList **lirs_list) 
*
*  FUNCTION
*     This function gets all limiation rule sets known by qmaster
*
*  INPUTS
*     lList **answer_list - answer list
*     lList **lirs_list   - copy of all limiation rule sets
*
*  RESULT
*     bool - true  on success
*            false on error
*
*  NOTES
*     MT-NOTE: limit_rule_get_all_via_gdi() is MT safe 
*
*******************************************************************************/
bool limit_rule_get_all_via_gdi(lList **answer_list, lList **lirs_list)
{
   bool ret = false;
   lEnumeration *what = lWhat("%T(ALL)", LIRS_Type);

   DENTER(TOP_LAYER, "limit_rule_get_all_via_gdi");

   *answer_list = sge_gdi(SGE_LIRS_LIST, SGE_GDI_GET, lirs_list, NULL, what);
   if (!answer_list_has_error(answer_list)) {
      ret = true;
   }

   lFreeWhat(&what);

   DRETURN(ret);
}

/****** sge_limit_rule_qconf/limit_rule_set_add() ******************************
*  NAME
*     limit_rule_set_add() -- add limitation rule sets
*
*  SYNOPSIS
*     bool limit_rule_set_add(lList **answer_list, const char *name) 
*
*  FUNCTION
*     This function provide a modify context for qconf to add new limitation
*     rule sets. If no name is given a template rule set is shown
*
*  INPUTS
*     lList **answer_list - answer list
*     const char *name    - comma seperated list of rule sets to add
*
*  RESULT
*     bool - true  on success
*            false on error
*
*  NOTES
*     MT-NOTE: limit_rule_set_add() is MT safe 
*
*******************************************************************************/
bool limit_rule_set_add(lList **answer_list, const char *name)
{
   bool ret = false;

   DENTER(TOP_LAYER, "limit_rule_set_add");
   if (name != NULL) {
      lList *lirs_list = NULL;
      lListElem *lirs = NULL;

      lString2List(name, &lirs_list, LIRS_Type, LIRS_name, ", ");
      for_each (lirs, lirs_list) {
         lirs = limit_rule_set_defaults(lirs);
      }

      ret = limit_rule_set_provide_modify_context(&lirs_list, answer_list, true);

      if (ret) {
         ret = limit_rule_set_add_del_mod_via_gdi(lirs_list, answer_list, SGE_GDI_ADD | SGE_GDI_SET_ALL);
      }

      lFreeList(&lirs_list);
   }

   DRETURN(ret);
}

/****** sge_limit_rule_qconf/limit_rule_set_modify() ***************************
*  NAME
*     limit_rule_set_modify() -- modify limitation rule sets
*
*  SYNOPSIS
*     bool limit_rule_set_modify(lList **answer_list, const char *name) 
*
*  FUNCTION
*     This function provides a modify context for qconf to modify limitation
*     rule sets.
*
*  INPUTS
*     lList **answer_list - answer list
*     const char *name    - comma seperated list of rule sets to modify
*
*  RESULT
*     bool - true  on success
*            false on error
*
*  NOTES
*     MT-NOTE: limit_rule_set_modify() is MT safe 
*
*******************************************************************************/
bool limit_rule_set_modify(lList **answer_list, const char *name)
{
   bool ret = false;
   lList *lirs_list = NULL;

   DENTER(TOP_LAYER, "limit_rule_set_modify");

   if (name != NULL) {
      lList *lirsref_list = NULL;

      lString2List(name, &lirsref_list, LIRS_Type, LIRS_name, ", ");
      ret = limit_rule_get_via_gdi(answer_list, lirsref_list, &lirs_list);
      lFreeList(&lirsref_list);
   } else {
      ret = limit_rule_get_all_via_gdi(answer_list, &lirs_list);
   }

   if (ret) {
      ret = limit_rule_set_provide_modify_context(&lirs_list, answer_list, false);
   }
   if (ret) {
      ret = limit_rule_set_add_del_mod_via_gdi(lirs_list, answer_list,
                                       SGE_GDI_MOD | SGE_GDI_SET_ALL);
   }

   lFreeList(&lirs_list);

   DRETURN(ret);
}

/****** sge_limit_rule_qconf/limit_rule_set_add_from_file() ********************
*  NAME
*     limit_rule_set_add_from_file() -- add limitation rule set from file
*
*  SYNOPSIS
*     bool limit_rule_set_add_from_file(lList **answer_list, const char 
*     *filename) 
*
*  FUNCTION
*     This function add new limitation rule sets from file.
*
*  INPUTS
*     lList **answer_list  - answer list
*     const char *filename - filename of new limitation rule sets
*
*  RESULT
*     bool - true  on success
*            false on error
*
*  NOTES
*     MT-NOTE: limit_rule_set_add_from_file() is MT safe 
*
*******************************************************************************/
bool limit_rule_set_add_from_file(lList **answer_list, const char *filename)
{
   bool ret = false;

   DENTER(TOP_LAYER, "cqueue_add_from_file");
   if (filename != NULL) {
      lList *lirs_list = NULL;
      spooling_field *fields = sge_build_LIRS_field_list(false, true);

      /* fields_out field does not work for lirs because of duplicate entry */
      lirs_list = spool_flatfile_read_list(answer_list, LIRS_Type, fields,
                                          NULL, true, &qconf_limit_rule_set_sfi,
                                          SP_FORM_ASCII, NULL, filename);
      if (lirs_list != NULL) {
      #if 0
         /* because fields_out does not work we verify the lirs_list explicit */
         if (limit_rule_sets_verify_attributes(lirs_list, answer_list, false)) {
      #endif
            ret = limit_rule_set_add_del_mod_via_gdi(lirs_list, answer_list, 
                                           SGE_GDI_ADD | SGE_GDI_SET_ALL); 
      #if 0                                       
         } 
      #endif      
      }

      lFreeList(&lirs_list);
      FREE(fields);
   }
   DRETURN(ret);
}

/****** sge_limit_rule_qconf/limit_rule_set_provide_modify_context() ***********
*  NAME
*     limit_rule_set_provide_modify_context() -- provide qconf modify context
*
*  SYNOPSIS
*     bool limit_rule_set_provide_modify_context(lList **lirs_list, lList 
*     **answer_list, bool ignore_unchanged_message) 
*
*  FUNCTION
*     This function provides a editor session to edit the selected limitation rule
*     sets interactively. 
*
*  INPUTS
*     lList **lirs_list             - limitation rule sets to modify
*     lList **answer_list           - answer list
*     bool ignore_unchanged_message - ignore unchanged message
*
*  RESULT
*     bool - true  on success
*            false on error
*
*  NOTES
*     MT-NOTE: limit_rule_set_provide_modify_context() is MT safe 
*
*******************************************************************************/
bool limit_rule_set_provide_modify_context(lList **lirs_list, lList **answer_list,
                                           bool ignore_unchanged_message)
{
   bool ret = false;
   
   DENTER(TOP_LAYER, "limit_rule_set_provide_modify_context");

   if (lirs_list != NULL && *lirs_list) {
      int status = 0;
      const char *filename = NULL;
      spooling_field *fields = sge_build_LIRS_field_list(false, true);
      
      filename = spool_flatfile_write_list(answer_list, *lirs_list, fields,
                                           &qconf_limit_rule_set_sfi, SP_DEST_TMP,
                                           SP_FORM_ASCII, filename, false);
      if (answer_list_output(answer_list)) {
         unlink(filename);
         FREE(filename);
         FREE(fields);
         DEXIT;
         SGE_EXIT(1);
      }
      status = sge_edit(filename);
      if (status >= 0) {
         lList *new_lirs_list = NULL;

         /* fields_out field does not work for lirs because of duplicate entry */
         new_lirs_list = spool_flatfile_read_list(answer_list, LIRS_Type, fields,
                                                  NULL, true, &qconf_limit_rule_set_sfi,
                                                  SP_FORM_ASCII, NULL, filename);
         if (answer_list_output(answer_list)) {
            lFreeList(&new_lirs_list);
         }
         #if 0
         /* because fields_out does not work we verify the lirs_list explicit */
         if (!limit_rule_sets_verify_attributes(new_lirs_list, answer_list, false)) {
            lFreeList(&new_lirs_list);
         } 
         #endif 
         if (new_lirs_list != NULL) {
            if (ignore_unchanged_message || object_list_has_differences(new_lirs_list, answer_list, *lirs_list, false)) {
               lFreeList(lirs_list);
               *lirs_list = new_lirs_list;
               ret = true;
            } else {
               lFreeList(&new_lirs_list);
               answer_list_add(answer_list, MSG_FILE_NOTCHANGED,
                               STATUS_ERROR1, ANSWER_QUALITY_ERROR);
            }
         } else {
            answer_list_add(answer_list, MSG_FILE_ERRORREADINGINFILE,
                            STATUS_ERROR1, ANSWER_QUALITY_ERROR);
         }
      } else{
         answer_list_add(answer_list, MSG_PARSE_EDITFAILED,
                         STATUS_ERROR1, ANSWER_QUALITY_ERROR);
      }

      unlink(filename);
      FREE(filename);
      FREE(fields);
   }
   DRETURN(ret);
}

/****** sge_limit_rule_qconf/limit_rule_set_add_del_mod_via_gdi() **************
*  NAME
*     limit_rule_set_add_del_mod_via_gdi() -- modfies qmaster limitation rule sets
*
*  SYNOPSIS
*     bool limit_rule_set_add_del_mod_via_gdi(lList *lirs_list, lList 
*     **answer_list, u_long32 gdi_command) 
*
*  FUNCTION
*     This function modifies via GDI the qmaster copy of the limitation rule sets.
*
*  INPUTS
*     lList *lirs_list     - limitation rule sets to modify on qmaster
*     lList **answer_list  - answer list from qmaster
*     u_long32 gdi_command - commands what to do
*
*  RESULT
*     bool - true  on success
*            false on error
*
*  NOTES
*     MT-NOTE: limit_rule_set_add_del_mod_via_gdi() is MT safe 
*
*******************************************************************************/
bool limit_rule_set_add_del_mod_via_gdi(lList *lirs_list, lList **answer_list,
                                        u_long32 gdi_command) 
{
   bool ret = false;
   
   DENTER(TOP_LAYER, "limit_rule_set_add_del_mod_via_gdi");

   if (lirs_list != NULL) {
      u_long32 operation = SGE_GDI_GET_OPERATION(gdi_command);
      bool do_verify = (operation == SGE_GDI_MOD) || (operation == SGE_GDI_ADD) ? true : false;

      if (do_verify) {
         ret = limit_rule_sets_verify_attributes(lirs_list, answer_list, false);
      }
      if (ret) {
         *answer_list = sge_gdi(SGE_LIRS_LIST, gdi_command, &lirs_list, NULL, NULL);
      }
   }
   DRETURN(ret);
}

/****** sge_limit_rule_qconf/limit_rule_set_modify_from_file() *****************
*  NAME
*     limit_rule_set_modify_from_file() -- modifies limitatin rule sets from file
*
*  SYNOPSIS
*     bool limit_rule_set_modify_from_file(lList **answer_list, const char 
*     *filename, const char* name) 
*
*  FUNCTION
*     This function allows to modify one or all limitation rule sets from a file
*
*  INPUTS
*     lList **answer_list  - answer list
*     const char *filename - filename with the limitati rule sets to change
*     const char* name     - comma separated list of rule sets to change
*
*  RESULT
*     bool - true  on success
*            false on error
*
*  NOTES
*     MT-NOTE: limit_rule_set_modify_from_file() is MT safe 
*
*******************************************************************************/
bool limit_rule_set_modify_from_file(lList **answer_list, const char *filename, const char* name) 
{
   bool ret = false;
   
   DENTER(TOP_LAYER, "limit_rule_set_modify_from_file");
   if (filename != NULL) {
      lList *lirs_list = NULL;

      spooling_field *fields = sge_build_LIRS_field_list(false, true);
      /* fields_out field does not work for lirs because of duplicate entry */
      lirs_list = spool_flatfile_read_list(answer_list, LIRS_Type, fields,
                                          NULL, true, &qconf_limit_rule_set_sfi,
                                          SP_FORM_ASCII, NULL, filename);
      if (lirs_list != NULL) {
      #if 0 
         /* because fields_out does not work we verify the lirs_list explicit */
         if (!limit_rule_sets_verify_attributes(lirs_list, answer_list, false)) {
            lFreeList(&lirs_list);
            ret = false;
         }
      #endif

         if (ret && name != NULL) {
            lList *selected_lirs_list = NULL;
            lListElem *tmp_lirs = NULL;
            lList *found_lirs_list = lCreateList("lirs_list", LIRS_Type);

            lString2List(name, &selected_lirs_list, LIRS_Type, LIRS_name, ", ");
            for_each(tmp_lirs, selected_lirs_list) {
               lListElem *found = limit_rule_set_list_locate(lirs_list, lGetString(tmp_lirs, LIRS_name));
               if (found != NULL) {
                  lAppendElem(found_lirs_list, lCopyElem(found));
               } else {
                  sprintf(SGE_EVENT, MSG_LIRSNOTFOUNDINFILE_SS, lGetString(tmp_lirs, LIRS_name), filename);
                  answer_list_add(answer_list, SGE_EVENT, STATUS_ERROR1, ANSWER_QUALITY_ERROR);
                  lFreeList(&found_lirs_list);
                  break;
               }
            }
            lFreeList(&selected_lirs_list);
            lFreeList(&lirs_list);
            lirs_list = found_lirs_list;
         }

         if (lirs_list != NULL) {
            ret = limit_rule_set_add_del_mod_via_gdi(lirs_list, answer_list, 
                                           SGE_GDI_MOD | SGE_GDI_SET_ALL); 
         }
      }
      FREE(fields);
      lFreeList(&lirs_list);
   }
   DRETURN(ret);
}
