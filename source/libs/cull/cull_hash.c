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

#include <stdio.h>
#include <stdlib.h>

#include "sgermon.h"
#include "sge_log.h"
#include "msg_cull.h"

#include "cull_list.h"
#include "cull_hash.h"
#include "cull_listP.h"
#include "cull_multitypeP.h"
#include "cull_multitype.h"
#include "sge_string.h"
#include "sge_hostname.h"

/****** cull/hash/--CULL_Hashtable **********************************************
*  NAME
*     htable -- Hashtable extensions for cull lists 
*
*  SYNOPSIS
*     htable cull_hash_create(const lDescr *descr);
*
*     void cull_hash_new(lList *lp, int name, int unique);
*
*     void cull_hash_insert(const lListElem *ep, const int pos);
*
*     void cull_hash_remove(const lListElem *ep, const int pos);
*
*     void cull_hash_elem(const lListElem *ep);
*
*     lListElem *cull_hash_first(const lList *lp, const int pos, 
*                         const void *key, const void **iterator);
*
*     lListElem *cull_hash_next(const lList *lp, const int pos, 
*                         const void *key, const void **iterator);
*
*     void cull_hash_free_descr(lDescr *descr);
*
*  FUNCTION
*     This module provides an abstraction layer between cull and 
*     the hash table implementation in libuti. It provides the 
*     necessary functions to use hash tables from libuti for cull lists.
*
*     The functions defined in this module implement hash tables with 
*     non unique keys, provide wrapper functions for hash insert, remove 
*     and search that are aware of the non unique hash implementation, 
*     functions that deal with the necessary extensions to the cull list
*     descriptor objects etc.
*
*  SEE ALSO
*     uti/hash/--Hashtable
*******************************************************************************/

/****** cull/hash/-CULL_Hashtable_Defines ****************************************************
*  NAME
*     Defines -- Constants for the cull hash implementation
*
*  SYNOPSIS
*     #define MIN_CULL_HASH_SIZE 4
*
*  FUNCTION
*     Provides constants to be used in the hash table implementation 
*     for cull lists.
*
*     MIN_CULL_HASH_SIZE - minimum size of a hash table. When a new 
*                          hash table is created, it will have the size 
*                          2^MIN_CULL_HASH_SIZE
*******************************************************************************/
#define MIN_CULL_HASH_SIZE 4

/****** cull/hash/-CULL_Hashtable_Typedefs ***************************************************
*  NAME
*     Typedefs -- Typedefs for cull hash implementation 
*
*  SYNOPSIS
*     typedef struct _non_unique_hash non_unique_hash;
*     
*     struct _non_unique_hash {
*        non_unique_hash *next;
*        const void *data;
*     };
*
*  FUNCTION
*     Internal data structure to handle hash tables with non unique 
*     keys. The hash table (from libuti) in this case will not store 
*     a pointer to the cull object itself, but a pointer to a list of 
*     cull objects. This list is implemented using the non_unique_hash 
*     structures.
*
*  SEE ALSO
*     uti/hash/--Hashtable
*******************************************************************************/
typedef struct _non_unique_hash non_unique_hash;

struct _non_unique_hash {
   non_unique_hash *next;
   const void *data;
};

/****** cull/hash/cull_hash_create() *******************************************
*  NAME
*     cull_hash_create() -- create a new hash table
*
*  SYNOPSIS
*     lHash* cull_hash_create(const lDescr *descr) 
*
*  FUNCTION
*     Creates a new hash table for a certain descriptor and returns the 
*     hash description (lHash) for it.
*
*  INPUTS
*     const lDescr *descr - descriptor for the data field in a 
*                           cull object.
*
*  RESULT
*     lHash* - initialized hash description
*******************************************************************************/
htable cull_hash_create(const lDescr *descr)
{
   switch(mt_get_type(descr->mt)) {
      case lStringT:
         return sge_htable_create(MIN_CULL_HASH_SIZE, dup_func_string, hash_func_string, hash_compare_string);
      case lHostT:
         return sge_htable_create(MIN_CULL_HASH_SIZE, dup_func_string, hash_func_string, hash_compare_string);
      case lUlongT:
         return sge_htable_create(MIN_CULL_HASH_SIZE, dup_func_u_long32, hash_func_u_long32, hash_compare_u_long32);
      default:
         unknownType("cull_create_hash");
         return NULL;
   }
}

/****** cull/hash/cull_hash_create_hashtables() ********************************
*  NAME
*     cull_hash_create_hashtables() -- create all hashtables on a list
*
*  SYNOPSIS
*     void cull_hash_create_hashtables(lList *lp) 
*
*  FUNCTION
*     Creates all hashtables for an empty list.
*
*  INPUTS
*     lList *lp - initialized list structure
*
*  NOTES
*     If the list already contains elements, these elements are not 
*     inserted into the hash lists.
*******************************************************************************/
void cull_hash_create_hashtables(lList *lp) 
{
   int i;

   if(lp == NULL) {
      return;
   }

   for(i = 0; lp->descr[i].mt != lEndT; i++) {
      if(mt_do_hashing(lp->descr[i].mt) && lp->descr[i].ht == NULL) {
         lp->descr[i].ht = cull_hash_create(&lp->descr[i]);
      }
   }
}

/****** cull/hash/cull_hash_insert() *******************************************
*  NAME
*     cull_hash_insert() -- insert a new element in a hash table
*
*  SYNOPSIS
*     void cull_hash_insert(const lListElem *ep, const int pos) 
*
*  FUNCTION
*     Inserts ep into the hash list for data field at position pos.
*     A hash key will be computed. The element will be inserted
*     in the corresponding hash table considering unique/non unique
*     hash storage.
*
*  INPUTS
*     const lListElem *ep - the cull object to be stored in a hash list
*     const int pos       - describes the data field of the objects that
*                           is to be hashed
*******************************************************************************/
void cull_hash_insert(const lListElem *ep, const int pos)
{
   char host_key[MAXHOSTLEN+1];
   lDescr *descr = NULL;
   void *key = NULL;

   if(ep == NULL || pos < 0) {
      return;
   }

   descr = &ep->descr[pos];

   if(descr->ht == NULL) {
      return;
   }

   switch(mt_get_type(descr->mt)) {
      case lUlongT:
         key = (void *)&(ep->cont[pos].ul);
         break;

      case lStringT:
         key = ep->cont[pos].str;
         break;
  
      case lHostT:
         if (ep->cont[pos].host != NULL) {
            sge_hostcpy(host_key,ep->cont[pos].host);
            sge_strtoupper(host_key,MAXHOSTLEN);
            key = host_key;
         }
         break;

      default:
         unknownType("cull_hash_insert");
         break;
   }
  
   if(key != NULL) {
      if(mt_is_unique(descr->mt)) {
         sge_htable_store(descr->ht, key, ep);
      } else {
         non_unique_hash *nuh = NULL;
         /* do we already have a list of elements with this key? */
         if(sge_htable_lookup(descr->ht, key, (const void **)&nuh) == True) {
            if(nuh->data != ep) {
               non_unique_hash *head = nuh; /* remember head of list */

               while(head->next != NULL && head->next->data != ep) {
                  head = head->next;
               }

               /* if element is already in list, do nothing, else create new list element */
               if(head->next == NULL) {
                  nuh = (non_unique_hash *)malloc(sizeof(non_unique_hash));
                  head->next = nuh;
                  nuh->data = ep;
                  nuh->next = NULL;
               }
            }
         } else { /* no list of non unique elements for this key, create new */
            nuh = (non_unique_hash *)malloc(sizeof(non_unique_hash));
            nuh->next = NULL;
            nuh->data = ep;
            sge_htable_store(descr->ht, key, nuh);
         }
      }
   }   
}

/****** cull/hash/cull_hash_remove() *******************************************
*  NAME
*     cull_hash_remove() -- remove a cull object from a hash list
*
*  SYNOPSIS
*     void cull_hash_remove(const lListElem *ep, const int pos) 
*
*  FUNCTION
*     Removes ep from a hash table for data field specified by pos.
*
*  INPUTS
*     const lListElem *ep - the cull object to be removed
*     const int pos       - position of the data field 
*******************************************************************************/
void cull_hash_remove(const lListElem *ep, const int pos)
{
   char host_key[MAXHOSTLEN+1];
   lDescr *descr = NULL;
   void *key = NULL;

   if(ep == NULL || pos < 0) {
      return;
   }

   descr = &ep->descr[pos];

   if(descr->ht == NULL) {
      return;
   }

   switch(mt_get_type(descr->mt)) {
      case lUlongT:
         key = (void *)&(ep->cont[pos].ul);
         break;

      case lStringT:
         key = ep->cont[pos].str;
         break;
      
      case lHostT:
         if (ep->cont[pos].host != NULL) {
            sge_hostcpy(host_key,ep->cont[pos].host);
            sge_strtoupper(host_key,MAXHOSTLEN);
            key = host_key;
         }
         break;

      default:
         unknownType("cull_hash_remove");
         break;
   }
  
   if(key != NULL) {
      if(mt_is_unique(descr->mt)) {
        sge_htable_delete(descr->ht, key);
      } else {
         non_unique_hash *nuh = NULL;
         if(sge_htable_lookup(descr->ht, key, (const void **)&nuh) == True) {
            /* first element is element to remove (and perhaps only element) */
            if(nuh->data == ep) {
               non_unique_hash *head = nuh->next;
               free(nuh);
               if(head != NULL) {
                  sge_htable_store(descr->ht, key, head);
               } else {
                  sge_htable_delete(descr->ht, key);
               }
            } else {
               while(nuh->next != NULL) {
                  if(nuh->next->data == ep) {
                     non_unique_hash *found = nuh->next;
                     nuh->next = found->next;
                     free(found);
                  } else {
                     nuh = nuh->next;
                  }
               }
            }
         }
      }
   }   
}

/****** cull/hash/cull_hash_elem() *********************************************
*  NAME
*     cull_hash_elem() -- insert cull object into associated hash tables
*
*  SYNOPSIS
*     void cull_hash_elem(const lListElem *ep) 
*
*  FUNCTION
*     Insert the cull element ep into all hash tables that are 
*     defined for the cull list ep is member of.
*
*  INPUTS
*     const lListElem *ep - the cull object to be hashed
*******************************************************************************/
void cull_hash_elem(const lListElem *ep) {
   int i;
  
   if(ep == NULL) {
      return;
   }
  
   for(i = 0; ep->descr[i].mt != lEndT; i++) {
      if(ep->descr[i].ht != NULL) {
         cull_hash_insert(ep, i);
      }
   }
}

/****** cull/hash/cull_hash_first() *******************************************
*  NAME
*     cull_hash_first() -- find first object for a certain key
*
*  SYNOPSIS
*     lListElem* cull_hash_first(const lList *lp, const int pos, 
*                                const void  *key, 
*                                const void **iterator) 
*
*  FUNCTION
*     Searches for key in the hash table for data field described by 
*     pos in the cull list lp.
*     If an element is found, it is returned.
*     If the hash table uses non unique hash keys, iterator returns the 
*     necessary data for consecutive calls of cull_hash_next() returning
*     objects with the same hash key.
*
*  INPUTS
*     const lList *lp       - the cull list to search
*     const int pos         - position of the data field for key
*     const void *key       - the key to use for the search
*     const void **iterator - iterator for calls of cull_hash_next
*
*  RESULT
*     lListElem* - first object found matching key, 
*                  if no object found: NULL
*
*  SEE ALSO
*     cull/hash/cull_hash_next()
******************************************************************************/
lListElem *cull_hash_first(const lList *lp, const int pos, const void *key, 
                           const void **iterator)
{
   lListElem *ep = NULL;
   lDescr *descr;

   if(lp == NULL) {
      return NULL;
   }

   descr = &lp->descr[pos];
   if(descr->ht == NULL) {
      *iterator = NULL;
      return NULL;  
   }   

   if(mt_is_unique(descr->mt)) {
      *iterator = NULL;
      if(sge_htable_lookup(descr->ht, key, (const void **)&ep) == True) {
         return ep;
      } else {
         return NULL;
      }
   } else {
      non_unique_hash *nuh;
      if(sge_htable_lookup(descr->ht, key, (const void **)&nuh) == True) {
         ep = (lListElem *)nuh->data;
         *iterator = nuh;
         return ep;
      } else {
         *iterator = NULL;
         return NULL;
      }
   }
}

/****** cull/hash/cull_hash_next() *********************************************
*  NAME
*     cull_hash_next() -- find next object matching a key
*
*  SYNOPSIS
*     lListElem* cull_hash_next(const lList *lp, const int pos, 
*                               const void *key, const void **iterator) 
*
*  FUNCTION
*     Returns the next object matching the same key as a previous call
*     to cull_hash_first or cull_hash_next.
*
*  INPUTS
*     const lList *lp       - the cull list to search
*     const int pos         - position of the data field for key
*     const void *key       - the key to use for the search
*     const void **iterator - iterator to use for the search.
*
*  RESULT
*     lListElem* - object if found, else NULL
*
*  NOTES
*     The order in which objects with the same key are returned is not
*     defined.
*
*  SEE ALSO
*     cull/hash/cull_hash_first()
*******************************************************************************/
lListElem *cull_hash_next(const lList *lp, const int pos, const void *key, const void **iterator)
{
   lListElem *ep = NULL;
   non_unique_hash *nuh = (non_unique_hash *)*iterator;
   lDescr *descr;
  
   if(lp == NULL) {
      return NULL;
   }

   descr = &lp->descr[pos];
  
   if(descr->ht == NULL || mt_is_unique(descr->mt) || nuh == NULL) {
      return NULL;
   }

   nuh = nuh->next;
   if(nuh != NULL) {
      ep = (lListElem *)nuh->data;
      *iterator = nuh;
   } else {
      *iterator = NULL;
   }

   return ep;
}

/****** cull/hash/cull_hash_delete_non_unique_chain() *************************
*  NAME
*     cull_hash_delete_non_unique_chain() -- del list of non unique obj.
*
*  SYNOPSIS
*     void cull_hash_delete_non_unique_chain(htable table, 
*                                            const void *key, 
*                                            const void **data) 
*
*  FUNCTION
*     For objects that are stored in a hash table with non unique keys, 
*     for each key a linked list of objects is created.
*     This function deletes this linked list for each key in the hash 
*     table. It is designed to be called by the function 
*     sge_htable_for_each from the libuti hash implementation.
*
*  INPUTS
*     htable table   - hash table in which to delete/free a sublist
*     const void *key   - key of the list to be freed 
*     const void **data - pointer to the sublist
*
*  SEE ALSO
*     uti/hash/sge_htable_for_each()
******************************************************************************/
void cull_hash_delete_non_unique_chain(htable table, const void *key, 
                                       const void **data)
{
   non_unique_hash *head = (non_unique_hash *)*data;
   while(head != NULL) {
      non_unique_hash *nuh = head;
      head = head->next;
      free(nuh);
   }
}

/****** cull/hash/cull_hash_free_descr() **************************************
*  NAME
*     cull_hash_free_descr() -- free the hash contents of a cull descr
*
*  SYNOPSIS
*     void cull_hash_free_descr(lDescr *descr) 
*
*  FUNCTION
*     Frees the memory used by the hashing information in a cull 
*     descriptor (lDescr). If a hash table is still associated to 
*     the descriptor, it is also deleted.
*
*  INPUTS
*     lDescr *descr - descriptor to free 
*
*  SEE ALSO
*     cull/hash/cull_hash_delete_non_unique()
*     uti/hash/sge_htable_destroy()
******************************************************************************/
void cull_hash_free_descr(lDescr *descr)
{
   int i;
   for(i = 0; descr[i].mt != lEndT; i++) {
      if(descr[i].ht != NULL) {
         if(!mt_is_unique(descr[i].mt)) {
            /* delete chain of non unique elements */
            sge_htable_for_each(descr[i].ht, cull_hash_delete_non_unique_chain);
         }
         sge_htable_destroy(descr[i].ht);
         descr[i].ht = NULL;
      }
   }
}


/****** cull/hash/cull_hash_new_check() ****************************************
*  NAME
*     cull_hash_new() -- create new hash table, if it does not yet exist
*
*  SYNOPSIS
*     int cull_hash_new_check(lList *lp, int nm, int unique) 
*
*  FUNCTION
*     Usually hash tables are defined in the object type definition
*     for each object type in libs/gdi.
*
*     There are cases where for a certain application additional hash 
*     tables shall be defined to speed up certain access methods.
*
*     cull_hash_new_check can be used to create a hash table for a list
*     on the contents of a certain field.
*     If it already exist, nothing is done.
*
*     The caller can choose whether the field contents have to be
*     unique within the list or not.
*
*  INPUTS
*     lList *lp  - the list to hold the new hash table
*     int nm     - the field on which to create the hash table 
*     int unique - unique contents or not 
*
*  RESULT
*     int - 1 on success, else 0
*
*  EXAMPLE
*     create a non unique hash index on the job owner for a job list
*     cull_hash_new_check(job_list, JB_owner, 0);
*
*  SEE ALSO
*     cull/hash/cull_hash_new()
*
*******************************************************************************/
int cull_hash_new_check(lList *lp, int nm, int unique)
{
   const lDescr *descr = lGetListDescr(lp);
   int pos = lGetPosInDescr(descr, nm);
  
   if (descr != NULL && pos >= 0) {
      if (descr[pos].ht == NULL)  {
         return cull_hash_new(lp, nm, unique);
      }
   }

   return 1;
}

/****** cull/hash/cull_hash_new() **********************************************
*  NAME
*     cull_hash_new() -- create new hash table
*
*  SYNOPSIS
*     int cull_hash_new(lList *lp, int nm, int unique) 
*
*  FUNCTION
*     Usually hash tables are defined in the object type definition
*     for each object type in libs/gdi.
*
*     There are cases where for a certain application additional hash 
*     tables shall be defined to speed up certain access methods.
*
*     cull_hash_new can be used to create a hash table for a list
*     on the contents of a certain field.
*     The caller can choose whether the field contents have to be
*     unique within the list or not.
*
*  INPUTS
*     lList *lp  - the list to hold the new hash table
*     int nm     - the field on which to create the hash table 
*     int unique - unique contents or not 
*
*  RESULT
*     int - 1 on success, else 0
*
*  EXAMPLE
*     create a non unique hash index on the job owner for a job list
*     cull_hash_new(job_list, JB_owner, 0);
*
*******************************************************************************/
int cull_hash_new(lList *lp, int nm, int unique)
{
   lDescr *descr;
   lListElem *ep;
   int pos;

   DENTER(CULL_LAYER, "cull_hash_new");

   if(lp == NULL) {
      DEXIT;
      return 0;
   }
 
   descr = lp->descr;
 
   pos = lGetPosInDescr(descr, nm);

   if(pos < 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_GETELEMSTRERRORXRUNTIMETYPE_S , lNm2Str(nm)));
      DEXIT;
      return 0;
   }

   if(descr[pos].ht != NULL) {
      WARNING((SGE_EVENT, MSG_CULL_HASHTABLEALREADYEXISTS_S, lNm2Str(nm)));
      DEXIT;
      return 0;
   }

   /* copy hashing information */
   descr[pos].mt |= CULL_HASH;
   if(unique) {
      descr[pos].mt |= CULL_UNIQUE;
   }

   descr[pos].ht = cull_hash_create(&descr[pos]);

   if(descr[pos].ht == NULL) {
      DEXIT;
      return 0;
   }

   /* insert all elements into the new hash table */
   for_each(ep, lp) {
      cull_hash_insert(ep, pos);
   }

   DEXIT;
   return 1;
}
