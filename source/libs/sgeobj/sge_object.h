#ifndef __SGE_OBJECT_H
#define __SGE_OBJECT_H
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

#include "cull.h"

#include "sge_dstring.h"

/****** sgeobj/object/--Object-Handling ***************************************
*
*  NAME
*     Object Handling -- utilities for gdi object access
*
*  FUNCTION
*     This module provides utility functions for accessing CULL 
*     objects, e.g. getting a string representation for fields, 
*     setting field contents from string representation etc.
*
*  NOTES
*     Functions like attr_mod_double from gdi_utility should also 
*     be moved here.
*
*  SEE ALSO
*     gdi/object/object_has_type()
*     gdi/object/object_get_type()
*     gdi/object/object_get_subtype()
*     gdi/object/object_get_primary_key()
*     gdi/object/object_get_name_prefix()
*     gdi/object/object_get_field_contents()
*     gdi/object/object_set_field_contents()
******************************************************************************/

#define NULL_OUT_NONE(ep, nm) \
   if (lGetString(ep, nm) != NULL && \
       strcasecmp(lGetString(ep, nm), "none") == 0) { \
      lSetString(ep, nm, NULL); \
   }


bool 
object_has_type(const lListElem *object, const lDescr *descr);

const lDescr *
object_get_type(const lListElem *object);

const lDescr *
object_get_subtype(int nm);

int 
object_get_primary_key(const lDescr *descr);

const char *
object_get_name_prefix(const lDescr *descr, dstring *buffer);

const char *
object_get_field_contents(const lListElem *object, lList **answer_list, 
                          dstring *buffer, const int nm);

bool 
object_set_field_contents(lListElem *object, lList **answer_list, const int nm,
                          const char *value);

void
object_delete_range_id(lListElem *object, lList **answer_list, 
                       const int rnm, const u_long32 id);

int 
object_set_range_id(lListElem *object, int rnm, u_long32 start, u_long32 end,
                            u_long32 step);

#endif /* __SGE_OBJECT_H */
