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

#ifdef SPOOLING_dynamic
#ifdef LINUX
#define __USE_GNU
#endif

#include <dlfcn.h>

#ifdef LINUX
#undef __USE_GNU
#endif

#ifdef SOLARIS
#include <link.h>
#endif
#endif

#include "sgermon.h"
#include "sge_log.h"

#include "sge_answer.h"

#ifdef SPOOLING_classic
#include "sge_spooling_classic.h"
#endif
#ifdef SPOOLING_flatfile
#include "sge_spooling_flatfile.h"
#endif

#include "msg_spoollib.h"
#include "msg_spoollib_dynamic.h"

#include "sge_spooling.h"

/****** spool/dynamic/spool_create_dynamic_context() *********************
*  NAME
*     spool_create_dynamic_context() -- create a spooling context
*
*  SYNOPSIS
*     lListElem * 
*     spool_create_dynamic_context(lList **answer_list, 
*                                  const char *shlib_name, const char *args) 
*
*  FUNCTION
*     Create a spooling context.
*     Which spooling context to use can either be defined at compile time, e.g. 
*     by calling "aimk -spool-classic" or "aimk -spool-flatfile".
*
*     Or dynamic loading of a spooling framework is activated (by building
*     with "aimk -spool-dynamic".
*     In this case a shared library is dynamically loaded and the spooling
*     framework implementation of that shared library is used.
*
*  INPUTS
*     lList **answer_list    - to return error messages
*     const char *shlib_name - name of a shared lib
*     const char *args       - arguments to be passed to the initialization
*                              function in the specified shared lib.
*
*  RESULT
*     lListElem * - on success a spooling context, else NULL.
*
*  SEE ALSO
*     spool/--Spooling
*******************************************************************************/
lListElem *
spool_create_dynamic_context(lList **answer_list, 
                             const char *shlib_name, const char *args)
{
#ifdef SPOOLING_dynamic
   void *shlib_handle;
#endif

   const char *spooling_name;
   spooling_create_context_func create_context;

   lListElem *context = NULL;

   bool init_ok = true;

   DENTER(TOP_LAYER, "spool_create_dynamic_context");

#ifdef SPOOLING_classic
   spooling_name = "classic";
   create_context = spool_classic_create_context;
#endif
#ifdef SPOOLING_flatfile
   spooling_name = "flatfile";
   create_context = spool_flatfile_create_context;
#endif
#ifdef SPOOLING_dynamic
   {
      spooling_get_method_func get_spooling_method;

      shlib_handle = dlopen(shlib_name, RTLD_NOW);
      if (shlib_handle == NULL) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_SPOOL_ERROROPENINGSHAREDLIB_SS, 
                                 shlib_name, dlerror());
         init_ok = false;
      } else {
         get_spooling_method = (spooling_get_method_func)
                               dlsym(shlib_handle, "get_spooling_method");
         if (get_spooling_method == NULL) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_SPOOL_SHLIBDOESNOTCONTAINSPOOLING_SS, 
                                    shlib_name, dlerror());
            init_ok = false;
         } else {
            char buffer[MAX_STRING_SIZE];
            dstring create_context_func_name;
            sge_dstring_init(&create_context_func_name, buffer, 
                             MAX_STRING_SIZE);
            spooling_name = get_spooling_method();

            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_INFO, 
                                    MSG_SPOOL_LOADINGSPOOLINGMETHOD_SS, 
                                    spooling_name, shlib_name);

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
                                       shlib_name, dlerror());
               init_ok = false;
            }
         }
      }
   }

#endif

   if (init_ok) {
      context = create_context(answer_list, args);
      if (context == NULL) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_SPOOL_ERRORCREATINGCONTEXT_S, 
                                 spooling_name);
      }
   }

   /* cleanup in case of initialization error */
   if (context == NULL) {
#ifdef SPOOLING_dynamic
      if (shlib_handle != NULL) {
         dlclose(shlib_handle);
      }
#endif
   }

   DEXIT;
   return context;
}


