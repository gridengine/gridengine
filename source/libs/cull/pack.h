#ifndef __PACK_H
#define __PACK_H
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



#ifndef __BASIS_TYPES_H
#   include "basis_types.h"
#endif

#ifdef COMMCOMPRESS
#include "zlib.h"
#endif

#ifdef  __cplusplus
extern "C" {
#endif

#define CHUNK  (1024*1024)

typedef struct {
#ifdef COMMCOMPRESS
      z_stream cpr;
#endif
      char *head_ptr;
      char *cur_ptr; 
      size_t mem_size;
      size_t bytes_used;
      int just_count;
      int  mode;  /* 0 for compression, 1 for decompression, -1 for neither */
} sge_pack_buffer;   

int init_packbuffer(sge_pack_buffer *pb, int initial_size, int just_count);     

int init_packbuffer_from_buffer(sge_pack_buffer *pb, char *buf, u_long32 buflen, int compressed);
void clear_packbuffer(sge_pack_buffer *pb);
int flush_packbuffer(sge_pack_buffer *pb);

int pb_filled(sge_pack_buffer *pb);
int pb_unused(sge_pack_buffer *pb);
int pb_used(sge_pack_buffer *pb);  

int pack_set_chunk(int sz);
int pack_get_chunk(void);  

int packint(register sge_pack_buffer *, register u_long32);
int packdouble(register sge_pack_buffer *, double);
int packstr(register sge_pack_buffer *, register const char *);
int packbuf(sge_pack_buffer *, char *, u_long32);

int unpackint(register sge_pack_buffer *, register u_long32 *);
int unpackdouble(register sge_pack_buffer *, register double *);
int unpackstr(register sge_pack_buffer *, register char **);
int unpackbuf(sge_pack_buffer *, char **, int);

void debugpack(int on_off); 

/*
   these return values should be supported by all
   un-/packing functions
*/
enum {
   PACK_SUCCESS = 0,
   PACK_ENOMEM = -1,
   PACK_FORMAT = -2,
   PACK_BADARG = -3 
};

#ifdef  __cplusplus
}
#endif

#endif /* __PACK_H */



