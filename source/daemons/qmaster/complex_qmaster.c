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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "sge.h"
#include "sgermon.h"
#include "sge_ja_task.h"
#include "sge_schedd_conf.h"
#include "commlib.h"
#include "sge_parse_num_par.h"
#include "complex_qmaster.h"
#include "sge_queue_qmaster.h"
#include "sge_m_event.h"
#include "complex_history.h"
#include "opt_history.h"
#include "path_history.h"
#include "sge_log.h"
#include "gdi_utility_qmaster.h"
#include "sge_complex_schedd.h"
#include "sort_hosts.h"
#include "sge_select_queue.h"
#include "sge_host.h"
#include "sge_stdio.h"
#include "read_write_queue.h"
#include "read_write_complex.h"
#include "sge_unistd.h"
#include "sge_spool.h"
#include "sge_answer.h"
#include "sge_schedd_conf.h"
#include "sge_queue.h"
#include "sge_job.h"
#include "sge_complex.h"

#include "msg_common.h"
#include "msg_qmaster.h"

static void sge_change_queue_version_complex(const char *cmplx_name);
static int verify_complex_deletion(lList **alpp, const char *userset_name);



/* ------------------------------------------------------------ */

int complex_mod(
lList **alpp,
lListElem *new_complex,
lListElem *ep,
int add,
char *ruser,
char *rhost,
gdi_object_t *object,
int sub_command 
) {
   const char *complex_name;
   lListElem *cep;

   DENTER(TOP_LAYER, "complex_mod");

   /* ---- CX_name */
   if (add) {
      complex_name = lGetString(ep, CX_name);
      if (verify_str_key(alpp, complex_name, "complex"))
         goto ERROR;
      lSetString(new_complex, CX_name, complex_name);
   } else
      complex_name = lGetString(new_complex, CX_name);


   /* ---- CX_entries */
   if (lGetPosViaElem(ep, CX_entries)>=0) {

      /* Are there references of deleted complex entries in other objects? */
      if (!add) {
         lListElem *old_cep;
         lListElem *new_cep;
         lList *new_cl;
        
         new_cl = lGetList(ep, CX_entries); 
         for_each(old_cep, lGetList(new_complex, CX_entries)) {
            const char* old_cep_CE_name = lGetString(old_cep, CE_name);
            new_cep = lGetElemStr(new_cl, CE_name, old_cep_CE_name );
            
            if (!new_cep) {
               lListElem *qep;
               lListElem *hep;
               lListElem *scep;
               const char* load_formula;

               /* try to find reference for CE element in each queue */
               for_each(qep, Master_Queue_List) {
                  lListElem *ceep;

                  ceep = lGetElemStr(lGetList(qep, QU_consumable_config_list), CE_name, old_cep_CE_name );   
                  if (ceep) {
                     ERROR((SGE_EVENT, MSG_ATTRSTILLREFINQUEUE_SS, old_cep_CE_name, lGetString(qep, QU_qname)));
                     answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
                     goto ERROR;
                  }
               }
               /* try to find reference for CE element in each Host */
               for_each(hep, Master_Exechost_List) { 
                  lListElem *ceep;

                  ceep = lGetElemStr(lGetList(hep, EH_consumable_config_list), CE_name, old_cep_CE_name );
                  if (ceep) {
                     ERROR((SGE_EVENT, MSG_ATTRSTILLREFINHOST_SS, old_cep_CE_name, lGetHost(hep, EH_name)));
                     answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
                     goto ERROR;
                  }        
               }
               /* try to find reference in schedulers load adjustments */
               scep = lFirst(Master_Sched_Config_List);
               if (scep) {
                  lListElem *ceep;

                  ceep = lGetElemStr(lGetList(scep, SC_job_load_adjustments), CE_name, old_cep_CE_name );
                  if (ceep) {
                     ERROR((SGE_EVENT, MSG_ATTRSTILLREFINSCHED_S, old_cep_CE_name ));
                     answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
                     goto ERROR;
                  }                      
               }
               /* try to find reference in schedulers load formula */
               load_formula = lGetString(scep, SC_load_formula);
               if (load_formula) {
                  int reference_found = 0;
                  char *first_occur;

                  first_occur = strstr(load_formula, old_cep_CE_name);
                  if (first_occur) {
                     char front, behind;
                     int begin = 0, end = 0;

                     reference_found = 0;
                     if (load_formula != first_occur) {
                        front = first_occur[-1];
                        if (front == '+' || front == '-' || front == '*') {
                           begin = 1;
                        } 
                     } else {
                        begin = 1;
                     }

                     behind = first_occur[strlen(old_cep_CE_name )];
                     if (behind == '+' || behind == '-' || behind == '*' 
                         || behind == '\0') {
                        end = 1; 
                     }
                     if (begin && end) {
                        reference_found = 1;
                     }   
                  }
                  if (reference_found) {
                     ERROR((SGE_EVENT, MSG_ATTRSTILLREFINSCHED_S, old_cep_CE_name ));
                     answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
                     goto ERROR;
                  }
               }
            } 
         }
      }

      for_each(cep, lGetList(ep, CX_entries)) {

         lListElem *cr, *qep, *hep, *cxep;
         const char *name;
         const char *shortcut;

         name = lGetString(cep, CE_name);
         shortcut = lGetString(cep, CE_shortcut);

         if (!name ||
             !shortcut ||
             !lGetString(cep, CE_stringval) ||
             !lGetString(cep, CE_default)) {
            ERROR((SGE_EVENT, MSG_CPLX_ATTRIBISNULL_SS,
                  name?name:MSG_NONAME, complex_name)); 
            answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
            goto ERROR;
         }

         /* silently ensure the 'slots' attribute has 
            a correct configuration it is too important
            (it'd be better hiding it totally from the user) */
         if (!strcmp("slots", name)) {
            lSetUlong(cep, CE_valtype, TYPE_INT);
            lSetString(cep, CE_stringval, "0");
            lSetUlong(cep, CE_relop, CMPLXLE_OP);
            lSetBool(cep, CE_request, TRUE);
            lSetBool(cep, CE_forced, FALSE);
            lSetBool(cep, CE_consumable, TRUE);
            lSetString(cep, CE_default, "1");
         }

         /* ensure that attribute does not appear in other complexes lists */
         for_each (cxep, Master_Complex_List) {
            /* skip this complex */
            if (!strcmp(complex_name, lGetString(cxep, CX_name)))
               continue;

            /* search for long name */
            if (find_attribute_in_complex_list(name, lFirst(lGetList(cxep, CX_entries)))) {
               ERROR((SGE_EVENT, MSG_CPLX_ATTRIBALREADY_SS, name, lGetString(cxep, CX_name))); 
               answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
               goto ERROR;
            } 

            /* search for shortcut */
            if (find_attribute_in_complex_list(shortcut, lFirst(lGetList(cxep, CX_entries)))) {
               ERROR((SGE_EVENT, MSG_CPLX_SHORTCUTALREADY_SS, shortcut, 
                      lGetString(cxep, CX_name))); 
               answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, 
                               ANSWER_QUALITY_ERROR);
               goto ERROR;
            } 
         }

         /* ensure that attribute does not appear multiple in this complexes list */
         /* search for long name in remaining attribs */
         if (find_attribute_in_complex_list(name, lNext(cep))) {
            ERROR((SGE_EVENT, MSG_CPLX_ATTRIBALREADY_SS, name, complex_name)); 
            answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
            goto ERROR;
         } 

         /* search for shortcut in remaining attribs */
         if (find_attribute_in_complex_list(shortcut, lNext(cep))) {
            ERROR((SGE_EVENT, MSG_CPLX_SHORTCUTALREADY_SS, shortcut, 
                   complex_name)); 
            answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, 
                            ANSWER_QUALITY_ERROR);
            goto ERROR;
         } 
      
         /* ensure CE_stringval and CE_default is parsable or resolve if it is a hostname */
         if (fill_and_check_attribute(cep, 0, 1)) {
            answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            goto ERROR;
         }

         /* ensure consumables are numeric types */
         if (lGetBool(cep, CE_consumable)) {
            u_long32 type = lGetUlong(cep, CE_valtype);
            if (type==TYPE_STR||type==TYPE_CSTR||type==TYPE_HOST) {
               ERROR((SGE_EVENT, MSG_CPLX_ATTRIBNOCONSUM_S, name));
               answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
               goto ERROR;
            }

            /* search through resource lists of queues and exechosts 
               reject this request in case there is an entry which is not parsable 
               -> to prevent mysterious errors in scheduler */
            for_each (hep, Master_Exechost_List) {  
               if ((cr = lGetSubStr(hep, CE_name, name, EH_consumable_config_list)) && 
                     !parse_ulong_val(NULL, NULL, type, lGetString(cr, CE_stringval), NULL, 0)) {
                  ERROR((SGE_EVENT, MSG_CPLX_ATTRIBNOCONSUMEH_SS, name, lGetHost(hep, EH_name))) ;
                  answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
                  goto ERROR;
               } 
            }

            for_each (qep, Master_Queue_List) {
               if ((cr = lGetSubStr(qep, CE_name, name, QU_consumable_config_list)) && 
                     !parse_ulong_val(NULL, NULL, type, lGetString(cr, CE_stringval), NULL, 0)) {
                  ERROR((SGE_EVENT, MSG_CPLX_ATTRIBNOCONSUMQ_SS, name, lGetString(qep, QU_qname)));
                  answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
                  goto ERROR;
               }
            }
         }

      }

      lSetList(new_complex, CX_entries, lCopyList("CX_entries", lGetList(ep, CX_entries)));
   }

   DEXIT;
   return 0;

ERROR:
   DEXIT;
   return STATUS_EUNKNOWN;
}

/* ------------------------------------------------------------ */

int complex_spool(
lList **alpp,
lListElem *cep,
gdi_object_t *object 
) {
   char fname[256], real_fname[256];

   DENTER(TOP_LAYER, "complex_spool");

   sprintf(fname, "%s/.%s", COMPLEX_DIR, lGetString(cep, CX_name));
   sprintf(real_fname, "%s/%s", COMPLEX_DIR, lGetString(cep, CX_name));
   if (write_cmplx(1, fname, lGetList(cep, CX_entries), NULL, alpp)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANTSPOOL_SS, MSG_OBJ_CPLX, 
               lGetString(cep, CX_name)));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return 1;
   } else {
      if (rename(fname, real_fname) == -1) {
         DEXIT;
         return 1;
      }  
   }

   DEXIT;
   return 0;
}

/* ------------------------------------------------------------ */

int complex_success(
lListElem *ep,
lListElem *old_ep,
gdi_object_t *object 
) {
   lListElem *jep, *gdil, *qep, *hep;
   int slots, qslots;

   DENTER(TOP_LAYER, "complex_success");

   sge_add_event(NULL, old_ep?sgeE_COMPLEX_MOD:sgeE_COMPLEX_ADD, 0, 0, lGetString(ep, CX_name), ep);
  
   /* throw away all old actual values lists and rebuild them from scratch */
   for_each (qep, Master_Queue_List) {
      lSetList(qep, QU_consumable_actual_list, NULL);
      debit_queue_consumable(NULL, qep, Master_Complex_List, 0);
   }
   for_each (hep, Master_Exechost_List) {
      lSetList(hep, EH_consumable_actual_list, NULL);
      debit_host_consumable(NULL, hep, Master_Complex_List, 0);
   }

   /* completely rebuild consumable_actual_list of all queues and execution hosts
      change versions of corresponding queues */ 
   for_each (jep, Master_Job_List) {
      lListElem* jatep;

      for_each (jatep, lGetList(jep, JB_ja_tasks)) {
         qep = NULL;
         slots = 0;
         for_each (gdil, lGetList(jatep, JAT_granted_destin_identifier_list)) {

            if (!(qep = queue_list_locate(Master_Queue_List, 
                                          lGetString(gdil, JG_qname)))) 
               continue;

            qslots = lGetUlong(gdil, JG_slots);
            debit_host_consumable(jep, host_list_locate(Master_Exechost_List,
                  lGetHost(qep, QU_qhostname)), Master_Complex_List, qslots);
            debit_queue_consumable(jep, qep, Master_Complex_List, qslots);

            slots += qslots;
         }
         debit_host_consumable(jep, host_list_locate(Master_Exechost_List,
            "global"), Master_Complex_List, slots);
      }
   }

   sge_change_queue_version_complex(lGetString(ep, CX_name));
   
   if (!is_nohist())
      write_complex_history(ep);

   DEXIT;
   return 0;
}

/**********************************************************************
 Qmaster function to read the complexes directory
 **********************************************************************/
int read_all_complexes()
{
   DIR *dir;
   SGE_STRUCT_DIRENT *dent;
   char fstr[256];
   int fd;
   lListElem *el;
   lList *answer = NULL;


   DENTER(TOP_LAYER, "read_all_complexes");

   if (!Master_Complex_List) {
      Master_Complex_List = lCreateList("complex list", CX_Type);
   }

   dir = opendir(COMPLEX_DIR);
   if (!dir) {
      ERROR((SGE_EVENT, MSG_FILE_NOOPENDIR_S, COMPLEX_DIR));
      DEXIT;
      return -1;
   }
   if (!sge_silent_get())
      printf(MSG_CONFIG_READINGINCOMPLEXES);

   while ((dent=SGE_READDIR(dir)) != NULL) {
      if (!strcmp(dent->d_name,"..") || !strcmp(dent->d_name,".")) {
         continue;
      }
      if (!sge_silent_get()) {
         printf(MSG_SETUP_COMPLEX_S, dent->d_name);
      }  
      if ((dent->d_name[0] == '.')) {
         char buffer[256]; 
             
         sprintf(buffer, "%s/%s", COMPLEX_DIR, dent->d_name);             
         unlink(buffer);
         continue;
      }

      if (verify_str_key(&answer, dent->d_name, "complex")) {
         DEXIT;
         return -1;
      }    
      sprintf(fstr, "%s/%s", COMPLEX_DIR, dent->d_name);
      
      if ((fd=open(fstr, O_RDONLY)) < 0) {
         ERROR((SGE_EVENT, MSG_FILE_NOOPEN_SS, fstr, strerror(errno)));
         continue;
      }
      close(fd);
      el = read_cmplx(fstr, dent->d_name, &answer);
      if (answer) {
         ERROR((SGE_EVENT, lGetString(lFirst(answer), AN_text)));
         answer = lFreeList(answer);
         DEXIT;
         return -1;
      }
      if (el) {
         lAppendElem(Master_Complex_List, el);
         /*
         ** make a start for the history when Sge first starts up
         ** or when history has been deleted
         */
         if (!is_nohist() && lGetString(el, CX_name) &&
             !is_object_in_history(STR_DIR_COMPLEXES, 
                lGetString(el, CX_name))) {
            int ret;
            
            ret = write_complex_history(el);
            if (ret) {
               WARNING((SGE_EVENT, MSG_FILE_NOWRITEHIST_S, lGetString(el, CX_name)));
            }
         }

      }
   }

   closedir(dir);

   DEXIT;
   return 0;
}


/**********************************************************************
 GDI: delete complex                                             
 **********************************************************************/
int sge_del_cmplx(
lListElem *cxp,
lList **alpp,
char *ruser,
char *rhost 
) {
   lListElem *ep;
   const char *cmplxname;
   int ret;

   DENTER(TOP_LAYER, "sge_del_complex");

   if ( !cxp || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   if (!(cmplxname = lGetString(cxp, CX_name))) {
      /* cxp is no complex element, if cxp has no CX_name */
      CRITICAL((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
                lNm2Str(CX_name), SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* check if complex is in Master_Complex_List */
   if (!(ep = lGetElemStr(Master_Complex_List, CX_name, cmplxname))) {
      ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, MSG_OBJ_CPLX, cmplxname));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* ensure there are no complex lists in
      a queue/host refering to this complex */
   if ((ret=verify_complex_deletion(alpp, cmplxname))!=STATUS_OK) {
      /* answerlist gets filled by complex_list_verify() in case of errors */
      DEXIT;
      return ret;
   }

   /* check if someone tries to delete queue/host/global complexes */
   if (!strcasecmp("host", cmplxname) || 
       !strcasecmp("queue", cmplxname) ||
       !strcasecmp("global", cmplxname))  {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANTDELCMPLX_S, cmplxname));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* If this is the qmaster we delete the complex from disk */
   if (sge_unlink(COMPLEX_DIR, cmplxname)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANTDELCMPLXDISK_S, cmplxname));
      answer_list_add(alpp, SGE_EVENT, STATUS_EDISK, 0);
      DEXIT;
      return STATUS_EDISK;
   }
   sge_add_event(NULL, sgeE_COMPLEX_DEL, 0, 0, cmplxname, NULL);

   /* change versions of corresponding queues */ 
   sge_change_queue_version_complex(cmplxname);

   /* now remove it from our internal list*/
   lRemoveElem(Master_Complex_List, ep);

   INFO((SGE_EVENT, MSG_SGETEXT_REMOVEDFROMLIST_SSSS, ruser, rhost, cmplxname, MSG_OBJ_CPLX));

   answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
   DEXIT;
   return STATUS_OK;
}

/* ----------------------------------------

   written for: qmaster

   increase version of queues having given complex

   having changed a complex we have to increase the versions
   of all queues containing this complex;
*/
static void sge_change_queue_version_complex(
const char *cmplx_name 
) {
   lListElem *ep;

   DENTER(TOP_LAYER, "sge_change_queue_version_complex");

   for_each(ep, Master_Queue_List) {
      sge_change_queue_version(ep, 0, 0);
      cull_write_qconf(1, 0, QUEUE_DIR, lGetString(ep, QU_qname), NULL, ep);
      sge_add_event(NULL, sgeE_QUEUE_MOD, 0, 0, lGetString(ep, QU_qname), ep);
   }
   for_each(ep, Master_Exechost_List)
      sge_add_event(NULL, sgeE_EXECHOST_MOD, 0, 0, lGetHost(ep, EH_name), ep);

   DEXIT;
   return;
}

static int verify_complex_deletion(
lList **alpp,
const char *complex_name 
) {
   int ret = STATUS_OK;
   lListElem *ep;

   DENTER(TOP_LAYER, "verify_userset_deletion");

   for_each (ep, Master_Queue_List) {
      if (lGetElemStr(lGetList(ep, QU_complex_list), CX_name, complex_name)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_COMPLEXSTILLREFERENCED_SSS, 
               complex_name, MSG_OBJ_QUEUE, lGetString(ep, QU_qname)));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         ret = STATUS_EUNKNOWN;
      }
   }

   for_each (ep, Master_Exechost_List) {
      if (lGetElemStr(lGetList(ep, EH_complex_list), CX_name, complex_name)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_COMPLEXSTILLREFERENCED_SSS, 
               complex_name, MSG_OBJ_EH, lGetHost(ep, EH_name)));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         ret = STATUS_EUNKNOWN;
      }
   }

   DEXIT;
   return ret;
}

