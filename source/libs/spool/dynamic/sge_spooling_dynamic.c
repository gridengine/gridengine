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

#ifdef LINUX
#define __USE_GNU
#endif

#include <dlfcn.h>
#include <string.h>

#ifdef LINUX
#undef __USE_GNU
#endif

#ifdef SOLARIS
#include <link.h>
#endif

#include "rmon/sgermon.h"
#include "uti/sge_log.h"
#include "sgeobj/sge_answer.h"

#include "spool/dynamic/msg_spoollib_dynamic.h"

#include "spool/dynamic/sge_spooling_dynamic.h"

static const char *spooling_method = "dynamic";

const char *get_spooling_method(void)
{
   return spooling_method;
}

lListElem *
spool_dynamic_create_context(lList **answer_list, const char *method,
                             const char *shlib_name, const char *args)
{
   bool ok = true;
   lListElem *context = NULL;

   /* shared lib name buffer and handle */
   char shlib_buffer[MAX_STRING_SIZE];
   dstring shlib_dstring;
   const char *shlib_fullname;
   void *shlib_handle;

   /* get_method function pointer and result */
   spooling_get_method_func get_spooling_method = NULL;
   const char *spooling_name = NULL;

   DENTER(TOP_LAYER, "spool_dynamic_create_context");

   /* build the full name of the shared lib - append architecture dependent
    * shlib postfix 
    */
   sge_dstring_init(&shlib_dstring, shlib_buffer, sizeof(shlib_buffer));
   shlib_fullname = sge_dstring_sprintf(&shlib_dstring, "%s.%s", shlib_name, 
#if defined(HP11) || defined(HP1164)
                                        "sl"
#elif defined(DARWIN)
                                        "dylib"
#else
                                        "so"
#endif
                                       );
   /* open the shared lib */
   shlib_handle = dlopen(shlib_fullname, RTLD_NOW 
#if defined(DARWIN)
                         | RTLD_GLOBAL
#endif
                        );
   if (shlib_handle == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_SPOOL_ERROROPENINGSHAREDLIB_SS, 
                              shlib_fullname, dlerror());
      ok = false;
   } 

   /* retrieve function pointer of get_method function in shared lib */
   if (ok) {
      get_spooling_method = (spooling_get_method_func)
                            dlsym(shlib_handle, "get_spooling_method");
      if (get_spooling_method == NULL) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_SPOOL_SHLIBDOESNOTCONTAINSPOOLING_SS, 
                                 shlib_fullname, dlerror());
         ok = false;
      }
   }

   /* retrieve name of spooling method in shared lib */
   if (ok) {
      spooling_name = get_spooling_method();

      if (spooling_name == NULL) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_INFO, 
                                 MSG_SPOOL_SHLIBGETMETHODRETURNSNULL_S, 
                                 shlib_fullname);
         ok = false;
      } else {
         if (strcmp(spooling_name, method) != 0) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_INFO, 
                                    MSG_SPOOL_SHLIBCONTAINSXWENEEDY_SSS, 
                                    shlib_fullname, spooling_name, method);
            ok = false;
         }
      }
   }

   /* create spooling context from shared lib */
   if (ok) {
      char buffer[MAX_STRING_SIZE];
      dstring create_context_func_name;
      spooling_create_context_func create_context;

      sge_dstring_init(&create_context_func_name, buffer, 
                       MAX_STRING_SIZE);
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_INFO, 
                              MSG_SPOOL_LOADINGSPOOLINGMETHOD_SS, 
                              spooling_name, shlib_fullname);

      sge_dstring_sprintf(&create_context_func_name, 
                          "spool_%s_create_context", spooling_name);

      create_context = 
            (spooling_create_context_func) 
            dlsym(shlib_handle, 
                  sge_dstring_get_string(&create_context_func_name));
      if (create_context == NULL) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_SPOOL_SHLIBDOESNOTCONTAINSPOOLING_SS,
                                 shlib_fullname, dlerror());
         ok = false;
      } else {
         context = create_context(answer_list, args);
      }
   }

   /* cleanup in case of initialization error */
   if (context == NULL) {
      if (shlib_handle != NULL) {
         dlclose(shlib_handle);
      }
   }

   DEXIT;
   return context;
}

