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
/*
 * Based on David Flanagan's Xmt libary's Hash.c
 */

#include <stdlib.h>
#include <strings.h>

#include "sge_hash.h"

/****** uti/hash/--Introduction *************************************************
*  NAME
*     HashTables -- A Hashtable Implementation for Grid Engine 
*
*  SYNOPSIS
*     HashTable HashTableCreate(int size, int (*hash_func)(const void *), int (*compare_func)(const void *, const void *));
*     void HashTableDestroy(HashTable ht);
*     void HashTableStore(HashTable ht, const void* key, const void* data);
*     bool HashTableLookup(HashTable ht, const void* key, const void** data);
*     void HashTableDelete(HashTable ht, const void* key);
*     void HashTableForEach(HashTable ht, HashTableForEachProc proc);
*
*  FUNCTION
*     This module provides a hash table implementation for Grid Engine.
*
*     Hash tables are used to have very fast access to objects stored in 
*     data structures like linked lists without having to traverse the 
*     whole list when searching a specific element.
*
*     An element in a hash table is characterized by a unique key.
*
*     This hash table implementation dynamically adjusts the size of the
*     hash table when necessary.
*     
*******************************************************************************/
typedef struct _Bucket {        /* Stores one entry. */
    const void* key;
    const void* data;
    struct _Bucket *next;
} Bucket;

typedef struct _HashTableRec {
    Bucket **table;             /* Pointer to array of hash entries. */
    long size;                   /* log2 of the size */
    long mask;                   /* Current size of hash table minus 1. */
    long numentries;             /* Number of entries currently in table. */
    int (*hash_func)(const void *);              /* pointer to hash function */
    int (*compare_func)(const void *, const void *);   /* pointer to compare function */
} HashTableRec;

/****** uti/hash/ResizeTable() *************************************************
*  NAME
*     ResizeTable() -- Resize the hash table
*
*  SYNOPSIS
*     static void ResizeTable(register HashTable ht, bool grow) 
*
*  FUNCTION
*     Hash tables are dynamically resized if necessary.
*     If the number of elements in a has table becomes too big, the hash
*     algorithm can no longer provide efficient access to the stored objects.
*     On the other hand, storing only a few elements in a much too big
*     hash table wastes memory.
*     Therefore the whole table can be resized.
*     If the hash table has to grow, it is doubled in size.
*     If it has to be shrunk, it is halfed in size.
*
*     Resizing implies rehashing all stored objects.
*
*  INPUTS
*     register HashTable ht - the hashtable  to resize 
*     bool grow             - true  = double size of the table,
*                             false = shrink table to half the size
*
*******************************************************************************/
static void ResizeTable(register HashTable ht, bool grow)    
{
    Bucket **otable;
    int otablesize;
    register Bucket *bucket, *next, **head;
    register int i;

    otable = ht->table;
    otablesize =  1 << ht->size;

    if (grow) ht->size++;
    else if (ht->size > 2) ht->size--;
    else return;
    
    ht->table = (Bucket **) calloc(1<<ht->size, sizeof(Bucket *));
    ht->mask = (1<<ht->size) - 1;

    for(i=0; i < otablesize; i++) {
        for(bucket = otable[i]; bucket; bucket = next) {
            next = bucket->next;
            head = &(ht->table[ht->hash_func(bucket->key) & ht->mask]);
            bucket->next = *head;
            *head = bucket;
        }
    }
    free((char *) otable);
}

/****** uti/hash/HashTableCreate() *********************************************
*  NAME
*     HashTableCreate() -- Create a new hash table
*
*  SYNOPSIS
*     HashTable HashTableCreate(int size, int (*hash_func)(const void *),
*                               int (*compare_func)(const void *, const void *)
*
*  FUNCTION
*     Creates an empty hash table and initializes its data structures.
*
*  INPUTS
*     int size            - Initial table size will be 2^n 
*     int (*hash_func)    - pointer to hash function 
*     int (*compare_func) - pointer to compare function 
*
*  RESULT
*     HashTable - the created hash table
*
*  EXAMPLE
*     HashTable MyHashTable = HashTableCreate(5, HashFunc_u_long32, HashCompare_u_long32);
*
*******************************************************************************/
HashTable HashTableCreate(int size, int (*hash_func)(const void *), int (*compare_func)(const void *, const void *))
{
    HashTable ht = (HashTable) malloc(sizeof(HashTableRec));

    ht->size = size;
    ht->mask = (1<<size)-1;
    ht->table = (Bucket **)calloc(ht->mask + 1, sizeof(Bucket *));
    ht->numentries = 0;
    ht->hash_func = hash_func;
    ht->compare_func = compare_func;
    return ht;
}

/****** uti/hash/HashTableDestroy() ********************************************
*  NAME
*     HashTableDestroy() -- Destroy a hash table
*
*  SYNOPSIS
*     void HashTableDestroy(HashTable ht) 
*
*  FUNCTION
*     Destroys a hash table and frees all used memory.
*
*  INPUTS
*     HashTable ht - the hash table to destroy
*
*  NOTES
*     The objecs managed by the hash table are not destroyed and have to be
*     handled separately.
*
*******************************************************************************/
void HashTableDestroy(HashTable ht)
{
    register int i;
    register Bucket *bucket, *next;

    for(i=0; i < ht->mask+1; i++) {
        for (bucket = ht->table[i]; bucket; bucket = next) {
            next = bucket->next;
            free((char *)bucket);
        }
    }
    free((char *) ht->table);
    free((char *) ht);
}

/****** uti/hash/HashTableForEach() ********************************************
*  NAME
*     HashTableForEach() -- Apply an action on all elements
*
*  SYNOPSIS
*     void HashTableForEach(HashTable table, HashTableForEachProc proc) 
*
*  FUNCTION
*     Calls a certain function for all elements in a hash table.
*
*  INPUTS
*     HashTable table           - the hash table
*     HashTableForEachProc proc - function to call for each element 
*
*******************************************************************************/
void HashTableForEach(HashTable table, HashTableForEachProc proc)
{
    register int i;
    register Bucket *bucket;

    for(i=0; i < table->mask+1; i++) {
        for (bucket = table->table[i]; bucket; bucket = bucket->next)
            (*proc)(table, bucket->key, &bucket->data);
    }
}    

/****** uti/hash/HashTableStore() **********************************************
*  NAME
*     HashTableStore() -- Store a new element in a hash table
*
*  SYNOPSIS
*     void HashTableStore(HashTable table, const void* key, const void* data) 
*
*  FUNCTION
*     Stores a new element in a hash table.
*     If there already exists an element with the same key in the table, 
*     it will be replaced by the new element.
*
*     If the number of elements in the table exceeds the table size, the
*     hash table will be resized.
*
*  INPUTS
*     HashTable table  - table to hold the new element
*     const void* key  - unique key 
*     const void* data - data to store, usually a pointer to an object
*
*  SEE ALSO
*     uti/hash/ResizeTable()
*******************************************************************************/
void HashTableStore(HashTable table, const void* key, const void* data)
{
    Bucket **head;
    register Bucket *bucket;

    head = &(table->table[table->hash_func(key) & table->mask]);
    for (bucket = *head; bucket; bucket = bucket->next) {
        if(table->compare_func(bucket->key, key) == 0) {
            bucket->data = data;
            return;
        }
    }
    bucket = (Bucket *)malloc(sizeof(Bucket));
    bucket->key = key;
    bucket->data = data;
    bucket->next = *head;
    *head = bucket;
    table->numentries++;
    if (table->numentries > table->mask)
        ResizeTable(table, True);
}

/****** uti/hash/HashTableLookup() *********************************************
*  NAME
*     HashTableLookup() -- search for an element
*
*  SYNOPSIS
*     bool HashTableLookup(HashTable table, const void* key, const void** data) 
*
*  FUNCTION
*     Search for a certain object characterized by a unique key in the 
*     hash table.
*     If an element can be found, it is returned in data.
*
*  INPUTS
*     HashTable table   - the table to search
*     const void* key   - unique key to search
*     const void** data - object if found, else NULL
*
*  RESULT
*     bool - true, when an object was found, else false
*
*******************************************************************************/
bool HashTableLookup(HashTable table, const void* key, const void** data)
{
    register Bucket *bucket;

    for (bucket = table->table[table->hash_func(key) & table->mask]; bucket; bucket = bucket->next) {
        if(table->compare_func(bucket->key, key) == 0) {
            *data = (void *)bucket->data;
            return True;
        }
    }
    return False;
}

/****** uti/hash/HashTableDelete() *********************************************
*  NAME
*     HashTableDelete() -- Delete an element in a hash table
*
*  SYNOPSIS
*     void HashTableDelete(HashTable table, const void* key) 
*
*  FUNCTION
*     Deletes an element in a hash table.
*     If the number of elements falls below a certain threshold
*     (half the size of the hash table), the hash table is resized (shrunk).
*
*  INPUTS
*     HashTable table - hash table that contains the element
*     const void* key - key of the element to delete
*
*  NOTES
*     Only deletes the entry in the hash table. The object itself is not
*     deleted.
*
*  SEE ALSO
*     uti/hash/ResizeTable()
*******************************************************************************/
void HashTableDelete(HashTable table, const void* key)
{
    register Bucket *bucket, **prev;

    for (prev = &(table->table[table->hash_func(key) & table->mask]); (bucket = *prev); prev = &bucket->next) {
        if (table->compare_func(bucket->key, key) == 0) {
            *prev = bucket->next;
            free((char *) bucket);
            table->numentries--;
            if (table->numentries < (table->mask>>1))
                ResizeTable(table, False);
            return;
        }
    }
}

/****** uti/hash/-Hash-Functions() *******************************************
*  NAME
*     HashFunc_<type>() -- Hash functions for selected data types
*
*  SYNOPSIS
*     int HashFunc_<type>(const void *key) 
*
*  FUNCTION
*     Depending on the data type of the hash key, different hash functions
*     have to be used.
*     Which hash function to use has to be specified when creating a new
*     hash table.
*
*  INPUTS
*     const void *key - key for which to compute a hash value
*
*  RESULT
*     int - the hash value
*
*  NOTES
*     The following data types are provided with this module:
*        - strings (char *)
*        - u_long32
*
*  SEE ALSO
*     uti/hash/HashTableCreate()
*******************************************************************************/
int HashFunc_u_long32(const void *key) 
{
   return (long)key;
}

int HashFunc_string(const void *key)
{
   int hash = 0;
   const char *c = key;

   if(c != NULL) {
      do {
         hash += (hash << 3) + *c;
      } while (*c++ != 0);
   }

   return hash;
}   

/****** uti/hash/-Compare-Functions() *******************************************
*  NAME
*     CompareFunc_<type>() -- Compare functions for selected data types
*
*  SYNOPSIS
*     int HashCompare_<type>(const void *a, const void *b) 
*
*  FUNCTION
*     Depending on the data type of the hash key, different functions
*     have to be used to compare two elements.
*     Which compare function to use has to be specified when creating a new
*     hash table.
*     Syntax and return value are similar to strcmp.
*
*  INPUTS
*     const void *a - first element
*     const void *b - second element
*
*  RESULT
*     int - 0, if the elements are equal
*           > 0, if the first element is bigger than the second
*           < 0, if the first element is smaller than the second
*
*  NOTES
*     The following data types are provided with this module:
*        - strings (char *)
*        - u_long32
*
*  SEE ALSO
*     uti/hash/HashTableCreate()
*******************************************************************************/
int HashCompare_u_long32(const void *a, const void *b)
{
   return (long)a - (long)b;
}

int HashCompare_string(const void *a, const void *b)
{
   return strcmp((const char *)a, (const char *)b);
}
