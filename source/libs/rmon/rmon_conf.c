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
#define DEBUG

#include "rmon_h.h"
#include "rmon_err.h"
#include "rmon_rmon.h"
#include "rmon_conf.h"
#include "msg_rmon.h"
#define TYPE    0
#define VALUE   1

char rmon_root[100] = RMON_ROOT;
char rmon_config_file_and_path[100] = "\0";

char rmond[81];
extern u_long timeout_value;
u_long max_messages = 100;

extern int name_is_valid;

void rmon_read_configuration()
{
   FILE *fp;
   struct hostent *hp;
   char *s, c, type[81], value[81];
   int rmond_name_found = 0;
   int merk, to_read = TYPE, i = 0;
   int lc = 1, cc = 0;          /* Line Counter, Character Counter */

#undef FUNC
#define FUNC "rmon_read_configuration"
   DENTER;

   s = getenv("SGE_ROOT");

   if (s) {
      if (s[strlen(s) - 1] != '/')
         strcat(s, "/");

      strcpy(rmon_root, s);
   }

   sprintf(rmon_config_file_and_path, "%sconf/%s", rmon_root, RMON_CONFIG_FILE);
   DPRINTF(("configuration path and file: %s\n", rmon_config_file_and_path));

   if (!(fp = fopen(rmon_config_file_and_path, "r")))
      rmon_errf(TERMINAL, MSG_RMON_CANNOTOPENXFORCONFIGDATAEXIT_S,
                rmon_config_file_and_path);

   while ((merk = getc(fp)) != EOF) {
      c = (char) merk;

      cc++;
      if (c == '\n') {
         lc++;
         cc = 0;
      }

      if (to_read == TYPE) {
         if ((c == ' ' || c == '\n') && i == 0)
            continue;

         if (c != ' ' && c != '\n') {
            if (c != '[' && i == 0)
               rmon_errf(TERMINAL, MSG_RMON_ERRORINXLINEYCHARACTERZ_SII ,
                         rmon_config_file_and_path, lc, cc);

            type[i++] = toupper(c);
            continue;
         }

         if ((c == ' ' || c == '\n') && i != 0) {
            type[i] = '\0';
            to_read = VALUE;
            i = 0;
            continue;
         }
      }

      if (to_read == VALUE) {
         if ((c == ' ' || c == '\n') && i == 0)
            continue;

         if (c != ' ' && c != '\n') {
            value[i++] = toupper(c);
            continue;
         }

         if ((c == ' ' || c == '\n') && i != 0) {
            value[i] = '\0';
            to_read = TYPE;
            i = 0;
         }
      }

      if (strcmp(type, "[RMOND_HOST_NAME]") == 0) {
         strcpy(rmond, value);
         rmond_name_found = 1;
         continue;
      }

      if (strcmp(type, "[TIMEOUT_VALUE]") == 0) {
         sscanf(value, "%ld", &timeout_value);
         continue;
      }

      if (strcmp(type, "[MAX_MESSAGES]") == 0) {
         sscanf(value, "%ld", &max_messages);
         continue;
      }

      rmon_errf(TRIVIAL, MSG_RMON_UNKNOWNCONFIGURATIONTYPEX_S , type);
   }

   fclose(fp);

   if (!rmond_name_found)
      rmon_errf(TERMINAL, MSG_RMON_CANTFINDRMONDHOSTNAMEINXEXIT_S,
                rmon_config_file_and_path);

   DPRINTF(("RMOND: %s\n", rmond));
   DPRINTF(("TIMEOUT:   %d\n", timeout_value));

   hp = gethostbyname(rmond);
   if (hp == (struct hostent *) NULL)
      rmon_errf(TERMINAL, MSG_RMON_HOSTNAMEXCOULDNOTBERESOLVED_S , rmond);

   name_is_valid = 1;
   DEXIT;
}

int rmon_save_conf_file()
{
   FILE *fp;

#undef FUNC
#define FUNC "rmon_save_conf_file"
   DENTER;

   if (!(fp = fopen(rmon_config_file_and_path, "w"))) {
      DPRINTF(("cannot open %s for writing\n", rmon_config_file_and_path));
      DEXIT;
      return 0;
   }

   fseek(fp, 0, 2);
   fprintf(fp, "[RMOND_HOST_NAME]\n");
   fprintf(fp, "%s\n\n", rmond);

   fprintf(fp, "[TIMEOUT_VALUE]\n");
   fprintf(fp, "%ld\n\n", timeout_value);

   fprintf(fp, "[MAX_MESSAGES]\n");
   fprintf(fp, "%ld\n\n", max_messages);

   fclose(fp);

   DEXIT;
   return 1;
}

u_long rmon_make_addr(
char *host 
) {
   u_long addr;
   struct hostent *he;

   if (host[0] >= '0' && host[0] <= '9') {
      addr = inet_addr(host);
      if (addr == (u_long) - 1)
         return 0;
   }
   else {
      if ((he = gethostbyname(host)) == NULL)
         return 0;

      bcopy((char *) he->h_addr, (char *) &addr, he->h_length);
   }

   addr = ntohl(addr);

   return addr;
}

int rmon_make_name(
char *host,
char *name 
) {
   u_long addr;
   struct hostent *he;

   if (!(host[0] >= '0' && host[0] <= '9')) {
      if ((he = gethostbyname(host)) == NULL)
         return 0;

      strcpy(name, host);
      return 1;
   }

   addr = inet_addr(host);
   if (addr == -1) {
      return 0;
   }

   if ((he = gethostbyaddr((char *) &addr, sizeof(u_long), AF_INET)) == NULL)
      return 0;

   bcopy(he->h_name, name, (int) strlen(he->h_name));
   return 1;
}
