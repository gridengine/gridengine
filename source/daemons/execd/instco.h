#ifndef INSTCO_H
#define INSTCO_H
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

/* Macros. */

/* Scope control pseudo-keywords. */
#define public		/* public is C default scope	*/
#define private	static	/* static really means private	*/


/* Types. */
#ifdef FALSE
#undef FALSE
#endif
#ifdef TRUE
#undef TRUE
#endif

#ifndef FALSE
typedef enum { FALSE, TRUE } bool;	/* boolean */
#endif /* FALSE */

#if defined(CRAY) || defined(ALPHA) || defined(ALPHA4)
typedef long int64;	/* 64-bit integer */
typedef unsigned long uint64;	/* unsigned 64-bit integer */
#else
typedef long long int64;	/* 64-bit integer */
typedef unsigned long long uint64;	/* unsigned 64-bit integer */
#endif

#ifndef LONG_LONG_MAX
#define LONG_LONG_MAX	0x7FFFFFFFFFFFFFFFLL
#endif

#ifndef ULONG_LONG_MAX
#define ULONG_LONG_MAX	0xFFFFFFFFFFFFFFFFULL
#endif

/* Structures. */
	/* None. */


/* Functions. */
public int main(int argc, char *argv[]);
char *GetCwdName(void);
public char *i64toa(int64 v, char buf[]);
void HexDump(FILE *st, void *start, int count);
void LibError_(char *libfcn, char *srcfile, int lineno);
public int64 strtoi64(char *string, char **endptr, int base);
uint64 strtou64(char *string, char **endptr, int base);
uint64 strtou64x(char *string);
char *u64toa(uint64 v, char buf[]);

/* Function macros. */

/* Number of elements in array a. */
#define DIM(a) (sizeof(a)/sizeof(*(a)))

/* Print simple variables simply. */
#define PR(v)  fprintf(stderr, #v " = %d\n", (int)v)
#define PRL(v) fprintf(stderr, #v " = %ld\n", (long)v)
#define PR64(v) {char b[32]; u64toa((uint64)(v), b); fprintf(stderr, #v " = %s\n", b);}
#define PRS(v) fprintf(stderr, #v " = %s\n", (char *)v)

/* Macro form for LibError_() library function. */
#define LibError(libfcn) (LibError_(#libfcn, __FILE__, __LINE__))


/* Shared data. */
	/* None. */


#endif	/* INSTCO_H */
