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
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#ifndef WIN32NATIVE
#   include <netinet/in.h>
#else
#   include <winsock2.h>
#endif

/* do not compile in monitoring code */
#ifndef NO_SGE_COMPILE_DEBUG
#   define NO_SGE_COMPILE_DEBUG
#endif

#ifndef WIN32
#   include <rpc/rpc.h>
#   include <rpc/types.h>
#endif

#ifdef WIN32
#   ifndef WIN32NATIVE
#      include <netdb.h>
#   endif
#endif



#ifndef __BASIS_TYPES_H
#   include "basis_types.h"
#endif

#ifndef __SGERMON_H
#   include "sgermon.h"
#endif

#include "cull_listP.h"
#include "sge_log.h"
#include "pack.h"
#include "msg_cull.h"
#if 0
#   undef PACK_LAYER
#   define PACK_LAYER BASIS_LAYER
#endif


/****** cull/pack/--CULL_Packing ***************************************
*
*  NAME
*     CULL_Packing -- platform independent exchange format
*
*  FUNCTION
*     The cull packing functions provide a framework for a 
*     platform independent data representation.
*
*     Data is written into a packbuffer. Individual data words
*     are written in network byte order.
*
*     Data in the packbuffer can be compressed.
*
*  NOTES
*     Other platform independent formats, like XML, should be 
*     implemented.
*
*  SEE ALSO
*     cull/pack/-Versioncontrol
*
*     cull/pack/packbitfield()
*     cull/pack/unpackbitfield()
****************************************************************************
*/

#ifdef COMMCOMPRESS
#include "zlib.h"
static int sge_compress(sge_pack_buffer *pb, unsigned char *buf, size_t len);
static int sge_resize_packbuffer(sge_pack_buffer *pb);
#endif

/* compression_level is set to getenv("SGE_COMPRESSION_LEVEL") by
 * sge_setup(). must be from 0 (no compression) to 9 (best compression)
 * only buffers > compression_threshold are compressed. must be indicated
 * by a flag in the message header when sent. can be overridden at run-time
 * compression_threshold is set to getenv("SGE_COMPRESSION_THRESHOLD") by
 * sge_setup()
 */
/* long compression_level = Z_NO_COMPRESSION; */
long compression_level = 0;
long compression_threshold = 10 * 1024;

/* -------------------------------

   set chunk_size

   returns chunk_size used before

 */
int pack_set_chunk(
int sz 
) {
   int oldsz = get_cull_state_chunk_size();
   
   set_cull_state_chunk_size(sz);

   return oldsz;
}

/* -------------------------------

   get chunk_size

 */
int pack_get_chunk(void)
{
   return get_cull_state_chunk_size();
}

#ifdef COMMCOMPRESS
/* -------------------------------
 * sge_resize_packbuffer
 * adjust the packbuffer if it gets
 * too small
 */
static int sge_resize_packbuffer(
sge_pack_buffer *pb 
) {
   float ratio = (float)pb->cpr.total_out/(float)pb->cpr.total_in;
   /* assume a worse packing ratio. factor 1.5 is hopefully ok */
   float factor = 1+ratio*1.5;
  
   DENTER(PACK_LAYER, "sge_resize_packbuffer");
   DPRINTF(("Resizing pack buffer from %d to %d\n", pb->cpr.total_out,
                                 (int)(pb->cpr.total_out * factor)));
   DPRINTF(("Original buffer would be %d, compression ratio is %f\n",
                                 pb->bytes_used, ratio));
   pb->head_ptr = (char*) realloc(pb->head_ptr, (int)(pb->cpr.total_out * factor));
   pb->cpr.next_out = (unsigned char*)pb->head_ptr + pb->cpr.total_out;
   if(!pb->head_ptr) {
      DEXIT;
      return PACK_ENOMEM;
   }
   pb->cpr.avail_out = ((int)(pb->cpr.total_out * factor)) - pb->cpr.total_out;
   DEXIT;
   return PACK_SUCCESS;
}

/* -------------------------------
 * sge_compress
 * compress the given buffer into 
 * a pb->cpr
 */
static int sge_compress(
sge_pack_buffer *pb,
unsigned char *buf,
size_t len 
) {
   int ret;

   DENTER(PACK_LAYER, "sge_compress");

   pb->cpr.next_in = buf;
   pb->cpr.avail_in = len;
   while((ret = deflate(&(pb->cpr), Z_NO_FLUSH)) == Z_OK && pb->cpr.avail_out == 0)
      if((ret = sge_resize_packbuffer(pb)) != PACK_SUCCESS)
         break;

   DEXIT;
   return ret;
}
#define SGE_COMPRESS(pb, buf, len) \
      if(sge_compress(pb, buf, len)) { \
         DEXIT; \
         return PACK_ENOMEM; \
      }
#endif

/****** cull/pack/init_packbuffer() *************************************************
*  NAME
*     init_packbuffer() -- initialize packing buffer
*
*  SYNOPSIS
*     int init_packbuffer(sge_pack_buffer *pb, int initial_size, 
*                         int just_count) 
*
*  FUNCTION
*     Initialize a packing buffer.
*     Allocates the necessary memory. If more memory is needed during the use
*     of the packbuffer, it will be reallocated increasing the size by 
*     chunk_size (see function pack_set_chunk).
*
*     Since version 6.0, version information is provided in the packbuffer and 
*     is included in sent messages.
*     For best possible backward interoperability with former versions, an 
*     integer with value 0 is padded before the version information as first 
*     word in the packbuffer. This triggeres error handling in former versions.
*
*  INPUTS
*     sge_pack_buffer *pb - the packbuffer to initialize
*     int initial_size    - the amount of memory to be allocated at 
*                           initialization.
*                           If a value of 0 is given as initial_size, a size 
*                           of chunk_size (global variable, see function 
*                           pack_set_chunk) will be used.
*     int just_count      - if true, no memory will be allocated and the 
*                           "just_count" property of the packbuffer will 
*                           be set.
*
*  RESULT
*     int - PACK_SUCCESS on success
*           PACK_ENOMEM  if memory allocation fails
*           PACK_FORMAT  if no valid packbuffer is passed
*
*  SEE ALSO
*     cull/pack/-Packing-typedefs
*     cull/pack/pack_set_chunk()
*******************************************************************************/
int init_packbuffer(
sge_pack_buffer *pb,
int initial_size,
int just_count 
) {
   DENTER(PACK_LAYER, "init_packbuffer");
#if 0
   if(!just_count) {
      srand(time(0));
      if((rand() % 5) == 0) {
         ERROR((SGE_EVENT, MSG_CULL_NOTENOUGHMEMORY_D, initial_size));
         DEXIT;
         return PACK_ENOMEM;
      }
   }   
#endif

   if (!pb) {
      ERROR((SGE_EVENT, MSG_CULL_ERRORININITPACKBUFFER_S, MSG_CULL_PACK_FORMAT));
      DEXIT;
      return PACK_FORMAT;
   }   


   if (!just_count) {
      if (initial_size == 0) {
         initial_size = get_cull_state_chunk_size();
      } else {
         initial_size += 2 * INTSIZE;  /* space for version information */
      }
      
      memset(pb, 0, sizeof(sge_pack_buffer));
#ifdef COMMCOMPRESS
      if(initial_size > compression_threshold
            && compression_level != 0) {
         /* buffers smaller than 1k are set to 512 bytes by default */
         /* until 5k assume a pack ratio of 0.4 */
         /* until 100k take 0.2, for  the rest 0.06 */
         /* these values work fine for all compression levels */
         /* one could find a better approximative function to optimize */
         /* memory consumption, but I don't know if it's worth the work */
         pb->cpr.avail_out = (initial_size<1024)?512:(
                                (initial_size<5000)?initial_size*0.5:(
                                   (initial_size<100000)?initial_size*0.3:(
                                      initial_size*0.1
                                   )
                                )
                             );
         pb->head_ptr = (char*)malloc(pb->cpr.avail_out);
         pb->cpr.next_out = (unsigned char*)pb->head_ptr;
         if(!pb->head_ptr) {
            ERROR((SGE_EVENT, MSG_CULL_NOTENOUGHMEMORY_D, pb->cpr.avail_out));
            DEXIT;
            return PACK_ENOMEM;
         }
         if(deflateInit(&(pb->cpr), (int)compression_level)) {
            ERROR((SGE_EVENT, MSG_CULL_NOTENOUGHMEMORY_D, pb->cpr.avail_out));
            DEXIT;
            return PACK_ENOMEM;
         }
         pb->mode = 0;    /* this indicates compression */
      } else
#endif
      {
         pb->head_ptr = malloc(initial_size);
         if (!pb->head_ptr) {
            ERROR((SGE_EVENT, MSG_CULL_NOTENOUGHMEMORY_D, initial_size));
            DEXIT;
            return PACK_ENOMEM;
         }
         pb->cur_ptr = pb->head_ptr;
         pb->mem_size = initial_size;
         pb->mode = -1;    /* this indicates no compression/decompression */
      }

      pb->bytes_used = 0;
      pb->just_count = 0;
      pb->version = CULL_VERSION;
      packint(pb, 0);              /* pad 0 byte -> error handing in former versions */
      packint(pb, pb->version);    /* version information is included in buffer      */
   } else {
      pb->head_ptr = pb->cur_ptr = NULL;
      pb->mem_size = 0;
      pb->mode = 0;    /* this indicates compression */
      pb->bytes_used = 0;
      pb->just_count = 1;
#ifdef COMMCOMPRESS
      pb->head_ptr = NULL;
#endif
   }

   DEXIT;
   return PACK_SUCCESS;
}

/**************************************************************
 initialize packing buffer out of a normal character buffer
 set read/write pointer to the beginning
 be sure to only call this function for unpacking when 
 compression is turned on.
 **************************************************************/
int init_packbuffer_from_buffer(
sge_pack_buffer *pb,
char *buf,
u_long32 buflen,
int compressed 
) {
   DENTER(PACK_LAYER, "init_packbuffer_from_buffer");

   if (!pb && !buf) {
      DEXIT;
      return PACK_FORMAT;
   }   
#ifndef COMMCOMPRESS
   /* don't understand compressed buffers */
   if(compressed) {
      DEXIT;
      return PACK_FORMAT;
   }   
#endif

   memset(pb, 0, sizeof(sge_pack_buffer));
#ifdef COMMCOMPRESS
   if(compressed) {
      if(inflateInit(&(pb->cpr))) {
         DEXIT;
         return PACK_ENOMEM;
      }   
      pb->cpr.avail_in = buflen;
      pb->cpr.next_in = (unsigned char*) buf;
   }
#endif

   pb->head_ptr = buf;
   pb->cur_ptr = buf;
   pb->mem_size = buflen;
   pb->bytes_used = 0;
   pb->mode = compressed?1:-1;    /* must decompress or not ? */

   /* check cull version (only if buffer contains any data) */
   if(buflen > 0) {
      int ret;
      u_long32 pad, version;

      if((ret = unpackint(pb, &pad)) != PACK_SUCCESS) {
         DEXIT;
         return ret;
      } 

      if((ret = unpackint(pb, &version)) != PACK_SUCCESS) {
         DEXIT;
         return ret;
      }

      if(pad != 0 || version != CULL_VERSION) {
         ERROR((SGE_EVENT, MSG_CULL_PACK_WRONG_VERSION_XX, version, CULL_VERSION));
         DEXIT;
         return PACK_VERSION;
      }

      pb->version = version;
   } else {
      pb->version = CULL_VERSION;
   }

   DEXIT;
   return PACK_SUCCESS;
}

/**************************************************************/
int flush_packbuffer(
sge_pack_buffer *pb 
) {
#ifdef COMMCOMPRESS
   int ret;

   DENTER(PACK_LAYER, "flush_packbuffer");

   /* nothing to do if we're not compressing at all... */
   if(pb->mode != 0)
      return PACK_SUCCESS;

   while((ret = deflate(&(pb->cpr), Z_FINISH)) != Z_STREAM_END)
      if(ret != Z_OK) {
         DEXIT;
         return PACK_ENOMEM;
      }
      else if(pb->cpr.avail_out == 0 && (ret = sge_resize_packbuffer(pb) != PACK_SUCCESS)) {
         DEXIT;
         return ret;
      }

   DPRINTF(("Reduced buffer from %d to %d\n", pb->bytes_used, pb->cpr.total_out));
   DPRINTF(("%d bytes left in buffer, compression ratio is %f\n", pb->cpr.avail_out, (float)pb->cpr.total_out/(float)pb->cpr.total_in));

   DEXIT;
#endif
   return PACK_SUCCESS;
}

/**************************************************************/
void clear_packbuffer(
sge_pack_buffer *pb 
) {
   if (!pb)
      return;

#ifdef COMMCOMPRESS
   if (pb->head_ptr) {
      free(pb->head_ptr);
      pb->head_ptr = NULL;
      if(pb->mode == 1)
         inflateEnd(&(pb->cpr));
      else if(pb->mode == 0 && !pb->just_count)
         deflateEnd(&(pb->cpr));
   }
#else   
   if (pb->head_ptr) {
      free(pb->head_ptr);
      pb->head_ptr = NULL;
   }
#endif
   return;
}

/*************************************************************
 look whether pb is filled
 *************************************************************/
int pb_filled(
sge_pack_buffer *pb 
) {
#if 0
#ifdef COMMCOMPRESS
   if(pb->mode != -1 ) {
      if(!pb->head_ptr)
         return 0;
      return (pb->cpr.next_out != (unsigned char*)pb->head_ptr);
   }
   else
#endif
   {
      if (!pb->head_ptr)
         return 0;
      return (pb->cur_ptr != pb->head_ptr);
   }
#endif

   /* do we have more bytes used than the version information? */
   return (pb_used(pb) > (2 * INTSIZE));
}

/*************************************************************
 look for the number of bytes that are unused
 i.e. that are not yet consumed
 *************************************************************/
int pb_unused(
sge_pack_buffer *pb 
) {
#ifdef COMMCOMPRESS
   if(pb->mode != -1)
      return pb->cpr.avail_in;
   else
#endif
      return (pb->mem_size - pb->bytes_used);
}

/*************************************************************
 look for the number of bytes that are used
 *************************************************************/
int pb_used(
sge_pack_buffer *pb 
) {
   return pb->bytes_used;
}

/* --------------------------------------------------------- */
/*                                                           */
/*            low level functions for packing                */
/*                                                           */
/* --------------------------------------------------------- */

/*
   return values:
   PACK_SUCCESS
   PACK_ENOMEM
   PACK_FORMAT
 */
int packint(
register sge_pack_buffer *pb,
register u_long32 i 
) {
   u_long32 J=0;

   DENTER(PACK_LAYER, "packint");

#ifdef COMMCOMPRESS
   if(pb->mode == 1) {
      CRITICAL((SGE_EVENT, MSG_CULL_CANTPACKINTTOUNPACKBUFF ));
      DEXIT;
      abort();
   }
#endif

   if (!pb->just_count) {
#ifdef COMMCOMPRESS
      if(pb->mode != -1) {
         J = htonl(i);
         SGE_COMPRESS(pb, (((unsigned char *) &J) + INTOFF), INTSIZE);
      }
      else
#endif
      {
         if (pb->bytes_used + INTSIZE > pb->mem_size) {
            DPRINTF(("realloc(%d + %d)\n", pb->mem_size, get_cull_state_chunk_size()));
            pb->mem_size += get_cull_state_chunk_size();
            pb->head_ptr = realloc(pb->head_ptr, pb->mem_size);
            if (!pb->head_ptr) {
               DEXIT;
               return PACK_ENOMEM;
            }
            pb->cur_ptr = pb->head_ptr + pb->bytes_used;
         }

         /* copy in packing buffer */
         J = htonl(i);
         memcpy(pb->cur_ptr, (((char *) &J) + INTOFF), INTSIZE);
         pb->cur_ptr += INTSIZE;
      }
   }
   pb->bytes_used += INTSIZE;

   DEXIT;
   return PACK_SUCCESS;
}

#define DOUBLESIZE 8

/*
   return values:
   PACK_SUCCESS
   PACK_ENOMEM
   PACK_FORMAT
 */
int packdouble(
register sge_pack_buffer *pb,
double d 
) {
/* CygWin does not know RPC u. XDR */
   char buf[32];
#ifndef WIN32                   /* var not needed */
   XDR xdrs;
   int doublesize;
#endif

   DENTER(PACK_LAYER, "packdouble");

#ifdef COMMCOMPRESS
   if(pb->mode == 1) {
      CRITICAL((SGE_EVENT, MSG_CULL_CANTPACKDOUBLETOUNPACKBUFF));
      DEXIT;
      abort();
   }
#endif

   if (!pb->just_count) {
      if (pb->mode == -1 && pb->bytes_used + DOUBLESIZE > pb->mem_size) {
         DPRINTF(("realloc(%d + %d)\n", pb->mem_size, get_cull_state_chunk_size()));
         pb->mem_size += get_cull_state_chunk_size();
         pb->head_ptr = realloc(pb->head_ptr, pb->mem_size);
         if (!pb->head_ptr) {
            DEXIT;
            return PACK_ENOMEM;
         }
         pb->cur_ptr = pb->head_ptr + pb->bytes_used;
      }

      /* copy in packing buffer */

#ifndef WIN32                   /* XDR not called */
      xdrmem_create(&xdrs, (caddr_t) buf, sizeof(buf), XDR_ENCODE);

      if (!(xdr_double(&xdrs, &d))) {
         DPRINTF(("error - XDR of double failed\n"));
         xdr_destroy(&xdrs);
         DEXIT;
         return PACK_FORMAT;
      }

      if ((doublesize = xdr_getpos(&xdrs)) != DOUBLESIZE) {
         DPRINTF(("error - size of XDRed double is %d\n", doublesize));
         xdr_destroy(&xdrs);
         DEXIT;
         return PACK_FORMAT;
      }

#ifdef COMMCOMPRESS
      if (pb->mode == 0) {
         SGE_COMPRESS(pb, (unsigned char*)buf, DOUBLESIZE);
      }
      else
#endif /* COMMCOMPRESS */
#endif /* WIN32 */
      {
         memcpy(pb->cur_ptr, buf, DOUBLESIZE);
   /* we have to increment the buffer even through WIN32 will not use it */
         pb->cur_ptr += DOUBLESIZE;
      }

#ifndef WIN32                   /* XDR not called */
      xdr_destroy(&xdrs);
#endif
   }
   pb->bytes_used += DOUBLESIZE;

   DEXIT;
   return PACK_SUCCESS;
}

/* ---------------------------------------------------------

   return values:
   PACK_SUCCESS
   PACK_ENOMEM
   PACK_FORMAT

 */
int packstr(
register sge_pack_buffer *pb,
register const char *str 
) {

   DENTER(PACK_LAYER, "packstr");

#ifdef COMMCOMPRESS
   if(pb->mode == 1) {
      CRITICAL((SGE_EVENT, MSG_CULL_CANTPACKSTRINGTOUNPACKBUFF ));
      DEXIT;
      abort();
   }
#endif

   /* determine string length */
   if (!str) {
      if (!pb->just_count) {
         /* is realloc necessary */
#ifdef COMMCOMPRESS
         if(pb->mode == 0)
            /* just pack length:=0 */
            packint(pb, 0);
         else
#endif
         {
            if ((pb->bytes_used + 1) > pb->mem_size) {

               /* realloc */
               DPRINTF(("realloc(%d + %d)\n", pb->mem_size, get_cull_state_chunk_size()));
               pb->mem_size += get_cull_state_chunk_size();
               pb->head_ptr = realloc(pb->head_ptr, pb->mem_size);
               if (!pb->head_ptr) {
                  DEXIT;
                  return PACK_ENOMEM;
               }

               /* update cur_ptr */
               pb->cur_ptr = pb->head_ptr + pb->bytes_used;
            }
            pb->cur_ptr[0] = '\0';
            /* update cur_ptr & bytes_packed */
            pb->cur_ptr++;
         }
      }
      pb->bytes_used++;
   }
   else {
      size_t n = strlen(str) + 1;

      if (!pb->just_count) {
         /* is realloc necessary */
#ifdef COMMCOMPRESS
         if(pb->mode == 0) {
            packint(pb, (int) n);
            SGE_COMPRESS(pb, (unsigned char*)str, n);
         }
         else
#endif
         {
            if ((pb->bytes_used + n) > pb->mem_size) {
               /* realloc */
               DPRINTF(("realloc(%d + %d)\n", pb->mem_size, get_cull_state_chunk_size()));
               while ((pb->bytes_used + n) > pb->mem_size)
                  pb->mem_size += get_cull_state_chunk_size();
               pb->head_ptr = realloc(pb->head_ptr, pb->mem_size);
               if (!pb->head_ptr) {
                  DEXIT;
                  return PACK_ENOMEM;
               }

               /* update cur_ptr */
               pb->cur_ptr = pb->head_ptr + pb->bytes_used;
            }
            memcpy(pb->cur_ptr, str, n);
            /* update cur_ptr & bytes_packed */
            pb->cur_ptr += n;

         }
      }
      pb->bytes_used += n;
   }

   DEXIT;
   return PACK_SUCCESS;
}


/****** cull/pack/packbitfield() ****************************************************
*  NAME
*     packbitfield() -- pack a bitfield 
*
*  SYNOPSIS
*     int packbitfield(sge_pack_buffer *pb, bitfield bitfield) 
*
*  FUNCTION
*     Writes the bitfield into the given packbuffer.
*     The following information will be written:
*        - the size of the bitfield in bits
*        - the bitfield itself as binary buffer
*
*  INPUTS
*     sge_pack_buffer *pb - the target packbuffer
*     bitfield bitfield   - the bitfield to pack
*
*  RESULT
*     int - PACK_SUCCESS on success,
*           else PACK_* error codes
*
*  SEE ALSO
*     uti/bitfield/--Bitfield
*     cull/pack/unpackbitfield()
*******************************************************************************/
int packbitfield(sge_pack_buffer *pb, bitfield bitfield)
{
   int ret;
   u_long32 char_size;

   DENTER(PACK_LAYER, "packbitfield");

   if((ret = packint(pb, bitfield->size)) != PACK_SUCCESS) {
      DEXIT;
      return ret;
   }
   
   char_size = bitfield->size / 8 + ((bitfield->size % 8) > 0 ? 1 : 0);
   if((ret = packbuf(pb, bitfield->bf, char_size)) != PACK_SUCCESS) {
      DEXIT;
      return ret;
   }

   DEXIT;
   return PACK_SUCCESS;
}

/* ---------------------------------------------------------

   return values:
   PACK_SUCCESS
   PACK_ENOMEM
   PACK_FORMAT

 */
int packbuf(
sge_pack_buffer *pb,
char *buf_ptr,
u_long32 buf_size 
) {

   DENTER(PACK_LAYER, "packbuf");

#ifdef COMMCOMPRESS
   if(pb->mode == 1) {
      CRITICAL((SGE_EVENT, MSG_CULL_CANTPACKBYTEARRAYTOUNPACKBUFF ));
      DEXIT;
      abort();
   }
#endif

   if (!pb->just_count) {
      /* is realloc necessary */
#ifdef COMMCOMPRESS
      if(pb->mode == 0) {
         SGE_COMPRESS(pb, (unsigned char*)buf_ptr, buf_size);
      }
      else
#endif
      {
#ifdef WIN32                    /* cast */
         if (buf_size + (u_long32) pb->bytes_used > (u_long32) pb->mem_size) {
#else
         if (buf_size + pb->bytes_used > pb->mem_size) {
#endif /* WIN32 */

            /* realloc */
            DPRINTF(("realloc(%d + %d)\n", pb->mem_size, get_cull_state_chunk_size()));
            pb->mem_size += get_cull_state_chunk_size();
            pb->head_ptr = realloc(pb->head_ptr, pb->mem_size);
            if (!(pb->head_ptr)) {
               DEXIT;
               return PACK_ENOMEM;
            }

            /* update cur_ptr */
            pb->cur_ptr = pb->head_ptr + pb->bytes_used;
         }

         /* copy in packing buffer */
         memcpy(pb->cur_ptr, buf_ptr, buf_size);
         /* update cur_ptr & bytes_packed */
         pb->cur_ptr += buf_size;
      }
   }
   pb->bytes_used += buf_size;

   DEXIT;
   return PACK_SUCCESS;
}


/* --------------------------------------------------------- */
/*                                                           */
/*            low level functions for unpacking              */
/*                                                           */
/* --------------------------------------------------------- */

/* ---------------------------------------------------------

   return values:
   PACK_SUCCESS
   (PACK_ENOMEM)
   PACK_FORMAT

 */
int unpackint(
register sge_pack_buffer *pb,
register u_long32 *ip 
) {
   DENTER(PACK_LAYER, "unpackint");

#ifdef COMMCOMPRESS
   if(pb->mode == 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_CANTUNPACKINTFROMPACKINGBUFF ));
      DEXIT;
      abort();
   }
#endif

#ifdef COMMCOMPRESS
   if(pb->mode == 1)
   {
      int ret;
      pb->cpr.next_out = ((unsigned char *) ip) + INTOFF;
      pb->cpr.avail_out = INTSIZE;
      if((ret = inflate(&(pb->cpr), Z_SYNC_FLUSH)) != Z_OK && ( ret == Z_STREAM_END && pb->bytes_used >= pb->cpr.total_out )) {
         /* either something has gone wrong ( != Z_OK )
          * or we want to read BEYOND the stream end */
         DEXIT;
         return PACK_FORMAT;
      }
      *ip = ntohl(*ip);
   }
   else
#endif
   {
      /* are there enough bytes ? */
      if (pb->bytes_used + INTSIZE > pb->mem_size) {
         *ip = 0;
         DEXIT;
         return PACK_FORMAT;
      }

      /* copy integer */
      memset(ip, 0, sizeof(u_long32));
      memcpy(((char *) ip) + INTOFF, pb->cur_ptr, INTSIZE);
      *ip = ntohl(*ip);

      /* update cur_ptr & bytes_unpacked */
      pb->cur_ptr += INTSIZE;
   }
   pb->bytes_used += INTSIZE;

   DEXIT;
   return PACK_SUCCESS;
}

/* ---------------------------------------------------------

   return values:
   PACK_SUCCESS
   (PACK_ENOMEM)
   PACK_FORMAT

 */
int unpackdouble(
register sge_pack_buffer *pb,
register double *dp 
) {

#ifndef WIN32                   /* var not needed */
   XDR xdrs;
   char buf[32];
#endif

   DENTER(PACK_LAYER, "unpackdouble");


#ifdef COMMCOMPRESS
   if(pb->mode == 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_CANTUNPACKDOUBLEFROMPACKINGBUFF ));
      DEXIT;
      abort();
   }
#endif

#ifdef COMMCOMPRESS
   if(pb->mode == 1)
   {
      int ret;
      pb->cpr.next_out = (unsigned char*)buf;
      pb->cpr.avail_out = DOUBLESIZE;
      if((ret = inflate(&(pb->cpr), Z_SYNC_FLUSH)) != Z_OK && ( ret == Z_STREAM_END && pb->bytes_used >= pb->cpr.total_out )) {
         /* either something has gone wrong ( != Z_OK )
          * or we want to read BEYOND the stream end */
         DEXIT;
         return PACK_FORMAT;
      }
      xdrmem_create(&xdrs, buf, DOUBLESIZE, XDR_DECODE);
      if (!(xdr_double(&xdrs, dp))) {
         *dp = 0;
         DPRINTF(("error unpacking XDRed double\n"));
         xdr_destroy(&xdrs);
         DEXIT;
         return PACK_FORMAT;
      }
   }
   else
#endif
   {
      /* are there enough bytes ? */
      if (pb->bytes_used + DOUBLESIZE > pb->mem_size) {
         *dp = 0;
         DEXIT;
         return PACK_FORMAT;
      }

      /* copy double */

      /* CygWin does not know RPC u. XDR */
#ifndef WIN32                   /* XDR not called */
      memcpy(buf, pb->cur_ptr, DOUBLESIZE);
      xdrmem_create(&xdrs, buf, DOUBLESIZE, XDR_DECODE);
      if (!(xdr_double(&xdrs, dp))) {
         *dp = 0;
         DPRINTF(("error unpacking XDRed double\n"));
         xdr_destroy(&xdrs);
         DEXIT;
         return PACK_FORMAT;
      }
#endif /* WIN32 */
      /* update cur_ptr & bytes_unpacked */
      pb->cur_ptr += DOUBLESIZE;
   }
   pb->bytes_used += DOUBLESIZE;

#ifndef WIN32                   /* XDR not called */
   xdr_destroy(&xdrs);
#endif /* WIN32 */

   DEXIT;
   return PACK_SUCCESS;
}

/* ---------------------------------------------------------

   return values:
   PACK_SUCCESS
   PACK_ENOMEM
   PACK_FORMAT
 */
int unpackstr(
register sge_pack_buffer *pb,
register char **str 
) {
   u_long32 n;

   DENTER(PACK_LAYER, "unpackstr");

#ifdef COMMCOMPRESS
   if(pb->mode == 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_CANTUNPACKSTRINGFROMPACKINGBUFF ));
      DEXIT;
      abort();
   }
#endif

   /* determine string length */
#ifdef COMMCOMPRESS
   if(pb->mode == 1) {
      if(unpackint(pb, &n) != PACK_SUCCESS) {
         DEXIT;
         return PACK_FORMAT;
      }
      if(!n) {
         *str = NULL;
         DEXIT;
         return PACK_SUCCESS;
      }
      else {
         int ret;
         *str = malloc(n+1);
         if (!*str) {
            DEXIT;
            return PACK_ENOMEM;
         }
         pb->cpr.next_out = (unsigned char*)*str;
         pb->cpr.avail_out = n;
         if((ret = inflate(&(pb->cpr), Z_SYNC_FLUSH)) != Z_OK && ( ret == Z_STREAM_END && pb->bytes_used >= pb->cpr.total_out )) {
            /* either something has gone wrong ( != Z_OK )
             * or we want to read BEYOND the stream end */
            DEXIT;
            return PACK_FORMAT;
         }
         (*str)[n] = '\0';
      }
   }
   else
#endif
   {
      if (!pb->cur_ptr[0]) {

         *str = NULL;

         /* update cur_ptr & bytes_unpacked */
         pb->cur_ptr++;
         pb->bytes_used++;

         /* are there enough bytes ? */
         if (pb->bytes_used > pb->mem_size) {
            DEXIT;
            return PACK_FORMAT;
         }

         DEXIT;
         return PACK_SUCCESS;
      }
      else {
         n = strlen(pb->cur_ptr) + 1;

         /* are there enough bytes ? */
         if (n + pb->bytes_used > pb->mem_size) {
            DEXIT;
            return PACK_FORMAT;
         }
         *str = strdup(pb->cur_ptr);
         if (!*str) {
            DEXIT;
            return PACK_ENOMEM;
         }
         /* update cur_ptr & bytes_unpacked */
         pb->bytes_used += n;
         pb->cur_ptr += n;
      }
   }

   DEXIT;
   return PACK_SUCCESS;
}

/* ---------------------------------------------------------

   return values:
   PACK_SUCCESS
   PACK_ENOMEM
   PACK_FORMAT
 */
int unpackbuf(
sge_pack_buffer *pb,
char **buf_ptr,
int buf_size 
) {

   DENTER(PACK_LAYER, "unpackbuf");

#ifdef COMMCOMPRESS
   if(pb->mode == 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_CANTUNPACKBYTEARRAYFROMPACKINGBUFF ));
      DEXIT;
      abort();
   }
#endif

   if (buf_size == 0) {
      DEXIT;
      return PACK_SUCCESS;
   }

   /* are there enough bytes ? */
   if (pb->mode == -1 && (buf_size + pb->bytes_used) > pb->mem_size) {
      DEXIT;
      return PACK_FORMAT;
   }

   /* copy buffer */
   *buf_ptr = malloc(buf_size);
   if (!*buf_ptr) {
      DEXIT;
      return PACK_ENOMEM;
   }
#ifdef COMMCOMPRESS
   if(pb->mode == 1) { 
      int ret;
      pb->cpr.next_out = (unsigned char*)*buf_ptr;
      pb->cpr.avail_out = buf_size;
      if((ret = inflate(&(pb->cpr), Z_SYNC_FLUSH)) != Z_OK && ( ret == Z_STREAM_END && pb->bytes_used >= pb->cpr.total_out )) {
         /* either something has gone wrong ( != Z_OK )
          * or we want to read BEYOND the stream end */
         DEXIT;
         return PACK_FORMAT;
      }
   }
   else
#endif
   {
      memcpy(*buf_ptr, pb->cur_ptr, buf_size);
      /* update cur_ptr & bytes_unpacked */
      pb->cur_ptr += buf_size;
   }
   pb->bytes_used += buf_size;

   DEXIT;
   return PACK_SUCCESS;
}

/****** cull/pack/unpackbitfield() ****************************************************
*  NAME
*     unpackbitfield() -- unpack a bitfield 
*
*  SYNOPSIS
*     int unpackbitfield(sge_pack_buffer *pb, bitfield *bitfield) 
*
*  FUNCTION
*     Unpacks a bitfield from a packbuffer.
*
*  INPUTS
*     sge_pack_buffer *pb - the source packbuffer
*     bitfield *bitfield  - used to return the unpacked bitfield
*
*  RESULT
*     int - PACK_SUCCESS on success,
*           else PACK_* error codes
*
*  SEE ALSO
*     uti/bitfield/--Bitfield
*     cull/pack/packbitfield()
*******************************************************************************/
int unpackbitfield(sge_pack_buffer *pb, bitfield *bitfield)
{
   int ret;
   u_long32 size, char_size;
   char *buffer;

   DENTER(PACK_LAYER, "unpackbitfield");

   /* unpack the size in bits */
   if((ret = unpackint(pb, &size)) != PACK_SUCCESS) {
      DEXIT;
      return ret;
   }

   /* create new bitfield */
   *bitfield = sge_bitfield_new(size);
   if(*bitfield == NULL) {
      DEXIT;
      return PACK_ENOMEM;
   }

   /* unpack contents of the bitfield */
   char_size = size / 8 + ((size % 8) > 0 ? 1 : 0);
   if((ret = unpackbuf(pb, &buffer, char_size)) != PACK_SUCCESS) {
      *bitfield = sge_bitfield_free(*bitfield);
      DEXIT;
      return ret;
   }
 
   /* copy and free buffer */
   memcpy((*bitfield)->bf, buffer, char_size);
   free(buffer);

   DEXIT;
   return PACK_SUCCESS;
}

const char *cull_pack_strerror(int errnum)
{
   switch(errnum) {
      case PACK_SUCCESS:
         return MSG_CULL_PACK_SUCCESS;
      case PACK_ENOMEM:
         return MSG_CULL_PACK_ENOMEM;
      case PACK_FORMAT:
         return MSG_CULL_PACK_FORMAT;
      case PACK_BADARG:
         return MSG_CULL_PACK_BADARG;
      case PACK_VERSION:
         return MSG_CULL_PACK_VERSION;
      default:
         /* JG: TODO: we should have some global error message for all strerror functions */
         return "";
   }
}

