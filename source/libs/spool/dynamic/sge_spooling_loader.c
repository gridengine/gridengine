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

#ifdef SPOOLING_classic
#include "sge_spooling_classic.h"
#endif
#ifdef SPOOLING_classic
#include "sge_spooling_flatfile.h"
#endif

#include "msg_spoollib.h"

#include "sge_spooling.h"

lListElem *
spool_create_dynamic_context(const char *shlib_name, const char *args)
{
#ifdef SPOOLING_dynamic
   void *shlib_handle;
#endif

   const char *spooling_name;
   spooling_create_context_func create_context;

   lListElem *context;

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
      dstring create_context_func_name = DSTRING_INIT;

      shlib_handle = dlopen(shlib_name, RTLD_NOW);
      if (shlib_handle == NULL) {
         ERROR((SGE_EVENT, "error opening shared lib "SFQ": "SFN"\n", 
                shlib_name, dlerror()));
         DEXIT;
         return NULL;
      }

      get_spooling_method = (spooling_get_method_func)
                            dlsym(shlib_handle, "get_spooling_method");
      if (get_spooling_method == NULL) {
         ERROR((SGE_EVENT, SFQ" does not contain a valid Grid Engine spooling method: "SFN"\n", 
                shlib_name, dlerror()));
         dlclose(shlib_handle);
         DEXIT;
         return NULL;
      }

      spooling_name = get_spooling_method();

      INFO((SGE_EVENT, "loading spooling method "SFQ" from "SFQ"\n", spooling_name, shlib_name));

      sge_dstring_sprintf(&create_context_func_name, "spool_%s_create_context", 
                          spooling_name);

      create_context = (spooling_create_context_func)
                       dlsym(shlib_handle, 
                             sge_dstring_get_string(&create_context_func_name));
      if (create_context == NULL) {
         ERROR((SGE_EVENT, SFQ" does not contain a valid Grid Engine spooling method: "SFN"\n", 
                shlib_name, dlerror()));
         dlclose(shlib_handle);
         DEXIT;
         return NULL;
      }
   }
#endif

   context = create_context(args);
   if (context == NULL) {
      ERROR((SGE_EVENT, "error creating a "SFQ" spooling context\n", 
             spooling_name));
#ifdef SPOOLING_dynamic
      dlclose(shlib_handle);
#endif
      DEXIT;
      return NULL;
   }

   DEXIT;
   return context;
}


