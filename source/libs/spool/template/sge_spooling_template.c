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

#include "sgermon.h"
#include "sge_log.h"

#include "sge_dstring.h"

#include "gdi_utility.h"
#include "sge_object.h"

#include "msg_common.h"
#include "msg_spoollib.h"
#include "msg_spoollib_template.h"

#include "sge_spooling_template.h"

static const char *spooling_method = "template";

const char *get_spooling_method(void)
{
   return spooling_method;
}


/****** spool/template/spool_template_create_context() ********************
*  NAME
*     spool_template_create_context() -- create a template spooling context
*
*  SYNOPSIS
*     lListElem* 
*     spool_template_create_context(int argc, char *argv[])
*
*  FUNCTION
*     Create a spooling context for the template spooling.
* 
*  INPUTS
*     int argc     - number of arguments in argv
*     char *argv[] - argument vector
*
*  RESULT
*     lListElem* - on success, the new spooling context, else NULL
*
*  SEE ALSO
*     spool/--Spooling
*     spool/template/--Template-Spooling
*******************************************************************************/
lListElem *
spool_template_create_context(int argc, char *argv[])
{
   lListElem *context = NULL;

   DENTER(TOP_LAYER, "spool_template_create_context");


   DEXIT;
   return context;
}

/****** spool/template/spool_template_default_startup_func() **************
*  NAME
*     spool_template_default_startup_func() -- setup 
*
*  SYNOPSIS
*     bool 
*     spool_template_default_startup_func(const lListElem *rule) 
*
*  FUNCTION
*
*  INPUTS
*     const lListElem *rule - the rule containing data necessary for
*                             the startup (e.g. path to the spool directory)
*
*  RESULT
*     bool - true, if the startup succeeded, else false
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/template/--Template-Spooling
*     spool/spool_startup_context()
*******************************************************************************/
bool
spool_template_default_startup_func(const lListElem *rule)
{
   const char *url;

   DENTER(TOP_LAYER, "spool_template_default_startup_func");

   url = lGetString(rule, SPR_url);

   DEXIT;
   return true;
}

/****** spool/template/spool_template_default_list_func() *****************
*  NAME
*     spool_template_default_list_func() -- read lists through template spooling
*
*  SYNOPSIS
*     bool 
*     spool_template_default_list_func(const lListElem *type, 
*                                      const lListElem *rule, 
*                                      lList **list, 
*                                      const sge_event_type event_type) 
*
*  FUNCTION
*
*  INPUTS
*     const lListElem *type           - object type description
*     const lListElem *rule           - rule to be used 
*     lList **list                    - target list
*     const sge_event_type event_type - object type
*
*  RESULT
*     bool - true, on success, else false
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/template/--Template-Spooling
*     spool/spool_read_list()
*******************************************************************************/
bool
spool_template_default_list_func(const lListElem *type, const lListElem *rule,
                                    lList **list, const sge_event_type event_type)
{
   DENTER(TOP_LAYER, "spool_template_default_list_func");

   DEXIT;
   return true;
}

/****** spool/template/spool_template_default_read_func() *****************
*  NAME
*     spool_template_default_read_func() -- read objects through template spooling
*
*  SYNOPSIS
*     lListElem* 
*     spool_template_default_read_func(const lListElem *type, 
*                                      const lListElem *rule, 
*                                      const char *key, 
*                                      const sge_event_type event_type) 
*
*  FUNCTION
*
*  INPUTS
*     const lListElem *type           - object type description
*     const lListElem *rule           - rule to use
*     const char *key                 - unique key specifying the object
*     const sge_event_type event_type - object type
*
*  RESULT
*     lListElem* - the object, if it could be read, else NULL
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/template/--Template-Spooling
*     spool/spool_read_object()
*******************************************************************************/
lListElem *
spool_template_default_read_func(const lListElem *type, const lListElem *rule,
                                 const char *key, 
                                 const sge_event_type event_type)
{
   lListElem *ep = NULL;

   DENTER(TOP_LAYER, "spool_template_default_read_func");

   DEXIT;
   return ep;
}

/****** spool/template/spool_template_default_write_func() ****************
*  NAME
*     spool_template_default_write_func() -- write objects through template spooling
*
*  SYNOPSIS
*     bool
*     spool_template_default_write_func(const lListElem *type, 
*                                       const lListElem *rule, 
*                                       const lListElem *object, 
*                                       const char *key, 
*                                       const sge_event_type event_type) 
*
*  FUNCTION
*     Writes an object through the appropriate template spooling functions.
*
*  INPUTS
*     const lListElem *type           - object type description
*     const lListElem *rule           - rule to use
*     const lListElem *object         - object to spool
*     const char *key                 - unique key
*     const sge_event_type event_type - object type
*
*  RESULT
*     bool - true on success, else false
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/template/--Template-Spooling
*     spool/spool_delete_object()
*******************************************************************************/
bool
spool_template_default_write_func(const lListElem *type, const lListElem *rule, 
                                  const lListElem *object, const char *key, 
                                  const sge_event_type event_type)
{
   DENTER(TOP_LAYER, "spool_template_default_write_func");

   DEXIT;
   return true;
}

/****** spool/template/spool_template_default_delete_func() ***************
*  NAME
*     spool_template_default_delete_func() -- delete object in template spooling
*
*  SYNOPSIS
*     bool
*     spool_template_default_delete_func(const lListElem *type, 
*                                        const lListElem *rule, 
*                                        const char *key, 
*                                        const sge_event_type event_type) 
*
*  FUNCTION
*     Deletes an object in the template spooling.
*
*  INPUTS
*     const lListElem *type           - object type description
*     const lListElem *rule           - rule to use
*     const char *key                 - unique key 
*     const sge_event_type event_type - object type
*
*  RESULT
*     bool - true on success, else false
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/template/--Template-Spooling
*     spool/spool_delete_object()
*******************************************************************************/
bool
spool_template_default_delete_func(const lListElem *type, const lListElem *rule,
                                   const char *key, 
                                   const sge_event_type event_type)
{
   DENTER(TOP_LAYER, "spool_template_default_delete_func");

   DEXIT;
   return true;
}

