#ifndef __SGE_C_GDI_H
#define __SGE_C_GDI_H
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



#ifndef __SGE_GDI_INTERN_H
#   include "sge_gdiP.h"
#endif

#include "cull.h"

typedef struct _gdi_object_t gdi_object_t;

typedef int (*modifier_func_t)(
   lList **alpp,
   lListElem *new_cal,   /* destination */
   lListElem *cep,       /* reduced element */
   int add,              /* 1 for add/0 for mod */
   const char *ruser,
   const char *rhost,
   gdi_object_t *object, /* some kind of "this" */
   int sub_command
);

typedef int (*writer_func_t)(
   lList **alpp,
   lListElem *ep,      /* new modified element */
   gdi_object_t *this   /* some kind of "this" */
);

/* allows to retrieve a master list */
typedef lList ** (*getMasterList)(void);

typedef int (*on_succuss_func_t)(
   lListElem *ep,       /* new modified and already spooled element */
   lListElem *old_ep,   /* old element is NULL in add case */
   gdi_object_t *this   /* some kind of "this" */
);

struct _gdi_object_t {
   u_long32           target;          /* SGE_QUEUE_LIST */
   int                key_nm;          /* QU_qname */
   lDescr             *type;           /* QU_Type */
   char               *object_name;    /* "queue" */
   lList              **master_list;   /* &Master_Calendar_List */
   getMasterList      getMasterList;   /* master list retrieve method    */
   modifier_func_t    modifier;        /* responsible for validating each our attribute modifier */
   writer_func_t      writer;          /* function that spools our object */
   on_succuss_func_t  on_success;      /* do everything what has to be done on successful writing */
};

gdi_object_t *get_gdi_object(u_long32);

int sge_gdi_add_mod_generic(lList **alpp, lListElem *instructions, int add, gdi_object_t *object, const char *ruser, const char *rhost, int sub_command);

void sge_c_gdi(char *host, sge_gdi_request *request, sge_gdi_request *answer);

int verify_request_version(lList **alpp, u_long32 version, char *host, char *commproc, int id);

#endif /* __SGE_C_GDI_H */

