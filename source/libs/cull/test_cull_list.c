#include <stdio.h>
#include <stdlib.h>

#define __SGE_GDI_LIBRARY_HOME_OBJECT_FILE__
#include "cull.h"

enum {
   TEST_int = 1,
   TEST_host,
   TEST_string,
   TEST_float,
   TEST_double,
   TEST_char,
   TEST_long,
   TEST_ulong,
   TEST_bool,
   TEST_list,
   TEST_object,
   TEST_ref
};

LISTDEF(TEST_Type)
   SGE_INT    (TEST_int,               CULL_DEFAULT)
   SGE_HOST   (TEST_host,              CULL_DEFAULT)
   SGE_STRING (TEST_string,            CULL_DEFAULT)
   SGE_FLOAT  (TEST_float,             CULL_DEFAULT)
   SGE_DOUBLE (TEST_double,            CULL_DEFAULT)
   SGE_CHAR   (TEST_char,              CULL_DEFAULT)
   SGE_LONG   (TEST_long,              CULL_DEFAULT)
   SGE_ULONG  (TEST_ulong,             CULL_DEFAULT)
   SGE_BOOL   (TEST_bool,              CULL_DEFAULT)
   SGE_LIST   (TEST_list, TEST_Type,   CULL_DEFAULT)
   SGE_OBJECT (TEST_object, TEST_Type, CULL_DEFAULT)
   SGE_REF    (TEST_ref, TEST_Type,    CULL_DEFAULT)
LISTEND

NAMEDEF(TEST_Name)
   NAME("TEST_int")
   NAME("TEST_host")
   NAME("TEST_string")
   NAME("TEST_float")
   NAME("TEST_double")
   NAME("TEST_char")
   NAME("TEST_long")
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
   lEnumeration *enp;
   int index;

   lInit(nmv);

   /* create an element */
   ep = lCreateElem(TEST_Type);
   obj = lCreateElem(TEST_Type);

   /* test field access functions */
   lSetInt(ep, TEST_int, 1);
   lSetHost(ep, TEST_host, "test_host");
   lSetString(ep, TEST_string, "test_string");
   lSetFloat(ep, TEST_float, 2.0);
   lSetDouble(ep, TEST_double, 3.1);
   lSetChar(ep, TEST_char, 'A');
   lSetLong(ep, TEST_long, 2);
   lSetUlong(ep, TEST_ulong, 3);
   lSetBool(ep, TEST_bool, TRUE);
   lSetList(ep, TEST_list, NULL);

   lSetInt(ep, TEST_int, 100);
   lSetObject(ep, TEST_object, obj);
   lSetRef(ep, TEST_ref, ep);
   printf("element after setting fields\n");
   lWriteElemTo(ep, stdout);

   /* test lCopyElem */
   copy = lCopyElem(ep);
   printf("copy of element\n");
   lWriteElemTo(copy, stdout);
   copy = lFreeElem(copy);

   /* test lCopyElemPartial */
   /* first copy the complete element */
   copy = lCreateElem(TEST_Type);
   enp = lWhat("%T(ALL)", TEST_Type);
   index = 0;
   lCopyElemPartial(copy, &index, ep, enp); 
   printf("complete copy of element\n");
   lWriteElemTo(copy, stdout);
   copy = lFreeElem(copy);
   enp = lFreeWhat(enp);
   /* now copy a reduced element */
   copy = lCreateElem(TEST_Type);
   enp = lWhat("%T(%I %I %I)", TEST_Type, TEST_string, TEST_float, TEST_double);
   index = lGetPosInDescr(TEST_Type, TEST_string);
   lCopyElemPartial(copy, &index, ep, enp); 
   printf("partial copy of element\n");
   lWriteElemTo(copy, stdout);
   copy = lFreeElem(copy);
   enp = lFreeWhat(enp);

   /* test reducing of elements */

   /* cleanup and exit */
   lFreeElem(ep);                  
   return EXIT_SUCCESS;
}


