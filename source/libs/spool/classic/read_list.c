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

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "sge_bootstrap.h"
#include "sge_unistd.h"

#include "sgermon.h"
#include "sge_log.h"

#include "sge_spool.h"
#include "sge_cqueue_qmaster.h"

#include "cull.h"

#include "commlib.h"
#include "commd_message_flags.h"

#include "sge_host.h"

#include "sge.h"
#include "spool/sge_dirent.h"
#include "sort_hosts.h"          /* JG: TODO: from libsched. Do we need it for spooling? */
#include "sge_complex_schedd.h"  /* JG: TODO: dito */
#include "sge_select_queue.h"    /* JG: TODO: dito */

#include "sge_answer.h"
#include "sge_calendar.h"
#include "sge_ckpt.h"
#include "sge_centry.h"
#include "sge_conf.h"
#include "sge_pe.h"
#include "sge_cqueue.h"
#include "sge_qinstance.h"
#include "sge_qinstance_state.h"
#include "sge_userprj.h"
#include "sge_userset.h"
#include "sge_utility.h"
#include "sge_todo.h"

#include "sge_str.h"

#include "sge_queue_event_master.h"

#include "read_write_cal.h"
#include "read_write_ckpt.h"
#include "read_write_complex.h"
#include "rw_configuration.h"
#include "read_write_host.h"
#include "read_write_pe.h"
#include "read_write_userprj.h"
#include "read_write_userset.h"
#include "read_write_cqueue.h"
#include "read_write_centry.h"
#include "sge_cuser.h"
#include "read_write_ume.h"

#include "sge_hgroup.h"
#include "read_write_host_group.h"
#include "read_write_qinstance.h"

#include "setup_path.h"
#include "sge_uidgid.h"

#include "msg_common.h"
#include "msg_spoollib_classic.h"

#include "read_list.h"

#define CONFIG_TAG_OBSOLETE_VALUE         0x0001

static int reresolve_host(lListElem *ep, int nm, const char *object_name, const char *object_dir);


int sge_read_host_group_entries_from_disk()
{ 
  lList*     direntries = NULL; 
  lListElem* direntry = NULL;
  lListElem* ep = NULL;
  const char*      hostGroupEntry = NULL;
  int        ret = 0;  /* 0 means ok */

  DENTER(TOP_LAYER, "sge_read_host_group_entries_from_disk");
 
 
  direntries = sge_get_dirents(HGROUP_DIR);
  if (direntries) {
     if (Master_HGroup_List == NULL) {
        Master_HGroup_List = lCreateList("", HGRP_Type);
     }  
     if (!sge_silent_get()) { 
        printf(MSG_CONFIG_READINGHOSTGROUPENTRYS);
     }
     
     for_each(direntry, direntries) {
        hostGroupEntry = lGetString(direntry, ST_name);

        if (hostGroupEntry[0] != '.') {
           if (!sge_silent_get()) { 
              printf(MSG_SETUP_HOSTGROUPENTRIES_S, hostGroupEntry);
           }

           ep = cull_read_in_host_group(HGROUP_DIR, hostGroupEntry , 1, 0, NULL, NULL); 
           if (strcmp(hostGroupEntry, lGetHost(ep, HGRP_name))) {
               ERROR((SGE_EVENT, MSG_HGROUP_INCFILE_S, hostGroupEntry));
               return -1;
           }
           lAppendElem(Master_HGroup_List, ep);
        } else {
           sge_unlink(HGROUP_DIR, hostGroupEntry);
        }   
     } 
     direntries = lFreeList(direntries);
  }

  /* everything is done very well ! */
  DEXIT; 
  return ret;
}

#ifndef __SGE_NO_USERMAPPING__

int sge_read_user_mapping_entries_from_disk()
{ 
  lList*     direntries = NULL; 
  lListElem* direntry = NULL;
  lListElem* ep = NULL;
  const char*      ume = NULL;
  int        ret = 0;  /* 0 means ok */

  DENTER(TOP_LAYER, "sge_read_user_mapping_entries_from_disk");
 
  direntries = sge_get_dirents(UME_DIR);
  if (direntries) {
     if (*(cuser_list_get_master_list()) == NULL) {
        *(cuser_list_get_master_list()) = 
           lCreateList("", CU_Type);
     }  
     if (!sge_silent_get()) { 
        printf(MSG_CONFIG_READINGUSERMAPPINGENTRY);
     }
     
     for_each(direntry, direntries) {
         ume = lGetString(direntry, ST_name);

         if (ume[0] != '.') {
            if (!sge_silent_get()) { 
               printf(MSG_SETUP_MAPPINGETRIES_S, ume);
            }

            ep = cull_read_in_ume(UME_DIR, ume , 1, 0, NULL, NULL); 
         
            if (ep != NULL) {
               lAppendElem(Master_Cuser_List, ep);
            } else {
               WARNING((SGE_EVENT, MSG_ANSWER_IGNORINGMAPPINGFOR_S,  ume ));  
               ep = lFreeElem(ep);
               ep = NULL; 
            } 
         } else {
            sge_unlink(UME_DIR, ume);
         }
     } 
     direntries = lFreeList(direntries);
  }
  
  /* everything is done very well ! */
  DEXIT; 
  return ret;
}

#endif

int sge_read_exechost_list_from_disk(lList **list, const char *directory)
{
   lList *direntries;
   lListElem *ep, *direntry;
   const char *host;

   DENTER(TOP_LAYER, "sge_read_exechost_list_from_disk");

   /* 
   ** read exechosts into Master_Exechost_List 
   */
   if (*list == NULL) {
      *list = lCreateList("", EH_Type);
   }

   direntries = sge_get_dirents(directory);
   if(direntries) {
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINEXECUTIONHOSTS);
      
      for_each(direntry, direntries) {

         host = lGetString(direntry, ST_name);
         if (host[0] != '.') {
            DPRINTF(("Host: %s\n", host));
            ep = cull_read_in_host(directory, host, CULL_READ_SPOOL, EH_name, 
                                   NULL, NULL);
            if (!ep) {
               direntries = lFreeList(direntries);
               DEXIT; 
               return -1;
            }

           /* resolve hostname anew */
            if (reresolve_host(ep, EH_name, "exec host", directory)) {
               DEXIT;
               return -1; /* general problems */
            }

            /* necessary to setup actual list of exechost */
            debit_host_consumable(NULL, ep, Master_CEntry_List, 0);

            /* necessary to init double values of consumable configuration */
            centry_list_fill_request(lGetList(ep, EH_consumable_config_list), 
                                     Master_CEntry_List, true, false, true);

            if (ensure_attrib_available(NULL, ep, EH_consumable_config_list)) {
               ep = lFreeElem(ep);
               DEXIT;
               return -1;
            }

            lAppendElem(*list, ep);
         } else {
            sge_unlink(directory, host);
         }
      }
      direntries = lFreeList(direntries);
   }

   DEXIT;
   return 0;
}

int sge_read_adminhost_list_from_disk(lList **list, const char *directory)
{
   lList *direntries;
   lListElem *ep, *direntry;
   const char *host;

   DENTER(TOP_LAYER, "sge_read_adminhost_list_from_disk");

   /* 
   ** read adminhosts into Master_Adminhost_List 
   */
   if (*list == NULL) {
      *list = lCreateList("", AH_Type);
   }

   direntries = sge_get_dirents(directory);
   if(direntries) {
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINADMINHOSTS);
      for_each(direntry, direntries) {
         host = lGetString(direntry, ST_name);

         if (host[0] != '.') {
            DPRINTF(("Host: %s\n", host));
            ep = cull_read_in_host(directory, host, CULL_READ_SPOOL, AH_name, NULL, NULL);
            if (!ep) {
               direntries = lFreeList(direntries);
               DEXIT; 
               return -1;
            } 

            /* resolve hostname anew */
            if (reresolve_host(ep, AH_name, "admin host", directory)) {
               direntries = lFreeList(direntries);
               DEXIT;
               return -1; /* general problems */
            }

            lAppendElem(*list, ep);
         } else {
            sge_unlink(directory, host);  
         }
      }
      direntries = lFreeList(direntries);
   }

   DEXIT;
   return 0;
}

int sge_read_submithost_list_from_disk(lList **list, const char *directory)
{
   lList *direntries;
   lListElem *ep, *direntry;
   const char *host;

   DENTER(TOP_LAYER, "sge_read_submithost_list_from_disk");

   /* 
   ** read submithosts into Master_Submithost_List 
   */
   if (*list == NULL) {
      *list = lCreateList("", SH_Type);
   }

   direntries = sge_get_dirents(directory);
   if(direntries) {
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINSUBMITHOSTS);
      for_each(direntry, direntries) {
         host = lGetString(direntry, ST_name);
         if (host[0] != '.') {
            DPRINTF(("Host: %s\n", host));
            ep = cull_read_in_host(directory, host, CULL_READ_SPOOL, 
               SH_name, NULL, NULL);
            if (!ep) {
               direntries = lFreeList(direntries);
               DEXIT; 
               return -1;
            } 

            /* resolve hostname anew */
            if (reresolve_host(ep, SH_name, "submit host", directory)) {
               DEXIT;
               return -1; /* general problems */
            }

            lAppendElem(*list, ep);
         } else {
            sge_unlink(directory, host);             
         }
      }
      direntries = lFreeList(direntries);
   }

   DEXIT;
   return 0;
}

int sge_read_pe_list_from_disk(lList **list, const char *directory)
{
   lList *direntries;
   lList *alp = NULL;
   lListElem *ep, *direntry;
   int ret = 0;
   const char *pe;

   DENTER(TOP_LAYER, "sge_read_pe_list_from_disk");
   
   if (*list == NULL) {
      *list = lCreateList("", PE_Type);
   }

   direntries = sge_get_dirents(directory);
   if(direntries) {
      if (!sge_silent_get()) {
         printf(MSG_CONFIG_READINGINGPARALLELENV);
      }
      for_each(direntry, direntries) {
         pe = lGetString(direntry, ST_name);
         if (pe[0] != '.') {
            if (!sge_silent_get()) {
               printf(MSG_SETUP_PE_S, pe);
            }
            if (verify_str_key(&alp, pe, "pe")) {
               DEXIT;
               return -1;
            }       
            ep = cull_read_in_pe(directory, pe, 1, 0, NULL, NULL);
            if (!ep) {
               ret = -1;
               break;
            }

            if (pe_validate(ep, NULL, 1)!=STATUS_OK) {
               ret = -1;
               break;
            }
            lAppendElem(*list, ep);
         } else {
            sge_unlink(directory, pe);
         }
      }
      direntries = lFreeList(direntries);
   }

   DEXIT;
   return ret;
}



int sge_read_cal_list_from_disk(lList **list, const char *directory)
{
   lList *direntries;
   lListElem *aep, *ep, *direntry;
   int ret = 0;
   const char *cal;
   const char *s;
   lList *alp = NULL;

   DENTER(TOP_LAYER, "sge_read_cal_list_from_disk");
   
   if (*list == NULL) {
      *list = lCreateList("", CAL_Type);
   }

   direntries = sge_get_dirents(directory);
   if(direntries) {
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINCALENDARS);
      for_each(direntry, direntries) {
         cal = lGetString(direntry, ST_name);

         if (cal[0] != '.') {
            if (!sge_silent_get()) {
               printf(MSG_SETUP_CALENDAR_S, cal);
            }
            if (verify_str_key(&alp, cal, "cal")) {
               DEXIT;
               return -1;
            }      
            ep = cull_read_in_cal(directory, cal, 1, 0, NULL, NULL);
            if (!ep) {
               ret = -1;
               break;
            }

            if (!calendar_parse_year(ep, &alp) || 
                !calendar_parse_week(ep, &alp)) {
               if (!(aep = lFirst(alp)) || !(s = lGetString(aep, AN_text)))
                  s = MSG_UNKNOWNREASON;
               ERROR((SGE_EVENT,MSG_CONFIG_FAILEDPARSINGYEARENTRYINCALENDAR_SS, 
                     cal, s));
               lFreeList(alp);
               ret = -1;
               break;
            }

            lAppendElem(*list, ep);
         } else {
            sge_unlink(directory, cal);  
         }
      }
      direntries = lFreeList(direntries);
   }

   DEXIT;
   return ret;
}

int sge_read_ckpt_list_from_disk(lList **list, const char *directory)
{
   lList *direntries;
   lListElem *ep, *direntry;
   const char *ckpt;

   DENTER(TOP_LAYER, "sge_read_ckpt_list_from_disk");
   
   if (*list == NULL) {
      *list = lCreateList("", CK_Type);
   } 

   direntries = sge_get_dirents(directory);
   if(direntries) {
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINCKPTINTERFACEDEFINITIONS);
      for_each(direntry, direntries) {
         ckpt = lGetString(direntry, ST_name);

         if (ckpt[0] != '.') {
            if (!sge_silent_get()) 
               printf(MSG_SETUP_CKPT_S, ckpt);
            ep = cull_read_in_ckpt(directory, ckpt, 1, 0, NULL, NULL);
            if (!ep) {
               DEXIT;
               return -1;
            }

            if (ckpt_validate(ep, NULL)!=STATUS_OK) {
               DEXIT;
               return -1;
            }
            
            lAppendElem(*list, ep);
         } else {
            sge_unlink(directory, ckpt);
         }
      }
      direntries = lFreeList(direntries);
   }
   DEXIT;
   return 0;
}

int sge_read_qinstance_list_from_disk(lListElem *cqueue)
{
   lList *dir_list;
   dstring qinstance_dir = DSTRING_INIT;
   const char *cqueue_name = lGetString(cqueue, CQ_name);

   DENTER(TOP_LAYER, "sge_read_qinstance_list_from_disk");

   sge_dstring_sprintf(&qinstance_dir, "%s/%s", QINSTANCES_DIR, cqueue_name);
   if (sge_is_directory(sge_dstring_get_string(&qinstance_dir))) {
      dir_list = sge_get_dirents(sge_dstring_get_string(&qinstance_dir));
      if (dir_list) {
         lListElem *dir;
         lList *qinstance_list = lGetList(cqueue, CQ_qinstances);
         
         for_each(dir, dir_list) {
            const char *hostname = lGetString(dir, ST_name);
            lListElem *qinstance = NULL;

            if (hostname[0] != '.') {
               qinstance = cull_read_in_qinstance(
                                        sge_dstring_get_string(&qinstance_dir), 
                                        hostname, 1, 0, NULL, NULL);
               if (qinstance == NULL) {
                  ERROR((SGE_EVENT, MSG_CONFIG_READINGFILE_SS, 
                         sge_dstring_get_string(&qinstance_dir), hostname));
                  DEXIT;
                  return -1;
               }
             
               if (qinstance_list == NULL) {
                  qinstance_list = lCreateList("", QU_Type);
                  lSetList(cqueue, CQ_qinstances, qinstance_list);
               } 
               lAppendElem(qinstance_list, qinstance);
            } else {
               sge_unlink(sge_dstring_get_string(&qinstance_dir), hostname);
            }
         }
         lFreeList(dir_list);
      }
   }
   sge_dstring_free(&qinstance_dir);

   DEXIT;
   return 0;
}   

int sge_read_cqueue_list_from_disk(lList **list, const char *directory)
{
   lList *alp = NULL, *direntries;
   lListElem *qep, *direntry, *exec_host;
   int config_tag = 0;

   DENTER(TOP_LAYER, "sge_read_cqueue_list_from_disk");

   if (*list == NULL) {
      *list = lCreateList("", CQ_Type);
   }

   direntries = sge_get_dirents(directory);
   if (direntries) {
      const char *queue_str;
      
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINQUEUES);
      for_each(direntry, direntries) {

         queue_str = lGetString(direntry, ST_name);
         if (queue_str[0] != '.') {
            config_tag = 0;
            if (!sge_silent_get()) {
               printf(MSG_SETUP_QUEUE_S, lGetString(direntry, ST_name));
            }
            if (verify_str_key(&alp, queue_str, "cqueue")) {
               DEXIT;
               return -1;
            }   
            qep = cull_read_in_cqueue(directory, 
                                      lGetString(direntry, ST_name), 1, 
                                      0, &config_tag, NULL);
            if (!qep) {
               ERROR((SGE_EVENT, MSG_CONFIG_READINGFILE_SS, directory, 
                      lGetString(direntry, ST_name)));
               DEXIT;
               return -1;
            }
            
            if (!strcmp(lGetString(direntry, ST_name), SGE_TEMPLATE_NAME) && 
                !strcmp(lGetString(qep, CQ_name), SGE_TEMPLATE_NAME)) {
               /* 
                  we do not keep the queue template in the main queue list 
                  to be compatible with other old code in the qmaster
               */
               qep = lFreeElem(qep);
               sge_unlink(directory, lGetString(direntry, ST_name));
               WARNING((SGE_EVENT, MSG_CONFIG_OBSOLETEQUEUETEMPLATEFILEDELETED));
            }
            else if (!strcmp(lGetString(qep, CQ_name), SGE_TEMPLATE_NAME)) {
               /*
                  oops!  found queue 'template', but not in file 'template'
               */
               ERROR((SGE_EVENT, MSG_CONFIG_FOUNDQUEUETEMPLATEBUTNOTINFILETEMPLATEIGNORINGIT));
               qep = lFreeElem(qep);
            } else {
               lList *qinstance_list = NULL;
               lListElem *qinstance = NULL;

               sge_read_qinstance_list_from_disk(qep);
               qinstance_list = lGetList(qep, CQ_qinstances);
               for_each(qinstance, qinstance_list) {
                  lList *master_list = *(centry_list_get_master_list());
                  lList *ccl = NULL;

                  /* 
                   * handle slots from now on as a consumble 
                   * attribute of queue 
                   */
                  qinstance_set_conf_slots_used(qinstance);

                  /* setup actual list of queue */
                  qinstance_debit_consumable(qinstance, NULL, master_list, 0);

                  ccl = lGetList(qinstance, QU_consumable_config_list);
                  centry_list_fill_request(ccl, master_list, 
                                           true, false, true); 
                  if (ensure_attrib_available(NULL, qinstance, QU_load_thresholds) ||
                      ensure_attrib_available(NULL, qinstance, QU_suspend_thresholds) ||
                      ensure_attrib_available(NULL, qinstance, QU_consumable_config_list)) {
                     qep = lFreeElem(qep); 
                     DEXIT;
                     return -1;
                  }
                  qinstance_state_set_unknown(qinstance, true);
                  qinstance_state_set_cal_disabled(qinstance, false);
                  qinstance_state_set_cal_suspended(qinstance, false);
                  qinstance_set_slots_used(qinstance, 0);
                  
                  if (!(exec_host = host_list_locate(Master_Exechost_List, 
                        lGetHost(qinstance, QU_qhostname)))) {

                     ERROR((SGE_EVENT, MSG_CONFIG_CANTRECREATEQEUEUE_SS,
                            lGetString(qinstance, QU_qname), 
                            lGetHost(qinstance, QU_qhostname)));
                     qep = lFreeElem(qep);
                     lFreeList(direntries);
                     DEXIT;
                     return -1;
                  } 
               }
               cqueue_list_add_cqueue(*list, qep);
            }
         } else {
            sge_unlink(directory, queue_str);
         }
      }
      lFreeList(direntries);
   }
   

   DEXIT;
   return 0;
}   


int sge_read_project_list_from_disk(lList **list, const char *directory)
{
   lList *alp = NULL, *direntries;
   lListElem *ep, *direntry;
   int config_tag = 0;

   DENTER(TOP_LAYER, "sge_read_project_list_from_disk");

   if (*list == NULL) {
      *list = lCreateList("", UP_Type);
   }
   
   direntries = sge_get_dirents(directory);
   if (direntries) {
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINPROJECTS);

      for_each(direntry, direntries) {
         const char *userprj_str;

         userprj_str = lGetString(direntry, ST_name);
         if (userprj_str[0] != '.') {
            config_tag = 0;
            if (!sge_silent_get()) 
               printf(MSG_SETUP_PROJECT_S, lGetString(direntry, ST_name));
            if (verify_str_key(&alp, userprj_str, "project")) {
               DEXIT;
               return -1;
            }  
            ep = cull_read_in_userprj(directory, lGetString(direntry, ST_name), 1,
                                       0, &config_tag);
            if (!ep) {
               ERROR((SGE_EVENT, MSG_CONFIG_READINGFILE_SS, directory, 
                      lGetString(direntry, ST_name)));
               DEXIT;
               return -1;
            }
            if (strcmp(lGetString(ep, UP_name), lGetString(direntry, ST_name))) {
               ERROR((SGE_EVENT, MSG_QMASTER_PRJINCORRECT_S,
                      lGetString(direntry, ST_name)));
               DEXIT;
               return -1;
            }
            lAppendElem(*list, ep);
         } else {
            sge_unlink(directory, userprj_str);
         }
      }
      lFreeList(direntries);
   }

   DEXIT;
   return 0;
}   

int sge_read_user_list_from_disk(lList **list, const char *directory)
{
   lList *direntries;
   lListElem *ep, *direntry;
   int config_tag = 0;

   DENTER(TOP_LAYER, "sge_read_user_list_from_disk");

   if (*list == NULL) {
      *list = lCreateList("", UP_Type);
   }

   direntries = sge_get_dirents(directory);
   if (direntries) {
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINUSERS);
      
      for_each(direntry, direntries) {
         const char *direntry_str;
         
         direntry_str = lGetString(direntry, ST_name); 
         if (direntry_str[0] != '.') { 
            config_tag = 0;
            if (!sge_silent_get()) 
               printf(MSG_SETUP_USER_S, lGetString(direntry, ST_name));

            ep = cull_read_in_userprj(directory, lGetString(direntry, ST_name), 1,
                                       1, &config_tag);
            if (!ep) {
               ERROR((SGE_EVENT, MSG_CONFIG_READINGFILE_SS, directory, 
                        lGetString(direntry, ST_name)));
               DEXIT;
               return -1;
            }

            lAppendElem(*list, ep);
         } else {
            sge_unlink(directory, direntry_str);
         }
      }
      direntries = lFreeList(direntries);
   }

   DEXIT;
   return 0;
}

int sge_read_userset_list_from_disk(lList **list, const char *directory)
{
   lList *alp = NULL, *direntries;
   lListElem *ep, *direntry;

   DENTER(TOP_LAYER, "sge_read_userset_list_from_disk");

   if (*list == NULL) {
      *list = lCreateList("", US_Type);
   }

   direntries = sge_get_dirents(directory);
   if (direntries) {
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINUSERSETS);

      for_each(direntry, direntries) {
         const char *userset = lGetString(direntry, ST_name);

         if (userset[0] != '.') {
            if (!sge_silent_get()) {
               printf(MSG_SETUP_USERSET_S , lGetString(direntry, ST_name));
            }
            if (verify_str_key(&alp, userset, "userset")) {
               DEXIT;
               return -1;
            }  

            ep = cull_read_in_userset(directory, userset, 1, 0, NULL); 
            if (!ep) {
               ERROR((SGE_EVENT, MSG_CONFIG_READINGFILE_SS, directory, 
                        userset));
               DEXIT;
               return -1;
            }

            if(userset_validate_entries(ep, NULL, 1) == STATUS_OK) {
               lAppendElem(*list, ep);
            } else {
               lFreeElem(ep);
            }
         } else {
            sge_unlink(directory, userset);
         }
      }
      direntries = lFreeList(direntries);
   }
   
   DEXIT;
   return 0;
}


static int reresolve_host(
lListElem *ep,
int nm,
const char *object_name,
const char *object_dir 
) {
   char *old_name;
   const char *new_name;
   int ret;
   int pos;
   int dataType;

   DENTER(TOP_LAYER, "reresolve_host");


   pos = lGetPosViaElem(ep, nm);
   dataType = lGetPosType(lGetElemDescr(ep),pos);
   if (dataType == lHostT) {
      old_name = strdup(lGetHost(ep, nm));
   } else {
      old_name = strdup(lGetString(ep, nm));
   }
   ret = sge_resolve_host(ep, nm);
#ifdef ENABLE_NGC
   if (ret != CL_RETVAL_OK ) {
      if (ret != CL_RETVAL_GETHOSTNAME_ERROR ) {
         /* finish qmaster setup only if hostname resolving
            does not work at all generally or a timeout
            indicates that commd itself blocks in resolving
            a host, e.g. when DNS times out */
         ERROR((SGE_EVENT, MSG_CONFIG_CANTRESOLVEHOSTNAMEX_SSS, object_name, old_name, cl_get_error_text(ret)));
         free(old_name);
         DEXIT;
         return -1;
      }
      WARNING((SGE_EVENT, MSG_CONFIG_CANTRESOLVEHOSTNAMEX_SS, object_name, old_name));
   }
#else
   if (ret != CL_OK ) {
      if (ret != COMMD_NACK_UNKNOWN_HOST && ret != COMMD_NACK_TIMEOUT) {
         /* finish qmaster setup only if hostname resolving
            does not work at all generally or a timeout
            indicates that commd itself blocks in resolving
            a host, e.g. when DNS times out */
         ERROR((SGE_EVENT, MSG_CONFIG_CANTRESOLVEHOSTNAMEX_SSS,
                  object_name, old_name, cl_errstr(ret)));
         free(old_name);
         DEXIT;
         return -1;
      }
      WARNING((SGE_EVENT, MSG_CONFIG_CANTRESOLVEHOSTNAMEX_SS,
               object_name, old_name));
   }
#endif

   /* rename config file if resolving changed name */
   if (dataType == lHostT) {
      new_name = lGetHost(ep, nm);
   } else {
      new_name = lGetString(ep, nm);
   }
   if (strcmp(old_name, new_name)) {
      if (!write_host(1, 2, ep, nm, NULL)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_CANTSPOOL_SS, object_name, new_name));
         free(old_name);
         DEXIT;
         return -1;
      }
      sge_unlink(object_dir, old_name);
   }
   free(old_name);

   DEXIT;
   return 0;
}

int read_all_centries(lList **list, const char *directory)
{
   DIR *dir;
   SGE_STRUCT_DIRENT *dent;
   char fstr[256];
   int fd;
   lListElem *el;
   lList *answer = NULL;


   DENTER(TOP_LAYER, "read_all_centries");

   if (*list == NULL) {
      *list = lCreateList("", CE_Type);
   }

   dir = opendir(directory);
   if (!dir) {
      ERROR((SGE_EVENT, MSG_FILE_NOOPENDIR_S, directory));
      DEXIT;
      return -1;
   }
   if (!sge_silent_get())
      printf(MSG_CONFIG_READINGINCOMPLEXATTRS);

   while ((dent=SGE_READDIR(dir)) != NULL) {
      if (!strcmp(dent->d_name,"..") || !strcmp(dent->d_name,".")) {
         continue;
      }
#if 0
      if (!sge_silent_get()) {
         printf(MSG_SETUP_COMPLEX_ATTR_S, dent->d_name);
      }  
#endif
      if ((dent->d_name[0] == '.')) {
         sge_unlink(directory, dent->d_name);
         continue;
      }

      sprintf(fstr, "%s/%s", directory, dent->d_name);
      
      if ((fd=open(fstr, O_RDONLY)) < 0) {
         ERROR((SGE_EVENT, MSG_FILE_NOOPEN_SS, fstr, strerror(errno)));
         continue;
      }
      close(fd);
      el = cull_read_in_centry(directory, dent->d_name , 1,0, *list);
      if (answer) {
         ERROR((SGE_EVENT, lGetString(lFirst(answer), AN_text)));
         answer = lFreeList(answer);
         DEXIT;
         return -1;
      }
      if (el) {
         lAppendElem(*list, el);
      }
   }

   closedir(dir);

   centry_list_sort(*list);

   DEXIT;
   return 0;
}

/*----------------------------------------------------
 * read_all_configurations
 * qmaster function to read all configurations
 * should work with absolute pathnames
 * Must be called after internal setup
 *----------------------------------------------------*/

 
int read_all_configurations(lList **lpp, 
                            const char *global_config_file, 
                            const char *local_config_dir)
{
   DIR *dir;
   SGE_STRUCT_DIRENT *dent;
   char fstr[256];
   lListElem *el;
   int ret;
   static int admin_user_initialized = 0;

   DENTER(TOP_LAYER, "read_all_configurations");

   if (!lpp) {
      DEXIT;
      return -1;
   }

   if (local_config_dir == NULL) {
      DEXIT;
      SGE_EXIT(1);
   }

   if (!*lpp) {
      *lpp = lCreateList("conf list", CONF_Type);
   }

   /* First read global configuration. */
   el = read_configuration(global_config_file, SGE_GLOBAL_NAME, FLG_CONF_SPOOL);
   if (el)
      lAppendElem(*lpp, el);
   else {
      DEXIT;
      SGE_EXIT(1);
   }

   if (!admin_user_initialized) {
      const char *admin_user = NULL;
      char err_str[MAX_STRING_SIZE];
      int lret;

      admin_user = bootstrap_get_admin_user();
      lret = sge_set_admin_username(admin_user, err_str);
      if (lret == -1) {
         ERROR((SGE_EVENT, err_str));
         DEXIT;
         return -1;
      }
      admin_user_initialized = 1;
   }

   
   /* read local configurations from local_conf_dir */ 

   dir = opendir(local_config_dir);
   if (!dir) {
      DEXIT;
      return -2;
   }

   while ((dent=SGE_READDIR(dir)) != NULL) {
      if (!dent->d_name)
                  continue;              /* May happen */
      if (!dent->d_name[0])
                  continue;              /* May happen */
                              
      if (!strcmp(dent->d_name,"..") || !strcmp(dent->d_name,"."))
         continue;

      sprintf(fstr, "%s/%s", local_config_dir, dent->d_name);
      
      el = read_configuration(fstr, dent->d_name, FLG_CONF_SPOOL);
      if (!el)
         continue;

      {
         char fname[SGE_PATH_MAX], real_fname[SGE_PATH_MAX];
         const char *new_name;
         char *old_name;
         lList *alp = NULL;

         /* resolve config name */
         old_name = strdup(lGetHost(el, CONF_hname));

         ret = sge_resolve_host(el, CONF_hname);
#ifdef ENABLE_NGC
         if (ret != CL_RETVAL_OK) {
            if (ret != CL_RETVAL_GETHOSTNAME_ERROR  ) {
               ERROR((SGE_EVENT, MSG_CONFIG_CANTRESOLVEHOSTNAMEX_SSS, "local configuration", old_name, cl_get_error_text(ret)));
               free(old_name);
               DEXIT;
               return -1;
            }
            WARNING((SGE_EVENT, MSG_CONFIG_CANTRESOLVEHOSTNAMEX_SS, "local configuration", old_name));
         }
#else
         if (ret != CL_OK) {
            if (ret != COMMD_NACK_UNKNOWN_HOST && ret != COMMD_NACK_TIMEOUT) {
               ERROR((SGE_EVENT, MSG_CONFIG_CANTRESOLVEHOSTNAMEX_SSS,
                        "local configuration", old_name, cl_errstr(ret)));
               free(old_name);
               DEXIT;
               return -1;
            }
            WARNING((SGE_EVENT, MSG_CONFIG_CANTRESOLVEHOSTNAMEX_SS,
                  "local configuration", old_name));
         }
#endif
         new_name = lGetHost(el, CONF_hname);

         /* simply ignore it if it exists already */
         if (*lpp && lGetElemHost(*lpp, CONF_hname, new_name)) {
            free(old_name);
            lFreeElem(el);
            continue;
         }

         /* rename config file if resolving changed name */
         if (strcmp(old_name, new_name)) {

            sprintf(fname, "%s/.%s", local_config_dir, new_name);
            sprintf(real_fname, "%s/%s", local_config_dir, new_name);

            DPRINTF(("global_config_file: %s\n", fname));
            if ((ret=write_configuration(1, &alp, fname, el, NULL, FLG_CONF_SPOOL))) {
               /* answer list gets filled in write_configuration() */
               free(old_name);
               sge_switch2start_user();
               DEXIT;
               return -1;
            } else {
               char old_fname[SGE_PATH_MAX];

               if (rename(fname, real_fname) == -1) {
                  free(old_name);
                  DEXIT;
                  return -1;
               }
               sprintf(old_fname, "%s/%s", local_config_dir, old_name);
               if (sge_unlink(NULL, old_fname)) {
                  DEXIT;
                  return -1;
               }
            }
         }
         lFreeList(alp);
         free(old_name);
      }

      lAppendElem(*lpp, el);
   }

   closedir(dir);

   DEXIT;
   return 0;
}


