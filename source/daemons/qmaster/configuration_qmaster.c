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
#include "sge_answerL.h"
#include "sge_usersetL.h"
#include "sge_userprjL.h"
#include "sge_gdi_intern.h"
#include "sge_confL.h"
#include "sge_eventL.h"
#include "sge_exit.h"
#include "sge_log.h"
#include "sge_me.h"
#include "sge_parse_num_par.h"
#include "sgermon.h"
#include "commlib.h"
#include "sge_conf.h"
#include "configuration_qmaster.h"
#include "cull.h"
#include "def.h"
#include "resolve_host.h"
#include "config_file.h"
#include "rw_configuration.h"
#include "utility.h"
#include "sge_hostL.h"
#include "sge_userset_qmaster.h"
#include "gdi_utility_qmaster.h"
#include "sge_userprj_qmaster.h"
#include "sge_parse_num_par.h"
#include "setup_path.h"
#include "msg_common.h"
#include "msg_utilib.h"
#include "msg_qmaster.h"
#include "sge_m_event.h"
#include "sge_string.h"
#include "sge_dirent.h"
#include "reschedule.h"
#include "sge_switch_user.h"

static int check_config(lList **alpp, lListElem *conf);
   
extern lList *Master_Config_List;
extern lList *Master_Project_List;
extern lList *Master_Exechost_List;

/* make chached values from configuration invalid */
int new_config = 1;

/*----------------------------------------------------
 * read_all_configurations
 * qmaster function to read all configurations
 * should work with absolute pathnames
 * Must be called after internal setup
 *----------------------------------------------------*/
int read_all_configurations(
lList **lpp 
) {
   DIR *dir;
   SGE_STRUCT_DIRENT *dent;
   char fstr[256];
   lListElem *el, *epc;
   int ret;
   static int admin_user_initialized = 0;

   DENTER(TOP_LAYER, "read_all_configurations");

   if (!lpp) {
      DEXIT;
      return -1;
   }

   if (!path.local_conf_dir) {
      DEXIT;
      SGE_EXIT(1);
   }

   if (!*lpp) {
      *lpp = lCreateList("conf list", CONF_Type);
   }

   /* First read global configuration. If configurations doesn't exist
      create one with compiled in defaults */

   el = read_configuration(path.conf_file, SGE_GLOBAL_NAME, FLG_CONF_SPOOL);
   if (el)
      lAppendElem(*lpp, el);
   else {
      lList *lpDefaults = NULL;
      char config_str[1024], real_config_str[1024];

      WARNING((SGE_EVENT, MSG_GLOBALCONFDOESNOTEXISTCREATING));
      lpDefaults = sge_set_defined_defaults(lpDefaults);
   
      epc = lCreateElem(CONF_Type);
      lSetHost(epc, CONF_hname, SGE_GLOBAL_NAME);
      lSetList(epc, CONF_entries, lCopyList("global config", lpDefaults));
      lAppendElem(*lpp, epc);
      lFreeList(lpDefaults);
      
      sprintf(config_str, "%s/%s/.%s", path.cell_root, 
            "common", "configuration"); 
      sprintf(real_config_str, "%s", path.conf_file); 
      if ((ret=write_configuration(1, NULL, config_str, epc, NULL, 
         FLG_CONF_SPOOL))) {
         /* answer list gets filled in write_configuration() */
         DEXIT;
         return ret;
      } else {
         if (rename(config_str, real_config_str) == -1) {
            DEXIT;
            return 1;
         }
      }
   }

   if (!admin_user_initialized) {
      const char *admin_user = NULL;
      char err_str[MAX_STRING_SIZE];
      int lret;

      admin_user = read_adminuser_from_configuration(el, path.conf_file,
                                             SGE_GLOBAL_NAME, FLG_CONF_SPOOL);
      lret = set_admin_username(admin_user, err_str);
      if (lret == -1) {
         ERROR((SGE_EVENT, err_str));
         DEXIT;
         return -1;
      }
      admin_user_initialized = 1;
   }
   
   /* read local configurations from local_conf_dir */ 

   dir = opendir(path.local_conf_dir);
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

      sprintf(fstr, "%s/%s", path.local_conf_dir, dent->d_name);
      
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

            sprintf(fname, "%s/.%s", path.local_conf_dir, new_name);
            sprintf(real_fname, "%s/%s", path.local_conf_dir, new_name);

            switch2admin_user();
            DPRINTF(("path.conf_file: %s\n", fname));
            if ((ret=write_configuration(1, &alp, fname, el, NULL, FLG_CONF_SPOOL))) {
               /* answer list gets filled in write_configuration() */
               free(old_name);
               switch2start_user();
               DEXIT;
               return -1;
            } else {
               if (rename(fname, real_fname) == -1) {
                  free(old_name);
                  switch2start_user();
                  DEXIT;
                  return -1;
               }
            }
            switch2start_user();
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
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   if (!(config_name = lGetHost(confp, CONF_hname))) {
      /* confp is no config element, if confp has no CONF_hname */
      CRITICAL((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
                lNm2Str(CONF_hname), SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* check if configuration is in Master_Config_List */
   if (!(ep = lGetElemHost(Master_Config_List, CONF_hname, config_name))) {
      ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, 
               MSG_OBJ_CONF, config_name));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* check if someone tries to delete global configuration */
   if (!strcasecmp(SGE_GLOBAL_NAME, config_name)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANT_DEL_CONFIG_S, config_name));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
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
         sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
         DEXIT;
         return STATUS_EEXIST;
      }
   }
#endif   

    

   if (sge_unlink(path.local_conf_dir, config_name)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANT_DEL_CONFIG_DISK_SS, config_name, strerror(errno)));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EDISK, 0);
      DEXIT;
      return STATUS_EDISK;
   }
  

    
   /* now remove it from our internal list*/
   lRemoveElem(Master_Config_List, ep);

   INFO((SGE_EVENT, MSG_SGETEXT_REMOVEDFROMLIST_SSSS,
         ruser, rhost, config_name, MSG_OBJ_CONF ));

   update_reschedule_unknown_timout_values(config_name);

   /* make chached values from configuration invalid */
   new_config = 1;

   sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   sge_add_event(NULL, sgeE_CONFIG_DEL, 0, 0, config_name, NULL);
   
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
   int added;
   char fname[SGE_PATH_MAX];
   char real_fname[SGE_PATH_MAX];
   u_long32 old_conf_version = 0;
   int reschedule_unknown_changed = 1;
   int ret;

   DENTER(TOP_LAYER, "sge_mod_configuration");

   if ( !confp || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   if (!(config_name = lGetHost(confp, CONF_hname))) {
      /* confp is no config element, if confp has no CONF_hname */
      CRITICAL((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
                lNm2Str(CONF_hname), SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
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
      added = TRUE; 
   }
   else {
      added = FALSE;
   }

   if (added == FALSE) {
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
                sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
                DEXIT;
                return STATUS_EUNKNOWN;
            }
         }
         paramPos++;
      }  /* while */
   }

   if (added == FALSE) {
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
   if (added == FALSE) {
      lSetUlong(ep, CONF_version, old_conf_version + 1);
      lSetUlong(confp, CONF_version, old_conf_version + 1);
   }
    
   if (!strcmp(SGE_GLOBAL_NAME, config_name)) {
      strcpy(fname, path.cell_root);
      strcat(fname, "/common");
      strcat(fname, "/.");
      strcat(fname, "configuration");
      strcpy(real_fname, path.conf_file);
      sge_add_event(NULL, sgeE_GLOBAL_CONFIG, 0, 0, NULL, NULL);
   } else {
      sprintf(fname, "%s/.%s", path.local_conf_dir, config_name);
      sprintf(real_fname, "%s/%s", path.local_conf_dir, config_name);
   }
   
   DPRINTF(("path.conf_file: %s\n", fname));   
   if ((ret=write_configuration(1, alpp, fname, confp, NULL, FLG_CONF_SPOOL))) {
      /* answer list gets filled in write_configuration() */
      DEXIT;
      return ret;
   } else {
      if (rename(fname, real_fname) == -1) {
         DEXIT;
         return 1;
      }
   }

   /*
   ** is the configuration change relevant for the qmaster itsself?
   ** if so, initialise conf struct anew
   */
   if (!strcmp(SGE_GLOBAL_NAME, config_name) || 
       !hostcmp(config_name, me.qualified_hostname)) {
      lListElem *lep = NULL;

      ret = select_configuration(me.qualified_hostname, Master_Config_List, &lep);
      if (ret) {
         ERROR((SGE_EVENT, MSG_CONF_CANTSELECTCONFIGURATIONFORHOST_SI,
               me.qualified_hostname, ret));
      }
      ret = merge_configuration(
         lGetElemHost(Master_Config_List, CONF_hname, SGE_GLOBAL_NAME), 
               lep, &conf, NULL);
      if (ret) {
         ERROR((SGE_EVENT, MSG_CONF_CANTMERGECONFIGURATIONFORHOST_SI,
                me.qualified_hostname, ret));
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
   sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   sge_add_event(NULL, added ? sgeE_CONFIG_ADD : sgeE_CONFIG_MOD, 0, 0, config_name, ep);
   
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
         sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
         DEXIT;
         return STATUS_EEXIST;
      }
      if (!value) {
         ERROR((SGE_EVENT, MSG_CONF_VALUEISNULLFORATTRXINCONFIGURATIONLISTOFY_SS,
                name, conf_name));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
         DEXIT;
         return STATUS_EEXIST;
      }
      if (!strcmp(name, "loglevel")) {
         u_long32 tmp_uval;
         if (sge_parse_loglevel_val(&tmp_uval,value) != 1) {
            ERROR((SGE_EVENT, MSG_CONF_GOTINVALIDVALUEXFORLOGLEVEL_S, value));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
            DEXIT;
            return STATUS_EEXIST;
         }
      }

      if (!strcmp(name, "shell_start_mode")) {
 
         if ( (strcasecmp("unix_behavior",value) != 0) && 
              (strcasecmp("posix_compliant",value) != 0) &&
              (strcasecmp("script_from_stdin",value) != 0) ) {
            ERROR((SGE_EVENT, MSG_CONF_GOTINVALIDVALUEXFORSHELLSTARTMODE_S, value));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
            DEXIT;
            return STATUS_EEXIST;
         }
      }

      if (!strcmp(name, "load_report_time")) {
         /* do not allow infinity entry for load_report_time */
         if (strcasecmp(value,"infinity") == 0) {
            ERROR((SGE_EVENT, MSG_CONF_INFNOTALLOWEDFORATTRXINCONFLISTOFY_SS, name, conf_name));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
            DEXIT;
            return STATUS_EEXIST;
         }
      }

      if (!strcmp(name, "stat_log_time")) {
         /* do not allow infinity entry */
         if (strcasecmp(value,"infinity") == 0) {
            ERROR((SGE_EVENT, MSG_CONF_INFNOTALLOWEDFORATTRXINCONFLISTOFY_SS, name, conf_name));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
            DEXIT;
            return STATUS_EEXIST;
         }
      }

      if (!strcmp(name, "max_unheard")) {
         /* do not allow infinity entry */
         if (strcasecmp(value,"infinity") == 0) {
            ERROR((SGE_EVENT, MSG_CONF_INFNOTALLOWEDFORATTRXINCONFLISTOFY_SS, name, conf_name));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
            DEXIT;
            return STATUS_EEXIST;
         }
      }


      if (!strcmp(name, "admin_user")) {
         if (strcasecmp(value, "none") && !getpwnam(value)) {
            ERROR((SGE_EVENT, MSG_CONF_GOTINVALIDVALUEXASADMINUSER_S, value));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
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
            sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
            DEXIT;
            return STATUS_EEXIST;
         }

         /* .. checking userset names */
         ok = (verify_acl_list(alpp, tmp, name, conf_name, "configuration")==STATUS_OK);
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
            sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
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
               sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, NUM_AN_ERROR);
               DEXIT;
               return STATUS_EEXIST;
            }
   
            /* ensure that variables are valid */
            if (replace_params(script, NULL, 0, prolog_epilog_variables)) {
               ERROR((SGE_EVENT, MSG_CONF_PARAMETERXINCONFIGURATION_SS, name, err_msg));
               sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
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
