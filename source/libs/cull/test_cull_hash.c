#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/times.h>

#include "basis_types.h"

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
   SGE_ULONG (TEST_ulong, CULL_DEFAULT)
   SGE_STRING (TEST_string, CULL_DEFAULT)
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

static void usage(const char *argv0) 
{
   fprintf(stderr, "usage: %s <num_objects> <num_names>\n", argv0);
   fprintf(stderr, "<num_objects> = number of objects to be created\n");
   fprintf(stderr, "<num_names> = number of entries in non unique hash\n");
   exit(EXIT_FAILURE);
}

static const char *random_string(int length)
{
   static char buf[1000];
   int i;

   for (i = 0; i < length; i++) {
      buf[i] = rand() % 26 + 64;
   }
   buf[i] = 0;

   return strdup(buf);
}

long clk_tck = 0;
const char **names = NULL;

const char *HEADER_FORMAT = "%s %s %8s %8s %8s %8s %8s %8s %8s %8s %8s\n";
const char *DATA_FORMAT   = "%s %s %8.3lf %8.3lf %8.3lf %8.3lf %8.3lf %8.3lf (%6d) %8.3lf (%6d)\n";

static void do_test(bool unique_hash, bool non_unique_hash, 
                    int num_objects, int num_names)
{
   lList *lp = NULL, *trash = NULL;
   lListElem *ep;
   clock_t now, start;
   struct tms tms_buffer;
   int entries_per_name = num_objects / num_names;
   int i;

   /* measurement data */
   double  prof_create,  /* create objects */
           prof_rau,     /* random access by unique attrib */
           prof_inu,     /* iterate by non unique attrib */
           prof_curo,    /* change unique attrib of random object */
           prof_cnuro,   /* change non unique attrib of random object */
           prof_dru,     /* delete random object by unique attribute */
           prof_dinu;    /* delete by iterating over non unique attrib */
   int     objs_dru,     /* objects deleted by random unique */
           objs_dinu;    /* objects deleted by iterate non unique */

   /* create list and hash tables */
   lp = lCreateList("test list ", DESCR);
   if (unique_hash) {
      cull_hash_new(lp, NM_ULONG, 1);
   }
   if (non_unique_hash) {
      cull_hash_new(lp, NM_STRING, 0);
   }

   start = times(&tms_buffer);

   /* TEST: build objects */
   for (i = 0; i < num_objects; i++) {
      ep = lAddElemUlong(&lp, NM_ULONG, i, DESCR);
      lSetString(ep, NM_STRING, names[rand() % num_names]);
   }

   /* measure time */
   now = times(&tms_buffer);
   prof_create = (now - start) * 1.0 / clk_tck;
   start = now;

   /* TEST: random access by unique attrib */
   for (i = 0; i < num_objects; i++) {
      ep = lGetElemUlong(lp, NM_ULONG, rand() % num_objects);
   }

   /* measure time */
   now = times(&tms_buffer);
   prof_rau = (now - start) * 1.0 / clk_tck;
   start = now;

   /* TEST: iterate by non unique attrib */
   for (i = 0; i < num_names; i++) {
      const void *iterator = NULL;
      lListElem *next_ep;

      next_ep = lGetElemStrFirst(lp, NM_STRING, names[i], &iterator);
      while ((ep = next_ep) != NULL) {
         next_ep = lGetElemStrNext(lp, NM_STRING, names[i], &iterator);
      }
   }

   /* measure time */
   now = times(&tms_buffer);
   prof_inu = (now - start) * 1.0 / clk_tck;
   start = now;

   /* TEST: change unique attribute of random object */
   /* measure time */
   for (i = 0; i < num_objects; i++) {
      /* object to access */
      int unique = rand() % num_objects;
      /* new value to set, make sure we stay unique */
      int newval = unique + num_objects;

      /* search object */
      ep = lGetElemUlong(lp, NM_ULONG, unique);

      /* set a new value */
      lSetUlong(ep, NM_ULONG, newval);

      /* restore old value */
      lSetUlong(ep, NM_ULONG, unique);
   }

   now = times(&tms_buffer);
   prof_curo = (now - start) * 1.0 / clk_tck;
   start = now;

   /* TEST: change non unique attribute of random object */
   for (i = 0; i < num_objects; i++) {
      /* search object */
      ep = lGetElemUlong(lp, NM_ULONG, rand() % num_objects);

      /* set a new value */
      lSetString(ep, NM_STRING, names[rand() % num_names]);
   }
   /* measure time */
   now = times(&tms_buffer);
   prof_cnuro = (now - start) * 1.0 / clk_tck;
   start = now;

   /* TEST: delete random object */
   objs_dru = 0;
   for (i = 0; i < num_objects / 2; i++) {
      /* search object */
      ep = lGetElemUlong(lp, NM_ULONG, rand() % num_objects);
      /* if same rand showed up earlier, object does no longer exist! */
      if (ep != NULL) {
         lRemoveElem(lp, ep);
         objs_dru++;
      }
   }   
   /* measure time */
   now = times(&tms_buffer);
   prof_dru = (now - start) * 1.0 / clk_tck;
   start = now;

   /* TEST: delete all objects having the same non unique attribute */
   objs_dinu = 0;
   for (i = 0; i < num_names; i++) {
      const void *iterator = NULL;
      lListElem *next_ep;

      next_ep = lGetElemStrFirst(lp, NM_STRING, names[i], &iterator);
      while ((ep = next_ep) != NULL) {
         next_ep = lGetElemStrNext(lp, NM_STRING, names[i], &iterator);
         lRemoveElem(lp, ep);
         objs_dinu++;
      }
   }

   /* measure time */
   now = times(&tms_buffer);
   prof_dinu = (now - start) * 1.0 / clk_tck;
   start = now;

   lFreeList(lp);

   printf(DATA_FORMAT, 
          unique_hash ? " * " : "   ",
          non_unique_hash ? " * " : "   ",
          prof_create, 
          prof_rau, prof_inu, 
          prof_curo, prof_cnuro,
          prof_dru, objs_dru, prof_dinu, objs_dinu);

/*
   printf("filled list with %d entries in %.3f s\n", lGetNumberOfElem(lp),
          (now - start) * 1.0 / clk_tck);
   start = times(&tms_buffer);

   if (hash) {
      cull_hash_new(lp, NM_STRING, FALSE);
      now = times(&tms_buffer);
      printf("created hash table in %.3f s\n", (now - start) * 1.0 / clk_tck);
   }

   trash = lCreateList("trash", DESCR);

   start = times(&tms_buffer);
   for (i = 0; i < num_names; i++) {
      const void *iterator = NULL;
      lListElem *next_ep;

      printf("list has %d entries, deleting all with name %s\n", lGetNumberOfElem(lp), names[i]);

      start = times(&tms_buffer);
      next_ep = lGetElemStrFirst(lp, NM_STRING, names[i], &iterator);
      next_ep = lGetElemStrNext(lp, NM_STRING, names[i], &iterator);
      while ((ep = next_ep) != NULL) {
         next_ep = lGetElemStrNext(lp, NM_STRING, names[i], &iterator);
         lDechainElem(lp, ep);
         lAppendElem(trash, ep);
      }

      now = times(&tms_buffer);
      printf("deleted entries of name %s in %.3f s\n", names[i],
             (now - start) * 1.0 / clk_tck);
   }

   printf("trash list contains %d elements\n", lGetNumberOfElem(trash));

   lFreeList(lp);
   lFreeList(trash);
   lp = trash = NULL;
   */
}

int main(int argc, char *argv[])
{
   int num_objects;
   int num_names;
   int i;

   if (argc < 3) {
      usage(argv[0]);
   }

   /* initialize globals */
   lInit(nmv);
   clk_tck = sysconf(_SC_CLK_TCK);

   /* we need random numbers */
   srand(time(0));

   /* parse commandline options */
   num_objects = atoi(argv[1]);
   num_names   = atoi(argv[2]);

   /* create name array */
   names = (const char **) malloc (num_names * sizeof(const char *));

   /* build random names */
   for (i = 0; i < num_names; i++) {
      const char *name = random_string(10);
      names[i] = name;
   }

   /* output header */
   printf(HEADER_FORMAT, "uh ", "nuh", 
          "create", "rau", "inu", "curo", "cnuro", 
          "dru", "(objs)", "dinu", "(objs)");

   /* do tests */
/*    do_test(false, false, num_objects, num_names); */
/*    do_test(false, true,  num_objects, num_names); */
   do_test(true,  false, num_objects, num_names);
   do_test(true,  true,  num_objects, num_names);

   return EXIT_SUCCESS;
}
