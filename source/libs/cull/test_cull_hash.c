#include <stdio.h>
#include <stdlib.h>

#define __SGE_GDI_LIBRARY_HOME_OBJECT_FILE__
#include "cull.h"

enum {
   TEST_ulong = 1,
   TEST_string
};

LISTDEF(TEST_Type)
   SGE_ULONGHU (TEST_ulong)
   SGE_STRING (TEST_string)
LISTEND

NAMEDEF(TEST_Name)
   NAME("TEST_ulong")
   NAME("TEST_string")
NAMEEND

#define TEST_Size sizeof(TEST_Name) / sizeof(char *)

lNameSpace nmv[] = {
   {1, TEST_Size, TEST_Name},
   {0, 0, NULL}
};

#define FIRST_NAME 3

int main(int argc, char *argv[])
{
   int hash;
   int entries;
   int num_names;
   int entries_per_name;
   int i, j, k;
   lList *lp, *trash;
   lListElem *ep;

   lInit(nmv);

   if (argc < 4) {
      fprintf(stderr, "usage: test_cull_hash <size> <hash [0|1]> <name_1> [<name_2> ... <name_n>]\n");
      return EXIT_FAILURE;
   }

   entries = atoi(argv[1]);
   hash = atoi(argv[2]);
   num_names = argc - FIRST_NAME;
   entries_per_name = entries / num_names;

   printf("creating %d entries using %d names, that is %d entries per name\n", entries, num_names, entries_per_name);

   for (k = 0, i = 0; i < entries_per_name; i++) {
      for (j = FIRST_NAME; j < argc; j++, k++) {
         ep = lAddElemUlong(&lp, TEST_ulong, k, TEST_Type);
         lSetString(ep, TEST_string, argv[j]);
      }
   }

   printf("filled list with %d entries\n", lGetNumberOfElem(lp));

   if (hash) {
      cull_hash_new(lp, TEST_string, &template_hash);
      printf("created hash table\n");
   }

   trash = lCreateList("trash", TEST_Type);

   for (j = FIRST_NAME; j < argc; j++) {
      const void *iterator = NULL;
      lListElem *next_ep;

      printf("list has %d entries, deleting all with name %s\n", lGetNumberOfElem(lp), argv[j]);

      next_ep = lGetElemStrFirst(lp, TEST_string, argv[j], &iterator);
      next_ep = lGetElemStrNext(lp, TEST_string, argv[j], &iterator);
      while ((ep = next_ep) != NULL) {
         next_ep = lGetElemStrNext(lp, TEST_string, argv[j], &iterator);
         lDechainElem(lp, ep);
         lAppendElem(trash, ep);
      }

      printf("deleted entries of name %s\n", argv[j]);
   }

   printf("trash list contains %d elements\n", lGetNumberOfElem(trash));

   lFreeList(lp);
   lFreeList(trash);

   return EXIT_SUCCESS;
}
