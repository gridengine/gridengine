#ifndef __DEBUG_MALLOC_H
#define __DEBUG_MALLOC_H
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
 *  License at http://www.gridengine.sunsource.net/license.html
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
/* install a function which is called when an error/log appears */
void derr_func(void func(char *error));

/* uses this file to report errors/logs */
void derr_file(char *fname);

/* called instead of malloc */
char *dmalloc(unsigned size, char *file, int line, char *ident);

/* called instead of free */
int dfree(void *ptr);

/* called instead of strdup */
char *dstrdup(char *str, char *file, int line, char *ident);

/* set logging level 1=only errors 9=everything default=1 */
void dlog_level(int i);

/* dump allocated blocks to file (memdump=1 -> show memory) */
void dstatus(char *fname, int memdump, int tstmalloc);

/* set memory tracepoint */
int dm_tracepoint(void);

#ifndef debug_malloc_itself
#ifdef DMALLOC

/* mapping defines */
#define malloc(a) dmalloc(a, __FILE__, __LINE__, "")
#define free(a) dfree(a)
#define strdup(a) dstrdup(a, __FILE__, __LINE__, "")

#endif
#endif
#endif /* __DEBUG_MALLOC_H */
