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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/* do not compile in monitoring code */
#ifndef NO_SGE_COMPILE_DEBUG
#define NO_SGE_COMPILE_DEBUG
#endif

#include "sge_log.h"
#include "sgermon.h"
#include "cull_listP.h"
#include "cull_list.h"
#include "cull_db.h"
#include "cull_parse.h"
#include "cull_multitype.h"
#include "cull_whatP.h"
#include "cull_lerrnoP.h"

#include "pack.h"
#include "cull_pack.h"
#include "cull_packL.h"
#include "msg_gdilib.h"

lListElem *lWhatToElem(const lEnumeration *what)
{
   lListElem *whatElem = NULL;
   sge_pack_buffer pb;
   int size;

   DENTER(CULL_LAYER, "lWhatToElem");
   /* 
    * retrieve packbuffer size to avoid large realloc's while packing 
    */
   init_packbuffer(&pb, 0, 1);
   if (cull_pack_enum(&pb, what) == PACK_SUCCESS) {
      size = pb_used(&pb);
      clear_packbuffer(&pb);

      /*
       * now we do the real packing
       */
      if (init_packbuffer(&pb, size, 0) == PACK_SUCCESS) {
         if (cull_pack_enum(&pb, what) == PACK_SUCCESS) {
            whatElem = lCreateElem(PACK_Type);
            lSetUlong(whatElem, PACK_id, SGE_WHAT);
#ifdef COMMCOMPRESS 
            if (pb.mode == 0) {
               if (flush_packbuffer(&pb) == PACK_SUCCESS){
                  setByteArray( (char*)pb.head_ptr,  pb.cpr.total_out, whereElem, PACK_String);
                  lSetBool(whatElem, PACK_Compressed, true);
               } else  {
                  whatElem = lFreeElem(whatElem);
               }
            } else
#endif
            {
               setByteArray( (char*)pb.head_ptr, pb.bytes_used, whatElem, PACK_string);
               lSetBool(whatElem, PACK_compressed, false);
            }
         }
      }
      clear_packbuffer(&pb); 
   }
   DEXIT;
   return whatElem;
}

lEnumeration *lWhatFromElem(const lListElem *what){
   lEnumeration *cond = NULL;
   sge_pack_buffer pb;
   int size=0;
   char *buffer;
   bool compressed = false;
   int ret=0;
   DENTER(CULL_LAYER, "lWhatFromCull");
   
   if (lGetUlong(what, PACK_id) == SGE_WHAT) {
      compressed = lGetBool(what, PACK_compressed);
      size = getByteArray(&buffer, what, PACK_string);
      if (size <= 0){
         ERROR((SGE_EVENT, MSG_PACK_INVALIDPACKDATA ));
      } else if ((ret = init_packbuffer_from_buffer(&pb, buffer, size, compressed)) == PACK_SUCCESS){
         cull_unpack_enum(&pb, &cond);
         clear_packbuffer(&pb); 
      } else {
         FREE(buffer);
         ERROR((SGE_EVENT, MSG_PACK_ERRORUNPACKING_S, cull_pack_strerror(ret)));
      }
   }
   else {
      ERROR((SGE_EVENT, MSG_PACK_WRONGPACKTYPE_UI, (long)lGetUlong(what, PACK_id), SGE_WHAT));
   }
   DEXIT;
   return cond;
}

