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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>

#include "sge.h"
#include "sge_log.h"
#include "sge_parse_num_par.h"
#include "sgermon.h"
#include "commlib.h"
#include "sge_conf.h"
#include "configuration_qmaster.h"
#include "cull.h"
#include "sge_host.h"
#include "config_file.h"
#include "rw_configuration.h"
#include "sge_userset_qmaster.h"
#include "sge_utility.h"
#include "sge_userprj_qmaster.h"
#include "sge_parse_num_par.h"
#include "setup_path.h"
#include "sge_event_master.h"
#include "sge_string.h"
#include "reschedule.h"
#include "sge_unistd.h"
#include "sge_hostname.h"
#include "sge_prog.h"
#include "sge_uidgid.h" 
#include "sge_spool.h"
#include "sge_answer.h"
#include "sge_userprj.h"
#include "sge_userset.h"
#include "sge_host.h"

#include "sge_spooling.h"

#include "msg_common.h"
#include "msg_qmaster.h"

static int check_config(lList **alpp, lListElem *conf);
   
/* make chached values from configuration invalid */
int new_config = 1;

/*-----------------------------------------------
 * sge_del_configuration
 *-----------------------------------------------*/
int sge_del_configuration(
lListElem *confp,
lList **alpp,
char *ruser,
char *rhost 
) {
   lListElem *ep;
   const char *config_name;

   DENTER(TOP_LAYER, "sge_del_configuration");

   if ( !confp || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   if (!(config_name = lGetHost(confp, CONF_hname))) {
      /* confp is no config element, if confp has no CONF_hname */
      CRITICAL((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
                lNm2Str(CONF_hname), SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* check if configuration is in Master_Config_List */
   if (!(ep = lGetElemHost(Master_Config_List, CONF_hname, config_name))) {
      ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, 
               MSG_OBJ_CONF, config_name));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* check if someone tries to delete global configuration */
   if (!strcasecmp(SGE_GLOBAL_NAME, config_name)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANT_DEL_CONFIG_S, config_name));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

#if 0    /* CR - This can't be done so easy, look at issue #79 for
                 further explanations  */
   {
      /* check if someone tries to delete local configuration
      with a local execd_spool_dir */

      lList *listEntries = NULL;
      const char* help = NULL;
      listEntries = lGetList(ep,CONF_entries);      
      if (lGetElemStr(listEntries, CF_name, "execd_spool_dir") != NULL) {
         ERROR((SGE_EVENT, MSG_CONF_DELLOCCONFFORXWITHEXECDSPOOLDENIED_S, config_name));
         answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EEXIST;
      }
   }
#endif   

    
   spool_delete_object(spool_get_default_context(), SGE_TYPE_CONFIG, 
                       config_name);
    
   /* now remove it from our internal list*/
   lRemoveElem(Master_Config_List, ep);

   INFO((SGE_EVENT, MSG_SGETEXT_REMOVEDFROMLIST_SSSS,
         ruser, rhost, config_name, MSG_OBJ_CONF ));

   update_reschedule_unknown_timout_values(config_name);

   /* make chached values from configuration invalid */
   new_config = 1;

   answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
   sge_add_event(NULL, 0, sgeE_CONFIG_DEL, 0, 0, config_name, NULL);
   
   DEXIT;
   return STATUS_OK;
}


/*-----------------------------------------------
 * sge_mod_configuration
 *-----------------------------------------------*/
int sge_mod_configuration(
lListElem *confp,
lList **alpp,
char *ruser,
char *rhost 
) {
   lListElem *ep;
   const char *config_name;
   const char *cp;
   bool added;
   u_long32 old_conf_version = 0;
   int reschedule_unknown_changed = 1;
   int ret;

   DENTER(TOP_LAYER, "sge_mod_configuration");

   if ( !confp || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   if (!(config_name = lGetHost(confp, CONF_hname))) {
      /* confp is no config element, if confp has no CONF_hname */
      CRITICAL((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
                lNm2Str(CONF_hname), SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   if ((ret=check_config(alpp, confp))) {
      /* answer list gets filled in check_config() */
      DEXIT;
      return ret; 
   }


   if (!(ep = lGetElemHost(Master_Config_List, CONF_hname, config_name))) {
      DTRACE;
      added = true; 
   }
   else {
      added = false;
   }

   if (added == false) {
      /* check for unchangeable parameters */
      lList *oldEntryList  = NULL;
      lList *newEntryList  = NULL;
      lListElem *oldEntryListElem  = NULL;
      lListElem *newEntryListElem  = NULL;
      const char *oldEntry = NULL;
      const char *newEntry = NULL;
      const char *paraName = NULL;
      int paramPos = 0;
      char  *deniedParams[] = { 
          "execd_spool_dir", 
          "qmaster_spool_dir", 
          "admin_user",
          "default_domain",
          "ignore_fqdn",
          NULL 
      };

      oldEntryList = lGetList(ep   , CONF_entries); 
      newEntryList = lGetList(confp, CONF_entries);

     
       
      while ( deniedParams[paramPos] != NULL ) {
         /* execd_spool_dir */
         paraName = deniedParams[paramPos];
         oldEntry = NULL;
         newEntry = NULL;
   
         oldEntryListElem = lGetElemStr(oldEntryList , CF_name, paraName );
         newEntryListElem = lGetElemStr(newEntryList , CF_name, paraName );
         if ((newEntryListElem != NULL) && (oldEntryListElem != NULL)) {
            oldEntry = lGetString(oldEntryListElem, CF_value);
            newEntry = lGetString(newEntryListElem, CF_value);
         }
         if ((newEntry != NULL) && (oldEntry != NULL)) { 
            DPRINTF(("paraName = %s\n",paraName));
            DPRINTF(("oldEntry = %s\n",oldEntry));
            DPRINTF(("newEntry = %s\n",newEntry));
   
            if (strcmp(oldEntry,newEntry) != 0) {
                ERROR((SGE_EVENT, MSG_CONF_CHANGEPARAMETERXONLYSUPONSHUTDOWN_S, paraName ));
                answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
                DEXIT;
                return STATUS_EUNKNOWN;
            }
         }
         paramPos++;
      }  /* while */
   }

   if (added == false) {
      const char *const name = "reschedule_unknown";
      lListElem *old_reschedule_unknown = NULL;
      lListElem *new_reschedule_unknown = NULL;

      old_reschedule_unknown = lGetElemStr(lGetList(ep, CONF_entries), CF_name,
                                           name);
      new_reschedule_unknown = lGetElemStr(lGetList(confp, CONF_entries),
                                           CF_name, name);
      if (old_reschedule_unknown && new_reschedule_unknown && 
          strcmp(lGetString(old_reschedule_unknown, CF_value),
                 lGetString(new_reschedule_unknown, CF_value)) == 0) {
         reschedule_unknown_changed = 0;
      } 
      DPRINTF(("reschedule_unknown_changed = %d\n", 
               reschedule_unknown_changed));
      old_conf_version = lGetUlong(ep, CONF_version);
      lRemoveElem(Master_Config_List, ep);
   }       

   ep = lCopyElem(confp);
   lAppendElem(Master_Config_List, ep);
   if (added == false) {
      lSetUlong(ep, CONF_version, old_conf_version + 1);
      lSetUlong(confp, CONF_version, old_conf_version + 1);
   }
   spool_write_object(spool_get_default_context(), confp, 
                      lGetHost(confp, CONF_hname), SGE_TYPE_CONFIG);
   /*
   ** is the configuration change relevant for the qmaster itsself?
   ** if so, initialise conf struct anew
   */
   if (!strcmp(SGE_GLOBAL_NAME, config_name) || 
       !sge_hostcmp(config_name, uti_state_get_qualified_hostname())) {
      lListElem *lep = NULL;

      ret = select_configuration(uti_state_get_qualified_hostname(), Master_Config_List, &lep);
      if (ret) {
         ERROR((SGE_EVENT, MSG_CONF_CANTSELECTCONFIGURATIONFORHOST_SI,
               uti_state_get_qualified_hostname(), ret));
      }
      ret = merge_configuration(
         lGetElemHost(Master_Config_List, CONF_hname, SGE_GLOBAL_NAME), 
               lep, &conf, NULL);
      if (ret) {
         ERROR((SGE_EVENT, MSG_CONF_CANTMERGECONFIGURATIONFORHOST_SI,
                uti_state_get_qualified_hostname(), ret));
      }
      sge_show_conf();

      /* pass new max_unheard value to commlib */
      set_commlib_param(CL_P_LT_HEARD_FROM_TIMEOUT, conf.max_unheard, 
         NULL, NULL);
   }
 
   if (reschedule_unknown_changed) { 
      update_reschedule_unknown_timout_values(config_name);
   }
 
   if (added)
      cp = MSG_SGETEXT_ADDEDTOLIST_SSSS;
   else
      cp = MSG_SGETEXT_MODIFIEDINLIST_SSSS;

   /* make chached values from configuration invalid */
   new_config = 1;

   INFO((SGE_EVENT, cp, ruser, rhost, config_name, MSG_OBJ_CONF ));
   answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
   sge_add_event(NULL, 0, added ? sgeE_CONFIG_ADD : sgeE_CONFIG_MOD, 0, 0, config_name, ep);
   
   DEXIT;
   return STATUS_OK;
}
   
   
static int check_config(
lList **alpp,
lListElem *conf 
) {
   lListElem *ep;
   const char *name, *value;
   const char *conf_name;
 
   DENTER(TOP_LAYER, "check_config");
 
   conf_name = lGetHost(conf, CONF_hname);
 
   for_each(ep, lGetList(conf, CONF_entries)) {
      name = lGetString(ep, CF_name);
      value = lGetString(ep, CF_value);
 
      if (!name) {
         ERROR((SGE_EVENT, MSG_CONF_NAMEISNULLINCONFIGURATIONLISTOFX_S,
               conf_name));
         answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EEXIST;
      }
      if (!value) {
         ERROR((SGE_EVENT, MSG_CONF_VALUEISNULLFORATTRXINCONFIGURATIONLISTOFY_SS,
                name, conf_name));
         answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EEXIST;
      }
      if (!strcmp(name, "loglevel")) {
         u_long32 tmp_uval;
         if (sge_parse_loglevel_val(&tmp_uval,value) != 1) {
            ERROR((SGE_EVENT, MSG_CONF_GOTINVALIDVALUEXFORLOGLEVEL_S, value));
            answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
            DEXIT;
            return STATUS_EEXIST;
         }
      }

      if (!strcmp(name, "shell_start_mode")) {
 
         if ( (strcasecmp("unix_behavior",value) != 0) && 
              (strcasecmp("posix_compliant",value) != 0) &&
              (strcasecmp("script_from_stdin",value) != 0) ) {
            ERROR((SGE_EVENT, MSG_CONF_GOTINVALIDVALUEXFORSHELLSTARTMODE_S, value));
            answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
            DEXIT;
            return STATUS_EEXIST;
         }
      }

      if (!strcmp(name, "load_report_time")) {
         /* do not allow infinity entry for load_report_time */
         if (strcasecmp(value,"infinity") == 0) {
            ERROR((SGE_EVENT, MSG_CONF_INFNOTALLOWEDFORATTRXINCONFLISTOFY_SS, name, conf_name));
            answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
            DEXIT;
            return STATUS_EEXIST;
         }
      }

      if (!strcmp(name, "stat_log_time")) {
         /* do not allow infinity entry */
         if (strcasecmp(value,"infinity") == 0) {
            ERROR((SGE_EVENT, MSG_CONF_INFNOTALLOWEDFORATTRXINCONFLISTOFY_SS, name, conf_name));
            answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
            DEXIT;
            return STATUS_EEXIST;
         }
      }

      if (!strcmp(name, "max_unheard")) {
         /* do not allow infinity entry */
         if (strcasecmp(value,"infinity") == 0) {
            ERROR((SGE_EVENT, MSG_CONF_INFNOTALLOWEDFORATTRXINCONFLISTOFY_SS, name, conf_name));
            answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
            DEXIT;
            return STATUS_EEXIST;
         }
      }


      if (!strcmp(name, "admin_user")) {
         if (strcasecmp(value, "none") && !getpwnam(value)) {
            ERROR((SGE_EVENT, MSG_CONF_GOTINVALIDVALUEXASADMINUSER_S, value));
            answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
            DEXIT;
            return STATUS_EEXIST;
         }
      }

      if (!strcmp(name, "user_lists")||!strcmp(name, "xuser_lists")) {
         lList *tmp = NULL;
         int ok;

         /* parse just for .. */ 
         if (lString2ListNone(value, &tmp, US_Type, US_name, " \t,")) {
            ERROR((SGE_EVENT, MSG_CONF_FORMATERRORFORXINYCONFIG_SS, name, conf_name));
            answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
            DEXIT;
            return STATUS_EEXIST;
         }

         /* .. checking userset names */
         ok = (userset_list_validate_acl_list(alpp, tmp, name, conf_name, "configuration")==STATUS_OK);
         lFreeList(tmp);
         if (!ok) {
            DEXIT;
            return STATUS_EEXIST;
         }
      }

      if (!strcmp(name, "projects")||!strcmp(name, "xprojects")) {
         lList *tmp = NULL;
         int ok=1;

         /* parse just for .. */ 
         if (lString2ListNone(value, &tmp, UP_Type, UP_name, " \t,")) {
            ERROR((SGE_EVENT, MSG_CONF_FORMATERRORFORXINYCONFIG_SS, name, conf_name));
            answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
            DEXIT;
            return STATUS_EEXIST;
         }

         /* .. checking project names */
         ok = (verify_userprj_list(alpp, tmp, Master_Project_List,
                    name, "configuration", conf_name)==STATUS_OK);
         lFreeList(tmp);
         if (!ok) {
            DEXIT;
            return STATUS_EEXIST;
         }
      }

      if (!strcmp(name, "prolog")||!strcmp(name, "epilog")) {
       
         if (value && strcasecmp(value, "none")) {
            const char *t, *script = value;

            /* skip user name */
            if ((t = strpbrk(script, "@ ")) && *t == '@')
               script = &t[1];

            /* force use of absolute paths if string <> none */
            if (script[0] != '/' ) {
               ERROR((SGE_EVENT, MSG_CONF_THEPATHGIVENFORXMUSTSTARTWITHANY_S, name));
               answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
               DEXIT;
               return STATUS_EEXIST;
            }
   
            /* ensure that variables are valid */
            if (replace_params(script, NULL, 0, prolog_epilog_variables)) {
               ERROR((SGE_EVENT, MSG_CONF_PARAMETERXINCONFIGURATION_SS, name, err_msg));
               answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
               DEXIT;
               return STATUS_EEXIST;
            }
         }
      }
   }
 
   DEXIT;
   return 0;
}
  

/*-------------------------------------------------------------------------*
 * NAME
 *   select_configuration - select a configuration from a list
 * PARAMETER
 *   config_name       - name of local configuration or "global",
 *                       name is being resolved before search
 *   lp                - configuration list, CONF_Type
 *   cepp              - pointer to list element containing requested
 *                       configuration, CONF_Type, should point to NULL
 *                       or otherwise will be freed
 * RETURN
 *   -1   NULL pointer received
 *   -2   resolving failed
 *   -3   config name for host not found
 * EXTERNAL
 *
 * DESCRIPTION
 *   resolves a config (host) name and searches this name in a
 *   configuration list, by case insensitive search.
 *-------------------------------------------------------------------------*/
int select_configuration(
const char *config_name,
lList *lp,
lListElem **cepp 
) {
   int ret;
   lListElem *hep = NULL;
   int is_global_requested = 0;

   DENTER(TOP_LAYER, "select_configuration");

   if (!config_name || !lp || !cepp) {
      DEXIT;
      return -1;
   }

   if (*cepp) {
      lFreeElem(*cepp);
      *cepp = NULL;
   }

   if (!strcasecmp(config_name, "global")) {
      is_global_requested = 1;
   }
   else {
      hep = lCreateElem(EH_Type);
      lSetHost(hep, EH_name, config_name);

      ret = sge_resolve_host(hep, EH_name);
      if (ret) {
         DPRINTF(("get_configuration: error %s resolving host %s\n", cl_errstr(ret), config_name));
         ERROR((SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_S, config_name));
         lFreeElem(hep);
         DEXIT;
         return -2;
      }
/*       DPRINTF(("get_configuration: unique for %s: %s\n", config_name, lGetHost(hep, EH_name))); */
   }
   
   *cepp = lGetElemHost(lp, CONF_hname, 
      (is_global_requested ? SGE_GLOBAL_NAME : lGetHost(hep, EH_name)));
   
   lFreeElem(hep);
   DEXIT;
   return *cepp ? 0 : -3;
}


/*-------------------------------------------------------------------------*
 * NAME
 *   is_configuration_up_to_date - as it says
 * PARAMETER
 *   hep               - host element, EH_Type, that owns the configurations in
 *                       to_check_list
 *   conf_list         - global configuration list that the qmaster has,
 *                       CONF_Type                    
 *   to_check_list     - configuration list to check for being up to date,
 *                       CONF_Type, the CF_entries sublists are not referred to
 * RETURN
 *   0                 - the configuration list is NOT up to date
 *   1                 - the configuration list is up to date
 * EXTERNAL
 *
 * DESCRIPTION
 *   checks if a configuration list is up to date
 *-------------------------------------------------------------------------*/
int is_configuration_up_to_date(
lListElem *hep,
lList *conf_list,
lList *to_check_list 
) {
   lListElem *ep, *ep_conf;

   DENTER(TOP_LAYER, "is_configuration_up_to_date");

   if (lGetNumberOfElem(to_check_list) == 0) {
      DPRINTF(("received empty configuration list from %s\n", \
         lGetHost(hep, EH_name)));
      DEXIT;
      return 0;
   }

   for_each(ep, to_check_list) {
      /*
       * find the corresponding element in the configuration list
       */
      ep_conf = lGetElemHost(conf_list, CONF_hname, lGetHost(ep, CONF_hname));
      /*
       * if there is no such element this means that the configuration has
       * been deleted meanwhile
       */
      if (!ep_conf) {
         DPRINTF(("configuration %s no longer exists\n", \
            lGetHost(ep, CONF_hname)));
         DEXIT;
         return 0;
      }
      if (lGetUlong(ep, CONF_version) != lGetUlong(ep_conf, CONF_version)) {
         DPRINTF(("configuration %s changed from version %ld to %ld\n", \
            lGetHost(ep, CONF_hname), lGetUlong(ep, CONF_version), \
            lGetUlong(ep_conf, CONF_version)));
         DEXIT;
         return 0;
      }
   }

   /*
    * we also have to check if a new local configuration 
    * has been added meanwhile
    */
   if (lGetNumberOfElem(to_check_list) == 1) {
      ep_conf = lGetElemHost(conf_list, CONF_hname, lGetHost(hep, EH_name));
      if (ep_conf) {
         DPRINTF(("new local configuration %s has been created\n", \
            lGetHost(hep, EH_name)));
         DEXIT;
         return 0;
      }
   }

   DEXIT;
   return 1;
}
