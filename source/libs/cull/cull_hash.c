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

#include "cull_list.h"
#include "cull_hash.h"
#include "cull_listP.h"
#include "cull_multitypeP.h"

/****** uti/hash/--Introduction *************************************************
*  NAME
*     Cull & HashTables -- Hashtable extensions for cull lists 
*
*  SYNOPSIS
*     lHash *cull_hash_copy_descr(const lDescr *descr);
*     lHash *cull_hash_create(const lDescr *descr);
*     void cull_hash_insert(const lListElem *ep, const int pos);
*     void cull_hash_remove(const lListElem *ep, const int pos);
*     void cull_hash_elem(const lListElem *ep);
*     lListElem *cull_hash_first(const lList *lp, const int pos, const void *key, const void **iterator);
*     lListElem *cull_hash_next(const lList *lp, const int pos, const void *key, const void **iterator);
*     void cull_hash_free_descr(lDescr *descr);
*
*  FUNCTION
*     This module provides an abstraction layer between cull and the hash table
*     implementation in libuti. It provides the necessary functions to use 
*     hash tables from libuti for cull lists.
*
*     The functions defined in this module implement hash tables with 
*     non unique keys, provide wrapper functions for hash insert, remove and 
*     search that are aware of the non unique hash implementation, 
*     functions that deal with the necessary extensions to the cull list
*     descriptor objects etc.
*
*  SEE ALSO
*     uti/hash/--Introduction
*
*******************************************************************************/

/****** cull/hash/-Defines ***************************************
*  NAME
*     Defines -- Constants for the cull hash implementation
*
*  SYNOPSIS
*     #define MIN_CULL_HASH_SIZE 4
*
*  FUNCTION
*     Provides constants to be used in the hash table implementation for 
*     cull lists.
*
*     MIN_CULL_HASH_SIZE - minimum size of a hash table. When a new hash table
*                          is created, it will have the size 
*                          2^MIN_CULL_HASH_SIZE
*
*******************************************************************************/
#define MIN_CULL_HASH_SIZE 4

/****** cull/hash/-Templates ***************************************
*  NAME
*     Templates -- Templates for typical hash table definitions 
*
*  SYNOPSIS
*     lHash template_hash = { ... };
*
*  FUNCTION
*     Some templates are provided for the definition of hash tables.
*     These templates are used in the cull definition of lists 
*     by the list definition macros, e.g. KSTRINGH or KSTRINGHU.
*  
*     Two templates are defined:
*     template_hash provides a hash table with non unique keys,
*     template_hash_unique provides a hash table with unique keys.
*
*******************************************************************************/
lHash template_hash = {
   0,
   NULL
};

lHash template_hash_unique = {
   1,
   NULL
};

/****** cull/hash/-Typedefs ***************************************
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
*     Internal data structure to handle hash tables with non unique keys.
*     The hash table (from libuti) in this case will not store a pointer
*     to the cull object itself, but a pointer to a list of cull objects.
*     This list is implemented using the non_unique_hash structures.
*
*  SEE ALSO
*     uti/hash/--Introduction
*******************************************************************************/
typedef struct _non_unique_hash non_unique_hash;

struct _non_unique_hash {
   non_unique_hash *next;
   const void *data;
};

/****** cull/hash/cull_hash_copy_descr() ***************************************
*  NAME
*     cull_hash_copy_descr() -- copy the hashing information in an lDescr
*
*  SYNOPSIS
*     lHash* cull_hash_copy_descr(const lDescr *descr) 
*
*  FUNCTION
*     Allocates and initializes/copies a cull lDescr object.
*
*  INPUTS
*     const lDescr *descr - the descriptor to be copied
*
*  RESULT
*     lHash* - the new hash element for a lDescr object
*
*******************************************************************************/
lHash *cull_hash_copy_descr(const lDescr *descr)
{
   lHash *hash;

   if((hash = (lHash *)malloc(sizeof(lHash))) == NULL) {
      return NULL;
   }

   hash->unique = descr->hash->unique;
   hash->table  = NULL;

   return hash;
}

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
*     const lDescr *descr - descriptor for the data field in a cull object.
*
*  RESULT
*     lHash* - initialized hash description
*
*******************************************************************************/
lHash *cull_hash_create(const lDescr *descr)
{
   lHash *hash;

   if((hash = cull_hash_copy_descr(descr)) == NULL) {
      return NULL;
   }

   switch(descr->mt) {
      case lStringT:
         if((hash->table  = HashTableCreate(MIN_CULL_HASH_SIZE, DupFunc_string, HashFunc_string, HashCompare_string)) == NULL) {
            free(hash);
            return NULL;
         }
         break;
      case lUlongT:
         if((hash->table  = HashTableCreate(MIN_CULL_HASH_SIZE, DupFunc_u_long32, HashFunc_u_long32, HashCompare_u_long32)) == NULL) {
            free(hash);
            return NULL;
         }
         break;
      default:
         unknownType("cull_create_hash");
         free(hash);
         return NULL;
   }
   
   return hash;
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
*     If the list already contains elements, these elements are not inserted
*     into the hash lists.
*
*******************************************************************************/
void cull_hash_create_hashtables(lList *lp) 
{
   int i;

   for(i = 0; lp->descr[i].mt != lEndT; i++) {
      if(lp->descr[i].hash != NULL && lp->descr[i].hash->table == NULL) {
         switch(lp->descr[i].mt) {
            case lStringT:
               if((lp->descr[i].hash->table  = HashTableCreate(MIN_CULL_HASH_SIZE, DupFunc_string, HashFunc_string, HashCompare_string)) == NULL) {
                  free(lp->descr[i].hash);
                  lp->descr[i].hash = NULL;
               }
               break;
            case lUlongT:
               if((lp->descr[i].hash->table  = HashTableCreate(MIN_CULL_HASH_SIZE, DupFunc_u_long32, HashFunc_u_long32, HashCompare_u_long32)) == NULL) {
                  free(lp->descr[i].hash);
                  lp->descr[i].hash = NULL;
               }
               break;
            default:
               unknownType("cull_create_hash");
               free(lp->descr[i].hash);
               lp->descr[i].hash = NULL;
               break;
         }
      }
   }
}

/****** cull_hash/cull_hash_insert() *******************************************
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
*
*******************************************************************************/
void cull_hash_insert(const lListElem *ep, const int pos)
{
   lDescr *descr = &ep->descr[pos];
   void *key = NULL;

   if(descr->hash == NULL || descr->hash->table == NULL) {
      return;
   }

   switch(descr->mt) {
      case lUlongT:
         key = (void *)&(ep->cont[pos].ul);
         break;

      case lStringT:
         key = ep->cont[pos].str;
         break;

      default:
         unknownType("cull_hash_insert");
         break;
   }
  
   if(key != NULL) {
      if(descr->hash->unique) {
         HashTableStore(descr->hash->table, key, ep);
      } else {
         non_unique_hash *nuh = NULL;
         /* do we already have a list of elements with this key? */
         if(HashTableLookup(descr->hash->table, key, (const void **)&nuh) == True) {
            non_unique_hash *head = nuh; /* remember head of list */

            while(nuh != NULL && nuh->data != ep) {
               nuh = nuh->next;
            }
            /* if element is already in list, do nothing, else create new list element */
            if(nuh == NULL) {
               nuh = (non_unique_hash *)malloc(sizeof(non_unique_hash));
               nuh->next = head;
               nuh->data = ep;
               HashTableStore(descr->hash->table, key, nuh);
            }
         } else { /* no list of non unique elements for this key, create new */
            nuh = (non_unique_hash *)malloc(sizeof(non_unique_hash));
            nuh->next = NULL;
            nuh->data = ep;
            HashTableStore(descr->hash->table, key, nuh);
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
*
*******************************************************************************/
void cull_hash_remove(const lListElem *ep, const int pos)
{
   lDescr *descr = &ep->descr[pos];
   void *key = NULL;

   if(descr->hash == NULL || descr->hash->table == NULL) {
      return;
   }

   switch(descr->mt) {
      case lUlongT:
         key = (void *)&(ep->cont[pos].ul);
         break;

      case lStringT:
         key = ep->cont[pos].str;
         break;

      default:
         unknownType("cull_hash_remove");
         break;
   }
  
   if(key != NULL) {
      if(descr->hash->unique) {
        HashTableDelete(descr->hash->table, key);
      } else {
         non_unique_hash *nuh = NULL;
         if(HashTableLookup(descr->hash->table, key, (const void **)&nuh) == True) {
            /* first element is element to remove (and perhaps only element) */
            if(nuh->data == ep) {
               non_unique_hash *head = nuh->next;
               free(nuh);
               if(head != NULL) {
                  HashTableStore(descr->hash->table, key, head);
               } else {
                  HashTableDelete(descr->hash->table, key);
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
*     Insert the cull element ep into all hash tables that are defined for
*     the cull list ep is member of.
*
*  INPUTS
*     const lListElem *ep - the cull object to be hashed
*
*******************************************************************************/
void cull_hash_elem(const lListElem *ep) {
   int i;
   
   for(i = 0; ep->descr[i].mt != lEndT; i++) {
      if(ep->descr[i].hash != NULL) {
         cull_hash_insert(ep, i);
      }
   }
}

/****** cull_hash/cull_hash_first() ********************************************
*  NAME
*     cull_hash_first() -- find first object for a certain key
*
*  SYNOPSIS
*     lListElem* cull_hash_first(const lList *lp, const int pos, 
*                                const void  *key, const void **iterator) 
*
*  FUNCTION
*     Searches for key in the hash table for data field described by pos in the
*     cull list lp.
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
*******************************************************************************/
lListElem *cull_hash_first(const lList *lp, const int pos, const void *key, const void **iterator)
{
   lListElem *ep = NULL;

   lDescr *descr = &lp->descr[pos];
   if(descr->hash == NULL || descr->hash->table == NULL) {
      *iterator = NULL;
      return NULL;  
   }   

   if(descr->hash->unique) {
      *iterator = NULL;
      if(HashTableLookup(descr->hash->table, key, (const void **)&ep) == True) {
         return ep;
      } else {
         return NULL;
      }
   } else {
      non_unique_hash *nuh;
      if(HashTableLookup(descr->hash->table, key, (const void **)&nuh) == True) {
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
   lDescr *descr = &lp->descr[pos];
   
   if(descr->hash == NULL || descr->hash->table == NULL || descr->hash->unique == 1 || nuh == NULL) {
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

/****** cull/hash/cull_hash_delete_non_unique_chain() **************************
*  NAME
*     cull_hash_delete_non_unique_chain() -- delete list of non unique objects
*
*  SYNOPSIS
*     void cull_hash_delete_non_unique_chain(HashTable table, const void *key, 
*                                            const void **data) 
*
*  FUNCTION
*     For objects that are stored in a hash table with non unique keys, for
*     each key a linked list of objects is created.
*     This function deletes this linked list for each key in the hash table.
*     It is designed to be called by the function HashTableForEach from
*     the libuti hash implementation.
*
*  INPUTS
*     HashTable table   - hash table in which to delete/free a sublist
*     const void *key   - key of the list to be freed 
*     const void **data - pointer to the sublist
*
*  SEE ALSO
*     uti/hash/HashTableForEach()
*******************************************************************************/
void cull_hash_delete_non_unique_chain(HashTable table, const void *key, const void **data)
{
   non_unique_hash *head = (non_unique_hash *)*data;
   while(head != NULL) {
      non_unique_hash *nuh = head;
      head = head->next;
      free(nuh);
   }
}

/****** cull/hash/cull_hash_free_descr() ***************************************
*  NAME
*     cull_hash_free_descr() -- free the hash contents of a cull descriptor
*
*  SYNOPSIS
*     void cull_hash_free_descr(lDescr *descr) 
*
*  FUNCTION
*     Frees the memory used by the hashing information in a cull descriptor 
*     (lDescr). If a hash table is still associated to the descriptor, it
*     is also deleted.
*
*  INPUTS
*     lDescr *descr - descriptor to free 
*
*  SEE ALSO
*     cull/hash/cull_hash_delete_non_unique()
*     uti/hash/HashTableDestroy()
*******************************************************************************/
void cull_hash_free_descr(lDescr *descr)
{
   int i;
   for(i = 0; descr[i].mt != lEndT; i++) {
      if(descr[i].hash != NULL) {
         if(descr[i].hash->table != NULL) {
            if(descr[i].hash->unique == 0) {
               /* delete chain of non unique elements */
               HashTableForEach(descr[i].hash->table, cull_hash_delete_non_unique_chain);
            }
            HashTableDestroy(descr[i].hash->table);
         }
         free(descr[i].hash);
         descr[i].hash = NULL;
      }
   }
}


