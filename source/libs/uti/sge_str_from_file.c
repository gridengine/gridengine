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
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "sge_str_from_file.h"
#include "sgermon.h"
#include "sge_log.h"
#include "msg_utilib.h"
#include "sge_stat.h" 

#define FILE_CHUNK (100*1024)

/* read a file into a string */
char *str_from_file(
const char *fname,
int *len 
) {
   FILE *fp;
   SGE_STRUCT_STAT statbuf;
   int size, i;
   char *str;

   DENTER(CULL_LAYER, "str_from_file");

   if (SGE_STAT(fname, &statbuf))
      return NULL;

   size = statbuf.st_size;
   if (len)
      *len = size;

   if (!(fp = fopen(fname, "r"))) {
      ERROR((SGE_EVENT, MSG_FILE_FOPENFAILED_SS, fname, strerror(errno)));
      DEXIT;
      return NULL;
   }

   if (!(str = malloc(size+1))) {
      fclose(fp);
      DEXIT;
      return NULL;
   }

   /*
   ** With fread(..., size, 1, ...),
   ** Windows cannot read <size> bytes here, because in
   ** text mode the trailing ^Z is ignored.
   ** CRLF -> LF conversion reduces size even further.
   ** Therefore, the file has less than size-1 bytes if read
   ** in text (ascii) mode. 
   ** Correctly, fread returns 0, because 0 elements of
   ** size <size> were read.
   */
#ifdef WIN32 /* fread call and evaluation of return value is different */
   i = fread(str, 1, size, fp);
   if (i == 0) {
      free(str);
      fclose(fp);
      DEXIT;
      return NULL;
   }
   str[i] = '\0';    /* delimit this string */
   if (len) {
      *len = i;
   }
#else
   i = fread(str, size, 1, fp);
   if (i != 1) {
      ERROR((SGE_EVENT, MSG_FILE_FREADFAILED_SS, fname, strerror(errno)));
      free(str);
      fclose(fp);
      DEXIT;
      return NULL;
   }
   str[size] = '\0';    /* delimit this string */
#endif


   fclose(fp);
   DEXIT;
   return str;
}

/***************************/
/* read string from stream */
/***************************/
char *str_from_stream(FILE *fp, int *len)
{
   char *str, *new;
   int filled = 0;
   int malloced_len, i;
  
   DENTER(TOP_LAYER, "str_from_stream");   

   if (!(str = malloc(FILE_CHUNK))) {
      DEXIT;
      return NULL;
   }
   malloced_len = FILE_CHUNK;

   /* malloced_len-filled-1 cause we reserve space for \0 termination */
   while ((i = fread(&str[filled], 1, malloced_len-filled-1, fp)) > 0) {
      filled += i;
      if (malloced_len == filled) {
         new = realloc(str, malloced_len+FILE_CHUNK);
         if (!new) {
            free(str);
            DEXIT;
            return NULL;
         }
         str = new;
         malloced_len += FILE_CHUNK;
      }

      if (feof(fp)) {
         DPRINTF(("got EOF\n"));
         break;
      }
   }
   str[filled] = '\0';  /* NULL termination */
   *len = filled;

   DEXIT;
   return str;
}

/***************************************************/
int str2file(
char *str,
int len,
char *fname 
) {
   FILE *fp;

   DENTER(TOP_LAYER, "str2file");

   if (!(fp = fopen(fname, "w"))) {
      ERROR((SGE_EVENT, MSG_FILE_OPENFAILED_S , fname));
      DEXIT;
      return -1;
   } 
   if (!len)
      len = strlen(str);

   if (fwrite(str, len, 1, fp) != 1) {
      int old_errno = errno;
      ERROR((SGE_EVENT, MSG_FILE_WRITEBYTESFAILED_IS , 
         len, fname));
      fclose(fp);
      unlink(fname);
      errno = old_errno;
      DEXIT;
      return -1;
   }

   fclose(fp);
   DEXIT;
   return 0;
}
