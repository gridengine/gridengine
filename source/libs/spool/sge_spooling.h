#ifndef __SGE_SPOOLING_H 
#define __SGE_SPOOLING_H 
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

#include "sge_mirror.h"
#include "sge_spoolingL.h"

/* function prototypes for spooling functions */
typedef int (*spooling_startup_func)(const lListElem *rule); 
typedef int (*spooling_shutdown_func)(const lListElem *rule); 

typedef int (*spooling_list_func)(const lListElem *type, const lListElem *rule, 
                                  lList **list, const sge_event_type event_type);
                                  
typedef int (*spooling_write_func)(const lListElem *type, const lListElem *rule, 
                                   const lListElem *object, const char *key, 
                                   const sge_event_type event_type);

typedef lListElem *(*spooling_read_func)(const lListElem *type, const lListElem *rule, 
                                         const char *key, const sge_event_type event_type);

typedef int (*spooling_delete_func)(const lListElem *type, const lListElem *rule, 
                                    const char *key, const sge_event_type event_type);

/* the default spooling context */
extern lListElem *Default_Spool_Context;

/* creation and maintenance of the spooling context */
lListElem *spool_create_context(const char *name);
lListElem *spool_free_context(lListElem *context);

void spool_set_default_context(lListElem *context);
lListElem *spool_get_default_context(void);

lListElem *spool_context_search_rule(const lListElem *context, const char *name);
lListElem *spool_context_create_rule(lListElem *context, 
                                     const char *name, const char *url,
                                     spooling_startup_func startup_func, 
                                     spooling_shutdown_func shutdown_func, 
                                     spooling_list_func list_func, 
                                     spooling_read_func read_func, 
                                     spooling_write_func write_func, 
                                     spooling_delete_func delete_func);

lListElem *spool_context_search_type(const lListElem *context, const sge_event_type event_type);
lListElem *spool_context_create_type(lListElem *context, const sge_event_type event_type);

lListElem *spool_type_search_default_rule(const lListElem *spool_type);
lListElem *spool_type_add_rule(lListElem *spool_type, const lListElem *rule, lBool is_default);

/* startup and shutdown */
int spool_startup_context(lListElem *context);
int spool_shutdown_context(lListElem *context);

/* reading */
int spool_read_list(const lListElem *context, lList **list, const sge_event_type event_type);
lListElem *spool_read_object(const lListElem *context, const sge_event_type event_type, const char *key);

/* writing */
int spool_write_object(const lListElem *context, const lListElem *object, const char *key, const sge_event_type event_type);

/* deleting */
int spool_delete_object(const lListElem *context, const sge_event_type event_type, const char *key);

/* compare spooled attributes of 2 objects */
int spool_compare_objects(const lListElem *context, const sge_event_type event_type, const lListElem *ep1, const lListElem *ep2);


#endif /* __SGE_SPOOLING_H */    
