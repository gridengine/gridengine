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
#include <stdlib.h>
#include <errno.h>

#include "sge.h"
#include "cull.h"
#include "read_write_host.h"
#include "config.h"
#include "read_object.h"
#include "sge_feature.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_string.h"
#include "sge_stdio.h"
#include "msg_common.h"
#include "spool/classic/msg_spoollib_classic.h"
#include "sge_spool.h"
#include "sge_io.h"
#include "sge_userprj.h"
#include "sge_userset.h"
#include "sge_host.h"
#include "sge_centry.h"
#include "sge_str.h"
#include "sge_load.h"
#include "sched/sge_resource_utilization.h"

static int intprt_as_scaling[] = { HS_name, HS_value, 0 };
static int intprt_as_load[] = { HL_name, HL_value, 0 };
static int intprt_as_load_thresholds[] = { CE_name, CE_stringval, 0 };
static int intprt_as_reschedule[] = { RU_job_number, RU_task_number,
   RU_state, 0 };


/*
 * read_host_work
 *
 * parsing_type: 
 *    0 - 'normal' parsing
 *    1 - parse definition lists as name lists 
 */
int read_host_work(
lList **alpp,
lList **clpp,
int fields[],
lListElem *ep,
int type,      /* CULL_READ_xxx */
int nm,
int *tag,
int parsing_type 
) {
   DENTER(TOP_LAYER, "read_host_work");

   /* --------- EH_name, AH_name, SH_name */
   if (!set_conf_string(alpp, clpp, fields, "hostname", ep, nm)) {
      DEXIT;
      return -1;
   }

   if (nm == EH_name) {
      int opt[10];
      opt[0] = NoName;

      /* --------- EH_scaling_list */
      if (parsing_type == 0) {
         if (!set_conf_deflist(alpp, clpp, fields, "load_scaling", ep, 
                  EH_scaling_list, HS_Type, intprt_as_scaling)) {
            DEXIT;
            return -1;
         }
      } else {
         if (!set_conf_list(alpp, clpp, fields, "load_scaling", ep,
                  EH_scaling_list, HS_Type, HS_name)) {
            DEXIT;
            return -1;
         } 
      }

      /* --------- EH_consumable_config_list */
      if (parsing_type == 0) {
         if (!set_conf_deflist(alpp, clpp, fields?fields:opt, "complex_values",
                  ep, EH_consumable_config_list,
               CE_Type, intprt_as_load_thresholds)) {
            DEXIT;
            return -1;
         }
      } else {
         if (!set_conf_list(alpp, clpp, fields?fields:opt, "complex_values",
             ep, EH_consumable_config_list, CE_Type, CE_name)) {
            DEXIT;
            return -1;
         }
      }

      /* --------- EH_consumable_config_list */
      if (getenv("MORE_INFO")) {
         if (parsing_type == 0) {
            if (!set_conf_deflist(alpp, clpp, fields?fields:opt, "complex_values_actual",
                     ep, EH_resource_utilization,
                  RUE_Type, intprt_as_load_thresholds)) {
               DEXIT;
               return -1;
            }
         } else {
            if (!set_conf_list(alpp, clpp, fields?fields:opt, "complex_values_actual",
                ep, EH_resource_utilization, RUE_Type, RUE_name)) {
               DEXIT;
               return -1;
            }
         }
      }

      if ((type == CULL_READ_SPOOL) || (type == CULL_READ_HISTORY)) {
         /* --------- EH_load_list */
         if (!set_conf_deflist(alpp, clpp, fields, "load_values", ep, 
                  EH_load_list, HL_Type, intprt_as_load)) {
            DEXIT;
            return -1;
         }

         /* --------- EH_processors */
         if (!set_conf_ulong(alpp, clpp, fields, "processors", ep, 
                  EH_processors)) {
            DEXIT;
            return -1;
         }
      }
   
      if (type == CULL_READ_SPOOL) {
         /* --------- EH_reschedule_unknown_list */
         if (!set_conf_deflist(alpp, clpp, fields, "reschedule_unknown_list", 
             ep, EH_reschedule_unknown_list, RU_Type, intprt_as_reschedule)) {
            DEXIT;
            return -1;
         }
      }

      /* --------- EH_acl  */
      if (!set_conf_list(alpp, clpp, fields?fields:opt, "user_lists", ep,
               EH_acl, US_Type, US_name)) {
         DEXIT;
         return -1;
      }

      /* --------- EH_xacl  */
      if (!set_conf_list(alpp, clpp, fields?fields:opt, "xuser_lists", ep,
               EH_xacl, US_Type, US_name)) {
         DEXIT;
         return -1;
      }

      /* --------- EH_prj  */
      if (!set_conf_list(alpp, clpp, fields?fields:opt, "projects", ep,
               EH_prj, UP_Type, UP_name)) {
         DEXIT;
         return -1;
      }

      /* --------- EH_xprj  */
      if (!set_conf_list(alpp, clpp, fields?fields:opt, "xprojects", ep,
               EH_xprj, UP_Type, UP_name)) {
         DEXIT;
         return -1;
      }

      /* --------- EH_usage_scaling_list */
      if (parsing_type == 0) {
         if (!set_conf_deflist(alpp, clpp, fields, "usage_scaling", ep, 
               EH_usage_scaling_list, HS_Type, intprt_as_scaling)) {
            DEXIT;
            return -1;
         }
      } else {
         if (!set_conf_list(alpp, clpp, fields, "usage_scaling", ep,
               EH_usage_scaling_list, HS_Type, HS_name)) {
            DEXIT;
            return -1;
         }            
      }

      /* --------- EH_report_variables */
      if (!set_conf_list(alpp, clpp, fields, "report_variables", ep, 
               EH_report_variables, STU_Type, STU_name)) {
         DEXIT;
         return -1;
         }
      
   }

   DEXIT;
   return 0;
}


/****
 **** cull_read_in_host
 ****/
lListElem *cull_read_in_host(
const char *dirname,
const char *filename,
int spool_type, /* CULL_READ_xxx */
int type,
int *tag,
int fields[] 
) {
   lListElem *ep;
   struct read_object_args args[] = {
      {EH_Type, "exechost", read_host_work},
      {AH_Type, "adminhost", read_host_work},
      {SH_Type, "submithost", read_host_work},
   };
   struct read_object_args *argp = NULL;
   int intern_tag = 0;

   DENTER(TOP_LAYER, "cull_read_in_host");

   switch (type) {
      case EH_name:
         argp = &args[0];
         break;
      case AH_name:
         argp = &args[1];
         break;
      case SH_name:
         argp = &args[2];
         break;
      default:
         DPRINTF(("!!!!!!!!!!!!!!!cull_read_in_host: unexpected type\n"));
         DEXIT;
         return NULL;
   }

   ep = read_object(dirname, filename, spool_type, type, 0,argp, 
      tag?tag:&intern_tag, fields);

   DEXIT;
   return ep;
}


/* ------------------------------------------------------------

   how may be:

   returns tmpfile name in case of how == TMPFILE

   spool:
      1 write for spooling
      0 write only user controlled fields

   how: 
      0 use stdout
      1 write into tmpfile
      2 write into spoolfile

   file:
      filename

*/
char *write_host(
int spool,
int how,
const lListElem *ep,
int nm,
char *file 
) {
   FILE *fp = NULL;
   char *dir = NULL;
   lListElem *sep = NULL;
   char real_filename[SGE_PATH_MAX], filename[SGE_PATH_MAX];
   int ret;
   dstring ds;
   char buffer[256];

   DENTER(TOP_LAYER, "write_host");

   sge_dstring_init(&ds, buffer, sizeof(buffer));
   switch (how) {
   case 0:
      fp = stdout;
      break;
   case 1:
   case 2:
      if (how==1) {
         DPRINTF(("writing to tmpfile\n"));
         if (!sge_tmpnam(filename)) {
            CRITICAL((SGE_EVENT, MSG_TMPNAM_GENERATINGTMPNAM));
            DEXIT;
            return NULL;
         }
      }
      else {
         switch (nm) {
         case EH_name:
            dir = EXECHOST_DIR;
            break;
         case SH_name:
            dir = SUBMITHOST_DIR;
            break;
         case AH_name:
            dir = ADMINHOST_DIR;
            break;
         default:
            DEXIT;
            return NULL;
         }
         sprintf(filename, "%s/.%s", dir, lGetHost(ep, nm));
         sprintf(real_filename, "%s/%s", dir, lGetHost(ep, nm));
         DPRINTF(("writing to %s\n", filename));
      }

      fp = fopen(filename, "w");
      if (!fp) {
         CRITICAL((SGE_EVENT, MSG_ERRORWRITINGFILE_SS, filename, 
            strerror(errno)));
         DEXIT;
         return NULL;
      }
      break;
   default:
      DEXIT;
      return NULL;
   }

   if (spool && sge_spoolmsg_write(fp, COMMENT_CHAR,
             feature_get_product_name(FS_VERSION, &ds)) < 0) {    
      goto FPRINTF_ERROR;
   }

   /* - -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - */
   /* print values controlled by SGE */
   if (lGetHost(ep, nm) != NULL) {
      FPRINTF((fp, "hostname                   %s\n", lGetHost(ep, nm)));
   } else {
      FPRINTF((fp, "hostname                   %s\n", "(null)"));
   }

#if 0
      CRITICAL((SGE_EVENT, MSG_FILE_ERRORWRITINGHOSTNAME));
      DEXIT;
      return NULL;
   }
#endif
   
   /* - -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - */
   if (nm==EH_name) {
      FPRINTF((fp, "load_scaling               "));
      sep = lFirst(lGetList(ep, EH_scaling_list));
      if (sep) {
         do {
            FPRINTF((fp, "%s=%.10g", lGetString(sep, HS_name), 
               lGetDouble(sep, HS_value)));
            sep = lNext(sep);
            if (sep) {
               FPRINTF((fp, ","));
            }
         } while (sep);
         FPRINTF((fp, "\n"));
      } else {
         FPRINTF((fp, "NONE\n"));
      }

      fprint_thresholds(fp, "complex_values             ", 
         lGetList(ep, EH_consumable_config_list), 1);
      if (getenv("MORE_INFO"))
         fprint_resource_utilizations(fp, "complex_values_actual ", 
            lGetList(ep, EH_resource_utilization), 1);

      if ((!spool && how==0) || spool || (how==3)) {
         int printed = 0;

         /* 
         ** print load values in readable form
         */
         FPRINTF((fp, "load_values                "));
         for_each(sep, lGetList(ep, EH_load_list)) {
            if ( (!spool && how==0) 
                  || sge_is_static_load_value(lGetString(sep, HL_name))) {
               if (printed) {
#if 0
                  if (how == 0) 
                     FPRINTF((fp, ",\n                           "));
                  else 
#endif
                     FPRINTF((fp, ","));
               }
               FPRINTF((fp, "%s=%s", lGetString(sep, HL_name), 
                  lGetString(sep, HL_value)));
               printed++;
            }
         }
         if (!printed) {
            FPRINTF((fp, "NONE"));
         }
         FPRINTF((fp, "\n"));

         /*
         ** print license data, which now consist of no more than 
         ** the number of processors
         */
         FPRINTF((fp, "processors                 %d\n", 
            (int) lGetUlong(ep, EH_processors)));
      }

      if (spool) {
         int printed = 0;

         /* reschedule unknown list */
         FPRINTF((fp, "reschedule_unknown_list    "));
         for_each(sep, lGetList(ep, EH_reschedule_unknown_list)) {
            if (printed) {
               FPRINTF((fp, ","));
            } 
            FPRINTF((fp, u32" "u32"="u32, lGetUlong(sep, RU_job_number),
               lGetUlong(sep, RU_task_number), lGetUlong(sep, RU_state)));
            printed=1;
         }
         if (!printed) {
            FPRINTF((fp, "NONE"));
         }
         FPRINTF((fp, "\n"));
      }

      ret = fprint_cull_list(fp,  "user_lists                 ", 
         lGetList(ep, EH_acl), US_name);
      if (ret == -1) {
         goto FPRINTF_ERROR;
      } 
      ret = fprint_cull_list(fp,  "xuser_lists                ", 
         lGetList(ep, EH_xacl), US_name);
      if (ret == -1) {
         goto FPRINTF_ERROR;
      } 
      {
         int print_elements[] = { HS_name, HS_value, 0 };
         const char *delis[] = {"=", ",", NULL};

         ret = fprint_cull_list(fp,  "projects                   ", 
            lGetList(ep, EH_prj), UP_name);
         if (ret == -1) {
            goto FPRINTF_ERROR;
         } 
         ret = fprint_cull_list(fp,  "xprojects                  ", 
            lGetList(ep, EH_xprj), UP_name);
         if (ret == -1) {
            goto FPRINTF_ERROR;
         } 
         /* print scaling list */
         FPRINTF((fp, "usage_scaling              "));
         ret = uni_print_list(fp, NULL, 0, lGetList(ep, EH_usage_scaling_list), 
                        print_elements, delis, 0);
         if (ret < 0) {
            goto FPRINTF_ERROR;
         }
         FPRINTF((fp, "\n"));

         /* print reporting variable list */
         ret = fprint_cull_list(fp,  "report_variables           ", 
            lGetList(ep, EH_report_variables), STU_name);
         if (ret == -1) {
            goto FPRINTF_ERROR;
         } 
      }
   }    /* only exec host */

   if (how != 0) {
      fclose(fp);
   }

   if (how == 2) {
      if (rename(filename, real_filename) == -1) {
         DEXIT;
         return NULL;
      } else {
         strcpy(filename, real_filename);
      }
   }       

   DEXIT;
   /* strdup in case of tmpfile writing */
   return (how==1)?sge_strdup(NULL, filename):filename; 

FPRINTF_ERROR:
   DEXIT;
   return NULL; 
}
