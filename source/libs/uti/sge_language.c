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
#include <fcntl.h>
#include <unistd.h>

#include "basis_types.h"
#include "sge_language.h"
#include "sgermon.h"
#include "sge_prog.h"
#include "sge_htable.h"


#ifdef __SGE_COMPILE_WITH_GETTEXT__ 

#define SGE_ENABLE_MSG_ID "SGE_ENABLE_MSG_ID"
#define PACKAGE "GRIDPACKAGE"
#define LOCALEDIR "GRIDLOCALEDIR"
#define SGE_DEFAULT_PACKAGE      "gridengine"
#define SGE_DEFAULT_LOCALEDIR    "locale"


/* this is to define the functions for the internationalization
   tools */
typedef struct {
    gettext_func_type          gettext_func        ;
    setlocale_func_type        setlocale_func      ;
    bindtextdomain_func_type   bindtextdomain_func ;
    textdomain_func_type       textdomain_func     ;
} language_functions_struct;

typedef struct {
   int id;                    /* message identification number */
   int category;              /* message category:             0 = common */
   u_long32 counter;          /* number of calls */
   const char* message;       /* message string */
   const char* local_message;  /* local translated message */
} sge_error_message_t;



static language_functions_struct sge_language_functions;
static int sge_message_id_view_flag = 0;
static int sge_enable_msg_id = 0;    /* used to enable/disable message ids */
static htable sge_message_hash_table = NULL; 


/* this is to find out if the sge_init_language_func() was called */
static int sge_are_language_functions_installed = FALSE;


/****** uti/language/sge_init_language() **************************************
*  NAME
*     sge_init_language() -- initialize language package for gettext() 
*
*  SYNOPSIS
*     int sge_init_language(char* package, char* localeDir)
*       
*
*  FUNCTION
*     starts up the language initialization for gettext(). This 
*     function should be called nearly after the main() function.
*  
*     sge_init_language_func() must be called first to install the 
*     correct function pointers for gettext() setlocale() etc. etc. 
*
*  INPUTS
*     char* package     -  package name like "gridengine" of binary 
*                          package *.mo file. 
*                          (if package is NULL sge_init_language tries 
*                          to get the package name from the invironment 
*                          variable "GRIDPACKAGE"). 
*     char* localeDir  -   path to the localisazion directory 
*                          (if localeDir is NULL sge_init_language 
*                          tries to get the localization directory path 
*                          from the invironment variable 
*                          "GRIDLOCALEDIR"). 
*
*  RESULT
*     int state         -  TRUE for seccess, FALSE on error.
*
*  SEE ALSO
*     uti/language/sge_init_language()
*     uti/language/sge_init_language_func()
*     uti/language/sge_gettext()
*     uti/language/sge_gettext_()
*     uti/language/sge_gettext__()
*     uti/language/sge_get_message_id_output()
*     uti/language/sge_set_message_id_output()
******************************************************************************/
int sge_init_languagefunc(char *package, char *localeDir) 
{
  char* packName = NULL;
  char* locDir   = NULL;
  char* language = NULL;
  char* pathName = NULL;
  char* sge_enable_msg_id_string = NULL;
  int   success  = FALSE;  
  int   back;

  DENTER(BASIS_LAYER, "sge_init_language");

  DPRINTF(("****** starting localisation procedure ... **********\n"));

  /* try to get the package name */
  if (package != NULL) {
     packName = strdup(package);
  } else if (getenv(PACKAGE) != NULL) {
     packName = strdup(getenv(PACKAGE));
     DPRINTF(("try to get language package name from environment "SFQ"\n",PACKAGE));
  } else {
     DPRINTF(("could not get environment variable "SFQ"\n", PACKAGE));
  }

  /* no package name given, using default one */
  if (packName == NULL) {
     packName = strdup(SGE_DEFAULT_PACKAGE);
  }
   
  /* try to get the localization directory */ 
  if (localeDir != NULL) {
     locDir = strdup(localeDir);
  } else if (getenv(LOCALEDIR) != NULL) {
     locDir = strdup(getenv(LOCALEDIR));
     DPRINTF(("try to get language package directory path from environment "SFQ"\n",LOCALEDIR));
  } else {
     DPRINTF(("could not get environment variable "SFQ"\n", LOCALEDIR));
  }

  /* no directory given, using default one */
  if (locDir == NULL) {
     const char *sge_root = sge_get_root_dir(0);
     char* root = NULL;
     
     if (sge_root != NULL) {
         root = strdup(sge_root);
     } 
     
     if ( root == NULL ) {
         locDir = strdup("/usr/local/share/nls/src");
     } else {
         locDir = malloc(sizeof(char)*(strlen(root)+strlen(SGE_DEFAULT_LOCALEDIR)+100));
         sprintf(locDir,"%s/%s",root,SGE_DEFAULT_LOCALEDIR);
     }
     free(root);
     root = NULL;
  }

  /* try to get a language stylename (only for output of package path) */

  if (getenv("LANGUAGE") != NULL) {
     language = strdup(getenv("LANGUAGE"));
  } else {
     if (getenv("LANG") != NULL) {
       language = strdup(getenv("LANG"));
     }
  }

  if (language == NULL) {
     DPRINTF(("environment LANGUAGE or LANG is not set; no language selected - using defaults\n"));
     language = strdup("C");
  } 
  

  /* packName, locDir and language strings are now surely not NULL,
     so we can now try to setup the choosen language package (*.mo - file) */
  pathName = malloc(sizeof(char)*(strlen(locDir)+strlen(language)+strlen(packName)+100));
  sprintf(pathName,"%s/%s/LC_MESSAGES/%s.mo",locDir,language,packName);
  DPRINTF(("locale directory: >%s<\n",locDir));
  DPRINTF(("package file:     >%s.mo<\n",packName));
  DPRINTF(("language (LANG):  >%s<\n",language));
  DPRINTF(("loading message file: %s\n",pathName ));
  

  /* is the package file allright */
  back = open (pathName, O_RDONLY);
  if (back >= 0) {
     success = TRUE;
     close(back);
  } else {
     success = FALSE;
  }
  
  /* now startup the language package for gettext */
/*   setlocale(LC_ALL, ""); */

  if ( (sge_language_functions.setlocale_func != NULL     ) &&
       (sge_language_functions.bindtextdomain_func != NULL) &&
       (sge_language_functions.textdomain_func != NULL    ) &&
       (sge_are_language_functions_installed == TRUE      )    ) {
     sge_language_functions.setlocale_func(LC_MESSAGES, "");
     sge_language_functions.bindtextdomain_func(packName, locDir );
     sge_language_functions.textdomain_func(packName);
  } else {
     DPRINTF(("sge_init_language() called without valid sge_language_functions pointer!\n"));
     success = FALSE;
  }
 
  free(packName);
  free(locDir);
  free(language);
  free(pathName);
  packName = NULL;
  locDir   = NULL;
  language = NULL; 
  pathName = NULL;

  if (success == TRUE) {
     if ( (strcmp(language, "en") == 0) ) {
        sge_enable_msg_id=0;
     } else {
        sge_enable_msg_id=1;
     }
  } 

  sge_enable_msg_id_string = getenv(SGE_ENABLE_MSG_ID);
  if (sge_enable_msg_id_string != NULL) {
     int env_value = 0;
     DPRINTF(("SGE_ENABLE_MSG_ID is set to \"%s\"\n",sge_enable_msg_id_string));  
     env_value = atoi(sge_enable_msg_id_string);
     if (env_value == 0) {
         sge_enable_msg_id = 0;
     } else {
         sge_enable_msg_id = 1;
     }
  }


  if (sge_enable_msg_id == 0) {
     DPRINTF(("error id output     : disabled\n"));
  } else {
     DPRINTF(("error id output     : enabled\n"));
  }

  if (success == TRUE) {
    DPRINTF(("****** starting localisation procedure ... success **\n"));
  } else {
    DPRINTF(("****** starting localisation procedure ... failed  **\n"));
  }

  DEXIT;
  return (success);
}

/****** uti/language/sge_init_language_func() **********************************
*  NAME
*     sge_init_language() -- install language functions
*
*  SYNOPSIS
*     void sge_init_language_func(gettext_func_type new_gettext, 
*                         setlocale_func_type new_setlocale, 
*                         bindtextdomain_func_type new_bindtextdomain, 
*                         textdomain_func_type new_textdomain);
*
*  FUNCTION
*     set the function pointer for the gettext(), setlocale(), 
*     bindtextdomain() and textdomain() function calls. This function 
*     must called before any call to sge_init_language() and 
*     sge_gettext().  
*
*  INPUTS
*     gettext_func_type        - pointer for gettext()
*     setlocale_func_type      - pointer for setlocale()
*     bindtextdomain_func_type - pointer for bindtextdomain()
*     textdomain_func_type     - pointer for textdomain()
*
*  SEE ALSO
*     uti/language/sge_init_language()
*     uti/language/sge_init_language_func()
*     uti/language/sge_gettext()
*     uti/language/sge_gettext_()
*     uti/language/sge_gettext__()
*     uti/language/sge_get_message_id_output()
*     uti/language/sge_set_message_id_output()
******************************************************************************/
void sge_init_language_func(gettext_func_type new_gettext, 
                            setlocale_func_type new_setlocale,
                            bindtextdomain_func_type new_bindtextdomain,
                            textdomain_func_type new_textdomain) 
{
   /* initialize the functions pointer to NULL */
   sge_language_functions.gettext_func = NULL;
   sge_language_functions.setlocale_func = NULL;
   sge_language_functions.bindtextdomain_func = NULL;
   sge_language_functions.textdomain_func = NULL;

   /* ok, the functions have now NULL pointer */
   sge_are_language_functions_installed = TRUE; 

   /* set the new functions */
   if (new_gettext != NULL) {
      sge_language_functions.gettext_func = new_gettext;
   }
 
   if (new_setlocale != NULL) {
      sge_language_functions.setlocale_func = new_setlocale;
   }

   if (new_bindtextdomain != NULL) {
      sge_language_functions.bindtextdomain_func = new_bindtextdomain;
   }

   if (new_textdomain != NULL) {
      sge_language_functions.textdomain_func = new_textdomain;
   } 
}

/****** uti/language/sge_set_message_id_output() *******************************
*  NAME
*     sge_set_message_id_output() -- enable message id number adding
*
*  SYNOPSIS
*     void sge_set_message_id_output(int flag) 
*
*  FUNCTION
*     This procedure is used to enable the adding of message id's when 
*     showing error messages. This function is used in the macro 
*     SGE_ADD_MSG_ID(x) to enable the message id for errors.
*
*  INPUTS
*     int flag - 0 = off ; 1 = on
*
*  SEE ALSO
*     uti/language/sge_init_language()
*     uti/language/sge_init_language_func()
*     uti/language/sge_gettext()
*     uti/language/sge_gettext_()
*     uti/language/sge_gettext__()
*     uti/language/sge_get_message_id_output()
*     uti/language/sge_set_message_id_output()
*******************************************************************************/
void sge_set_message_id_output(int flag) {
   if (flag == 0) {
      sge_message_id_view_flag = 0;
   } else {
      sge_message_id_view_flag = 1;
   }
}
/****** uti/language/sge_get_message_id_output() *******************************
*  NAME
*     sge_get_message_id_output() -- check if message id should be added
*
*  SYNOPSIS
*     int sge_get_message_id_output(void) 
*
*  FUNCTION
*     This function returns the value stored in the static global 
*     variable sge_message_id_view_flag.
*
*  RESULT
*     int - value of sge_message_id_view_flag
*
*  SEE ALSO
*     uti/language/sge_init_language()
*     uti/language/sge_init_language_func()
*     uti/language/sge_gettext()
*     uti/language/sge_gettext_()
*     uti/language/sge_gettext__()
*     uti/language/sge_get_message_id_output()
*     uti/language/sge_set_message_id_output()
*******************************************************************************/
int sge_get_message_id_output(void) {
   if (sge_enable_msg_id == 0) {
      return 0;
   } 
   return sge_message_id_view_flag;
}

/****** uti/language/sge_gettext() *********************************************
*  NAME
*     sge_gettext() -- dummy gettext() function 
*
*  SYNOPSIS
*     const char* sge_gettext(char *x) 
*
*  FUNCTION
*     This function returns the given argument
*
*  INPUTS
*     char *x - string
*
*  RESULT
*     const char* - input string
*
*  SEE ALSO
*     uti/language/sge_init_language()
*     uti/language/sge_init_language_func()
*     uti/language/sge_gettext()
*     uti/language/sge_gettext_()
*     uti/language/sge_gettext__()
*     uti/language/sge_get_message_id_output()
*     uti/language/sge_set_message_id_output()
*******************************************************************************/
const char *sge_gettext(char *x) {
   return x;
}

/****** uti/language/sge_gettext_() ********************************************
*  NAME
*     sge_gettext_() -- add error id to message
*
*  SYNOPSIS
*     const char* sge_gettext_(int msg_id, const char *msg_str) 
*
*  FUNCTION
*     This function is used for adding the message id to the translated 
*     gettext message string. The message id is only added when the 
*     function sge_get_message_id_output() returns not "0" and the 
*     message string contains at least one SPACE character.
*
*  INPUTS
*     int msg_id          - message id
*     const char *msg_str - message to translate
*
*  RESULT
*     const char* - translated (L10N) message with message id 
*
*  SEE ALSO
*     uti/language/sge_init_language()
*     uti/language/sge_init_language_func()
*     uti/language/sge_gettext()
*     uti/language/sge_gettext_()
*     uti/language/sge_gettext__()
*     uti/language/sge_get_message_id_output()
*     uti/language/sge_set_message_id_output()
*******************************************************************************/
const char *sge_gettext_(int msg_id, const char *msg_str) 
{
#ifndef __SGE_COMPILE_WITH_GETTEXT__
   return msg_str;
#else
   sge_error_message_t* message_p = NULL;
   long key;

   DENTER(TOP_LAYER, "sge_gettext_");

   if (msg_str == NULL) {
      DEXIT;
      return NULL;
   } 

   key = msg_id;
   /* check if message is not just one word (strstr) */
   if ( (sge_get_message_id_output() != 0)  && 
        (strstr(msg_str, " ") != NULL)   ) {  
       
      if (sge_message_hash_table == NULL) {
         sge_message_hash_table = sge_htable_create(8,   /* 2^8 = 256 table start size */
                                                    dup_func_long, 
                                                    hash_func_long, 
                                                    hash_compare_long);
      }
      if (sge_htable_lookup(sge_message_hash_table, &key, (const void**)&message_p) == False) {
         /* add element */ 
         sge_error_message_t* new_mp = NULL;
         char* org_message = NULL;
         char* trans_message = NULL;
         const char* gettext_return_string = NULL;

         
         gettext_return_string = sge_gettext__((char*)msg_str);

         org_message   = malloc(strlen(msg_str)+1);
         trans_message = malloc(strlen(gettext_return_string)+1+8); /* max "(99999) "*/
         new_mp        = malloc(sizeof(sge_error_message_t));
         if (new_mp != NULL && org_message != NULL && trans_message != NULL) {
            DPRINTF(("add new hash table entry for message id: %d\n",msg_id));
            new_mp->id = msg_id;
            new_mp->category = 0;
            new_mp->counter = 1;
            strcpy(org_message, msg_str);
            new_mp->message = org_message;
            if (msg_id <= 99999) {
               sprintf( trans_message , "[%d] %s", msg_id, gettext_return_string);
            } else {
               sprintf( trans_message , "%s", gettext_return_string);
            }
            new_mp->local_message = trans_message;

            sge_htable_store(sge_message_hash_table, &key, new_mp);
            DEXIT;
            return new_mp->local_message;
         }
      } else {
         /* check element */
         DPRINTF(("using old hash entry for message id: %d\n",msg_id));
         if (strcmp(msg_str,message_p->message) != 0) {
            DPRINTF(("duplicate message id error: returning gettext() message"));
            DPRINTF(("msg in : \"%s\"\n",msg_str));
            DPRINTF(("msg out: \"%s\"\n",message_p->message));
            DEXIT;
            return sge_gettext__((char*)msg_str);
         } else {
            message_p->counter = (message_p->counter) + 1;
            DPRINTF(("message count: "U32CFormat"\n", u32c(message_p->counter)));
            DEXIT;
            return message_p->local_message;
         }
      } 
   }
   DEXIT;
   return sge_gettext__((char*)msg_str);
#endif
}

/****** uti/language/sge_gettext__() *******************************************
*  NAME
*     sge_gettext__() -- get translated message from message file
*
*  SYNOPSIS
*     char *sge_gettext__(char *x)
*
*  FUNCTION
*     makes a call to sge_language_functions.gettext_func(x) if 
*     gettext_func is not NULL, otherwise it returns the input
*     string. 
*
*  INPUTS
*     char *x - pointer to message which should be internationalizied    
*
*  RESULT
*     char*   - pointer internationalized message
*
*  SEE ALSO
*     uti/language/sge_init_language()
*     uti/language/sge_init_language_func()
*     uti/language/sge_gettext()
*     uti/language/sge_gettext_()
*     uti/language/sge_gettext__()
*     uti/language/sge_get_message_id_output()
*     uti/language/sge_set_message_id_output()
*******************************************************************************/
const char *sge_gettext__(char *x) 
{
   char *z;
   DENTER(GDI_LAYER, "sge_gettext__");

   if ( (sge_language_functions.gettext_func != NULL) && 
        (sge_are_language_functions_installed == TRUE)   ) {
      z = sge_language_functions.gettext_func(x);
   } else {
      z = x;
      DPRINTF(("sge_gettext() called without valid gettext function pointer!\n"));
   }

   /*z = gettext(x);*/
  
   /*DPRINTF(("gettext: '%s' -> '%s'\n", x ? x : "", z ? z : ""));*/ 

   DEXIT;
   return z;
}
  
#endif
