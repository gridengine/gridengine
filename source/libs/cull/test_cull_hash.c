#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>

#define __SGE_GDI_LIBRARY_HOME_OBJECT_FILE__
#include "cull.h"


#ifdef TEST_USE_JOBL
#include "sge_jobL.h"
int NM_ULONG  = JB_job_number;
int NM_STRING = JB_owner;
lDescr *DESCR = JB_Type;

lNameSpace nmv[] = {
   {1, JBS, JBN },
   {0, 0, NULL}
};

#else
enum {
   TEST_ulong = 1,
   TEST_string
};

LISTDEF(TEST_Type)
   SGE_ULONG (TEST_ulong)
   SGE_STRING (TEST_string)
LISTEND

NAMEDEF(TEST_Name)
   NAME("TEST_ulong")
   NAME("TEST_string")
NAMEEND

#define TEST_Size sizeof(TEST_Name) / sizeof(char *)

int NM_ULONG  = TEST_ulong;
int NM_STRING = TEST_string;
lDescr *DESCR = TEST_Type;

lNameSpace nmv[] = {
   {1, TEST_Size, TEST_Name },
   {0, 0, NULL}
};
#endif

#define FIRST_NAME 3

int main(int argc, char *argv[])
{
   int hash;
   int entries;
   int num_names;
   int entries_per_name;
   int i, j, k, l;
   lList *lp = NULL, *trash = NULL;
   lListElem *ep;

   clock_t now, start;
   struct tms tms_buffer;

   lInit(nmv);

   if (argc < 4) {
      fprintf(stderr, "usage: test_cull_hash <size> <hash [0|1]> <name_1> [<name_2> ... <name_n>]\n");
      return EXIT_FAILURE;
   }

   entries = atoi(argv[1]);
   hash = atoi(argv[2]);
   num_names = argc - FIRST_NAME;
   entries_per_name = entries / num_names;
for(l = 0; l < 1; l++) {
   printf("creating %d entries using %d names, that is %d entries per name\n", entries, num_names, entries_per_name);

   start = times(&tms_buffer);

   for (k = 0, i = 0; i < entries_per_name; i++) {
      for (j = FIRST_NAME; j < argc; j++, k++) {
         ep = lAddElemUlong(&lp, NM_ULONG, k, DESCR);
         lSetString(ep, NM_STRING, argv[j]);
      }
   }

   now = times(&tms_buffer);
   printf("filled list with %d entries in %.3f s\n", lGetNumberOfElem(lp),
          (now - start) * 1.0 / CLK_TCK);
   start = times(&tms_buffer);

   if (hash) {
      cull_hash_new(lp, NM_STRING, &template_hash);
      now = times(&tms_buffer);
      printf("created hash table in %.3f s\n", (now - start) * 1.0 / CLK_TCK);
   }

   trash = lCreateList("trash", DESCR);

   start = times(&tms_buffer);
   for (j = FIRST_NAME; j < argc; j++) {
      const void *iterator = NULL;
      lListElem *next_ep;

      printf("list has %d entries, deleting all with name %s\n", lGetNumberOfElem(lp), argv[j]);

      start = times(&tms_buffer);
      next_ep = lGetElemStrFirst(lp, NM_STRING, argv[j], &iterator);
/*       next_ep = lGetElemStrNext(lp, NM_STRING, argv[j], &iterator); */
      while ((ep = next_ep) != NULL) {
         next_ep = lGetElemStrNext(lp, NM_STRING, argv[j], &iterator);
         lDechainElem(lp, ep);
         lAppendElem(trash, ep);
      }

      now = times(&tms_buffer);
      printf("deleted entries of name %s in %.3f s\n", argv[j],
             (now - start) * 1.0 / CLK_TCK);
   }

   printf("trash list contains %d elements\n", lGetNumberOfElem(trash));

   lFreeList(lp);
   lFreeList(trash);
   lp = trash = NULL;
}
   return EXIT_SUCCESS;
}
