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
#include "sge_all_listsL.h"
#include "sge_gdi_intern.h"
#include "qmaster_to_execd.h"
#include "sge_prognames.h"
#include "sgermon.h"
#include "sge_log.h"
#include "msg_qmaster.h"
#include "commlib.h"

static int notify_new_conf(const char *hostname, int progname_id);

/*
** NAME
**   notify_new_conf_2_execd - tell execd that its conf is outdated
** PARAMETER
**   hep     - host element to be notified, EH_Type
** RETURN
**   -1      - could not send message
**   -2      - no execd host known on that host
** EXTERNAL
**   prognames
** DESCRIPTION
**   sends the message TAG_GET_NEW_CONF to an execution daemon.
**   This function is executed when the qmaster receives a configuration
**   report which is not up to date.
*/
int notify_new_conf_2_execd(
lListElem *hep 
) {
   int ret;
   const char *hostname;
   
   DENTER(BASIS_LAYER, "notify_new_conf_2_execd");
   
   hostname = lGetHost(hep, EH_name);
   ret = notify_new_conf(hostname, EXECD);

   DEXIT;
   return ret;
}

/*
** NAME
**   notify_new_conf_2_qstd - tell sge_qstd that its conf is outdated
** PARAMETER
**   hep     - host element to be notified, EH_Type
** RETURN
**   -1      - could not send message
**   -2      - no execd host known on that host
** EXTERNAL
**   prognames
** DESCRIPTION
**   sends the message TAG_GET_NEW_CONF to an execution daemon.
**   This function is executed when the qmaster receives a configuration
**   report which is not up to date.
*/
int notify_new_conf_2_qstd(
lListElem *hep 
) {
   int ret;
   const char *hostname;
   
   DENTER(BASIS_LAYER, "notify_new_conf_2_qstd");
   
   hostname = lGetString(hep, EH_real_name);
   ret = notify_new_conf(hostname, QSTD);

   DEXIT;
   return ret;
}


/*-------------------------------------------------------------------------*/
static int notify_new_conf(
const char *hostname,
int progname_id 
) {
   int ret;
   u_short id;

   DENTER(BASIS_LAYER, "notify_new_conf");


   id = 1;
   if (!last_heard_from(prognames[progname_id], &id, hostname)) {
      ERROR((SGE_EVENT, MSG_NOXKNOWNONHOSTYTOSENDCONFNOTIFICATION_SS, 
               prognames[progname_id], hostname));
      DEXIT;
      return -2;
   }

   if (send_message_pb(0, prognames[progname_id], 0, hostname, TAG_GET_NEW_CONF,
                    NULL, 0))
      ret = -1;
   else
      ret = 0;

   DEXIT;
   return ret;
}

