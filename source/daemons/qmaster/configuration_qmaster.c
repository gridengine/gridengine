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
#include "sgermon.h"
#include "commlib.h"
#include "sge_conf.h"
#include "configuration_qmaster.h"
#include "cull.h"
#include "config_file.h"
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
#include "sge_host.h"
#include "sge_prog.h"
#include "sge_uidgid.h" 
#include "sge_spool.h"
#include "sge_answer.h"
#include "sge_userprj.h"
#include "sge_userset.h"
#include "sge_mtutil.h"

#include "sge_persistence_qmaster.h"
#include "spool/sge_spooling.h"

#include "msg_common.h"
#include "msg_qmaster.h"

/*
 * make chached values from configuration invalid.
 * 
 * TODO: Replace this stupid global variable by a function, e.g.
 * 'bool sge_configuration_did_change()'.
 */
int new_config = 1;


typedef struct { 
   pthread_mutex_t  mutex;  /* used for thread exclusion                           */
   lList*           list;   /* 'CONF_Type' list, holding the cluster configuration */
} cluster_config_t;


static cluster_config_t Cluster_Config = {PTHREAD_MUTEX_INITIALIZER, NULL};

/* Static configuration entries may not be changed at runtime */
static char *Static_Conf_Entries[] = { "execd_spool_dir", NULL };


static int check_config(lList **alpp, lListElem *conf);
static int do_mod_config(char *aConfName, lListElem *anOldConf, lListElem *aNewConf, lList**anAnswer);
static lListElem* is_reprioritize_missing(lList *theOldConfEntries, lList *theNewConfEntries);
static int check_static_conf_entries(lList *theOldConfEntries, lList *theNewConfEntries, lList **anAnswer);
static int exchange_conf_by_name(char *aConfName, lListElem *anOldConf, lListElem *aNewConf, lList**anAnswer);
static bool has_reschedule_unknown_change(lList *theOldConfEntries, lList *theNewConfEntries);
static int do_add_config(char *aConfName, lListElem *aConf, lList**anAnswer);
static int remove_conf_by_name(char *aConfName);
static lListElem *get_entry_from_conf(lListElem *aConf, const char *anEntryName);


/* 
 * Read the cluster configuration from secondary storage using 'aSpoolContext'.
 * This is the bootstrap function for the configuration module. It does populate
 * the list with the cluster configuration.
 */
int sge_read_configuration(lListElem *aSpoolContext, lList *anAnswer)
{
   lListElem *local = NULL;
   lListElem *global = NULL;
   int ret = -1;

   DENTER(TOP_LAYER, "sge_read_configuration");
   
   sge_mutex_lock("cluster_config_mutex", SGE_FUNC, __LINE__, &Cluster_Config.mutex);

   spool_read_list(&anAnswer, aSpoolContext, &Cluster_Config.list, SGE_TYPE_CONFIG);

   sge_mutex_unlock("cluster_config_mutex", SGE_FUNC, __LINE__, &Cluster_Config.mutex);
   
   answer_list_output(&anAnswer);

   if ((local = sge_get_configuration_for_host(uti_state_get_qualified_hostname())) == NULL)
   {
      ERROR((SGE_EVENT, MSG_CONFIG_ERRORXSELECTINGCONFIGY_S, uti_state_get_qualified_hostname()));
      DEXIT;
      return -1;      
   }
         
   if ((global = sge_get_configuration_for_host(SGE_GLOBAL_NAME)) == NULL)
   {
      ERROR((SGE_EVENT, MSG_CONFIG_ERRORXSELECTINGCONFIGY_S, SGE_GLOBAL_NAME));
      DEXIT;
      return -1;
   }

   ret = merge_configuration(global, local, &conf, NULL);

   local = lFreeElem(local);
   global = lFreeElem(global);
   
   if (0 != ret)
   {
      ERROR((SGE_EVENT, MSG_CONFIG_ERRORXMERGINGCONFIGURATIONY_IS, ret, uti_state_get_qualified_hostname()));
      DEXIT;
      return -1;
   }
   
   sge_show_conf();         

   DEXIT;
   return 0;
}

/*
 * Delete configuration 'confp' from cluster configuration.
 *
 * TODO: A fix for IZ issue #79 is needed. For this to be done it may be 
 * necessary to introduce something like 'protected' configuration entries.
 */
int sge_del_configuration(lListElem *aConf, lList **anAnswer, char *aUser, char *aHost)
{
   const char *tmp_name = NULL;
   char unique_name[MAXHOSTLEN];
   int ret = -1;

   DENTER(TOP_LAYER, "sge_del_configuration");

   if (!aConf || !aUser || !aHost)
   {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(anAnswer, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }
   
   if ((tmp_name = lGetHost(aConf, CONF_hname)) == NULL)
   {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS, lNm2Str(CONF_hname), SGE_FUNC));
      answer_list_add(anAnswer, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   if ((ret = sge_resolve_hostname(tmp_name, unique_name, EH_name)) != CL_RETVAL_OK)
   {
      DPRINTF(("%s: error %s resolving host %s\n", SGE_FUNC, cl_get_error_text(ret), tmp_name));
      ERROR((SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_S, tmp_name));
      answer_list_add(anAnswer, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* Do not allow to delete global configuration */
   if (!strcasecmp(SGE_GLOBAL_NAME, unique_name))
   {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANT_DEL_CONFIG_S, unique_name));
      answer_list_add(anAnswer, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   sge_event_spool(anAnswer, 0, sgeE_CONFIG_DEL, 0, 0, unique_name, NULL, NULL, NULL, NULL, NULL, true, true);
    
   remove_conf_by_name(unique_name);
   
   INFO((SGE_EVENT, MSG_SGETEXT_REMOVEDFROMLIST_SSSS, aUser, aHost, unique_name, MSG_OBJ_CONF ));
   answer_list_add(anAnswer, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
   
   update_reschedule_unknown_timout_values(unique_name);

   new_config = 1; /* invalidate cached configuration values */
   
   DEXIT;
   return STATUS_OK;
}


/*
 * Modify cluster configuration. 'confp' is a pointer to a 'CONF_Type' list element
 * and does contain the modified configuration entry. Adding a new configuration entry
 * is also viewed as a modification.
 */
int sge_mod_configuration(lListElem *aConf, lList **anAnswer, char *aUser, char *aHost)
{
   lListElem *old_conf;
   const char *tmp_name = NULL;
   char unique_name[MAXHOSTLEN];
   int ret = -1;

   DENTER(TOP_LAYER, "sge_mod_configuration");

   if (!aConf || !aUser || !aHost)
   {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(anAnswer, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   if ((tmp_name = lGetHost(aConf, CONF_hname)) == NULL)
   {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS, lNm2Str(CONF_hname), SGE_FUNC));
      answer_list_add(anAnswer, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   if ((ret = sge_resolve_hostname(tmp_name, unique_name, EH_name)) != CL_RETVAL_OK)
   {
      DPRINTF(("%s: error %s resolving host %s\n", SGE_FUNC, cl_get_error_text(ret), tmp_name));
      ERROR((SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_S, tmp_name));
      answer_list_add(anAnswer, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }
   
   if ((ret = check_config(anAnswer, aConf)))
   {
      DEXIT;
      return ret; 
   }

   if ((old_conf = sge_get_configuration_for_host(unique_name)) != NULL)
   {
      int ret = -1;
      
      ret = do_mod_config(unique_name, old_conf, aConf, anAnswer);
      
      lFreeElem(old_conf);
      
      if (ret == 0)
      {    
         INFO((SGE_EVENT, MSG_SGETEXT_MODIFIEDINLIST_SSSS, aUser, aHost, unique_name, MSG_OBJ_CONF));
         answer_list_add(anAnswer, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
      } 
      else
      {
         DEXIT;
         return STATUS_EUNKNOWN;
      }
   }
   else
   {
      do_add_config(unique_name, aConf, anAnswer);
            
      INFO((SGE_EVENT, MSG_SGETEXT_ADDEDTOLIST_SSSS, aUser, aHost, unique_name, MSG_OBJ_CONF));            
      answer_list_add(anAnswer, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
   }
   
   if (!strcmp(SGE_GLOBAL_NAME, unique_name))
   {
      sge_add_event(0, sgeE_GLOBAL_CONFIG, 0, 0, NULL, NULL, NULL, NULL);
   }

   /*
   ** is the configuration change relevant for the qmaster itsself?
   ** if so, initialise conf struct anew
   */
   if (!strcmp(unique_name, SGE_GLOBAL_NAME) || !sge_hostcmp(unique_name, uti_state_get_qualified_hostname()))
   {
      lListElem *local = NULL;
      lListElem *global = NULL;

      if ((local = sge_get_configuration_for_host(uti_state_get_qualified_hostname())) == NULL)
      {
         ERROR((SGE_EVENT, MSG_CONFIG_ERRORXSELECTINGCONFIGY_S, uti_state_get_qualified_hostname()));
      }
      
      if ((global = sge_get_configuration_for_host(SGE_GLOBAL_NAME)) == NULL)
      {
         ERROR((SGE_EVENT, MSG_CONFIG_ERRORXSELECTINGCONFIGY_S, SGE_GLOBAL_NAME));
      }
            
      if (merge_configuration(global, local, &conf, NULL) != 0) 
      {
         ERROR((SGE_EVENT, MSG_CONF_CANTMERGECONFIGURATIONFORHOST_S, uti_state_get_qualified_hostname()));
      }
      
      lFreeElem(local);
      lFreeElem(global);
      
      sge_show_conf();

      /* 'max_unheard' may have changed */
      cl_commlib_set_connection_param(cl_com_get_handle("qmaster", 1), HEARD_FROM_TIMEOUT, conf.max_unheard);
   }
    
   new_config = 1; /* invalidate configuration cache */
   
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
         ok = (userset_list_validate_acl_list(tmp, alpp)==STATUS_OK);
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
  

/*
 * Compare configuration 'aConf' for host 'aHost' with the cluster configuration.
 * Return '0' if 'aConf' is equal to the cluster configuration, '1' otherwise.
 *
 * 'aHost' is of type 'EH_Type', 'aConf' is of type 'CONF_Type'.
 */
int sge_compare_configuration(lListElem *aHost, lList *aConf)
{
   lListElem *conf_entry = NULL;

   DENTER(TOP_LAYER, "sge_compare_configuration");

   if (lGetNumberOfElem(aConf) == 0)
   {
      DPRINTF(("%s: configuration for %s is empty\n", SGE_FUNC, lGetHost(aHost, EH_name)));
      DEXIT;
      return 1;
   }
   
   for_each(conf_entry, aConf)
   {
      lListElem *master_conf = NULL;
      const char *host_name = NULL;
      u_long32 conf_version, master_version;
      
      host_name = lGetHost(conf_entry, CONF_hname);
      
      master_conf = sge_get_configuration_for_host(host_name);
            
      if (NULL == master_conf)
      {
         DPRINTF(("%s: no master configuration for %s found\n", SGE_FUNC, host_name));
         DEXIT;
         return 1;
      }
      
      conf_version = lGetUlong(conf_entry, CONF_version);
      master_version = lGetUlong(master_conf, CONF_version);
      
      if (master_version != conf_version)
      {
         DPRINTF(("%s: configuration for %s changed from version %ld to %ld\n", SGE_FUNC, host_name, master_version, conf_version));
         lFreeElem(master_conf);
         DEXIT;
         return 1;
      }
      
      lFreeElem(master_conf);
   }

   DEXIT;
   return 0;
}


/*
 * Return a *COPY* of configuration entry 'anEntryName'. First we do query the
 * local configuration 'aHost'. If that is fruitless, we try the global
 * configuration. 
 */
lListElem *sge_get_configuration_entry_by_name(const char *aHost, const char *anEntryName)
{
   lListElem *conf = NULL;
   
   DENTER(TOP_LAYER, "sge_get_configuration_value_by_name");
   
   SGE_ASSERT((NULL != aHost) && (NULL != anEntryName));
   
   /* try local configuration first */
   if ((conf = sge_get_configuration_for_host(aHost)) != NULL)
   {
      lListElem *elem = NULL;
      
      if ((elem = get_entry_from_conf(conf, anEntryName)) != NULL)
      {
         lFreeElem(conf);         
         DEXIT;
         return elem;
      }
   }
   
   /* local configuration did not work, try global one */
   if ((conf = sge_get_configuration_for_host(SGE_GLOBAL_NAME)) != NULL)
   {
      lListElem *elem = NULL;
      
      if ((elem = get_entry_from_conf(conf, anEntryName)) != NULL)
      {
         lFreeElem(conf);         
         DEXIT;
         return elem;
      }
   }
   
   DEXIT;
   return NULL;
}

lListElem *get_entry_from_conf(lListElem *aConf, const char *anEntryName)
{
   lList *entries = NULL;
   lListElem *elem = NULL;
   
   entries = lGetList(aConf, CONF_entries);
   
   elem = lGetElemStr(entries, CF_name, anEntryName);
   
   return lCopyElem(elem);
}

/*
 * Return a *COPY* of the master configuration.
 */
lList* sge_get_configuration(void)
{
   lList *conf = NULL;
   
   DENTER(TOP_LAYER, "sge_get_configuration");

   sge_mutex_lock("cluster_config_mutex", SGE_FUNC, __LINE__, &Cluster_Config.mutex);
   
   conf = lCopyListHash(lGetListName(Cluster_Config.list), Cluster_Config.list, false);

   sge_mutex_unlock("cluster_config_mutex", SGE_FUNC, __LINE__, &Cluster_Config.mutex);
   
   DEXIT;
   return conf;
}

/*
 * Return a *COPY* of the configuration for host 'aName'. The host name 'aName'
 * will be resolved to eliminate any host name differences caused by the
 * various host name formats or the host name alias mechanism.
 */
lListElem* sge_get_configuration_for_host(const char* aName)
{
   lListElem *conf = NULL;
   char unique_name[MAXHOSTLEN];
   int ret = -1;

   DENTER(TOP_LAYER, "sge_get_configuration_for_host");

   SGE_ASSERT((NULL != aName));

   ret = sge_resolve_hostname(aName, unique_name, EH_name);
   
   if (CL_RETVAL_OK != ret)
   {
      DPRINTF(("%s: error %s resolving host %s\n", SGE_FUNC, cl_get_error_text(ret), aName));
      ERROR((SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_S, aName));
      DEXIT;
      return NULL;
   }

   sge_mutex_lock("cluster_config_mutex", SGE_FUNC, __LINE__, &Cluster_Config.mutex);
   
   conf = lCopyElem(lGetElemHost(Cluster_Config.list, CONF_hname, unique_name));

   sge_mutex_unlock("cluster_config_mutex", SGE_FUNC, __LINE__, &Cluster_Config.mutex);

   DEXIT;
   return conf;
}


void sge_set_conf_reprioritize(lListElem *aConf, bool aFlag)
{
   lList *entries = NULL;
   lListElem *ep = NULL;

   DENTER(TOP_LAYER, "sge_set_conf_reprioritize");

   SGE_ASSERT((NULL != aConf));

   entries = lGetList(aConf, CONF_entries);

   ep = lGetElemStr(entries, CF_name, REPRIORITIZE);

   if (NULL == ep)
   {
      ep = lCreateElem(CF_Type);
      lSetString(ep, CF_name, REPRIORITIZE);
      lAppendElem(entries, ep);
   }

   lSetString(ep, CF_value, ((true == aFlag) ? "1" : "0"));
   lSetUlong(ep, CF_local, 0);

   DEXIT;
   return;
} /* sge_set_conf_reprioritize */


bool sge_get_conf_reprioritize(lListElem *aConf)
{
   lList *entries = NULL;
   lListElem *ep = NULL;
   bool res = false;

   DENTER(TOP_LAYER, "sge_get_conf_reprioritize");

   SGE_ASSERT((NULL != aConf));

   entries = lGetList(aConf, CONF_entries);

   ep = lGetElemStr(entries, CF_name, REPRIORITIZE);

   if (NULL == ep)
   {
      res = false; /* no conf value */
   }
   else
   {
      const char *val;

      val = lGetString(ep, CF_value);
      res = ((strncasecmp(val, "0", sizeof("0")) == 0) ? false :true);
   }

   DEXIT;
   return res;
} /* sge_get_conf_reprioritize */


bool sge_conf_is_reprioritize(void)
{
   bool res = false;
   lListElem *conf = NULL;

   DENTER(TOP_LAYER, "sge_conf_is_reprioritize");

   conf = sge_get_configuration_for_host(SGE_GLOBAL_NAME);

   res = sge_get_conf_reprioritize(conf);

   conf = lFreeElem(conf);

   DEXIT;
   return res;
}
 
/* 
 * Modify configuration with name 'aConfName'. 'anOldConf' is a *COPY* of this
 * configuration. 'aNewConf' is the new configuration.
 *
 * NOTE: Either 'anOldConf' or 'aNewConf' could be an empty configuration!
 * Empty configurations do not contain any 'CONF_entries'.
 */
static int do_mod_config(char *aConfName, lListElem *anOldConf, lListElem *aNewConf, lList**anAnswer)
{
   lList *old_entries = NULL;
   lList *new_entries = NULL;
   lListElem* reprio = NULL;
   
   DENTER(TOP_LAYER, "do_mod_config");
   
   old_entries = lGetList(anOldConf, CONF_entries); 
   new_entries = lGetList(aNewConf, CONF_entries);
   
   if ((reprio = is_reprioritize_missing(old_entries, new_entries)) != NULL)
   {
      lAppendElem(new_entries, reprio);
   }

   if (check_static_conf_entries(old_entries, new_entries, anAnswer) != 0)
   {
      DEXIT;
      return -1;
   }
      
   exchange_conf_by_name(aConfName, anOldConf, aNewConf, anAnswer);

   if (has_reschedule_unknown_change(old_entries, new_entries) == true)
   {
      update_reschedule_unknown_timout_values(aConfName);
   }
         
   DEXIT;
   return 0;
}

/*
 * Static configuration entries may not be changed.
 */
static int check_static_conf_entries(lList *theOldConfEntries, lList *theNewConfEntries, lList **anAnswer)
{
   int entry_idx = 0;
   
   DENTER(TOP_LAYER, "check_static_conf_entries");

   while (NULL != Static_Conf_Entries[entry_idx])
   {
      lListElem *old_entry, *new_entry = NULL;
      const char *old_value, *new_value = NULL;
      const char *entry_name = Static_Conf_Entries[entry_idx];
      
      old_entry = lGetElemStr(theOldConfEntries, CF_name, entry_name);
      new_entry = lGetElemStr(theNewConfEntries, CF_name, entry_name);
      
      if ((NULL != old_entry) && (NULL != new_entry))
      {
         old_value = lGetString(old_entry, CF_value);
         new_value = lGetString(new_entry, CF_value);
         
         if (((NULL != old_value) && (NULL != new_value)) && (strcmp(old_value, new_value) != 0))
         {
            ERROR((SGE_EVENT, MSG_CONF_CHANGEPARAMETERXONLYSUPONSHUTDOWN_S, entry_name));
            answer_list_add(anAnswer, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            DEXIT;
            return -1;
         }
      }      
      entry_idx++;
   }
   
   DEXIT;
   return 0;
}

/*
 * If 'theOldConfEntries' do contain a reprioritize element and 'theNewConfEntries'
 * do not, return a *COPY* of 'theNewConfEntries' reprioritize element.
 */
static lListElem* is_reprioritize_missing(lList *theOldConfEntries, lList *theNewConfEntries)
{
   lListElem *old_reprio = NULL;
   lListElem *new_reprio = NULL;
   lListElem *res = NULL;

   DENTER(TOP_LAYER, "veify_reprioritize");

   old_reprio = lGetElemStr(theOldConfEntries, CF_name, REPRIORITIZE);
   new_reprio = lGetElemStr(theNewConfEntries, CF_name, REPRIORITIZE);
   
   if ((NULL == new_reprio) && (NULL != old_reprio))
   {
      res = lCopyElem(old_reprio);
   }
   
   DEXIT;
   return res;
}

/*
 * Remove configuration 'aConfName' from cluster configuration and append 'aNewConf' to it. 
 *
 * NOTE: 'anOldConf' is a *COPY* of the old configuration entry.
 */
static int exchange_conf_by_name(char *aConfName, lListElem *anOldConf, lListElem *aNewConf, lList**anAnswer)
{
   lListElem *elem = NULL;
   u_long32 old_version = 0;
   
   DENTER(TOP_LAYER, "remove_conf_by_name");

   old_version = lGetUlong(anOldConf, CONF_version);
   
   lSetUlong(aNewConf, CONF_version, old_version++); 
     
   /* Make sure, 'aNewConf' does have a unique name */
   lSetHost(aNewConf, CONF_hname, aConfName);   

   sge_mutex_lock("cluster_config_mutex", SGE_FUNC, __LINE__, &Cluster_Config.mutex);
   
   elem = lGetElemHost(Cluster_Config.list, CONF_hname, aConfName);

   lRemoveElem(Cluster_Config.list, elem);
   
   elem = lCopyElem(aNewConf);
   
   lAppendElem(Cluster_Config.list, elem);

   sge_mutex_unlock("cluster_config_mutex", SGE_FUNC, __LINE__, &Cluster_Config.mutex);

   sge_event_spool(anAnswer, 0, sgeE_CONFIG_MOD, 0, 0, aConfName, NULL, NULL, elem, NULL, NULL, true, true);
   
   DEXIT;
   return 0;
}

static bool has_reschedule_unknown_change(lList *theOldConfEntries, lList *theNewConfEntries)
{
   lListElem *old_elem = NULL;
   lListElem *new_elem = NULL;
   const char *old_value = NULL;
   const char *new_value = NULL;
   bool res = false;
   
   DENTER(TOP_LAYER, "has_reschedule_unknown_change");
   
   old_elem = lGetElemStr(theOldConfEntries, CF_name, "reschedule_unknown");
   new_elem = lGetElemStr(theNewConfEntries, CF_name, "reschedule_unknown");
   
   old_value = (NULL != old_elem) ? lGetString(old_elem, CF_value) : NULL;
   new_value = (NULL != new_elem) ? lGetString(new_elem, CF_value) : NULL;
      
   if ((NULL == old_value) || (NULL == new_value))
   {
      res = true;  /* change by omission in one configuration */
   }
   else if (((NULL != old_value) && (NULL != new_value)) && (strcmp(old_value, new_value) != 0))
   {
      res = true;  /* value did change */
   }
   
   if (true == res)
   {
      DPRINTF(("%s: reschedule_unknown did change!\n", SGE_FUNC));   
   }
   
   DEXIT;
   return res;
}

static int do_add_config(char *aConfName, lListElem *aConf, lList**anAnswer)
{
   lListElem *elem = NULL;

   DENTER(TOP_LAYER, "do_add_config");

   elem = lCopyElem(aConf);

   sge_mutex_lock("cluster_config_mutex", SGE_FUNC, __LINE__, &Cluster_Config.mutex);
   
   lAppendElem(Cluster_Config.list, elem);

   sge_mutex_unlock("cluster_config_mutex", SGE_FUNC, __LINE__, &Cluster_Config.mutex);

   sge_event_spool(anAnswer, 0, sgeE_CONFIG_ADD, 0, 0, aConfName, NULL, NULL, elem, NULL, NULL, true, true);
   
   DEXIT;
   return 0;
}

static int remove_conf_by_name(char *aConfName)
{
   lListElem *elem = NULL;
   
   DENTER(TOP_LAYER, "remove_conf_by_name");

   sge_mutex_lock("cluster_config_mutex", SGE_FUNC, __LINE__, &Cluster_Config.mutex);

   elem = lGetElemHost(Cluster_Config.list, CONF_hname, aConfName);

   lRemoveElem(Cluster_Config.list, elem);

   sge_mutex_unlock("cluster_config_mutex", SGE_FUNC, __LINE__, &Cluster_Config.mutex);
   
   DEXIT;
   return 0;
}
