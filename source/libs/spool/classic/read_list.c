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

#include "sge_unistd.h"

#include "sgermon.h"
#include "sge_log.h"

#include "sge_spool.h"

#include "cull.h"

#include "commlib.h"
#include "commd_message_flags.h"

#include "sge_host.h"

#include "sge.h"
#include "sge_dirent.h"
#include "gdi_utility.h"
#include "sort_hosts.h"          /* JG: TODO: from libsched. Do we need it for spooling? */
#include "sge_complex_schedd.h"  /* JG: TODO: dito */
#include "slots_used.h"          /* JG: TODO: dito */
#include "sge_select_queue.h"    /* JG: TODO: dito */
#include "opt_history.h"
#include "complex_history.h"
#include "path_history.h"

#include "sge_answer.h"
#include "sge_calendar.h"
#include "sge_ckpt.h"
#include "sge_complex.h"
#include "sge_conf.h"
#include "sge_host.h"
#include "sge_pe.h"
#include "sge_queue.h"
#include "sge_userprj.h"
#include "sge_userset.h"
#include "sge_utility.h"
#include "sge_todo.h"

#include "sge_stringL.h"

#include "sge_queue_event_master.h"

#include "read_write_cal.h"
#include "read_write_ckpt.h"
#include "read_write_complex.h"
#include "rw_configuration.h"
#include "read_write_host.h"
#include "read_write_pe.h"
#include "read_write_queue.h"
#include "read_write_userprj.h"
#include "read_write_userset.h"

#ifndef __SGE_NO_USERMAPPING__
#include "sge_usermap.h"
#include "read_write_ume.h"

#include "sge_hostgroup.h"
#include "read_write_host_group.h"
#endif


#include "setup_path.h"
#include "sge_uidgid.h"

#include "msg_common.h"
#include "msg_spoollib_classic.h"

#include "read_list.h"

#define CONFIG_TAG_OBSOLETE_VALUE         0x0001

static int reresolve_host(lListElem *ep, int nm, char *object_name, char *object_dir);


#ifndef __SGE_NO_USERMAPPING__
int sge_read_host_group_entries_from_disk()
{ 
  lList*     direntries = NULL; 
  lListElem* direntry = NULL;
  lListElem* ep = NULL;
  const char*      hostGroupEntry = NULL;
  int        ret = 0;  /* 0 means ok */

  DENTER(TOP_LAYER, "sge_read_host_group_entries_from_disk");
 
 
  direntries = sge_get_dirents(HOSTGROUP_DIR);
  if (direntries) {
     if (Master_Host_Group_List == NULL) {
        Master_Host_Group_List = lCreateList("main host group list", GRP_Type);
     }  
     if (!sge_silent_get()) { 
        printf(MSG_CONFIG_READINGHOSTGROUPENTRYS);
     }
     
     for_each(direntry, direntries) {
        hostGroupEntry = lGetString(direntry, STR);

        if (hostGroupEntry[0] != '.') {
           if (!sge_silent_get()) { 
              printf(MSG_SETUP_HOSTGROUPENTRIES_S, hostGroupEntry);
           }

           ep = cull_read_in_host_group(HOSTGROUP_DIR, hostGroupEntry , 1, 0, NULL); 
           lAppendElem(Master_Host_Group_List, ep);
        } else {
           sge_unlink(HOSTGROUP_DIR, hostGroupEntry);
        }   
     } 
     direntries = lFreeList(direntries);
 
     ep = Master_Host_Group_List->first;  

     while (ep != NULL) {
        hostGroupEntry = lGetString(ep, GRP_group_name);
        if (hostGroupEntry != NULL) {
           DPRINTF(("----------------> checking group '%s'\n",hostGroupEntry));
        }  
        if (sge_verify_host_group_entry(NULL, Master_Host_Group_List,ep,hostGroupEntry) == FALSE) {
           WARNING((SGE_EVENT, MSG_ANSWER_IGNORINGHOSTGROUP_S, hostGroupEntry  ));
           
           lDechainElem(Master_Host_Group_List, ep);
           lFreeElem(ep);
           ep = NULL;
           ep = Master_Host_Group_List->first;
        } else {
           ep = ep->next;
        }
     } 
  }

  /* everything is done very well ! */
  DEXIT; 
  return ret;
}


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
     if (Master_Usermapping_Entry_List == NULL) {
        Master_Usermapping_Entry_List = 
           lCreateList("Master_Usermapping_Entry_List", UME_Type);
     }  
     if (!sge_silent_get()) { 
        printf(MSG_CONFIG_READINGUSERMAPPINGENTRY);
     }
     
     for_each(direntry, direntries) {
         ume = lGetString(direntry, STR);

         if (ume[0] != '.') {
            if (!sge_silent_get()) { 
               printf(MSG_SETUP_MAPPINGETRIES_S, ume);
            }

            ep = cull_read_in_ume(UME_DIR, ume , 1, 0, NULL); 
         
            if (sge_verifyMappingEntry(NULL, Master_Host_Group_List,ep, ume, 
               Master_Usermapping_Entry_List) == TRUE) {
               lAppendElem(Master_Usermapping_Entry_List, ep);
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

int sge_read_host_list_from_disk()
{
   int ret;

   DENTER(TOP_LAYER, "sge_read_host_list_from_disk");

   if((ret = sge_read_exechost_list_from_disk()) != 0) {
      DEXIT;
      return ret;
   }
   
   if((ret = sge_read_adminhost_list_from_disk()) != 0) {
      DEXIT;
      return ret;
   }

   if((ret = sge_read_submithost_list_from_disk()) != 0) {
      DEXIT;
      return ret;
   }

   DEXIT;
   return 0;
}

int sge_read_exechost_list_from_disk()
{
   lList *direntries;
   lListElem *ep, *direntry;
   const char *host;

   DENTER(TOP_LAYER, "sge_read_exechost_list_from_disk");

   /* 
   ** read exechosts into Master_Exechost_List 
   */
   if (!Master_Exechost_List)
      Master_Exechost_List = lCreateList("Master_Exechost_List", EH_Type);

   direntries = sge_get_dirents(EXECHOST_DIR);
   if(direntries) {
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINEXECUTIONHOSTS);
      
      for_each(direntry, direntries) {

         host = lGetString(direntry, STR);
         if (host[0] != '.') {
            DPRINTF(("Host: %s\n", host));
            ep = cull_read_in_host(EXECHOST_DIR, host, CULL_READ_SPOOL, EH_name, 
                                   NULL, NULL);
            if (!ep) {
               direntries = lFreeList(direntries);
               DEXIT; 
               return -1;
            }

           /* resolve hostname anew */
            if (reresolve_host(ep, EH_name, "exec host", EXECHOST_DIR)) {
               DEXIT;
               return -1; /* general problems */
            }

            /* necessary to setup actual list of exechost */
            debit_host_consumable(NULL, ep, Master_Complex_List, 0);

            /* necessary to init double values of consumable configuration */
            sge_fill_requests(lGetList(ep, EH_consumable_config_list), 
                  Master_Complex_List, 1, 0, 1);

            if (complex_list_verify(lGetList(ep, EH_complex_list), NULL, 
                                    "host", lGetHost(ep, EH_name))!=STATUS_OK) {
               DEXIT;
               return -1;
            }
            if (ensure_attrib_available(NULL, ep, EH_consumable_config_list)) {
               ep = lFreeElem(ep);
               DEXIT;
               return -1;
            }

            lAppendElem(Master_Exechost_List, ep);
            /*
            ** make a start for the history when Sge first starts up
            ** or when history has been deleted
            */
            if (!is_nohist() && lGetHost(ep, EH_name) &&
                !is_object_in_history(STR_DIR_EXECHOSTS, 
                   lGetHost(ep, EH_name))) {
               int ret;
            
               ret = write_host_history(ep);
               if (ret) {
                  WARNING((SGE_EVENT, MSG_CONFIG_CANTWRITEHISTORYFORHOSTX_S,
                           lGetHost(ep, EH_name)));
               }
            }
         } else {
            sge_unlink(EXECHOST_DIR, host);
         }
      }
      direntries = lFreeList(direntries);
   }

   DEXIT;
   return 0;
}

int sge_read_adminhost_list_from_disk()
{
   lList *direntries;
   lListElem *ep, *direntry;
   const char *host;

   DENTER(TOP_LAYER, "sge_read_adminhost_list_from_disk");

   /* 
   ** read adminhosts into Master_Adminhost_List 
   */
   if (!Master_Adminhost_List)
      Master_Adminhost_List = lCreateList("Master_Adminhost_List", AH_Type);

   direntries = sge_get_dirents(ADMINHOST_DIR);
   if(direntries) {
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINADMINHOSTS);
      for_each(direntry, direntries) {
         host = lGetString(direntry, STR);

         if (host[0] != '.') {
            DPRINTF(("Host: %s\n", host));
            ep = cull_read_in_host(ADMINHOST_DIR, host, CULL_READ_SPOOL, AH_name, NULL, NULL);
            if (!ep) {
               direntries = lFreeList(direntries);
               DEXIT; 
               return -1;
            } 

            /* resolve hostname anew */
            if (reresolve_host(ep, AH_name, "admin host", ADMINHOST_DIR)) {
               direntries = lFreeList(direntries);
               DEXIT;
               return -1; /* general problems */
            }

            lAppendElem(Master_Adminhost_List, ep);
         } else {
            sge_unlink(ADMINHOST_DIR, host);  
         }
      }
      direntries = lFreeList(direntries);
   }

   DEXIT;
   return 0;
}

int sge_read_submithost_list_from_disk()
{
   lList *direntries;
   lListElem *ep, *direntry;
   const char *host;

   DENTER(TOP_LAYER, "sge_read_submithost_list_from_disk");

   /* 
   ** read submithosts into Master_Submithost_List 
   */
   if (!Master_Submithost_List)
      Master_Submithost_List = lCreateList("Master_Submithost_List", SH_Type);

   direntries = sge_get_dirents(SUBMITHOST_DIR);
   if(direntries) {
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINSUBMITHOSTS);
      for_each(direntry, direntries) {
         host = lGetString(direntry, STR);
         if (host[0] != '.') {
            DPRINTF(("Host: %s\n", host));
            ep = cull_read_in_host(SUBMITHOST_DIR, host, CULL_READ_SPOOL, 
               SH_name, NULL, NULL);
            if (!ep) {
               direntries = lFreeList(direntries);
               DEXIT; 
               return -1;
            } 

            /* resolve hostname anew */
            if (reresolve_host(ep, SH_name, "submit host", SUBMITHOST_DIR)) {
               DEXIT;
               return -1; /* general problems */
            }

            lAppendElem(Master_Submithost_List, ep);
         } else {
            sge_unlink(SUBMITHOST_DIR, host);             
         }
      }
      direntries = lFreeList(direntries);
   }

   DEXIT;
   return 0;
}

int sge_read_pe_list_from_disk()
{
   lList *direntries;
   lList *alp = NULL;
   lListElem *ep, *direntry;
   int ret = 0;
   const char *pe;

   DENTER(TOP_LAYER, "sge_read_pe_list_from_disk");
   
   if (!Master_Pe_List)
      Master_Pe_List = lCreateList("Master_Pe_List", PE_Type);

   direntries = sge_get_dirents(PE_DIR);
   if(direntries) {
      if (!sge_silent_get()) {
         printf(MSG_CONFIG_READINGINGPARALLELENV);
      }
      for_each(direntry, direntries) {
         pe = lGetString(direntry, STR);
         if (pe[0] != '.') {
            if (!sge_silent_get()) {
               printf(MSG_SETUP_PE_S, pe);
            }
            if (verify_str_key(&alp, pe, "pe")) {
               DEXIT;
               return -1;
            }       
            ep = cull_read_in_pe(PE_DIR, pe, 1, 0, NULL, NULL);
            if (!ep) {
               ret = -1;
               break;
            }

            if (pe_validate(1, ep, NULL)!=STATUS_OK) {
               ret = -1;
               break;
            }
            lAppendElem(Master_Pe_List, ep);
         } else {
            sge_unlink(PE_DIR, pe);
         }
      }
      direntries = lFreeList(direntries);
   }

   DEXIT;
   return ret;
}



int sge_read_cal_list_from_disk()
{
   lList *direntries;
   lListElem *aep, *ep, *direntry;
   int ret = 0;
   const char *cal;
   const char *s;
   lList *alp = NULL;

   DENTER(TOP_LAYER, "sge_read_cal_list_from_disk");
   
   if (!Master_Calendar_List)
      Master_Calendar_List = lCreateList("Master_Calendar_List", CAL_Type);

   direntries = sge_get_dirents(CAL_DIR);
   if(direntries) {
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINCALENDARS);
      for_each(direntry, direntries) {
         cal = lGetString(direntry, STR);

         if (cal[0] != '.') {
            if (!sge_silent_get()) {
               printf(MSG_SETUP_CALENDAR_S, cal);
            }
            if (verify_str_key(&alp, cal, "cal")) {
               DEXIT;
               return -1;
            }      
            ep = cull_read_in_cal(CAL_DIR, cal, 1, 0, NULL, NULL);
            if (!ep) {
               ret = -1;
               break;
            }

            if (parse_year(&alp, ep) || parse_week(&alp, ep)) {
               if (!(aep = lFirst(alp)) || !(s = lGetString(aep, AN_text)))
                  s = MSG_UNKNOWNREASON;
               ERROR((SGE_EVENT,MSG_CONFIG_FAILEDPARSINGYEARENTRYINCALENDAR_SS, 
                     cal, s));
               lFreeList(alp);
               ret = -1;
               break;
            }

            lAppendElem(Master_Calendar_List, ep);
         } else {
            sge_unlink(CAL_DIR, cal);  
         }
      }
      direntries = lFreeList(direntries);
   }

   DEXIT;
   return ret;
}

int sge_read_ckpt_list_from_disk()
{
   lList *direntries;
   lListElem *ep, *direntry;
   const char *ckpt;

   DENTER(TOP_LAYER, "sge_read_ckpt_list_from_disk");
   
   if (!Master_Ckpt_List)
      Master_Ckpt_List = lCreateList("Master_Ckpt_List", CK_Type);

   direntries = sge_get_dirents(CKPTOBJ_DIR);
   if(direntries) {
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINCKPTINTERFACEDEFINITIONS);
      for_each(direntry, direntries) {
         ckpt = lGetString(direntry, STR);

         if (ckpt[0] != '.') {
            if (!sge_silent_get()) 
               printf(MSG_SETUP_CKPT_S, ckpt);
            ep = cull_read_in_ckpt(CKPTOBJ_DIR, ckpt, 1, 0, NULL, NULL);
            if (!ep) {
               DEXIT;
               return -1;
            }

            if (validate_ckpt(ep, NULL)!=STATUS_OK) {
               DEXIT;
               return -1;
            }
            
            lAppendElem(Master_Ckpt_List, ep);
         } else {
            sge_unlink(CKPTOBJ_DIR, ckpt);
         }
      }
      direntries = lFreeList(direntries);
   }
   DEXIT;
   return 0;
}

int sge_read_queue_list_from_disk()
{
   lList *alp = NULL, *direntries;
   lListElem *qep, *direntry;
   int config_tag = 0;
   u_long32 state;

   DENTER(TOP_LAYER, "sge_read_queue_list_from_disk");

   direntries = sge_get_dirents(QUEUE_DIR);
   if (direntries) {
      const char *queue_str;
      
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINQUEUES);
      for_each(direntry, direntries) {

         queue_str = lGetString(direntry, STR);
         if (queue_str[0] != '.') {
            config_tag = 0;
            if (!sge_silent_get()) {
               printf(MSG_SETUP_QUEUE_S, lGetString(direntry, STR));
            }
            if (verify_str_key(&alp, queue_str, "queue")) {
               DEXIT;
               return -1;
            }   
            qep = cull_read_in_qconf(QUEUE_DIR, lGetString(direntry, STR), 1, 
                  0, &config_tag, NULL);
            if (!qep) {
               ERROR((SGE_EVENT, MSG_CONFIG_READINGFILE_SS, QUEUE_DIR, 
                        lGetString(direntry, STR)));
               DEXIT;
               return -1;
            }
            if (config_tag & CONFIG_TAG_OBSOLETE_VALUE) {
               /* an obsolete config value was found in the file.
                  spool it out again to have the newest version on disk. */
               cull_write_qconf(1, 0, QUEUE_DIR, lGetString(direntry, STR), 
                     NULL, qep);
               INFO((SGE_EVENT, MSG_CONFIG_QUEUEXUPDATED_S, 
                     lGetString(direntry, STR)));
            }
            
            if (!strcmp(lGetString(direntry, STR), SGE_TEMPLATE_NAME) && 
                !strcmp(lGetString(qep, QU_qname), SGE_TEMPLATE_NAME)) {
               /* 
                  we do not keep the queue template in the main queue list 
                  to be compatible with other old code in the qmaster
               */
               qep = lFreeElem(qep);
               sge_unlink(QUEUE_DIR, lGetString(direntry, STR));
               WARNING((SGE_EVENT, MSG_CONFIG_OBSOLETEQUEUETEMPLATEFILEDELETED));
            }
            else if (!strcmp(lGetString(qep, QU_qname), SGE_TEMPLATE_NAME)) {
               /*
                  oops!  found queue 'template', but not in file 'template'
               */
               ERROR((SGE_EVENT, MSG_CONFIG_FOUNDQUEUETEMPLATEBUTNOTINFILETEMPLATEIGNORINGIT));
               qep = lFreeElem(qep);
            }
            else {
               lListElem *exec_host;

               /* handle slots from now on as a consumble attribute of queue */
               slots2config_list(qep); 

               /* setup actual list of queue */
               debit_queue_consumable(NULL, qep, Master_Complex_List, 0);

               /* init double values of consumable configuration */
               sge_fill_requests(lGetList(qep, QU_consumable_config_list), Master_Complex_List, 1, 0, 1);

               if (complex_list_verify(lGetList(qep, QU_complex_list), NULL, 
                                       "queue", lGetString(qep, QU_qname))
                    !=STATUS_OK) {
                  qep = lFreeElem(qep);            
                  DEXIT;
                  return -1;
               }
               if (ensure_attrib_available(NULL, qep, QU_load_thresholds) ||
                   ensure_attrib_available(NULL, qep, QU_suspend_thresholds) ||
                   ensure_attrib_available(NULL, qep, QU_consumable_config_list)) {
                  qep = lFreeElem(qep); 
                  DEXIT;
                  return -1;
               }

               queue_list_add_queue(qep);
               state = lGetUlong(qep, QU_state);
               SETBIT(QUNKNOWN, state);
               state &= ~(QCAL_DISABLED|QCAL_SUSPENDED);
               lSetUlong(qep, QU_state, state);

               set_qslots_used(qep, 0);
               
               if (!(exec_host = host_list_locate(Master_Exechost_List, 
                     lGetHost(qep, QU_qhostname)))) {
                  if (lGetUlong(qep, QU_qtype) & TQ) { /* JG: TODO: we no longer have transfer queues */
                     ERROR((SGE_EVENT, MSG_CONFIG_CANTRECREATEQEUEUEXFROMDISKBECAUSEOFUNKNOWNHOSTY_SS,
                     lGetString(qep, QU_qname), lGetHost(qep, QU_qhostname)));
                     lRemoveElem(Master_Queue_List, qep);
                  }
                  else {
                     /* JG: TODO: if we get a queue and don't know the exec host:
                      * old behaviour: create it. Does this make sense?
                      * or better report an error?
                      * for now, report an error, as sge_add_host_of_type
                      * raises unsolvable dependency problems!
                      */
#if 0
                     if (sge_add_host_of_type(lGetHost(qep, QU_qhostname), 
				SGE_EXECHOST_LIST)) {
                        qep = lFreeElem(qep);
                        lFreeList(direntries);
                        DEXIT;
                        return -1;
                     }
#else
                     ERROR((SGE_EVENT, MSG_CONFIG_CANTRECREATEQEUEUEXFROMDISKBECAUSEOFUNKNOWNHOSTY_SS,
                     lGetString(qep, QU_qname), lGetHost(qep, QU_qhostname)));
                     lRemoveElem(Master_Queue_List, qep);
                     qep = lFreeElem(qep);
                     lFreeList(direntries);
                     DEXIT;
                     return -1;
#endif
                  }
               } 

               /*
               ** make a start for the history when Sge first starts up
               ** or when history has been deleted
               */
               if (!is_nohist() && lGetString(qep, QU_qname) &&
                   !is_object_in_history(STR_DIR_QUEUES, lGetString(qep, QU_qname))) {
                  int ret;
                  
                  ret = sge_write_queue_history(qep);
                  if (ret) {
                     WARNING((SGE_EVENT, MSG_CONFIG_CANTWRITEHISTORYFORQUEUEX_S,
                        lGetString(qep, QU_qname)));
                  }
               }
            }
         } else {
            sge_unlink(QUEUE_DIR, queue_str);
         }
      }
      lFreeList(direntries);
   }
   

   DEXIT;
   return 0;
}   


int sge_read_project_list_from_disk()
{
   lList *alp = NULL, *direntries;
   lListElem *ep, *direntry;
   int config_tag = 0;

   DENTER(TOP_LAYER, "sge_read_project_list_from_disk");

   Master_Project_List = lCreateList("project list", UP_Type);
   direntries = sge_get_dirents(PROJECT_DIR);
   if (direntries) {
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINPROJECTS);

      for_each(direntry, direntries) {
         const char *userprj_str;

         userprj_str = lGetString(direntry, STR);
         if (userprj_str[0] != '.') {
            config_tag = 0;
            if (!sge_silent_get()) 
               printf(MSG_SETUP_PROJECT_S, lGetString(direntry, STR));
            if (verify_str_key(&alp, userprj_str, "project")) {
               DEXIT;
               return -1;
            }  
            ep = cull_read_in_userprj(PROJECT_DIR, lGetString(direntry, STR), 1,
                                       0, &config_tag);
            if (!ep) {
               ERROR((SGE_EVENT, MSG_CONFIG_READINGFILE_SS, PROJECT_DIR, 
                        lGetString(direntry, STR)));
               DEXIT;
               return -1;
            }

            lAppendElem(Master_Project_List, ep);
         } else {
            sge_unlink(PROJECT_DIR, userprj_str);
         }
      }
      lFreeList(direntries);
   }

   DEXIT;
   return 0;
}   

int sge_read_user_list_from_disk()
{
   lList *direntries;
   lListElem *ep, *direntry;
   int config_tag = 0;

   DENTER(TOP_LAYER, "sge_read_user_list_from_disk");

   Master_User_List = lCreateList("user list", UP_Type);
   direntries = sge_get_dirents(USER_DIR);
   if (direntries) {
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINUSERS);
      
      for_each(direntry, direntries) {
         const char *direntry_str;
         
         direntry_str = lGetString(direntry, STR); 
         if (direntry_str[0] != '.') { 
            config_tag = 0;
            if (!sge_silent_get()) 
               printf(MSG_SETUP_USER_S, lGetString(direntry, STR));

            ep = cull_read_in_userprj(USER_DIR, lGetString(direntry, STR), 1,
                                       1, &config_tag);
            if (!ep) {
               ERROR((SGE_EVENT, MSG_CONFIG_READINGFILE_SS, USER_DIR, 
                        lGetString(direntry, STR)));
               DEXIT;
               return -1;
            }

            lAppendElem(Master_User_List, ep);
         } else {
            sge_unlink(USER_DIR, direntry_str);
         }
      }
      direntries = lFreeList(direntries);
   }

   DEXIT;
   return 0;
}

int sge_read_userset_list_from_disk()
{
   lList *alp = NULL, *direntries;
   lListElem *ep, *direntry;

   DENTER(TOP_LAYER, "sge_read_userset_list_from_disk");

   Master_Userset_List = lCreateList("user set list", US_Type);
   direntries = sge_get_dirents(USERSET_DIR);
   if (direntries) {
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINUSERSETS);

      for_each(direntry, direntries) {
         const char *userset = lGetString(direntry, STR);

         if (userset[0] != '.') {
            if (!sge_silent_get()) {
               printf(MSG_SETUP_USERSET_S , lGetString(direntry, STR));
            }
            if (verify_str_key(&alp, userset, "userset")) {
               DEXIT;
               return -1;
            }  

            ep = cull_read_in_userset(USERSET_DIR, userset, 1, 0, NULL); 
            if (!ep) {
               ERROR((SGE_EVENT, MSG_CONFIG_READINGFILE_SS, USERSET_DIR, 
                        userset));
               DEXIT;
               return -1;
            }

            if(userset_validate_entries(ep, NULL, 1) == STATUS_OK) {
               lAppendElem(Master_Userset_List, ep);
            } else {
               lFreeElem(ep);
            }
         } else {
            sge_unlink(USERSET_DIR, userset);
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
char *object_name,
char *object_dir 
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

int read_all_complexes(void)
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
         sge_unlink(COMPLEX_DIR, dent->d_name);
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

      admin_user = sge_get_confval("admin_user", global_config_file);
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

         if ((ret = sge_resolve_host(el, CONF_hname))!= CL_OK) {
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
            sge_switch2admin_user();
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
                  sge_switch2start_user();
                  DEXIT;
                  return -1;
               }
               sprintf(old_fname, "%s/%s", local_config_dir, old_name);
               if (sge_unlink(NULL, old_fname)) {
                  sge_switch2start_user();
                  DEXIT;
                  return -1;
               }
            }
            sge_switch2start_user();
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


