#include <stdio.h>
#include <stdlib.h>

#define __SGE_GDI_LIBRARY_HOME_OBJECT_FILE__
#include "cull.h"

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
   SGE_HOST   (TEST_host,              CULL_DEFAULT)
   SGE_STRING (TEST_string,            CULL_DEFAULT)
   SGE_DOUBLE (TEST_double,            CULL_DEFAULT)
   SGE_ULONG  (TEST_ulong,             CULL_DEFAULT)
   SGE_BOOL   (TEST_bool,              CULL_DEFAULT)
   SGE_LIST   (TEST_list, TEST_Type,   CULL_DEFAULT)
   SGE_OBJECT (TEST_object, TEST_Type, CULL_DEFAULT)
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
   lListElem *ep, *obj, *copy;
   sge_pack_buffer pb, copy_pb;
   int pack_ret;
   FILE *fd;

   lInit(nmv);

   /* create an element */
   ep = lCreateElem(TEST_Type);
   obj = lCreateElem(TEST_Type);

   /* test field access functions */
   lSetHost(ep, TEST_host, "test_host");
   lSetString(ep, TEST_string, "test_string");
   lSetDouble(ep, TEST_double, 3.1);
   lSetUlong(ep, TEST_ulong, 3);
   lSetBool(ep, TEST_bool, TRUE);
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

   if((pack_ret = init_packbuffer_from_buffer(&copy_pb, (char *)pb.head_ptr, pb.bytes_used, 0)) != PACK_SUCCESS) {
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
   copy = lFreeElem(copy);

   /* test lDump functions */
   if(lDumpElem("test_cull_pack.txt", ep, 1)) {
      printf("error dumping element\n");
      return EXIT_FAILURE;
   }

   if((fd = fopen("test_cull_pack.txt", "r")) == NULL) {
      printf("error opening dump file test_cull_pack.txt\n");
      return EXIT_FAILURE;
   }

   if((copy = lUndumpElem(fd, TEST_Type)) == NULL) {
      fclose(fd);
      printf("error undumping element\n");
      return EXIT_FAILURE;
   }
   fclose(fd);
   printf("element after dumping and undumping\n");
   lWriteElemTo(copy, stdout);
   copy = lFreeElem(copy);

   /* cleanup and exit */
   lFreeElem(ep);                  
   return EXIT_SUCCESS;
}


