#ifndef __SGE_MIRROR_H
#define __SGE_MIRROR_H
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
#include "sge_gdi.h"

#include "sge_object.h"

#include "sge_eventL.h"

/****** Eventmirror/--Eventmirror ***************************************
*
*  NAME
*     Eventmirror -- mirroring of master lists through event client interface
*
*  FUNCTION
*     The event mirror interface provides a means to easily implement 
*     Grid Engine components that need to have access to the masters 
*     object lists and therefore have to mirror them.
*     
*     Such components can be schedulers, proxies, monitoring tools etc.
*  
*     It is designed as a layer above the event client interface.
*
*     Mirroring can be restricted to certain event types / object types.
*     Callback functions can be installed to perform actions additional to 
*     pure mirroring.
*
*  BUGS
*     Not yet operable on the usermapping related objects and lists.
*
*  SEE ALSO
*     Eventmirror/-Eventmirror-Typedefs
*     Eventmirror/sge_mirror_initialize()
*     Eventmirror/sge_mirror_shutdown()
*     Eventmirror/sge_mirror_subscribe()
*     Eventmirror/sge_mirror_unsubscribe()
*     Eventmirror/sge_mirror_process_events()
*     Eventmirror/sge_mirror_update_master_list()
*     Eventmirror/sge_mirror_update_master_list_host_key()
*     Eventmirror/sge_mirror_update_master_list_str_key()
*     Eventmirror/sge_mirror_strerror()
****************************************************************************
*/

/****** Eventmirror/-Eventmirror-Typedefs ***************************************
*
*  NAME
*     Eventmirror -- mirroring of master lists through event client interface
*
*  SYNOPSIS
*     typedef enum {
*        ...
*     } sge_event_action;
*     
*     typedef enum {
*        ...
*     } sge_mirror_error;
*     
*     typedef int (*sge_mirror_callback)(sge_object_type type, 
*                                        sge_event_action action, 
*                                        lListElem *event, void *clientdata);
*     
*
*  FUNCTION
*     The following types are defined for use with the event mirroring 
*     interface:
*
*
*     Different event actions are defined in the enumeration sge_event_action:
*        SGE_EMA_LIST      - the whole master list has been sent 
*                            (used at initialization)
*        SGE_EMA_ADD       - a new object has been created
*        SGE_EMA_MOD       - an object has been modified
*        SGE_EMA_DEL       - an object has been deleted
*        SGE_EMA_TRIGGER   - a certain action has been triggered, 
*                            e.g. a scheduling run or a shutdown.
*   
*     Most functions of the event mirroring interface return error codes that 
*     are defined in the enumeration sge_mirror_error:
*        SGE_EM_OK              - action performed successfully
*        SGE_EM_NOT_INITIALIZED - the interface is not yet initialized 
*        SGE_EM_BAD_ARG         - some input parameter was incorrect
*        SGE_EM_TIMEOUT         - a timeout occured
*        SGE_EM_DUPLICATE_KEY   - an object should be added, but an object 
*                                 with the same unique identifier already 
*                                 exists.
*        SGE_EM_KEY_NOT_FOUND   - an object with the given key was not found.
*        SGE_EM_CALLBACK_FAILED - a callback function failed
*        SGE_EM_PROCESS_ERRORS  - an error occured during event processing
*
*     The event mirroring interface allows to install callback funktions for
*     actions on certain event types. These callback functions have to have 
*     the same prototype as given by the function typedef sge_mirror_callback.
*  
****************************************************************************
*/
typedef enum {
   SGE_EMA_LIST = 1,
   SGE_EMA_ADD,
   SGE_EMA_MOD,
   SGE_EMA_DEL,
   SGE_EMA_TRIGGER
} sge_event_action;

typedef bool (*sge_mirror_callback)(sge_object_type type, sge_event_action action, lListElem *event, void *clientdata);

typedef enum {
   SGE_EM_OK = 0,
   SGE_EM_NOT_INITIALIZED,
   
   SGE_EM_BAD_ARG,
   SGE_EM_TIMEOUT,
   
   SGE_EM_DUPLICATE_KEY,
   SGE_EM_KEY_NOT_FOUND,

   SGE_EM_CALLBACK_FAILED,

   SGE_EM_PROCESS_ERRORS,

   SGE_EM_LAST_ERRNO
} sge_mirror_error;

/* Initialization - Shutdown */
sge_mirror_error sge_mirror_initialize(ev_registration_id id, const char *name);
sge_mirror_error sge_mirror_shutdown(void);

/* Subscription */
sge_mirror_error sge_mirror_subscribe(sge_object_type type, 
                                      sge_mirror_callback callback_before, 
                                      sge_mirror_callback callback_after, 
                                      void *clientdata);
sge_mirror_error sge_mirror_unsubscribe(sge_object_type type);

/* Event Processing */
sge_mirror_error sge_mirror_process_events(void);

sge_mirror_error sge_mirror_update_master_list(lList **list, const lDescr *list_descr,
                                               lListElem *ep, const char *key, 
                                               sge_event_action action, lListElem *event);
sge_mirror_error sge_mirror_update_master_list_host_key(lList **list, const lDescr *list_descr, 
                                                        int key_nm, const char *key, 
                                                        sge_event_action action, lListElem *event);
sge_mirror_error sge_mirror_update_master_list_str_key(lList **list, const lDescr *list_descr, 
                                                       int key_nm, const char *key, 
                                                       sge_event_action action, lListElem *event);


/* Error Handling */
const char *sge_mirror_strerror(sge_mirror_error num);

#endif /* __SGE_MIRROR_H */
