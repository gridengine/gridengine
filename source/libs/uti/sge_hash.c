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
#include "sge_hash.h"

typedef struct _Bucket {        /* Stores one entry. */
    void* key;
    void* data;
    struct _Bucket *next;
} Bucket;

typedef struct _HashTableRec {
    Bucket **table;             /* Pointer to array of hash entries. */
    long size;                   /* log2 of the size */
    long mask;                   /* Current size of hash table minus 1. */
    long numentries;             /* Number of entries currently in table. */
} HashTableRec;


#define Hash(ht,key) \
    (ht)->table[((long)(key) >> 2) & (ht)->mask]

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
            head = &Hash(ht, bucket->key);
            bucket->next = *head;
            *head = bucket;
        }
    }
    free((char *) otable);
}

HashTable HashTableCreate(int size)
{
    HashTable ht = (HashTable) malloc(sizeof(HashTableRec));

    ht->size = size;
    ht->mask = (1<<size)-1;
    ht->table = (Bucket **)calloc(ht->mask + 1, sizeof(Bucket *));
    ht->numentries = 0;
    return ht;
}

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

void HashTableForEach(HashTable table, HashTableForEachProc proc)
{
    register int i;
    register Bucket *bucket;

    for(i=0; i < table->mask+1; i++) {
        for (bucket = table->table[i]; bucket; bucket = bucket->next)
            (*proc)(table, bucket->key, &bucket->data);
    }
}    

void HashTableStore(HashTable table, void* key, void* data)
{
    Bucket **head;
    register Bucket *bucket;

    head = &Hash(table, key);
    for (bucket = *head; bucket; bucket = bucket->next) {
        if (bucket->key == key) {
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

bool HashTableLookup(HashTable table, void* key, void** data)
{
    register Bucket *bucket;

    for (bucket = Hash(table, key); bucket; bucket = bucket->next)
    {
        if (bucket->key == key) {
            *data = bucket->data;
            return True;
        }
    }
    return False;
}

void HashTableDelete(HashTable table, void* key)
{
    register Bucket *bucket, **prev;

    for (prev = &Hash(table, key); (bucket = *prev); prev = &bucket->next) {
        if (bucket->key == key) {
            *prev = bucket->next;
            free((char *) bucket);
            table->numentries--;
            if (table->numentries < (table->mask>>1))
                ResizeTable(table, False);
            return;
        }
    }
}
