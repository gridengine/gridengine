#ifndef __SGE_HASH_H
#define __SGE_HASH_H
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
 * Based on the code of David Flanagan's Xmt library
 */

#define True    1
#define False   0

typedef long bool;
typedef struct _HashTableRec *HashTable;

typedef void (*HashTableForEachProc)(
    HashTable, const void*, const void**
);

extern HashTable HashTableCreate(int size, const void *(*dup_func)(const void *), int (*hash_func)(const void *), int (*compare_func)(const void *, const void *));
extern void HashTableDestroy(HashTable ht);
extern void HashTableStore(HashTable ht, const void* key, const void* data);
extern bool HashTableLookup(HashTable ht, const void* key, const void** data);
extern void HashTableDelete(HashTable ht, const void* key);
extern void HashTableForEach(HashTable ht, HashTableForEachProc proc);

extern const char *HashStatistics(HashTable ht);

extern const void *DupFunc_u_long32(const void *key);
extern const void *DupFunc_string(const void *key);

extern int HashFunc_u_long32(const void *key);
extern int HashFunc_string(const void *key);

extern int HashCompare_u_long32(const void *a, const void *b);
extern int HashCompare_string(const void *a, const void *b);

#endif /* __SGE_HASH_H */
