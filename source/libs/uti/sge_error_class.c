/*___INFO__MARK_BEGIN__*/
/*************************************************************************
 * 
 *  The Contents of thiz file are made available subject to the terms of
 *  the Sun Industry Standards Source License Version 1.2
 * 
 *  Sun Microsystems Inc., March, 2001
 * 
 * 
 *  Sun Industry Standards Source License Version 1.2
 *  =================================================
 *  The contents of thiz file are subject to the Sun Industry Standards
 *  Source License Version 1.2 (the "License"); You may not use thiz file
 *  except in compliance with the License. You may obtain a copy of the
 *  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
 * 
 *  Software provided under thiz License is provided on an "AS IS" basis,
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

#include <string.h>
#include "sge.h"
#include "sgermon.h"
#include "sge_error_class.h"
#include "sge_string.h"

typedef struct sge_error_message_str sge_error_message_t;


struct sge_error_message_str {
   int error_quality;
   int error_type;
   char *message;
   sge_error_message_t *next;
};

typedef struct sge_error_iterator_str sge_error_iterator_t;

struct sge_error_iterator_str {
   bool is_first_flag;
   sge_error_message_t *current;
};

typedef struct {
   sge_error_message_t *first;
   sge_error_message_t *last;
} sge_error_t;

static sge_error_iterator_class_t* sge_error_class_iterator(sge_error_class_t *thiz);
static bool sge_error_has_error(sge_error_class_t* eh);
static bool sge_error_has_quality(sge_error_class_t* thiz, int error_quality);
static bool sge_error_has_type(sge_error_class_t* thiz, int error_type);

static void sge_error_error(sge_error_class_t* thiz, int error_type, int error_quality, 
                            const char*fmt, ...);
                            
static void sge_error_clear(sge_error_t* et);
static void sge_error_class_clear(sge_error_class_t* thiz);
void sge_error_destroy(sge_error_t **t);
void sge_error_message_destroy(sge_error_message_t** elem);

static sge_error_iterator_class_t* sge_error_iterator_class_create(sge_error_class_t *ec);
static const char* sge_error_iterator_get_message(sge_error_iterator_class_t* emc);
static int sge_error_iterator_get_quality(sge_error_iterator_class_t* emc);
static int sge_error_iterator_get_type(sge_error_iterator_class_t* emc);
static bool sge_error_iterator_next(sge_error_iterator_class_t* thiz);

                               
sge_error_class_t* sge_error_class_create(void) {
   
   sge_error_class_t *ret = (sge_error_class_t*)sge_malloc(sizeof(sge_error_class_t));
   if( ret == NULL ) {
      return NULL;
   }
   memset(ret,0,sizeof(sge_error_class_t));

   ret->sge_error_handle = sge_malloc(sizeof(sge_error_t));
   memset(ret->sge_error_handle, 0, sizeof(sge_error_t));
   
   ret->has_error = sge_error_has_error;
   ret->has_quality = sge_error_has_quality;
   ret->has_type = sge_error_has_type;
   ret->error = sge_error_error;
   ret->clear = sge_error_class_clear;
   ret->iterator = sge_error_class_iterator;
   return ret;
}

static sge_error_iterator_class_t* sge_error_class_iterator(sge_error_class_t *thiz)
{
   return sge_error_iterator_class_create(thiz);
}

void sge_error_class_destroy(sge_error_class_t **ec) 
{
   sge_error_t *et = NULL;
   
   if ( ec == NULL  || *ec == NULL )
      return;
   
   et = (sge_error_t*)(*ec)->sge_error_handle;
   
   sge_error_destroy(&et);   
   
   FREE(*ec);
   *ec = NULL;
}

static void sge_error_class_clear(sge_error_class_t* thiz) {
   if( thiz != NULL ) { 
      sge_error_t *et = (sge_error_t*)thiz->sge_error_handle;
      sge_error_clear(et);
   }
}

static void sge_error_clear(sge_error_t *et) {
   sge_error_message_t *elem = NULL;
   sge_error_message_t *next = NULL;
   
   DENTER(TOP_LAYER, "sge_error_clear");
   
   if( et != NULL ) {
      elem = et->first;
      while (elem) {
         next = elem->next;
         sge_error_message_destroy(&elem);
         elem = next;
      }
      et->first = NULL;
      et->last = NULL;
   }
   DEXIT;
}


void sge_error_destroy(sge_error_t **t) {
   
   if( t == NULL || *t == NULL ) {
      return;
   }
   sge_error_clear(*t);
   FREE(*t);
   *t = NULL;
}


void sge_error_message_destroy(sge_error_message_t** elem) {
   if (elem == NULL || *elem == NULL) {
      return;
   }
   FREE((*elem)->message);
   (*elem)->message = NULL;
   *elem = NULL;
}

static bool sge_error_has_error(sge_error_class_t* eh) {   
   sge_error_t *et = (sge_error_t*)eh->sge_error_handle;
   return et->first != NULL;
}

static bool sge_error_has_quality(sge_error_class_t* thiz, int error_quality) {
   bool ret = FALSE;
   
   DENTER(TOP_LAYER, "sge_error_has_quality");
   if(thiz) {
      sge_error_t *et = (sge_error_t*)thiz->sge_error_handle;
      sge_error_message_t *elem = et->first;
      while(elem) {
         if(elem->error_quality == error_quality ) {
            ret = TRUE;
            break;
         }
      }
   }
   DEXIT;
   return ret;
}

static bool sge_error_has_type(sge_error_class_t* thiz, int error_type) 
{
   bool ret = FALSE;
   
   DENTER(TOP_LAYER, "sge_error_has_type");
   if(thiz) {
      sge_error_t *et = (sge_error_t*)thiz->sge_error_handle;
      sge_error_message_t *elem = et->first;
      while(elem) {
         if(elem->error_type == error_type ) {
            ret = TRUE;
            break;
         }
      }
   }
   DEXIT;
   return ret;
}

static sge_error_iterator_class_t* sge_error_iterator_class_create(sge_error_class_t *ec)
{
   sge_error_iterator_class_t* ret = NULL;
   sge_error_iterator_t *elem = NULL;
   sge_error_t *et = (sge_error_t*)ec->sge_error_handle;
   
   DENTER(TOP_LAYER, "sge_error_message_class_create");
   
   elem = (sge_error_iterator_t*)sge_malloc(sizeof(sge_error_iterator_t));
   elem->current = et->first;
   elem->is_first_flag = TRUE;
   
   ret = (sge_error_iterator_class_t*)sge_malloc(sizeof(sge_error_iterator_class_t));
   ret->sge_error_iterator_handle = elem;
   ret->get_message = sge_error_iterator_get_message;
   ret->get_quality = sge_error_iterator_get_quality;
   ret->get_type = sge_error_iterator_get_type;
   ret->next = sge_error_iterator_next;
   
   return ret;
}


static void sge_error_error(sge_error_class_t* thiz, int error_type, int error_quality, 
                            const char*format, ...) {
   
   sge_error_message_t *error = NULL;
   sge_error_t *et = (sge_error_t*)thiz->sge_error_handle;
   va_list ap;
   dstring ds = DSTRING_INIT;
   
   DENTER(TOP_LAYER, "sge_error_error");

   va_start(ap, format);

   error = (sge_error_message_t*)sge_malloc(sizeof(sge_error_message_t));

   error->error_quality = error_quality;
   error->error_type = error_type;

   sge_dstring_vsprintf(&ds, format, ap);
   
   error->message = strdup(sge_dstring_get_string(&ds));
   error->next = NULL;
   sge_dstring_free(&ds);
   
   DPRINTF(("error: %s\n", error->message));

   if (et->first == NULL) {
      et->first = error;      
      et->last = error;
   } else {
      et->last->next = error;
      et->last = error;
   }

   DEXIT;
}


void sge_error_iterator_class_destroy(sge_error_iterator_class_t** thiz)
{
   sge_error_iterator_t *elem = NULL;
   
   if (!thiz) {
      return;
   }

   elem = (sge_error_iterator_t *)(*thiz)->sge_error_iterator_handle;

   FREE(elem);
   FREE(*thiz);
   *thiz = NULL;
}

static const char* sge_error_iterator_get_message(sge_error_iterator_class_t* thiz)
{
   sge_error_iterator_t *elem = (sge_error_iterator_t*)thiz->sge_error_iterator_handle;

   if (elem && elem->current) {
      return elem->current->message;
   }
   return NULL;
}

static int sge_error_iterator_get_quality(sge_error_iterator_class_t* thiz)
{
   sge_error_iterator_t *elem = (sge_error_iterator_t*)thiz->sge_error_iterator_handle;

   if (elem && elem->current) {
      return elem->current->error_quality;
   }
   return -1;
}

static int sge_error_iterator_get_type(sge_error_iterator_class_t* thiz)
{
   sge_error_iterator_t *elem = (sge_error_iterator_t*)thiz->sge_error_iterator_handle;

   if (elem && elem->current) {
      return elem->current->error_type;
   }
   return -1;
}

static bool sge_error_iterator_next(sge_error_iterator_class_t* thiz)
{
   sge_error_iterator_t *elem = (sge_error_iterator_t*)thiz->sge_error_iterator_handle;

   if (!elem) {
     return FALSE;
   }
   if (elem->is_first_flag ) {
      elem->is_first_flag = false;
      return (elem->current != NULL);
   } 
   if (elem->current) {
      elem->current = elem->current->next;
      return (elem->current != NULL);
   }
   return FALSE;
}
