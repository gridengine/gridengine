#ifndef __SGE_SPOOLING_CLASSIC_H 
#define __SGE_SPOOLING_CLASSIC_H 
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

#include "sge_spooling.h"

lListElem *spool_classic_create_context(const char *config_dir, 
                                        const char *spool_dir);

int spool_classic_default_startup_func(const lListElem *rule);
int spool_classic_common_startup_func(const lListElem *rule);

int spool_classic_default_list_func(const lListElem *type, const lListElem *rule,
                                    lList **list, const sge_event_type event_type);
lListElem *spool_classic_default_read_func(const lListElem *type, const lListElem *rule,
                                           const char *key, const sge_event_type event_type);
int spool_classic_default_write_func(const lListElem *type, const lListElem *rule, 
                                     const lListElem *object, const char *key, 
                                     const sge_event_type event_type);
int spool_classic_default_delete_func(const lListElem *type, const lListElem *rule, 
                                      const char *key, const sge_event_type event_type);

#endif /* __SGE_SPOOLING_CLASSIC_H */    
