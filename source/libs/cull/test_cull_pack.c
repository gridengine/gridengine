#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>

#define __SGE_GDI_LIBRARY_HOME_OBJECT_FILE__
#include "cull/cull.h"

#include "uti/sge_profiling.h"
#include "uti/sge_stdio.h"

#include "sgeobj/sge_jobL.h"

#include "msg_common.h"

enum {
   TEST_host = 1,
   TEST_string,
   TEST_double,
   TEST_ulong,
   TEST_bool,
   TEST_list,
   TEST_object,
   TEST_ref
};

LISTDEF(TEST_Type)
   SGE_HOST   (TEST_host,              CULL_DEFAULT | CULL_SPOOL)
   SGE_STRING (TEST_string,            CULL_DEFAULT)
   SGE_DOUBLE (TEST_double,            CULL_DEFAULT)
   SGE_ULONG  (TEST_ulong,             CULL_DEFAULT)
   SGE_BOOL   (TEST_bool,              CULL_DEFAULT)
   SGE_LIST   (TEST_list, TEST_Type,   CULL_DEFAULT | CULL_SPOOL)
   SGE_OBJECT (TEST_object, TEST_Type, CULL_DEFAULT | CULL_SPOOL)
   SGE_REF    (TEST_ref, TEST_Type,    CULL_DEFAULT)
LISTEND

NAMEDEF(TEST_Name)
   NAME("TEST_host")
   NAME("TEST_string")
   NAME("TEST_double")
   NAME("TEST_ulong")
   NAME("TEST_bool")
   NAME("TEST_list")
   NAME("TEST_object")
   NAME("TEST_ref")
NAMEEND   

#define TEST_Size sizeof(TEST_Name) / sizeof(char *)

lNameSpace nmv[] = {
   {1, TEST_Size, TEST_Name},
   {0, 0, NULL}
};

int main(int argc, char *argv[])
{
#if 0
   const char *const filename = "test_cull_pack.txt";
   lListElem *ep, *obj, *copy;
   sge_pack_buffer pb, copy_pb;
   int pack_ret;
   FILE *fd;
   char *buffer;

   lInit(nmv);
   sge_prof_setup();

   /* create an element */
   ep = lCreateElem(TEST_Type);
   obj = lCreateElem(TEST_Type);

   /* test field access functions */
   lSetHost(ep, TEST_host, "test_host");
   lSetString(ep, TEST_string, "test_string");
   lSetDouble(ep, TEST_double, 3.1);
   lSetUlong(ep, TEST_ulong, 3);
   lSetBool(ep, TEST_bool, true);
   lSetList(ep, TEST_list, NULL);

   lSetHost(obj, TEST_host, "test sub host");
   lSetString(obj, TEST_string, "test sub string");
   lSetObject(ep, TEST_object, obj);

   lSetRef(ep, TEST_ref, ep);
   printf("element after setting fields\n");
   lWriteElemTo(ep, stdout);

   /* test packing */
   if((pack_ret = init_packbuffer(&pb, 100, 0)) != PACK_SUCCESS) {
      printf("intializing packbuffer failed: %s\n", cull_pack_strerror(pack_ret));
      return EXIT_FAILURE;
   }

   if((pack_ret = cull_pack_elem(&pb, ep)) != PACK_SUCCESS) {
      printf("packing element failed: %s\n", cull_pack_strerror(pack_ret));
      return EXIT_FAILURE;
   }

   buffer = (char *)malloc(pb.bytes_used);
   memcpy(buffer, pb.head_ptr, pb.bytes_used);
   if((pack_ret = init_packbuffer_from_buffer(&copy_pb, buffer, pb.bytes_used)) != PACK_SUCCESS) {
      printf("initializing packbuffer from packed data failed: %s\n", cull_pack_strerror(pack_ret));
      return EXIT_FAILURE;
   }

   if((pack_ret = cull_unpack_elem(&copy_pb, &copy, TEST_Type)) != PACK_SUCCESS) {
      printf("unpacking element failed: %s\n", cull_pack_strerror(pack_ret));
      return EXIT_FAILURE;
   }
   clear_packbuffer(&pb);
   clear_packbuffer(&copy_pb);

   printf("element after packing and unpacking\n");
   lWriteElemTo(copy, stdout);
   lFreeElem(&copy);

   /* test partial packing */
   if((pack_ret = init_packbuffer(&pb, 100, 0)) != PACK_SUCCESS) {
      printf("intializing packbuffer failed: %s\n", cull_pack_strerror(pack_ret));
      return EXIT_FAILURE;
   }

   if((pack_ret = cull_pack_elem_partial(&pb, ep, NULL, CULL_SPOOL)) != PACK_SUCCESS) {
      printf("partially packing element failed: %s\n", cull_pack_strerror(pack_ret));
      return EXIT_FAILURE;
   }

   buffer = (char *)malloc(pb.bytes_used);
   memcpy(buffer, pb.head_ptr, pb.bytes_used);
   if((pack_ret = init_packbuffer_from_buffer(&copy_pb, buffer, pb.bytes_used)) != PACK_SUCCESS) {
      printf("initializing packbuffer from partially packed data failed: %s\n", cull_pack_strerror(pack_ret));
      return EXIT_FAILURE;
   }

   if((pack_ret = cull_unpack_elem_partial(&copy_pb, &copy, TEST_Type, CULL_SPOOL)) != PACK_SUCCESS) {
      printf("partially unpacking element failed: %s\n", cull_pack_strerror(pack_ret));
      return EXIT_FAILURE;
   }
   clear_packbuffer(&pb);
   clear_packbuffer(&copy_pb);

   printf("element after partial packing and unpacking\n");
   lWriteElemTo(copy, stdout);
   lFreeElem(&copy);

   /* test lDump functions */
   if(lDumpElem(filename, ep, 1)) {
      printf("error dumping element\n");
      return EXIT_FAILURE;
   }

   if((fd = fopen(filename, "r")) == NULL) {
      printf("error opening dump file test_cull_pack.txt\n");
      return EXIT_FAILURE;
   }

   if((copy = lUndumpElemFp(fd, TEST_Type)) == NULL) {
      FCLOSE(fd);
      printf("error undumping element\n");
      return EXIT_FAILURE;
   }
   FCLOSE(fd);
   printf("element after dumping and undumping\n");
   lWriteElemTo(copy, stdout);
   lFreeElem(&copy);

   /* cleanup and exit */
   lFreeElem(&ep);
   return EXIT_SUCCESS;
FCLOSE_ERROR:
   printf(MSG_FILE_ERRORCLOSEINGXY_SS, filename, strerror(errno));
   return EXIT_FAILURE;
#else

   #define LOOP 5000
   lListElem *ep;
   struct rusage before;
   struct rusage after;
   double time = 0;
   int i;

   sge_prof_setup();

   ep = lCreateElem(JB_Type);

   getrusage(RUSAGE_SELF, &before);
   {
      sge_pack_buffer pb;
      init_packbuffer(&pb, 0, 0);
      cull_pack_elem(&pb, ep);
      for (i=0; i < LOOP; i++) {
         sge_pack_buffer copy_pb;
         char *buffer;
         buffer = (char *)malloc(pb.bytes_used);
         memcpy(buffer, pb.head_ptr, pb.bytes_used);
         init_packbuffer_from_buffer(&copy_pb, buffer, pb.bytes_used);
         clear_packbuffer(&copy_pb);
      }
      clear_packbuffer(&pb);
   }
   getrusage(RUSAGE_SELF, &after);
   time = (after.ru_utime.tv_usec + after.ru_stime.tv_usec) - (before.ru_utime.tv_usec + before.ru_stime.tv_usec);
   time = (after.ru_utime.tv_sec + after.ru_stime.tv_sec) - (before.ru_utime.tv_sec + before.ru_stime.tv_sec) + (time/1000000);
   printf("memcpy took %.2fs\n", time);

   getrusage(RUSAGE_SELF, &before);
   for (i=0; i < LOOP; i++) {
      sge_pack_buffer pb;
      init_packbuffer(&pb, 0, 0);
      cull_pack_elem(&pb, ep);
      clear_packbuffer(&pb);
   }
   getrusage(RUSAGE_SELF, &after);
   time = (after.ru_utime.tv_usec + after.ru_stime.tv_usec) - (before.ru_utime.tv_usec + before.ru_stime.tv_usec);
   time = (after.ru_utime.tv_sec + after.ru_stime.tv_sec) - (before.ru_utime.tv_sec + before.ru_stime.tv_sec) + (time/1000000);
   printf("pack_elem took %.2fs\n", time);

   lFreeElem(&ep);

   return 0;
#endif
}


