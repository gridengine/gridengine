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

#include "sgermon.h"
#include "sge_log.h"
#include "cull.h"
#include "sge_stdio.h"
#include "sge_stdlib.h"
#include "sge_string.h"
#include "sge_answer.h"
#include "sge_schedd_conf.h"
#include "sge_complex.h"

#include "msg_gdilib.h"

lList *Master_Sched_Config_List = NULL;

int schedd_conf_is_valid_load_formula(lListElem *schedd_conf,
                                      lList **answer_list,
                                      lList *cmplx_list)
{
   const char *load_formula = NULL;
   int ret = 1;
   DENTER(TOP_LAYER, "schedd_conf_is_valid_load_formula");

   /* Modify input */
   {
      char *new_load_formula = NULL;

      load_formula = lGetString(schedd_conf, SC_load_formula);
      new_load_formula = sge_strdup(new_load_formula, load_formula);
      sge_strip_blanks(new_load_formula);
      lSetString(schedd_conf, SC_load_formula, new_load_formula);
      sge_free(new_load_formula);
   }
   load_formula = lGetString(schedd_conf, SC_load_formula);

   /* Check for keyword 'none' */
   if (ret == 1) {
      if (!strcasecmp(load_formula, "none")) {
         answer_list_add(answer_list, MSG_NONE_NOT_ALLOWED, STATUS_ESYNTAX, 
                         ANSWER_QUALITY_ERROR);
         ret = 0;
      }
   }

   /* Check complex attributes and type */
   if (ret == 1) {
      const char *delimitor = "+-*";
      const char *attr, *next_attr;

      next_attr = sge_strtok(load_formula, delimitor);
      while ((attr = next_attr)) {
         lListElem *cmplx_attr = NULL;

         next_attr = sge_strtok(NULL, delimitor);

         cmplx_attr = complex_list_locate_attr(cmplx_list, attr);
         if (cmplx_attr != NULL) {
            int type = lGetUlong(cmplx_attr, CE_valtype);

            if (type == TYPE_STR || type == TYPE_CSTR || type == TYPE_HOST) {
               SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_WRONGTYPE_ATTRIBUTE_S, attr));
               answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, 
                               ANSWER_QUALITY_ERROR);
               ret = 0;
            }
         } else {
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_NOTEXISTING_ATTRIBUTE_S, attr));
            answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, 
                            ANSWER_QUALITY_ERROR);
            ret = 0;
         }
      }
   }
   DEXIT;
   return ret;
}

/***************************************************************
 Find an attribute in a complex list. 
 Iterate over all Complexes and look into their attribute lists.
 ***************************************************************/
lListElem *find_attribute_in_complex_list(const char *attrname,
                                          const lListElem *cmplxl)
{
   const lListElem *attr;
   const char *str;
   int pos_CE_name, pos_CE_shortcut;

   DENTER(CULL_LAYER, "find_attribute_in_complex_list");

   if (!attrname || !cmplxl) {
      DEXIT;
      return NULL;
   }

   pos_CE_name      = lGetPosViaElem(cmplxl, CE_name);
   pos_CE_shortcut  = lGetPosViaElem(cmplxl, CE_shortcut);

   for (attr=cmplxl; attr; attr = lNext(attr)) {
      /* attrname may be the name or a shortcut */
      if ((str = lGetPosString(attr, pos_CE_name)) && !strcmp(attrname, str)) {
         DEXIT;
         return (lListElem *)attr;
      }
      if ((str = lGetPosString(attr, pos_CE_shortcut)) && !strcmp(attrname, str)) {
         DEXIT;
         return (lListElem *)attr;
      }
   }

   DEXIT;
   return NULL;
}

/****** gdi/schedd_conf/schedd_conf_update_master_list() *****************************
*  NAME
*     schedd_conf_update_master_list() -- update the master list of scheduler configurations
*
*  SYNOPSIS
*     int schedd_conf_update_master_list(sge_event_type type, 
*                                        sge_event_action action, 
*                                        lListElem *event, void *clientdata) 
*
*  FUNCTION
*     Update the global master list of scheduler configurations
*     based on an event.
*     The function is called from the event mirroring interface.
*     The list only contains one element that is replaced when a 
*     modify event arrives.
*
*  INPUTS
*     sge_event_type type     - event type
*     sge_event_action action - action to perform
*     lListElem *event        - the raw event
*     void *clientdata        - client data
*
*  RESULT
*     int - TRUE, if update is successfull, else FALSE
*
*  NOTES
*     The function should only be called from the event mirror interface.
*
*  SEE ALSO
*     Eventmirror/--Eventmirror
*******************************************************************************/
int schedd_conf_update_master_list(sge_event_type type, sge_event_action action, 
                                       lListElem *event, void *clientdata)
{
   lList **list;
   lDescr *list_descr;

   lList *data_list;
   lListElem *ep = NULL;
   
   DENTER(TOP_LAYER, "schedd_conf_update_master_list");

   list = &Master_Sched_Config_List;
   list_descr = SC_Type;

   /* We always update the whole list (consisting of one list element) */
   if(*list != NULL) {
      *list = lFreeList(*list);
   }

   if((data_list = lGetList(event, ET_new_version)) != NULL) {
      if((ep = lFirst(data_list)) != NULL) {
         ep = lDechainElem(data_list, ep);
      }
   }

   /* if neccessary, create list and copy schedd info */
   if(ep != NULL) {
      *list = lCreateList("schedd config", list_descr);
      lAppendElem(*list, ep);
   }

   DEXIT;
   return TRUE;
}
