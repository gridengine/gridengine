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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "basis_types.h"
#include "sge_string.h"
#include "err_trace.h"
#include "config_file.h"
#include "msg_daemons_common.h"


/* handle the config file
   This file is used to transfer parameters to the shepherd */
typedef struct config_entry {
   char *name;
   char *value;
   struct config_entry *next;
} config_entry;

static config_entry *config_list = NULL;

/* these variables may get used to replace variables in pe_start */
char *pe_variables[] = {
   "pe_hostfile",
   "host",
   "job_owner",
   "job_id",
   "job_name",
   "pe",
   "pe_slots",
   "processors",
   "queue",
   NULL
};

/* these variables may get used to replace variables in prolog/epilog */
char *prolog_epilog_variables[] = {
   "host",
   "job_owner",
   "job_id",
   "job_name",
   "processors",
   "queue",
   NULL
};


/* these variables may get used to replace variables in the allocation rule of a pe */
char *pe_alloc_rule_variables[] = {
   "pe_slots",
   "fill_up",
   "round_robin",
   NULL
};

char *ckpt_variables[] = {
   "host",
   "job_owner",
   "job_id",
   "job_name",
   "queue",
   "job_pid",
   "ckpt_dir",
   "ckpt_signal",
   NULL
};

char *ctrl_method_variables[] = {
   "host",
   "job_owner",
   "job_id",
   "job_name",
   "queue",
   "job_pid",
   NULL
};


static config_entry *find_conf_entry(char *name, config_entry *ptr);

void (*config_errfunc)(char *) = NULL;

/*****************************************************
 read configuration file to memory.
 returns:
 - 0 Ok
 - 1 systemerror -> errno is valid
 - 2 malloc error
 *****************************************************/
int read_config(
char *fname 
) {
   FILE *fp;
   char buf[100000], *name, *value;

   delete_config();

   fp = fopen(fname, "r");
   if (!fp)
      return 1;

   while (fgets(buf, sizeof(buf), fp)) {
      name = strtok(buf, " =");
      if (!name) {
         break;
      }   
      value = strtok(NULL, "\n");

      if (add_config_entry(name, value))
         return 2;
   }
   fclose(fp);

   return 0;
}

/******************************************************/
int add_config_entry(name, value)
char *name, *value;
{
   config_entry *new;

   if ((new = (config_entry *)malloc(sizeof(config_entry))) == NULL) {
      return 1;
   }
   
   if((new->name = strdup(name)) == NULL) {
      free(new);
      return 1;
   }
  
   if(value != NULL) {
      if((new->value = strdup(value)) == NULL) {
         free(new->name);
         free(new);
         return 1;
      }
   } else {
      new->value = NULL;
   }
  
   new->next = config_list;
   config_list = new;

   return 0;
}

/****************************************************/

static config_entry *find_conf_entry(
char *name,
config_entry *ptr 
) {
   while (ptr) {
      if (!strcmp(ptr->name, name))
         return ptr;

      ptr = ptr->next;
   }
   return NULL;
}

/***************************************************/
char *get_conf_val(
char *name 
) {
   config_entry *ptr = config_list;
   char err_str[10000];
   
   ptr = find_conf_entry(name, config_list);
   if (ptr)
      return ptr->value;
   
   sprintf(err_str, MSG_CONF_NOCONFVALUE_S,
           name);
   if (config_errfunc)
      config_errfunc(err_str);
   return NULL;
}

/***************************************************/
char *search_conf_val(
char *name 
) {
   config_entry *ptr = config_list;
   
   ptr = find_conf_entry(name, config_list);
   if (ptr)
      return ptr->value;
   else
      return NULL;
}

/**************************************************
   return NULL if conf value does not exist or
   if "none" is it's value
*/
char *search_nonone_conf_val(
char *name 
) {
   char *s;

   s = search_conf_val(name);
   if (s && !strcasecmp("none", s))
      return NULL;
   else 
      return s;
}

/**************************************************/
void delete_config()
{
   config_entry *next;

   while (config_list) {
      next = config_list->next;
      if (config_list->name)
         free(config_list->name);
      if (config_list->value)
         free(config_list->value);
      free(config_list);
      config_list = next;
   }
}


/**************************************************

   replace_params()

DESCRIPTION 

   can be used to 
      check if all variables contained in src are allowed
   or to 
      build a new string in dst by replacing allowed variables
      from config_list 

   a variable starts with a "$" sign 
   the name contains of chars from isalnum() or "_" 

   the allowed variable names are given using a string array 
   that contains accessable variables or all variable from 
   config list are allowed if NULL is passed

ERROR

   0  no error    

   there are two classes of errors:
      -1 a lack of an __allowed__ variable in the config_list   - execd damaged
      1  an syntax error or a request of a not allowed variable - pe damaged
  
REM
   
   may be the config_list should be passed as an argument

*/
int replace_params(
const char *src,
char *dst,
int dst_len,
char **allowed  
) {
   char err_str[4096];
   char name[256];
   int name_len;
   const char *sp;
   char *dp;
   char **spp, *value = NULL;
   int just_check = 0;

   /* does caller just want to validate */
   if (!dst) {
      just_check = 1;
   }
   dp=dst;

   sp=src; 
   while (*sp) {
      switch (*sp) {
      case '$':

         /* eat '$' */
         sp++;

         /* get variable length */
         name_len = 0;
         /* here get variable names defined */
         while (isalnum((int) sp[name_len]) || sp[name_len]=='_') { 
            name_len++;
         }

         if (name_len==0) {
            sprintf(err_str, MSG_CONF_ATLEASTONECHAR);
            if (config_errfunc)
               config_errfunc(err_str);
            return 1;
         }

         if (name_len>(sizeof(name)-1)) {
            sprintf(err_str, MSG_CONF_REFVAR_S, sp);
            if (config_errfunc)
               config_errfunc(err_str);
            return 1;
         }

         /* copy variable name in local buffer */
         strncpy(name, sp, name_len);
         name[name_len] = '\0';

         /* eat variable name */
         sp += name_len;

         /* check if this variable is allowed */
         if (allowed) {
            for (spp=allowed; *spp && strcmp(*spp, name); spp++)
               ;
               
            if (!*spp) {
               /* this variable may be known by us but not by the user */
               sprintf(err_str, MSG_CONF_UNKNOWNVAR_S, name);
               if (config_errfunc)
                  config_errfunc(err_str);
               return 1;
            } 
         }

         /* check if this variable is in the config_entry list */
         if (!just_check) {
            value=get_conf_val(name);
            if (!value) { 
               /* error func is called by get_conf_val() */
               return -1;/* Arghh! we have promised to serve such a variable */
            }
         }

         /* copy value into dst buffer */
         if (!just_check) {
            while (*value)
               *dp++ = *value++;
         }

         break;

      default:
         if (!just_check) 
            *dp++ = *sp; 
         sp++;
         break;
      }
   }

   if (!just_check) 
      *dp = '\0';

   return 0;
}
