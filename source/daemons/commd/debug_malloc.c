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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>

#define debug_malloc_itself 1
#include "debug_malloc.h"

static void error(char *errtxt, int level);

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef struct mem_block {
   void *ptr;
   unsigned size;
   char file[256];
   int line;
   char ident[256];
   struct mem_block *next;
} mem_block;

static mem_block *mem_blocks = NULL;
static void (*err_func) (char *error) = NULL;
static char err_file[256] = "";
static int loglevel = 1;
static char *first_alloc = (char *) 0xffffffff;         /* ptr to first known allocated address */
static char *last_alloc = NULL; /* ptr to last known allocated address */
static char *tp = NULL;         /* memory trace point */
int print_on_unknown_mem = 0;

/**************************************************/

void derr_func(func)
void func(char *error);
{
   err_func = func;
}

/**************************************************/

void derr_file(
char *fname 
) {
   strncpy(err_file, fname, sizeof(err_file));
   err_file[sizeof(err_file) - 1] = '\0';
}

/**************************************************/

void dlog_level(
int i 
) {
   loglevel = i;
}

/**************************************************/

static void error(
char *errtxt,
int level 
) {
   FILE *fp;

   if (level > loglevel)
      return;

   if (err_file[0]) {
      fp = fopen(err_file, "a");
      if (fp) {
         fprintf(fp, "MEMORY: %s\n", errtxt);
         fclose(fp);
      }
   }

   if (err_func)
      err_func(errtxt);
}

/**************************************************/
char *dmalloc(
unsigned size,
char *file,
int line,
char *ident 
) {
   char *ptr;
   mem_block *mbptr;
   char errtxt[256];
   int space;

   if (!tp)
      print_on_unknown_mem = (getenv("print_on_unknown_mem") != NULL);

   if ((space = dm_tracepoint()) && print_on_unknown_mem)
      printf("unknown memory alloc or free changed sbrk by %d\n", space);

   ptr = malloc(size);
   if (!ptr) {
      /* out of memory error */
      sprintf(errtxt, "malloc %d bytes failed", size);
      error(errtxt, 1);
      return ptr;
   }

   mbptr = (mem_block *) malloc(sizeof(mem_block));
   if (!mbptr) {
      /* out of memory error */
      free(ptr);
      sprintf(errtxt, "error malloc %d bytes failed", size);
      error(errtxt, 1);
      return NULL;
   }

   sprintf(errtxt, "malloc %p %d", ptr, size);
   error(errtxt, 9);

   mbptr->next = mem_blocks;
   mbptr->ptr = ptr;
   mbptr->size = size;

   if (ptr < first_alloc)
      first_alloc = ptr;

   if (ptr > last_alloc)
      last_alloc = ptr;

   if (file) {
      strncpy(mbptr->file, file, sizeof(mbptr->file));
      (mbptr->file)[sizeof(mbptr->file) - 1] = '\0';
   }
   else
      *(mbptr->file) = '\0';

   mbptr->line = line;
   if (ident) {
      strncpy(mbptr->ident, ident, sizeof(mbptr->ident));
      (mbptr->ident)[sizeof(mbptr->ident) - 1] = '\0';
   }
   else
      *(mbptr->ident) = '\0';
   mem_blocks = mbptr;

   dm_tracepoint();
   return ptr;
}

/**************************************************/
int dfree(
void *ptr 
) {
   mem_block *mbptr = mem_blocks, *last = NULL;
   char errtxt[256];
   int space;

   if (!tp)
      print_on_unknown_mem = (getenv("print_on_unknown_mem") != NULL);

   if ((space = dm_tracepoint()) && print_on_unknown_mem)
      printf("unknown memory alloc or free changed sbrk by %d\n", space);

   while (mbptr && (mbptr->ptr != ptr)) {
      last = mbptr;
      mbptr = mbptr->next;
   }

   if (mbptr) {

      sprintf(errtxt, "free ptr=%p size=%d file=%s line=%d ident=%s", ptr,
              mbptr->size, mbptr->file, mbptr->line, mbptr->ident);
      error(errtxt, 9);

      if (last)
         last->next = mbptr->next;
      else
         mem_blocks = mbptr->next;
      free(mbptr);
   }
   else {
      sprintf(errtxt, "error freeing not malloced ptr %p", ptr);
      error(errtxt, 1);
   }

   free(ptr);
   dm_tracepoint();
   return 0;
}

/**************************************************/
char *dstrdup(
char *str,
char *file,
int line,
char *ident 
) {
   char *ptr;
   mem_block *mbptr;
   char errtxt[256];
   unsigned size;
   int space;

   if (!tp)
      print_on_unknown_mem = (getenv("print_on_unknown_mem") != NULL);

   if ((space = dm_tracepoint()) && print_on_unknown_mem)
      printf("unknown memory alloc or free changed sbrk by %d\n", space);

   ptr = strdup(str);
   if (!ptr) {
      /* out of memory error */
      sprintf(errtxt, "strdup failed");
      error(errtxt, 1);
      return ptr;
   }

   mbptr = (mem_block *) malloc(sizeof(mem_block));
   if (!mbptr) {
      /* out of memory error */
      free(ptr);
      sprintf(errtxt, "error strdup failed");
      error(errtxt, 1);
      return NULL;
   }

   size = strlen(ptr);
   sprintf(errtxt, "strdup %p %d", ptr, size);
   error(errtxt, 9);

   mbptr->next = mem_blocks;
   mbptr->ptr = ptr;
   mbptr->size = size;

   if (ptr < first_alloc)
      first_alloc = ptr;

   if (ptr + size - 1 > last_alloc)
      last_alloc = ptr + size - 1;

   if (file) {
      strncpy(mbptr->file, file, sizeof(mbptr->file));
      (mbptr->file)[sizeof(mbptr->file) - 1] = '\0';
   }
   else
      *(mbptr->file) = '\0';

   mbptr->line = line;
   if (ident) {
      strncpy(mbptr->ident, ident, sizeof(mbptr->ident));
      (mbptr->ident)[sizeof(mbptr->ident) - 1] = '\0';
   }
   else
      *(mbptr->ident) = '\0';
   mem_blocks = mbptr;

   dm_tracepoint();
   return ptr;
}

/**************************************************/

void dstatus(
char *fname,
int memdump,
int tstalloc 
) {
   mem_block *mbptr = mem_blocks, *tmp_mbptr;
   FILE *fp;
   int i;
   char *cp;
   char *tmp_last_alloc, *ptr;
   unsigned long size;

   if (tstalloc) {              /* fill gaps with testblocks */
      tmp_last_alloc = last_alloc;

      size = last_alloc - first_alloc;
      while (size >= 1) {
         while ((ptr = dmalloc(size, "tst", 0, "tstmalloc")) < last_alloc);
         if (ptr)
            dfree(ptr);
         last_alloc = tmp_last_alloc;   /* this should not advance the upper limit */
         if (size > 8)
            size -= 8;
         else
            size -= 1;
      }
   }

   fp = fopen(fname, "w");
   if (!fp)
      return;

   fprintf(fp, "%p first known allocation\n", first_alloc);
   while (mbptr) {
      fprintf(fp, "%p %d %s %d %s\n",
        mbptr->ptr, mbptr->size, mbptr->file, mbptr->line, mbptr->ident);
      if (memdump) {
         i = mbptr->size;
         cp = mbptr->ptr;
         while (i--) {
            if (isprint(*cp))
               fprintf(fp, "%c", *cp);
            else
               fprintf(fp, "<%d>", *cp);
            cp++;
         }
         fprintf(fp, "\n");
      }
      if (!mbptr->line) {
         tmp_mbptr = mbptr->next;
         dfree(mbptr->ptr);
         mbptr = tmp_mbptr;
      }
      else
         mbptr = mbptr->next;
   }
   fprintf(fp, "%p last known allocation", last_alloc);

   fclose(fp);
}

/**************************************************************
 Used for tracing memory usage we do not know anything 
 about.
 Tracepoint returns 1 if there is a memorychange since our last
 Tracepoint we know nothing about.
 **************************************************************/
int dm_tracepoint()
{
   static char *oldtp;

   oldtp = tp;
   tp = sbrk(0);

   if (!oldtp)                  /* first call ? */
      return 0;
   return tp - oldtp;
}
