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
#include <string.h>

#include "sge.h"
#include "sge_gdi_intern.h"
#include "commlib.h"
#include "sge_hostL.h"
#include "sge_complexL.h"
#include "sgermon.h"
#include "sge_log.h"
#include "resolve_host.h"

/* ------------------------------------------------------------ */
int sge_resolve_host(
lListElem *ep,
int nm 
) {
   int pos, ret;
   char unique[MAXHOSTLEN];
   const char *hostname;

   DENTER(TOP_LAYER, "sge_resolve_host");

   /* ep is no host element, if ep has no nm */
   if ((pos = lGetPosViaElem(ep, nm)) < 0) {
      DEXIT;
      return -1;
   }

   hostname = lGetPosString(ep, pos);

   ret = sge_resolve_hostname(hostname, unique, nm);
   
   switch ( ret ) {
   case NACK_UNKNOWN_HOST:
      DEXIT;
      return ret;
   case CL_OK:
      lSetPosString(ep, pos, unique);
      DEXIT;
      return ret;
   default:
      DEXIT; /* can't say */
      return ret;
   }
}

/* ------------------------------------------------------------ */
int sge_resolve_hostname(
const char *hostname,
char *unique,
int nm 
) {
   int ret;

   DENTER(TOP_LAYER, "sge_resolve_hostname");

   if ( !hostname ) {
      DEXIT;
      return CL_RANGE;
   }

   /* these names are resolved */
   switch (nm) {
   case CE_stringval:
      if (!strcmp(hostname, SGE_UNKNOWN_NAME)) {
         strcpy(unique, hostname);
         DEXIT;
         return 0;
      }
      break;

   case EH_name:
      if (!strcmp(hostname, SGE_GLOBAL_NAME) ||
        !strcmp(hostname, SGE_TEMPLATE_NAME)) {
         strcpy(unique, hostname);
         DEXIT;
         return 0;
      }
      break;

   default:
      break;
   }

   /* try to resolve hostname */
   ret=getuniquehostname(hostname, unique, 0);

   DEXIT;
   return ret;
}
