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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sge_copy_append.h"
#include "sgermon.h"
                     
#define FALSE      0
#define TRUE       1
#define BUFFER     4096

/*------------------------------------------------------
 * sge_copy_append
 * Append content from src to dst.
 *------------------------------------------------------*/
int sge_copy_append(src, dst, mode)
char *dst;
char *src;
int mode;
{
#define CPBUF 1024

 char buf[CPBUF];
 int fdsrc, fddst, error, modus, rs, ws;

 DENTER(TOP_LAYER, "sge_copy_append");
  
 if (src == NULL || dst == NULL || strlen(src) == 0 || strlen(dst) == 0 ||
    !(mode == SGE_APPEND || mode == SGE_COPY)) {
    DEXIT;
    return -1;   
 }
 if (!strcmp(src, dst)) {
    DEXIT;
    return -1;
 }   
 
 /* Return if source file doesn't exist */
 if ((fdsrc = open(src, O_RDONLY)) == -1) {
    DEXIT;
    return -1;
 }   
     
 if (mode == SGE_APPEND)
    modus = O_WRONLY | O_APPEND | O_CREAT;
 else
    modus = O_WRONLY | O_CREAT;      
    
 if ((fddst = open(dst, modus, 0666)) == -1) {
    DEXIT;
    return -1;
 }    
    
 error = FALSE;
 while (TRUE) {
    rs = read(fdsrc, buf, 512);
    if (rs == -1 && errno == EINTR)
       continue;
    else if (rs == -1)
       error = TRUE;
    
    if (!error && rs > 0) {      
       while (TRUE) {   
          ws = write(fddst, buf, rs);
          if (ws == -1 && errno == EINTR)   
             continue;
          else if (ws == -1) {
             error = TRUE;
             break;
          } 
          else
             break;
       }
    }
    if (error)
       break;
    if (rs == 0)
       break;   
 }         
    
 close(fdsrc);
 close(fddst);
 
 DEXIT;
 return (error ? -1: 0);
}


/*------------------------------------------------------
 * sge_bin2string
 * read binary stram from given file descriptor and
 * write it to (dynamically) malloced buffer as "ascii" format
 *
 * Ascii format means: '\0' is written as '\\' '0'
 *                     '\\' is written as '\\' '\\'  
 *
 * A '0' byte is written at the end of the buffer
 * 
 * maxsize is the size of the buffer
 * After reading the string is copied and the
 *------------------------------------------------------*/
char *sge_bin2string(
FILE *fp,
int size 
) {
   int i, fd;
   char inbuf[BUFFER], outbuf[2*BUFFER];
   char *inp, *outp;
   char *dstbuf;
   int len,             /* length of current tmp buffer */
       dstbuflen,       /* total length of destination buffer */
       chunksize,       /* chunks for realloc */
       lastpos,         /* last position in destination buffer */
       error;

   
   if ((fd = fileno(fp)) == -1)
      return NULL;

   chunksize = 20480;
   
   if (size <= 0)       /* no idea about buffer, malloc in chunks */
      size = chunksize;

   dstbuf = (char *) malloc(size+1);
   dstbuflen = size;
   lastpos = 0;

   error = FALSE;

   while (1) {
      i = read(fd, inbuf, BUFFER);
      if (i > 0) {
         inp = inbuf;
         outp = outbuf;
         while (inp < &inbuf[i]) {
            if (*inp == '\\') {
               *outp++ = '\\';
               *outp++ = '\\';
            }
            else if (*inp == '\0') {
               *outp++ = '\\';
               *outp++ = '0';
            }
            else
               *outp++ = *inp;
            inp++;
         }


         len = outp - outbuf;

         if (lastpos + len > dstbuflen) {
            if ((dstbuf = realloc(dstbuf, lastpos + len + chunksize)) == NULL) {
               error = TRUE;
               break;
            }   
            dstbuflen = lastpos + len + chunksize;

         }
         
         memcpy(&dstbuf[lastpos], outbuf, len);
         lastpos += len;

      }
      else if (i == 0) {
         break;
      }
      else {
         if (errno != EINTR) {
            error=TRUE;
            break;
         }
      }
   }

   if (error) {
      free(dstbuf);
      return NULL;
   }
   else {
      if ((dstbuf = realloc(dstbuf, lastpos+1)) == NULL)
         return NULL;
      dstbuf[lastpos] = '\0';
      return dstbuf;
   }
}

/*------------------------------------------------------
 * sge_string2bin
 * 
 *------------------------------------------------------*/
int sge_string2bin(
FILE *fp,
char *buf 
) {
   char outbuf[BUFFER];
   char *outp;
   int fd;
      
   if ((fd = fileno(fp)) == -1)
      return -1;
   
   if (!buf)
      return -1;
         
   while (*buf) {
      outp = outbuf;
      while (*buf && (outp - outbuf < BUFFER)) {
         if (*buf == '\\') {
            if (*(buf+1) == '\\')
               *outp++ = '\\';
            else
               *outp++ = '\0';
            buf+=2;    
         }   
         else
            *outp++ = *buf++;
      }
      
      if (write(fd, outbuf, outp - outbuf) != outp - outbuf)
         return -1;
   }      
   return 0;
}

/*-------------------------------------------------------*/
#ifdef TEST
main(int argc, char *argv[1])
{
 int ret;
 
 if (argc != 3) {
    printf("kopiere: brauche zwei Argumente\n");
    exit(1);
 }   
  
  printf("kopiere von %s nach %s\n", argv[1], argv[2]);
  ret = sge_copy_append(argv[1], argv[2], SGE_COPY);
  
  printf("kopiere status: %d\n", ret);
}
#endif
