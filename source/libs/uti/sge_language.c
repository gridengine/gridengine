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

#ifdef __SGE_COMPILE_WITH_GETTEXT__ 

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

static language_functions_struct sge_language_functions;

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
******************************************************************************/
int sge_init_languagefunc(char *package, char *localeDir) 
{
  char* packName = NULL;
  char* locDir   = NULL;
  char* language = NULL;
  char* pathName = NULL;
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
*     uti/language/sge_gettext()
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

/****** uti/language/sge_gettext() ********************************************
*  NAME
*     sge_gettext() -- get translated message from message file
*
*  SYNOPSIS
*     char *sge_gettext(char *x)
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
*******************************************************************************/
const char *sge_gettext(char *x) 
{
   char *z;
   DENTER(GDI_LAYER, "sge_gettext");

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
