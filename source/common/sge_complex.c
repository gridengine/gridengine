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
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "sge_gdi_intern.h"
#include "resolve_host.h"
#include "sge_complexL.h"
#include "sge_answerL.h"
#include "sge_sched.h"
#include "cull.h"
#include "commlib.h"
#include "sge_parse_num_par.h"
#include "sge_complex.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_string.h"
#include "sge_exit.h"
#include "sge_stdio.h"
#include "msg_common.h"
#include "sge_spoolmsg.h"
#include "sge_feature.h"

static int parse_flag(lList **alpp, const char *cp, lListElem *ep, int nm, const char *name, const char *fname, int line);

static int parse_requestable(lList **alpp, const char *cp, lListElem *ep, const char *fname, int line);

/* 
   Stuff to handle complexes.
   A complex is made out of a variable amount of complex elements.
   The target of complexes is to give some hints to the scheduler 
   whether a job can be scheduled. So complexes define present conditions.
   This conditions are grouped to sge objects.
   There are 4 sorts of complexes in sge. Global, Queue, Host and 
   Definable complexes. The actual values of the first three are given
   by actual values of the sge system.

   The global complex contains global conditions. E.g. the number of available
   hosts is a global condition. There may be a job submitted who only wants
   to run if more than 10 hosts are available.

   The queue complex contains actual data of a queue. E.g. the type of the
   queue is an interesting issue for the scheduler.

   The host complex contains actual data of a host. This contains the actual
   load of the host.

   Definable complexes are groups of attributes the administrator of a sge
   cluster can choose free. He may invent attributes like machine_size and can
   use big and slow as attribute values. This complexes are bound to queues.
   Now the user can request a queue with "qsub -l machine_size.eq.big"

   Complex element structure:

   CE_name,      Name of the complex element e.g. "queue_name"
   CE_shortcut,  Shortcut e.g. "q"
   CE_valtype,   Type of this entry TYPE_INT| TYPE_STR| TYPE_TIM| 
                                    TYPE_MEM, TYPE_BOO, TYPE_CSTR, TYPE_HOST
   CE_stringval, Value
   CE_doubleval,  
   CE_relop,     Relation operator for comparing requests
   CE_request    Is it requestable?

   A complex file might look like this:

   #name        shortcut        type    value   relop   requestable
   queue        none            STR     tralla  ==      y
   dimension    d               INT     1       <       n

   where type = INT | STRING | TIME | MEMORY | BOOL | DOUBLE
   '#' starts a comment, but comments are not saved. 

*/

/****** complex/read_cmplx() ***************************************************
*  NAME
*     read_cmplx() -- Read complex template from file. 
*
*  SYNOPSIS
*     lListElem* read_cmplx(char *fname, char *cmplx_name, lList **alpp) 
*
*  FUNCTION
*     This functions returns a CX_Type list. This list will be read from 
*     fname. If no answer list is provided the function will terminate
*     the using application.
*
*  INPUTS
*     char *fname      - filename 
*     char *cmplx_name - complex name 
*     lList **alpp     - Answer list pointer or NULL 
*
*  RESULT
*     lListElem* - a CX_Type list
*******************************************************************************/
lListElem *read_cmplx(
const char *fname,
const char *cmplx_name,
lList **alpp 
) {
   FILE *fp;
   lListElem *ep=NULL, *epc;
   int line = 0;
   int type = 0;
   const char *name;
   int relop = 0;
   char buf[10000], *cp;
   const char *s;
   lList *lp;
   int ret;
   double dval;

   DENTER(TOP_LAYER, "read_cmplx");

   lp = lCreateList("complex entries", CE_Type);
   
   if (!(fp = fopen(fname, "r"))) {
      ERROR((SGE_EVENT, MSG_FILE_NOOPEN_SS, fname, strerror(errno)));
      if (alpp) {
         sge_add_answer(alpp, SGE_EVENT, STATUS_EDISK, 0);
         lp = lFreeList(lp);
         DEXIT;
         return NULL;
      }
      else
         SGE_EXIT(1);
   }
   
   while (fgets(buf, sizeof(buf), fp)) {
      line++;
      cp = buf;

      /* NAME */
      if ((s = sge_strtok(cp, " \t\n"))) {
         if (*s == '#')
            continue;
         else {
            name = s;
            ep = lCreateElem(CE_Type);
            lSetString(ep, CE_name, s);
         }
      }
      else
          continue;

      DPRINTF(("\t%s\n", name));
      cp = NULL;

      /* SHORTCUT */
      if ((s = sge_strtok(cp, " \t\n")) && (*s != '#')) {
         lSetString(ep, CE_shortcut, s);
      }
      else {
         ERROR((SGE_EVENT, MSG_PARSE_CANTPARSECPLX_SI, fname, line));
         if (alpp) {
            sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, 0);
            lp = lFreeList(lp);
            DEXIT;
            return NULL;
         }
         else
            SGE_EXIT(1);
      }

      /* TYPE */
      if (((s = sge_strtok(cp, " \t\n"))) && (*s != '#')) {
         int i;

         type = 0;
         for (i=TYPE_FIRST; !type && i<=TYPE_DOUBLE; i++) {
            if (!strcasecmp(s, map_type2str(i)))  
               type = i;
         }
         if (!type) {
            ERROR((SGE_EVENT, MSG_PARSE_INVALIDCPLXTYPE_SS, fname, s));
         if (alpp) {
            sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, 0);
            lp = lFreeList(lp);
            DEXIT;
            return NULL;
         }
         else
            SGE_EXIT(1);
         }
         lSetUlong(ep, CE_valtype, type);
      }
      else {
         ERROR((SGE_EVENT, MSG_PARSE_CANTPARSECPLX_SI, fname, line));
         if (alpp) {
            sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, 0);
            lp = lFreeList(lp);
            DEXIT;
            return NULL;
         }
         else
            SGE_EXIT(1);
      }

      /* VALUE */
      if (((s = sge_strtok(cp, " \t\n")) && (*s != '#'))) {

         lSetString(ep, CE_stringval, s);    /* save string representation */

         switch (type) {
         case TYPE_INT:
         case TYPE_TIM:
         case TYPE_MEM:
         case TYPE_BOO:
         case TYPE_DOUBLE:
            if (!parse_ulong_val(&dval, NULL, type, s, SGE_EVENT, sizeof(SGE_EVENT)-1)) {
               SGE_LOG(LOG_ERR, SGE_EVENT);
               ERROR((SGE_EVENT, MSG_PARSE_CANTPARSECPLX_S, fname));
               if (alpp) {
                  sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, 0);
                  lp = lFreeList(lp);
                  DEXIT;
                  return NULL;
               }
               else
                  SGE_EXIT(1);

            }
            lSetDouble(ep, CE_doubleval, dval);
            break;
         case TYPE_HOST:
            /* resolve hostname and store it */
            ret = sge_resolve_host(ep, CE_stringval);
            if (ret) {
               if (ret == COMMD_NACK_UNKNOWN_HOST) {
                  ERROR((SGE_EVENT, MSG_COM_COMMDLOCKED));
                  ERROR((SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_S, s));
               } else {
                  ERROR((SGE_EVENT, MSG_PARSE_CANTPARSECPLX_S, fname));
                  ERROR((SGE_EVENT, MSG_SGETEXT_INVALIDHOST_S, s));
               }
               if (alpp) {
                  sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
                  lp = lFreeList(lp);
                  DEXIT;
                  return NULL;
               }
               else
                  SGE_EXIT(1);
            }
            break;
         }
      }
      else {
         ERROR((SGE_EVENT, MSG_PARSE_CANTPARSECPLX_SI, fname, line));
         if (alpp) {
            sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, 0);
            lp = lFreeList(lp);
            DEXIT;
            return NULL;
         }
         else
            SGE_EXIT(1);
      }
      
      /* RELOP */
      if (((s = sge_strtok(cp, " \t\n"))) && (*s != '#')) {
         int i;

         relop = 0;
         for (i=CMPLXEQ_OP; !relop && i<=CMPLXNE_OP; i++) {
            if (!strcmp(s, map_op2str(i)))  
               relop = i;
         }
         if (!relop) {
            ERROR((SGE_EVENT, MSG_PARSE_INVALIDCPLXRELOP_SS, fname, s));
            if (alpp) {
               sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, 0);
               lp = lFreeList(lp);
               DEXIT;
               return NULL;
            }
            else
               SGE_EXIT(1);
         }
         lSetUlong(ep, CE_relop, relop);
      }
      else {
         ERROR((SGE_EVENT, MSG_PARSE_CANTPARSECPLX_SI, fname, line));
         if (alpp) {
            sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, 0);
            lp = lFreeList(lp);
            DEXIT;
            return NULL;
         }
         else
            SGE_EXIT(1);
      }
      
      /* REQUESTABLE */
      if (parse_requestable(alpp, cp, ep, fname, line)) {
         lp = lFreeList(lp);
         if (alpp) {
            DEXIT;
            return NULL;
         } else
            SGE_EXIT(1);
      }

      /* CONSUMABLE */
      if (parse_flag(alpp, cp, ep, CE_consumable, "consumable", fname, line)) {
         lp = lFreeList(lp);
         if (alpp) {
            DEXIT;
            return NULL;
         } else
            SGE_EXIT(1);
      }
      /* do not allow string types being consumable */
      if (lGetUlong(ep, CE_consumable) && 
         (type==TYPE_HOST || 
          type==TYPE_STR ||
          type==TYPE_CSTR)) {
         ERROR((SGE_EVENT, MSG_PARSE_INVALIDCPLXCONSUM_SSS, fname, lGetString(ep, CE_name), map_type2str(type)));
         if (alpp) {
            sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, 0);
            lp = lFreeList(lp);
            DEXIT;
            return NULL;
         }
         else
            SGE_EXIT(1);
      }

      /* DEFAULT */
      if (((s = sge_strtok(cp, " \t\n")) && (*s != '#'))) {

         lSetString(ep, CE_default, s);    /* save string representation */

         switch (type) {
         case TYPE_INT:
         case TYPE_TIM:
         case TYPE_MEM:
         case TYPE_BOO:
         case TYPE_DOUBLE:
            if (!parse_ulong_val(&dval, NULL, type, s, SGE_EVENT, sizeof(SGE_EVENT)-1)) {
               SGE_LOG(LOG_ERR, SGE_EVENT);
               ERROR((SGE_EVENT, MSG_PARSE_CANTPARSECPLX_S, fname));
               if (alpp) {
                  sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, 0);
                  lp = lFreeList(lp);
                  DEXIT;
                  return NULL;
               }
               else
                  SGE_EXIT(1);

            }
/*             lSetDouble(ep, CE_defaultdouble, dval); */
            break;
         case TYPE_HOST:
            /* resolve hostname and store it */
            ret = sge_resolve_host(ep, CE_stringval);
            if (ret) {
               if (ret == COMMD_NACK_UNKNOWN_HOST) {
                  ERROR((SGE_EVENT, MSG_COM_COMMDLOCKED));
                  ERROR((SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_S, s));
               } else {
                  ERROR((SGE_EVENT, MSG_PARSE_CANTPARSECPLX_S, fname));
                  ERROR((SGE_EVENT, MSG_SGETEXT_INVALIDHOST_S, s));
               }
               if (alpp) {
                  sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
                  lp = lFreeList(lp);
                  DEXIT;
                  return NULL;
               }
               else
                  SGE_EXIT(1);
            }
            break;
         }
      }
      else {
         ERROR((SGE_EVENT, MSG_PARSE_CANTPARSECPLX_SI, fname, line));
         if (alpp) {
            sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, 0);
            lp = lFreeList(lp);
            DEXIT;
            return NULL;
         }
         else
            SGE_EXIT(1);
      }
      
      /* ANYTHING ELSE ? */
      if (((s = sge_strtok(cp, " \t\n"))) && (*s != '#')) {
         ERROR((SGE_EVENT, MSG_PARSE_CANTPARSECPLX_SI, fname, line));
         if (alpp) {
            sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, 0);
            lp = lFreeList(lp);
            DEXIT;
            return NULL;
         }
         else
            SGE_EXIT(1);
      }
      lAppendElem(lp, ep);
   }

   fclose(fp);

   epc = lCreateElem(CX_Type);
   lSetString(epc, CX_name, cmplx_name);
   lSetList(epc, CX_entries, lp);

   DEXIT;
   return epc;
}

static int parse_flag(
lList **alpp,
const char *cp,
lListElem *ep,
int nm,
const char *name,
const char *fname,
int line 
) {
   int flag;
   const char *s;

   DENTER(TOP_LAYER, "parse_flag");

   if (((s = sge_strtok(cp, " \t\n"))) && (*s != '#')) {
      if (!strcasecmp(s, "y") || !strcasecmp(s, "yes"))
         flag = 1;
      else if (!strcasecmp(s, "n") || !strcasecmp(s, "no"))
         flag = 0;
      else {
         ERROR((SGE_EVENT, MSG_PARSE_INVALIDCPLXENTRY_SSS, fname, name, s));
         if (alpp)
            sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, 0);
         DEXIT;
         return 1;
      }
      lSetUlong(ep, nm, flag);
   }
   else {
      ERROR((SGE_EVENT, MSG_PARSE_CANTPARSECPLX_SI, fname, line));
      if (alpp) 
         sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, 0);
      DEXIT;
      return 1;
   }

   DEXIT;
   return 0;
}
      
static int parse_requestable(
lList **alpp,
const char *cp,
lListElem *ep,
const char *fname,
int line 
) {
   int flag;
   const char *s;

   DENTER(TOP_LAYER, "parse_flag");

   if (((s = sge_strtok(cp, " \t\n"))) && (*s != '#')) {
      if (!strcasecmp(s, "y") || !strcasecmp(s, "yes"))
         flag = 1;
      else if (!strcasecmp(s, "n") || !strcasecmp(s, "no"))
         flag = 0;
      else if (!strcasecmp(s, "f") || !strcasecmp(s, "forced"))
         flag = 2;
      else {
         ERROR((SGE_EVENT, MSG_PARSE_INVALIDCPLXREQ_SS, fname, s));
         if (alpp)
            sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, 0);
         DEXIT;
         return 1;
      }

      if (flag == 2) {
         lSetUlong(ep, CE_forced, 1);
         lSetUlong(ep, CE_request, 1);
      } else {
         lSetUlong(ep, CE_forced, 0);
         lSetUlong(ep, CE_request, flag);
      }
   }
   else {
      ERROR((SGE_EVENT, MSG_PARSE_CANTPARSECPLX_SI, fname, line));
      if (alpp) 
         sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, 0);
      DEXIT;
      return 1;
   }

   DEXIT;
   return 0;
}
      
/**********************************************************************
   write complex in human readable (and editable) form to file or 
   fileptr. If fname == NULL use fpout for writing.
 **********************************************************************/
int write_cmplx(
int spool,
const char *fname,
lList *lpc,
FILE *fpout,
lList **alpp 
) {
   FILE *fp;
   lListElem *ep=NULL;

   DENTER(TOP_LAYER, "write_cmplx");

   if (fname) {
      SGE_FOPEN(fp, fname, "w");
      if (fp == NULL) {
         ERROR((SGE_EVENT, MSG_FILE_NOOPEN_SS, fname, strerror(errno)));
         if (alpp) {
            sge_add_answer(alpp, SGE_EVENT, STATUS_EDISK, 0); 
            DEXIT;
            return -1; 
         }
         else
            SGE_EXIT(1);
      }
   }
   else
      fp = fpout;

   if (spool && sge_spoolmsg_write(fp, COMMENT_CHAR,
       feature_get_product_name(FS_VERSION)) < 0) {
      goto FPRINTF_ERROR;
   }  

   FPRINTF((fp, "%-16s %-10s %-6s %-15s %-5s %-11s %-10s %-5s\n", 
	         "#name", "shortcut", "type",
            "value", "relop", "requestable", "consumable", "default"));
   FPRINTF((fp, "#-------------------------------------------------"
            "-------------------------------------\n"));
   
   for_each(ep, lpc) {
      FPRINTF((fp, "%-16s %-10s %-6s %-15s %-5s %-11s %-10s %-5s\n", 
	      lGetString(ep, CE_name), 
         lGetString(ep, CE_shortcut), 
         map_type2str(lGetUlong(ep, CE_valtype)), 
         lGetString(ep, CE_stringval), 
         map_op2str(lGetUlong(ep, CE_relop)), 
         (lGetUlong(ep, CE_forced)) ? "FORCED" : 
                           (lGetUlong(ep, CE_request)) ? "YES" : "NO",
         (lGetUlong(ep, CE_consumable)) ? "YES" : "NO",
         lGetString(ep, CE_default)));
   }

   
   FPRINTF((fp, "#"SFN"",MSG_COMPLEX_STARTSCOMMENTBUTNOSAVE));
   
   if (fname) {
      FCLOSE(fp);
   }

   DEXIT;
   return 0;

FPRINTF_ERROR:
   if(errno != 0)
      CRITICAL((SGE_EVENT, MSG_FILE_ERRORWRITING_SS, fname, strerror(errno)));
   if(fname != NULL) {
      FCLEANUP(fp, fname);
   }   
   if (alpp) 
      sge_add_answer(alpp, SGE_EVENT, STATUS_EDISK, 0); 
   DEXIT;
   return -1;
}

/****** complex/sge_fill_requests() ********************************************
*  NAME
*     sge_fill_requests() -- fills and checks list of complex entries 
*
*  SYNOPSIS
*     int sge_fill_requests(lList *re_entries, lList *complex_list, int 
*     allow_non_requestable, int allow_empty_boolean, int allow_neg_consumable) 
*
*  FUNCTION
*     This function fills a given list of complex entries with missing
*     attributes which can be found in the complex. It checks also wether
*     the given in the re_entries-List are valid. 
*
*  INPUTS
*     lList *re_entries         - resources as complex list CE_Type 
*     lList *complex_list       - the global complex list 
*     int allow_non_requestable - needed for qstat -l or qmon customize dialog 
*     int allow_empty_boolean   - boolean 
*        1 => NULL values of boolean attributes will be replaced with "TRUE" 
*        0 => NULL values will be handled as error 
*     int allow_neg_consumable  - boolean
*        1 => negative values for consumable resources are allowed. 
*        0 => function will return with -1 if it finds consumable resources
*             with a negative value
*
*  RESULT
*     int - error
*        0 on success
*       -1 on error
*        an error message will be written into SGE_EVENT
*******************************************************************************/
int sge_fill_requests(
lList *re_entries, 
lList *complex_list,     
int allow_non_requestable, 
int allow_empty_boolean,  
int allow_neg_consumable 
) {
   lListElem *c, *entry, *cep;
   const char *name;

   DENTER(TOP_LAYER, "sge_fill_requests");

   for_each(entry, re_entries) {
      name = lGetString(entry, CE_name);
   
      cep = NULL;
      for_each (c, complex_list) {
         if ((cep = find_attribute_in_complex_list(name, 
                                             lFirst(lGetList(c, CX_entries)))))
            break;
      }
      if (!cep) {
         ERROR((SGE_EVENT, MSG_SGETEXT_UNKNOWN_RESOURCE_S, name));
         DEXIT;
         return -1;
      }

      if (!allow_non_requestable && !lGetUlong(cep, CE_request)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_RESOURCE_NOT_REQUESTABLE_S, name));
         DEXIT;
         return -1;
      }

      /* replace name in request/threshold/consumable list, 
         it may have been a shortcut */
      lSetString(entry, CE_name, lGetString(cep, CE_name));

      /* we found the right complex attrib */
      /* so we know the type of the requested data */
      lSetUlong(entry, CE_valtype, lGetUlong(cep, CE_valtype));

      /* we also know wether it is a consumable attribute */
      lSetUlong(entry, CE_consumable, lGetUlong(cep, CE_consumable)); 

      if (fill_and_check_attribute(entry, allow_empty_boolean, allow_neg_consumable)) {
         /* no error msg here - fill_and_check_attribute() makes it */
         DEXIT;
         return -1;
      }
   }

   DEXIT;
   return 0;
}

/****** complex/fill_and_check_attribute() *************************************
*  NAME
*     fill_and_check_attribute() -- fill and check the attribute 
*
*  SYNOPSIS
*     int fill_and_check_attribute(lListElem *cep, int allow_empty_boolean, 
*                                  int allow_neg_consumable) 
*
*  FUNCTION
*     fill and check the attribute 
*
*  INPUTS
*     lListElem *cep           - CE_Type, this object will be checked 
*     int allow_empty_boolean  - boolean
*        1 => NULL values of boolean attributes will be replaced with "TRUE" 
*        0 => NULL values will be handled as error 
*     int allow_neg_consumable - boolean
*        1 => negative values for consumable resources are allowed. 
*        0 => function will return with -1 if it finds consumable resources
*             with a negative value
*
*  RESULT
*     int - error
*        0 on success
*       -1 on error
*        an error message will be written into SGE_EVENT
*******************************************************************************/
int fill_and_check_attribute(
lListElem *cep,
int allow_empty_boolean,
int allow_neg_consumable
) {
   static char tmp[1000];
   const char *name, *s;
   u_long32 type;
   double dval;
   int ret;

   DENTER(TOP_LAYER, "fill_and_check_attribute");

   name = lGetString(cep, CE_name);
   s = lGetString(cep, CE_stringval);

   if (!s) {
      if (allow_empty_boolean && lGetUlong(cep, CE_valtype)==TYPE_BOO) {
         lSetString(cep, CE_stringval, "TRUE");
         s = lGetString(cep, CE_stringval);
      }
      else {
         ERROR((SGE_EVENT, MSG_CPLX_VALUEMISSING_S, name));
         DEXIT;
         return -1;
      }
   }

   switch ( type = lGetUlong(cep, CE_valtype) ) {
      case TYPE_INT:
      case TYPE_TIM:
      case TYPE_MEM:
      case TYPE_BOO:
      case TYPE_DOUBLE:
         if (!parse_ulong_val(&dval, NULL, type, s, tmp, sizeof(tmp)-1)) {
            ERROR((SGE_EVENT, MSG_CPLX_WRONGTYPE_SSS, name, s, tmp));
            DEXIT;
            return -1;
         }
         lSetDouble(cep, CE_doubleval, dval);
   
         /* also the CE_default must be parsable for numeric types */ 
         if ((s=lGetString(cep, CE_default)) 
            && !parse_ulong_val(&dval, NULL, type, s, tmp, sizeof(tmp)-1)) {
            ERROR((SGE_EVENT, MSG_CPLX_WRONGTYPE_SSS, name, s, tmp));
            DEXIT;
            return -1;
         }

         /* negative values are not allowed for consumable attributes */
         if (!allow_neg_consumable && lGetUlong(cep, CE_consumable)
             && lGetDouble(cep, CE_doubleval) < (double)0.0) {
            ERROR((SGE_EVENT, MSG_CPLX_ATTRIBISNEG_S, name));

            DEXIT;
            return -1;
         }     
         break;
      case TYPE_HOST:
         /* resolve hostname and store it */
         ret = sge_resolve_host(cep, CE_stringval);
         if (ret) {
            if (ret == COMMD_NACK_UNKNOWN_HOST) {
               ERROR((SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_S, s));
            } else {
               ERROR((SGE_EVENT, MSG_SGETEXT_INVALIDHOST_S, s));
            }
            DEXIT;
            return -1;
         }
         break;

      case TYPE_STR:
      case TYPE_CSTR:
         /* no restrictions - so everything is ok */
         break;

      default:
         ERROR((SGE_EVENT, MSG_SGETEXT_UNKNOWN_ATTR_TYPE_U, u32c(type)));
         DEXIT;
         return -1;
   }
   
   DEXIT;
   return 0;
}

void init_complex_double_values(
lList *cl 
) {
   lListElem *cle, *cattr;

   if (cl) {
      for_each(cle, cl) {
         for_each(cattr, lGetList(cle,CX_entries)) {
            double new_val;

            parse_ulong_val(&new_val, NULL, lGetUlong(cattr, CE_valtype),
               lGetString(cattr, CE_stringval), NULL, 0);
            lSetDouble(cattr, CE_doubleval, new_val);
         }
      }
   }
}

