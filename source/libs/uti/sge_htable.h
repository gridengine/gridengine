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

#define True   1
#define False  0 

typedef struct _htable_rec *htable;

typedef void (*sge_htable_for_each_proc)(
    htable, const void*, const void**
);

extern htable sge_htable_create(int size, const void *(*dup_func)(const void *), int (*hash_func)(const void *), int (*compare_func)(const void *, const void *));
extern void sge_htable_destroy(htable ht);
extern void sge_htable_store(htable ht, const void* key, const void* data);
extern int sge_htable_lookup(htable ht, const void* key, const void** data);
extern void sge_htable_delete(htable ht, const void* key);
extern void sge_htable_for_each(htable ht, sge_htable_for_each_proc proc);

extern const char *sge_htable_statistics(htable ht);

extern const void *dup_func_u_long32(const void *key);
extern const void *dup_func_string(const void *key);

extern int hash_func_u_long32(const void *key);
extern int hash_func_string(const void *key);

extern int hash_compare_u_long32(const void *a, const void *b);
extern int hash_compare_string(const void *a, const void *b);

#endif /* __SGE_HASH_H */
