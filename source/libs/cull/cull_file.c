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
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

/* do not compile in monitoring code */
#ifndef NO_SGE_COMPILE_DEBUG
#define NO_SGE_COMPILE_DEBUG
#endif

#include "sge_gdi_intern.h"
#include "cull_list.h"
#include "cull_lerrnoP.h"
#include "cull_listP.h"
#include "cull_multitypeP.h"
#include "cull_whatP.h"
#include "cull_whereP.h"
#include "cull_pack.h"
#include "cull_parse.h"
#include "cull_file.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_io.h"
#include "sge_unistd.h"

#include "msg_cull.h"

extern long compression_level;

/****
 **** lWriteElemToDisk
 ****
 **** Writes the Element `ep` to the file named `prefix`/`name`.
 **** either prefix or name can be null.
 **** Returns 1 on error, else 0.
 ****/
int lWriteElemToDisk(
const lListElem *ep,
const char *prefix,
const char *name,
const char *obj_name 
) {
   stringT filename;
   sge_pack_buffer pb;
   int ret, size, fd;
#ifdef COMMCOMPRESS
   long old_compression_level;
#endif

   DENTER(TOP_LAYER, "lWriteElemToDisk");

   if (!prefix && !name) {
      ERROR((SGE_EVENT, MSG_CULL_NOPREFIXANDNOFILENAMEINWRITEELMTODISK ));
      DEXIT;
      return 1;
   }

   /* init packing buffer */
   init_packbuffer(&pb, 0, 1);
   cull_pack_elem(&pb, ep);
   size = pb_used(&pb);
   clear_packbuffer(&pb);

#ifdef COMMCOMPRESS
   /* this is a lousy trick to forbid compression of the element */
   old_compression_level = compression_level;
   compression_level = 0;
   ret = init_packbuffer(&pb, size, 0);
   compression_level = old_compression_level;
#else
   ret = init_packbuffer(&pb, size, 0);
#endif

   /* pack ListElement */
   if(ret == PACK_SUCCESS) {
      ret = cull_pack_elem(&pb, ep);
   }

   switch (ret) {
   case PACK_SUCCESS:
      break;

   case PACK_ENOMEM:
      ERROR((SGE_EVENT, MSG_CULL_NOTENOUGHMEMORYFORPACKINGXY_SS ,
             obj_name, name ? name : "null"));
      clear_packbuffer(&pb);
      DEXIT;
      return 1;

   case PACK_FORMAT:
      ERROR((SGE_EVENT, MSG_CULL_FORMATERRORWHILEPACKINGXY_SS ,
             obj_name, name ? name : "null"));
      clear_packbuffer(&pb);
      DEXIT;
      return 1;

   default:
      ERROR((SGE_EVENT, MSG_CULL_UNEXPECTEDERRORWHILEPACKINGXY_SS ,
             obj_name, name ? name : "null"));
      clear_packbuffer(&pb);
      DEXIT;
      return 1;
   }

   /* create full file name */
   if (prefix && name) {
      sprintf(filename, "%s/%s", prefix, name);
   } else if (prefix) {
      sprintf(filename, "%s", prefix);
   } else {
      sprintf(filename, "%s", name);
   }

   /* open file */
   if ((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_CANTOPENXFORWRITINGOFYZ_SSS ,
                filename, obj_name, strerror(errno)));
      clear_packbuffer(&pb);
      DEXIT;
      return 1;
   }

   /* write packing buffer */
#ifdef COMMCOMPRESS
   if(pb.mode == 0) {
      if (flush_packbuffer(&pb) != PACK_SUCCESS || sge_writenbytes(fd, 
          (char*)pb.head_ptr, pb.cpr.total_out) != pb.cpr.total_out) {
         CRITICAL((SGE_EVENT, MSG_CULL_CANTWRITEXTOFILEY_SS , obj_name, 
                  filename));
         clear_packbuffer(&pb);
         close(fd);
         DEXIT;
         return 1;
      }
   }
   else
#endif
      if (sge_writenbytes(fd, pb.head_ptr, pb_used(&pb)) != pb_used(&pb)) {
         CRITICAL((SGE_EVENT, MSG_CULL_CANTWRITEXTOFILEY_SS , obj_name, 
                  filename));
         clear_packbuffer(&pb);
         close(fd);
         DEXIT;
         return 1;
      }

   /* close file and exit */
   close(fd);
   clear_packbuffer(&pb);

   DEXIT;
   return 0;
}

/****
 **** lReadElemFromDisk
 ****
 **** Reads a lListElem of the specified type from the
 **** file `prefix`/`name`.
 **** either prefix or name can be null.
 **** Returns pointer to the read Element or Null in 
 **** case of an error.
 ****/
lListElem *lReadElemFromDisk(
const char *prefix,
const char *name,
const lDescr *type,
const char *obj_name 
) {
   stringT filename;
   sge_pack_buffer pb;
   SGE_STRUCT_STAT statbuf;
   lListElem *ep;
   int ret, fd;
   void* buf;
   size_t size;

   DENTER(TOP_LAYER, "lReadElemFromDisk");

   if (!prefix && !name) {
      ERROR((SGE_EVENT,  MSG_CULL_NOPREFIXANDNOFILENAMEINREADELEMFROMDISK ));
      DEXIT;
      return NULL;
   }

   /* create full file name */
   if (prefix && name)
      sprintf(filename, "%s/%s", prefix, name);
   else if (prefix)
      sprintf(filename, "%s", prefix);
   else
      sprintf(filename, "%s", name);

   /* get file size */
   if (SGE_STAT(filename, &statbuf) == -1) {
      CRITICAL((SGE_EVENT, MSG_CULL_CANTGETFILESTATFORXFILEY_SS , obj_name, filename));
      DEXIT;
      return NULL;
   }

   if (!statbuf.st_size) {
      CRITICAL((SGE_EVENT, MSG_CULL_XFILEYHASZEROSIYE_SS , obj_name, filename));
      DEXIT;
      return NULL;
   }

   /* init packing buffer */
   size = statbuf.st_size;
   if (((SGE_OFF_T)size != statbuf.st_size)
       || !(buf = malloc(statbuf.st_size))) {
      CRITICAL((SGE_EVENT, MSG_CULL_LEMALLOC));
      clear_packbuffer(&pb);
      DEXIT;
      return NULL;
   }

   /* open file */
   if ((fd = open(filename, O_RDONLY)) < 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_CANTREADXFROMFILEY_SS , obj_name, filename));
      clear_packbuffer(&pb);    /* this one frees buf */
      DEXIT;
      return NULL;
   }

   /* read packing buffer */
   if (sge_readnbytes(fd, buf, statbuf.st_size) != statbuf.st_size) {
      CRITICAL((SGE_EVENT, MSG_CULL_ERRORREADINGXINFILEY_SS , obj_name, filename));
      close(fd);
      DEXIT;
      return NULL;
   }

   /* unpack lListElem, never compressed */
   init_packbuffer_from_buffer(&pb, buf, statbuf.st_size, 0);
   ret = cull_unpack_elem(&pb, &ep, type);
   close(fd);
   clear_packbuffer(&pb);     /* this one frees buf */

   switch (ret) {
   case PACK_SUCCESS:
      break;

   case PACK_ENOMEM:
      ERROR((SGE_EVENT, MSG_CULL_NOTENOUGHMEMORYFORUNPACKINGXY_SS ,
             obj_name, filename));
      DEXIT;
      return NULL;

   case PACK_FORMAT:
      ERROR((SGE_EVENT, MSG_CULL_FORMATERRORWHILEUNPACKINGXY_SS ,
             obj_name, filename));
      DEXIT;
      return NULL;

   case PACK_BADARG:
      ERROR((SGE_EVENT, MSG_CULL_BADARGUMENTWHILEUNPACKINGXY_SS ,
             obj_name, filename));
      DEXIT;
      return NULL;

   default:
      ERROR((SGE_EVENT, MSG_CULL_UNEXPECTEDERRORWHILEUNPACKINGXY_SS ,
             obj_name, filename));
      DEXIT;
      return NULL;
   }

   DEXIT;
   return ep;
}
