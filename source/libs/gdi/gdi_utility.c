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
#include <ctype.h>
#include "cull.h"
#include "sgermon.h"
#include "sge_string.h"
#include "sge_log.h"
#include "sge_gdi.h"
#include "gdi_utility.h"
#include "sge_any_request.h"
#include "sge_parse_num_par.h"
#include "sge_complex_schedd.h"
#include "sort_hosts.h"
#include "config_file.h"
#include "sge_answer.h"
#include "sge_queue.h"
#include "sge_host.h"
#include "sge_userset.h"
#include "sge_complex.h"

#include "msg_common.h"
#include "msg_qmaster.h"



/* MT-NOTE: this module is not MT safe */
/* MT-NOTE: this module is not needed for GDI client */

char err_msg[1000];

/* ------------------------------

   register passed error string in
   err_msg for later use

*/
void error(
char *err_str 
) {
   if (err_str) {
      strncpy(err_msg, err_str, sizeof(err_msg)-1);
      err_msg[sizeof(err_msg)-1] = '\0';
   }
}

/* control methods */
int attr_mod_ctrl_method(
lList **alpp,
lListElem *qep,
lListElem *new_ep,
int nm,
char *attr_name 
) {
   const char *s;

   DENTER(TOP_LAYER, "attr_mod_ctrl_method");

   /* ---- attribute nm */
   if (lGetPosViaElem(qep, nm)>=0) {
      DPRINTF(("got new %s\n", attr_name));

      s = lGetString(qep, nm);
      if (s) {
         /* For scripts we accept only complete pathes.
            So if the method begins with a "/" it must be a path 
            and can only be checked at the execution machine. In 
            all other cases we assume the user entered a 
            signal specifier. A signal specifier either has a
            "SIG" prefix or is a number.  */
         if (s[0] != '/' ) {
            if (!isdigit((int) s[0]) && strncmp(s, "SIG", 3)) {
               ERROR((SGE_EVENT, MSG_GDI_SIG_DIGIT_SS, attr_name, s));
               answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
               DEXIT;
               return STATUS_EUNKNOWN;
            }
         } else {
            if (replace_params(s, NULL, 0, ctrl_method_variables )) {
               ERROR((SGE_EVENT, MSG_GDI_METHOD_VARS_SS, attr_name, err_msg));
               answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
               DEXIT;
               return STATUS_EEXIST;
            }
         }
      }
      lSetString(new_ep, nm, s);
   }

   DEXIT;
   return 0;
}

/* used for verification and for copying prolog/epilog/pe_start/pe_stop */
int attr_mod_procedure(
lList **alpp,
lListElem *qep,
lListElem *new_ep,
int nm,
char *attr_name,
char *variables[] 
) {
   DENTER(TOP_LAYER, "attr_mod_procedure");

   /* ---- attribute nm */
   if (lGetPosViaElem(qep, nm)>=0) {
      const char *s; 
      DPRINTF(("got new %s\n", attr_name));

      s = lGetString(qep, nm);
      if (s) {
         char *t; 
         const char *script = s;

         /* skip user name */
         if ((t = strpbrk(script, "@ ")) && *t == '@')
            script = &t[1];

         /* force use of absolut pathes */ 
         if (script[0] != '/' ) { 
            ERROR((SGE_EVENT, MSG_GDI_APATH_S, attr_name));
            answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            DEXIT;
            return STATUS_EEXIST;
         } 
        
         /* ensure that variables are valid */
         if (replace_params(script, NULL, 0, variables )) {
            ERROR((SGE_EVENT, MSG_GDI_VARS_SS, attr_name, err_msg));
            answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
            DEXIT;
            return STATUS_EEXIST;
         }
      }
      lSetString(new_ep, nm, s);
   }

   DEXIT;
   return 0;
}


/* raw strings without any verification 
   NULL is a valid value */       
int attr_mod_zerostr(
lListElem *qep,
lListElem *new_ep,
int nm,
char *attr_name 
) {
   DENTER(TOP_LAYER, "attr_mod_str");

   /* ---- attribute nm */
   if (lGetPosViaElem(qep, nm)>=0) {
      DPRINTF(("got new %s\n", attr_name));
      lSetString(new_ep, nm, lGetString(qep, nm));
   }

   DEXIT;
   return 0;
}

/* raw strings without any verification 
   except that it may not be NULL */       
int attr_mod_str(
lList **alpp,
lListElem *qep,
lListElem *new_ep,
int nm,
char *attr_name 
) {
   int dataType;
   int pos;
  
   DENTER(TOP_LAYER, "attr_mod_str");

   /* ---- attribute nm */
   if ((pos=lGetPosViaElem(qep, nm))>=0) {
      const char *s;

      DPRINTF(("got new %s\n", attr_name));

      dataType = lGetPosType(lGetElemDescr(qep),pos);
      switch (dataType) {
         case lStringT:
            if (!(s = lGetString(qep, nm))) {
               ERROR((SGE_EVENT, MSG_GDI_VALUE_S, attr_name));
               answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
               DEXIT;
               return STATUS_EUNKNOWN;
            }
            lSetString(new_ep, nm, s);
            break;
         case lHostT:
            if (!(s = lGetHost(qep, nm))) {
               ERROR((SGE_EVENT, MSG_GDI_VALUE_S, attr_name));
               answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
               DEXIT;
               return STATUS_EUNKNOWN;
            }
            lSetHost(new_ep, nm, s);
            break;
         default:
            DPRINTF(("unexpected data type\n"));
            DEXIT;
            return STATUS_EUNKNOWN;
      }
   }

   DEXIT;
   return 0;
}

/* raw booleans without any verification */       
int attr_mod_bool(
lListElem *qep,
lListElem *new_ep,
int nm,
char *attr_name 
) {
   DENTER(TOP_LAYER, "attr_mod_ulong");

   /* ---- attribute nm */
   if (lGetPosViaElem(qep, nm)>=0) {
      DPRINTF(("got new %s\n", attr_name));
      lSetBool(new_ep, nm, lGetBool(qep, nm));
   }

   DEXIT;
   return 0;
}

/* raw ulongs without any verification */       
int attr_mod_ulong(
lListElem *qep,
lListElem *new_ep,
int nm,
char *attr_name 
) {
   DENTER(TOP_LAYER, "attr_mod_ulong");

   /* ---- attribute nm */
   if (lGetPosViaElem(qep, nm)>=0) {
      DPRINTF(("got new %s\n", attr_name));
      lSetUlong(new_ep, nm, lGetUlong(qep, nm));
   }

   DEXIT;
   return 0;
}

/* raw doubles without any verification */       
int attr_mod_double(
lListElem *qep,
lListElem *new_ep,
int nm,
char *attr_name 
) {
   DENTER(TOP_LAYER, "attr_mod_double");

   /* ---- attribute nm */
   if (lGetPosViaElem(qep, nm)>=0) {
      DPRINTF(("got new %s\n", attr_name));
      lSetDouble(new_ep, nm, lGetDouble(qep, nm));
   }

   DEXIT;
   return 0;
}

int attr_mod_mem_str(
lList **alpp,
lListElem *qep,
lListElem *new_ep,
int nm,
char *attr_name 
) {
   DENTER(TOP_LAYER, "attr_mod_mem_str");

   /* ---- attribute nm */
   if (lGetPosViaElem(qep, nm)>=0) {
      const char *str;

      str = lGetString(qep, nm);
      DPRINTF(("got new %s\n", attr_name));

      if(!parse_ulong_val(NULL, NULL, TYPE_MEM, str, NULL, 0)) {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_TYPE_MEM_SS, attr_name, str?str:"(null)"));
         answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_ESYNTAX;
      }

      lSetString(new_ep, nm, str);
   }

   DEXIT;
   return 0;
}
int attr_mod_time_str(
lList **alpp,
lListElem *qep,
lListElem *new_ep,
int nm,
char *attr_name,
int enable_infinity 
) {
   DENTER(TOP_LAYER, "attr_mod_time_str");

   /* ---- attribute nm */
   if (lGetPosViaElem(qep, nm)>=0) {
      const char *str; 

      str = lGetString(qep, nm);
      DPRINTF(("got new %s\n", attr_name));

      if (str != NULL ) {
         /* don't allow infinity for these parameters */
         if ((strcasecmp(str, "infinity") == 0) && (enable_infinity == 0)) { 
              DPRINTF(("ERROR! Infinity value for \"%s\"\n",attr_name));
              SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_SIG_DIGIT_SS, attr_name, str));
              answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
              DEXIT;
              return STATUS_ESYNTAX;
         }
      }

      if(!parse_ulong_val(NULL, NULL, TYPE_TIM, str, NULL, 0)) {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_TYPE_TIME_SS, attr_name, str?str:"(null)"));
         answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_ESYNTAX;
      }

      lSetString(new_ep, nm, str);
   }

   DEXIT;
   return 0;
}

int multiple_occurances(
lList **alpp,
lList *lp1,
lList *lp2,
int nm,
const char *name,
const char *obj_name 
) {
   lListElem *ep1;
   const char *s;

   DENTER(TOP_LAYER, "multiple_occurances");

   if (!lp1 || !lp2) {
      DEXIT;
      return 0;
   }

   for_each (ep1, lp1) {
      s = lGetString(ep1, nm);
      if (lGetElemStr(lp2, nm, s)) {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_MULTIPLE_OCCUR_SSSS, 
                  (nm==US_name)?MSG_OBJ_USERSET:MSG_JOB_PROJECT, s, obj_name, name));
         answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         DEXIT;
         return -1;
      }
   }

   DEXIT;
   return 0;
}

void normalize_sublist(
lListElem *ep,
int nm 
) {
   lList *lp;

   if ((lp=lGetList(ep, nm)) && lGetNumberOfElem(lp)==0)
      lSetList(ep, nm, NULL);
}

void attr_mod_sub_list(
lList **alpp,
lListElem *this_elem,
int this_elem_name,
int this_elem_primary_key,
lListElem *delta_elem,
int sub_command,
char *sub_list_name,
char *object_name,
int no_info 
) {
   DENTER(TOP_LAYER, "attr_mod_sub_list");  

   if (lGetPosViaElem(delta_elem, this_elem_name)<0) { 
      return;
   }

   if (sub_command == SGE_GDI_CHANGE ||
       sub_command == SGE_GDI_APPEND ||
       sub_command == SGE_GDI_REMOVE) {
      lList *reduced_sublist;
      lList *full_sublist;
      lListElem *reduced_element, *next_reduced_element;
      lListElem *full_element, *next_full_element;

      reduced_sublist = lGetList(delta_elem, this_elem_name);
      full_sublist = lGetList(this_elem, this_elem_name);
      next_reduced_element = lFirst(reduced_sublist);
      /*
      ** we try to find each element of the delta_elem
      ** in the sublist if this_elem. Elements which can be found
      ** will be moved into sublist of this_elem.
      */
      while ((reduced_element = next_reduced_element)) {
         int restart_loop = 0;

         next_reduced_element = lNext(reduced_element);
         next_full_element = lFirst(full_sublist);
         while ((full_element = next_full_element)) {
            next_full_element = lNext(full_element);

            if (!strcmp(lGetString(reduced_element, this_elem_primary_key),
                   lGetString(full_element, this_elem_primary_key))) {
               lListElem *new_sub_elem;
               lListElem *old_sub_elem;          

               next_reduced_element = lNext(reduced_element);
               new_sub_elem =
                  lDechainElem(reduced_sublist, reduced_element);
               old_sub_elem = lDechainElem(full_sublist, full_element);
               if (sub_command == SGE_GDI_CHANGE ||
                   sub_command == SGE_GDI_APPEND) {

                  if (!no_info && sub_command == SGE_GDI_APPEND) {
                     INFO((SGE_EVENT, SFQ" already exists in "SFQ" of "SFQ"\n",
                        lGetString(reduced_element, this_elem_primary_key), 
                        sub_list_name, object_name));
                     answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
                  }
                  lFreeElem(old_sub_elem);
                  lAppendElem(full_sublist, new_sub_elem);

                  restart_loop = 1;
                  break;
               } else if (sub_command == SGE_GDI_REMOVE) {

                  lFreeElem(old_sub_elem);
                  lFreeElem(new_sub_elem);

                  restart_loop = 1;
                  break;
               }
            }
         }
         if (restart_loop) {
            next_reduced_element = lFirst(reduced_sublist);
         }
      }         
      if (sub_command == SGE_GDI_CHANGE ||
          sub_command == SGE_GDI_APPEND ||
          sub_command == SGE_GDI_REMOVE) {
         next_reduced_element = lFirst(reduced_sublist);
         while ((reduced_element = next_reduced_element)) {
            lListElem *new_sub_elem;

            next_reduced_element = lNext(reduced_element);
            
            if (!no_info && sub_command == SGE_GDI_REMOVE) {
               INFO((SGE_EVENT, SFQ" does not exist in "SFQ" of "SFQ"\n",
                   lGetString(reduced_element, this_elem_primary_key),
                   sub_list_name, object_name));
               answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
            } else {
               if (!full_sublist) {
                  if (!no_info && sub_command == SGE_GDI_CHANGE) {
                     INFO((SGE_EVENT, SFQ" of "SFQ" is empty - "
                        "Adding new element(s).\n",
                        sub_list_name, object_name));
                     answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
                  } 
                  lSetList(this_elem, this_elem_name, lCopyList("",
                     lGetList(delta_elem, this_elem_name)));
                  full_sublist = lGetList(this_elem, this_elem_name);
                  break;
               } else {
                  if (!no_info && sub_command == SGE_GDI_CHANGE) {
                     INFO((SGE_EVENT, "Unable to find "SFQ" in "SFQ" of "SFQ
                        " - Adding new element.\n",
                        lGetString(reduced_element, this_elem_primary_key),
                        sub_list_name, object_name));
                     answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
                  }   
                  new_sub_elem =
                     lDechainElem(reduced_sublist, reduced_element);
                  lAppendElem(full_sublist, new_sub_elem);
               }
            }
         }
      }
   } else if (sub_command == SGE_GDI_SET) {
      /*
      ** Overwrite the complete list
      */
      lSetList(this_elem, this_elem_name, lCopyList("",
         lGetList(delta_elem, this_elem_name)));
   } 
   /*
   ** If the list does not contain any elements, we will delete
   ** the list itself
   */
   if (lGetList(this_elem, this_elem_name)
       && !lGetNumberOfElem(lGetList(this_elem, this_elem_name))) {
      lSetList(this_elem, this_elem_name, NULL);
   }    
   DEXIT;
}
