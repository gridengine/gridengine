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
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "sgermon.h"
#include "sge_log.h"
#include "qmaster_heartbeat.h"

#include "msg_daemons_common.h"

/*--------------------------------------------------------------
 * Name:   get_qmaster_heartbeat
 * Descr:  get number found in qmaster heartbeat file
 * Return: number found in given heartbeat file
 *         -1 if file not found or entry couldn't be read
 *-------------------------------------------------------------*/
int get_qmaster_heartbeat(
char *file 
) {
   FILE *fp;
   int hb;

   DENTER(TOP_LAYER, "get_qmaster_heartbeat");

   fp = fopen(file, "r");
   if (!fp) {
      ERROR((SGE_EVENT, MSG_HEART_CANNOTOPEN, file, strerror(errno))); 
      DEXIT;
      return -1;
   }

   if (fscanf(fp, "%d", &hb) != 1) {
      fclose(fp);
      DEXIT;
      return -1;
   }

   fclose(fp);
   DEXIT;
   return hb;
}

/*--------------------------------------------------------------
 * Name:   inc_qmaster_heartbeat
 * Descr:  increment number found in qmaster heartbeat file
 *         wrap at 99999
 *         if file doesn't exist create it
 * Return: 0 if operation was successfull
 *         -1 if not
 *-------------------------------------------------------------*/
int inc_qmaster_heartbeat(
char *file 
) {
   FILE *fp;
   int hb;

   DENTER(TOP_LAYER, "inc_qmaster_heartbeat");

   fp = fopen(file, "r+");
   if (!fp) {
      fp = fopen(file, "w+");
      if (!fp) {
         DEXIT;
         return -1;
      }
   }

   if (fscanf(fp, "%d", &hb) != 1)
      hb = 1;
   else if (++hb > 99999)
      hb = 1;

   fseek(fp, 0, 0);
   if (fprintf(fp, "%d\n", hb) == EOF) {
      fclose(fp);
      DEXIT;
      return -1;
   }

   fclose(fp);

   DEXIT;
   return 0;
}
