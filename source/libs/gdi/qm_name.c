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
#include <stdlib.h>
#include <errno.h>

#include "sge_log.h"
#include "sgermon.h"
#include "basis_types.h"
#include "qm_name.h"
#include "setup_path.h"
#include "commd.h"
#include "msg_gdilib.h"

static char cached_master_name[MAXHOSTLEN] = "";

/*------------------------------------------------------------
 * sge_get_master
 *
 * get hostname of qmaster
 *
 * if read_master_file==0 we return the cached master_name
 *                   ==1 we look into the master_file
 *------------------------------------------------------------*/
const char *sge_get_master(
int read_master_file 
) {
   char err_str[SGE_PATH_MAX+128];

   DENTER(GDI_LAYER, "sge_get_master");

   if (!read_master_file && cached_master_name[0] != '\0') {
      DEXIT;
      return cached_master_name;
   }

   if (get_qm_name(cached_master_name, path.act_qmaster_file, err_str)) {
      ERROR((SGE_EVENT, MSG_GDI_READMASTERNAMEFAILED_S , err_str));
      DEXIT;
      return NULL;
   }

   DPRINTF(("got qmaster host \"%s\"\n", cached_master_name));
   DEXIT;
   return cached_master_name;
}

/*-----------------------------------------------------------------------
 * Read name of qmaster from master_file
 * -> master_file
 * <- return -1  error in err_str
 *           0   host name of master in master_host
 *           don't copy error to err_str if err_str = NULL
 *    master_file name of file which should point to act_qmaster file
 *    copy name of qmaster host to master_host
 *-----------------------------------------------------------------------*/
int get_qm_name(
char *master_host,
const char *master_file,
char *err_str 
) {
   FILE *fp;
   char buf[MAXHOSTLEN*3+1], *cp, *first;
   int len;

   DENTER(TOP_LAYER, "get_qm_name");
   
   if (!master_host || !master_file) {
      if (err_str)
         sprintf(err_str, MSG_GDI_NULLPOINTERPASSED );
      DEXIT;
      return -1;
   }

   if (!(fp=fopen(master_file,"r"))) {
      ERROR((SGE_EVENT, MSG_GDI_FOPEN_FAILED, master_file, strerror(errno)));
      if (err_str)
         sprintf(err_str, MSG_GDI_OPENMASTERFILEFAILED_S , master_file);
      DEXIT;
      return -1;
   }    

   /* read file in one sweep and append O Byte to the end */
   if (!(len = fread(buf, 1, MAXHOSTLEN*3, fp))) {
      if (err_str)
         sprintf(err_str, MSG_GDI_READMASTERHOSTNAMEFAILED_S , master_file);
   }
   buf[len] = '\0';
   
   /* Skip white space including newlines */
   cp = buf;
   while (*cp && (*cp == ' ' || *cp == '\t' || *cp == '\n'))
      cp++;
   
   first = cp;

   /* read all non white space characters */
   while (*cp && !(*cp == ' ' || *cp == '\t' || *cp == '\n'))
      cp++;
      
   *cp = '\0';
   len = cp - first;

   if (len == 0) {
      if (err_str)
         sprintf(err_str, MSG_GDI_MASTERHOSTNAMEHASZEROLENGTH_S , master_file);
      fclose(fp);
      DEXIT;
      return -1;
   }   
       
   if (len > MAXHOSTLEN - 1) {
      if (err_str)
         sprintf(err_str, MSG_GDI_MASTERHOSTNAMEEXCEEDSCHARS_SI , 
                 master_file, (int) MAXHOSTLEN);
      fclose(fp);
      DEXIT;
      return -1;
   }

   fclose(fp);
   strcpy(master_host, first);
   DEXIT;
   return 0;
}

/*********************************************************************
 Write the actual qmaster into the master_file
 -> master_file and master_host
 <- return -1   error in err_str
            0   means OK
 *********************************************************************/
int write_qm_name(
const char *master_host,
const char *master_file,
char *err_str 
) {
   FILE *fp;

   if (!(fp = fopen(master_file, "w"))) {
      if (err_str)
         sprintf(err_str, MSG_GDI_OPENWRITEMASTERHOSTNAMEFAILED_SS, 
                 master_file, strerror(errno));
      return -1;
   }

   if (fprintf(fp, "%s\n", master_host) == EOF) {
      if (err_str)
         sprintf(err_str, MSG_GDI_WRITEMASTERHOSTNAMEFAILED_S , 
                 master_file);
      fclose(fp);
      return -1;
   } 

   fclose(fp);
   return 0;
}
