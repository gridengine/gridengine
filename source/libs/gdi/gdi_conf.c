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

#include "sge.h"
#include "cull.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_gdi.h"
#include "sge_conf.h"
#include "sge_string.h"
#include "sge_host.h"
#include "sge_answer.h"
#include "sge_prog.h"
#include "sge_time.h"
#include "sge_unistd.h"
#include "commlib.h"
#include "gdi_conf.h"
#include "sge_any_request.h"

#include "msg_gdilib.h"
#include "msg_sgeobjlib.h"
#include "msg_common.h"

/*-------------------------------------------------------------------------*
 * NAME
 *   get_configuration - retrieves configuration from qmaster
 * PARAMETER
 *   config_name       - name of local configuration or "global",
 *                       name is being resolved before action
 *   gepp              - pointer to list element containing global
 *                       configuration, CONF_Type, should point to NULL
 *                       or otherwise will be freed
 *   lepp              - pointer to list element containing local configuration
 *                       by name given by config_name, can be NULL if global
 *                       configuration is requested, CONF_Type, should point
 *                       to NULL or otherwise will be freed
 * RETURN
 *   -1   NULL pointer received
 *   -2   error resolving host
 *   -3   invalid NULL pointer received for local configuration
 *   -4   request to qmaster failed
 *   -5   there is no global configuration
 *   -6   commproc already registered
 *   -7   no permission to get configuration
 * EXTERNAL
 *
 * DESCRIPTION
 *   retrieves a configuration from the qmaster. If the configuration
 *   "global" is requested, then this function requests only this one.
 *   If not, both the global configuration and the requested local
 *   configuration are retrieved.
 *   This function was introduced to make execution hosts independent
 *   of being able to mount the local_conf directory.
 *-------------------------------------------------------------------------*/
int get_configuration(
const char *config_name,
lListElem **gepp,
lListElem **lepp 
) {
   lCondition *where;
   lEnumeration *what;
   lList *alp = NULL;
   lList *lp = NULL;
   int is_global_requested = 0;
   int ret;
   lListElem *hep = NULL;
   int success;
   static int already_logged = 0;
   u_long32 status;
   
   DENTER(TOP_LAYER, "get_configuration");

   if (!config_name || !gepp) {
      DEXIT;
      return -1;
   }

   if (*gepp) {
      lFreeElem(*gepp);
      *gepp = NULL;
   }
   if (lepp && *lepp) {
      lFreeElem(*lepp);
      *lepp = NULL;
   }

   if (!strcasecmp(config_name, "global")) {
      is_global_requested = 1;
   }
   else {
      hep = lCreateElem(EH_Type);
      lSetHost(hep, EH_name, config_name);

      ret = sge_resolve_host(hep, EH_name);

      if ( sge_get_communication_error() == CL_RETVAL_ENDPOINT_NOT_UNIQUE) {
         CRITICAL((SGE_EVENT, "endpoint not unique error"));
         DEXIT;
         return -6;
      }

      if (ret != CL_RETVAL_OK) {
         DPRINTF(("get_configuration: error %d resolving host %s: %s\n", ret, config_name, cl_get_error_text(ret)));
         lFreeElem(hep);
         ERROR((SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_S, config_name));
         DEXIT;
         return -2;
      }

      DPRINTF(("get_configuration: unique for %s: %s\n", config_name, lGetHost(hep, EH_name)));
   }

   if (!is_global_requested && !lepp) {
      ERROR((SGE_EVENT, MSG_NULLPOINTER));
      lFreeElem(hep);
      DEXIT;
      return -3;
   }

   if (is_global_requested) {
      /*
       * they might otherwise send global twice
       */
      where = lWhere("%T(%I c= %s)", CONF_Type, CONF_hname, SGE_GLOBAL_NAME);
      DPRINTF(("requesting global\n"));
   }
   else {
      where = lWhere("%T(%I c= %s || %I h= %s)", CONF_Type, CONF_hname, SGE_GLOBAL_NAME, CONF_hname,
                     lGetHost(hep, EH_name));
      DPRINTF(("requesting global and %s\n", lGetHost(hep, EH_name)));
   }
   what = lWhat("%T(ALL)", CONF_Type);
   alp = sge_gdi(SGE_CONFIG_LIST, SGE_GDI_GET, &lp, where, what);

   lFreeWhat(what);
   lFreeWhere(where);

   success = ((status= lGetUlong(lFirst(alp), AN_status)) == STATUS_OK);
   if (!success) {
      if (!already_logged) {
         ERROR((SGE_EVENT, MSG_CONF_GETCONF_S, lGetString(lFirst(alp), AN_text)));
         already_logged = 1;       
      }
                   
      lFreeList(alp);
      lFreeList(lp);
      lFreeElem(hep);
      DEXIT;
      return (status != STATUS_EDENIED2HOST)?-4:-7;
   }
   lFreeList(alp);

   if (lGetNumberOfElem(lp) > (2 - is_global_requested)) {
      WARNING((SGE_EVENT, MSG_CONF_REQCONF_II, 2 - is_global_requested, lGetNumberOfElem(lp)));
   }

   if (!(*gepp = lGetElemHost(lp, CONF_hname, SGE_GLOBAL_NAME))) {
      ERROR((SGE_EVENT, MSG_CONF_NOGLOBAL));
      lFreeList(lp);
      lFreeElem(hep);
      DEXIT;
      return -5;
   }
   lDechainElem(lp, *gepp);

   if (!is_global_requested) {
      if (!(*lepp = lGetElemHost(lp, CONF_hname, lGetHost(hep, EH_name)))) {
         if (*gepp) {
            WARNING((SGE_EVENT, MSG_CONF_NOLOCAL_S, lGetHost(hep, EH_name)));
         }
         lFreeList(lp);
         lFreeElem(hep);
         already_logged = 0;
         DEXIT;
         return 0;
      }
      lDechainElem(lp, *lepp);
   }
   
   lFreeElem(hep);
   lFreeList(lp);
   already_logged = 0;
   DEXIT;
   return 0;
}

int get_conf_and_daemonize(
tDaemonizeFunc dfunc,
lList **conf_list
) {
   lListElem *global = NULL;
   lListElem *local = NULL;
   int ret;
   time_t now, last;

   DENTER(TOP_LAYER, "get_conf_and_daemonize");
   /*
    * for better performance retrieve 2 configurations
    * in one gdi call
    */
   DPRINTF(("qualified hostname: %s\n",  uti_state_get_qualified_hostname()));

   now = last = (time_t) sge_get_gmt();
   while ((ret = get_configuration(uti_state_get_qualified_hostname(), &global, &local))) {
      if (ret==-6 || ret==-7) {
         /* confict: COMMPROC ALREADY REGISTERED */
         DEXIT;
         return -1;
      }
      if (!uti_state_get_daemonized()) {
         ERROR((SGE_EVENT, MSG_CONF_NOCONFBG));
         if (!getenv("SGE_ND"))
            dfunc();
         else
            WARNING((SGE_EVENT, MSG_CONF_NOCONFSLEEP));
         sleep(10);
      }
      else {
         DTRACE;
         sleep(60);
         now = (time_t) sge_get_gmt();
         if (last > now)
            last=now;
         if (now - last > 1800) {
            last = now;
            ERROR((SGE_EVENT, MSG_CONF_NOCONFSTILL));
         }
      }
   }

   ret = merge_configuration(global, local, &conf, NULL);
   if (ret) {
      DPRINTF((
         "Error %d merging configuration \"%s\"\n", ret, uti_state_get_qualified_hostname()));
   }

   /*
    * we don't keep all information, just the name and the version
    * the entries are freed
    */
   lSetList(global, CONF_entries, NULL);
   lSetList(local, CONF_entries, NULL);
   *conf_list = lFreeList(*conf_list);
   *conf_list = lCreateList("config list", CONF_Type);
   lAppendElem(*conf_list, global);
   lAppendElem(*conf_list, local);
   DEXIT;
   return 0;
}

/*-------------------------------------------------------------------------*
 * NAME
 *   get_merged_conf - requests new configuration set from master
 * RETURN
 *   -1      - could not get configuration from qmaster
 *   -2      - could not merge global and local configuration
 * EXTERNAL
 *
 *-------------------------------------------------------------------------*/
int get_merged_configuration(
lList **conf_list
) {
   lListElem *global = NULL;
   lListElem *local = NULL;
   int ret;

   DENTER(TOP_LAYER, "get_merged_configuration");

   DPRINTF(("qualified hostname: %s\n",  uti_state_get_qualified_hostname()));
   ret = get_configuration(uti_state_get_qualified_hostname(), &global, &local);
   if (ret) {
      ERROR((SGE_EVENT, MSG_CONF_NOREADCONF_IS, ret, uti_state_get_qualified_hostname()));
      lFreeElem(global);
      lFreeElem(local);
      return -1;
   }

   ret = merge_configuration(global, local, &conf, NULL);
   if (ret) {
      ERROR((SGE_EVENT, MSG_CONF_NOMERGECONF_IS, ret, uti_state_get_qualified_hostname()));
      lFreeElem(global);
      lFreeElem(local);
      return -2;
   }
   /*
    * we don't keep all information, just the name and the version
    * the entries are freed
    */
   lSetList(global, CONF_entries, NULL);
   lSetList(local, CONF_entries, NULL);

   *conf_list = lFreeList(*conf_list);
   *conf_list = lCreateList("config list", CONF_Type);
   lAppendElem(*conf_list, global);
   lAppendElem(*conf_list, local);

   DEXIT;
   return 0;
}

