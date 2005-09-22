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
 *   Copyright: 2003 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>

#include "cull/cull_multitype.h"
#include "sgeobj/sge_all_listsL.h"
#include "sgeobj/sge_range.h"
#include "uti/sge_profiling.h"
#include "category.h"

typedef struct {
   int        test_nr;                 /* test number */
   u_long32   type;                    /* the job type */
   char       *project;                /* the job project */
   char       *owner;                  /* the job owner */
   char       *group;                  /* the job group */
   char       *checkpointing;          /* the checkpointing */
   char       *hard_resource_list;     /* the hard requested resources */
   char       *soft_resource_list;     /* the soft requested resources */
   char       *hard_queue_list;        /* the hard requested queues */
   char       *hard_master_queue_list; /* hard master queue list */
   char       *pe;                     /* the requested pe */
   int        is_access_list;          /* if 1, generate a access list */
}data_entry_t;


/*
 * This describes the acces list configuration. Each line is one acces list. The first
 * item is the access_list name, the others are the users in the access list
 */
static char *AccessList[] = {"test2_acc user test_user irgendwas",
                             "test1_acc help user what-ever",
                             "test0_acc nothing",
                             NULL};

/**
 *
 * Test setup:
 * Each line specifies one test. se data_entry_t documentation the meaning of each element. Please ensure
 * that for each line you have also 1 result_category line with the expected category string
 *
 **/
static data_entry_t tests[] = { {1, 128, NULL, "user", NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0},
                                {2, 128, "my_pr", "user", NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0},
                                {3, 128, NULL, "user", NULL, "my_check", NULL, NULL, NULL, NULL, NULL, 0},
                                {4, 128, "my_pr", "user", NULL, "my_check", NULL, NULL, NULL, NULL, NULL, 0},
                                {5, 128, NULL, "user", NULL, NULL, "arch test_arch lic 1 memory 1GB", NULL, NULL, NULL, NULL, 0},
                                {6, 128, "my_pr", "user", NULL, "my_check", "arch test_arch lic 1 memory 1GB", NULL, NULL, NULL, NULL, 0},
                                {7, 128, NULL, "user", NULL, NULL, NULL, "arch test_arch lic 1 memory 1GB", NULL, NULL, NULL, 0},
                                {8, 128, "my_pr", "user", NULL, "my_check", "arch test_arch lic 1 memory 1GB", "arch test_arch lic 1 memory 1GB", 
                                    NULL, NULL, NULL, 0}, 
                                {9, 128, NULL, "user", NULL, NULL, NULL, NULL, "my.q@test m1.q@what-ever test@*", NULL, NULL, 0},
                                {10, 128, "my_pr", "user", NULL, "my_check", "arch test_arch lic 1 memory 1GB", "arch test_arch lic 1 memory 1GB", 
                                    "my.q@test m1.q@what-ever test@*", NULL, NULL, 0},
                                {11, 128, NULL, "user", NULL, NULL, NULL, NULL, NULL, "my.q@test m1.q@what-ever test@*", NULL, 0},
                                {12, 128, "my_pr", "user", NULL, "my_check", "arch test_arch lic 1 memory 1GB", "arch test_arch lic 1 memory 1GB", 
                                    "my.q@test m1.q@what-ever test@*", "my.q@test m1.q@what-ever test@*", NULL, 0},
                                {13, 128, NULL, "user", NULL, NULL, NULL, NULL, NULL, NULL, "my_pe 1-10", 0},
                                {14, 128, "my_pr", "user", NULL, "my_check", "arch test_arch lic 1 memory 1GB", "arch test_arch lic 1 memory 1GB", 
                                    "my.q@test m1.q@what-ever test@*", "my.q@test m1.q@what-ever test@*", "my_pe 1-10", 0},
                                {15, 128, NULL, "user", NULL, NULL, NULL, NULL, NULL, NULL, NULL, 1},    
                                {16, 128, "my_pr", "user", NULL, "my_check", "arch test_arch lic 1 memory 1GB", "arch test_arch lic 1 memory 1GB", 
                                    "my.q@test m1.q@what-ever test@*", "my.q@test m1.q@what-ever test@*", "my_pe 1-10", 1},

/* stop entry */                {-1,  0, NULL,   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0} 
                              };  

/**
 * result strings
 **/
static char *result_category[] = { NULL,
                                   "-P my_pr",
                                   "-ckpt my_check",
                                   "-ckpt my_check -P my_pr",
                                   "-l arch=test_arch,lic=1,memory=1GB",
                                   "-l arch=test_arch,lic=1,memory=1GB -ckpt my_check -P my_pr",
                                   "-soft -l arch=test_arch,lic=1,memory=1GB",
                                   "-l arch=test_arch,lic=1,memory=1GB -soft -l arch=test_arch,lic=1,memory=1GB -ckpt my_check -P my_pr",
                                   "-q my.q@test,m1.q@what-ever,test@*",
                                   "-q my.q@test,m1.q@what-ever,test@* -l arch=test_arch,lic=1,memory=1GB -soft -l arch=test_arch,lic=1,memory=1GB -ckpt my_check -P my_pr",
                                   "-masterq my.q@test,m1.q@what-ever,test@*",
                                   "-q my.q@test,m1.q@what-ever,test@* -masterq my.q@test,m1.q@what-ever,test@* -l arch=test_arch,lic=1,memory=1GB -soft -l arch=test_arch,lic=1,memory=1GB -ckpt my_check -P my_pr",
                                   "-pe my_pe 1-10",
                                   "-q my.q@test,m1.q@what-ever,test@* -masterq my.q@test,m1.q@what-ever,test@* -l arch=test_arch,lic=1,memory=1GB -soft -l arch=test_arch,lic=1,memory=1GB -pe my_pe 1-10 -ckpt my_check -P my_pr",
                                   "-U test2_acc,test1_acc",
                                   "-U test2_acc,test1_acc -q my.q@test,m1.q@what-ever,test@* -masterq my.q@test,m1.q@what-ever,test@* -l arch=test_arch,lic=1,memory=1GB -soft -l arch=test_arch,lic=1,memory=1GB -pe my_pe 1-10 -ckpt my_check -P my_pr",
                                   NULL
                                 };

/****** test_category/test_create_access() *************************************
*  NAME
*     test_create_access() -- creates an access list from AccessList
*
*  SYNOPSIS
*     lList* test_create_access() 
*
*  RESULT
*     lList* - NULL or valid acces list
*
*  NOTES
*     MT-NOTE: test_create_access() is not MT safe 
*
*******************************************************************************/
static lList *test_create_access(void)
{
   lList *access_list = NULL;

   access_list = lCreateList("access", US_Type);
   
   if (access_list != NULL) {
      int i;

      for (i = 0; AccessList[i] != NULL; i++) {
         char *access_cp = NULL;
         char *access_str = NULL;
         char *iter_dash = NULL;

         access_cp = strdup(AccessList[i]);
        
         for (access_str = strtok_r(access_cp, " ", &iter_dash); access_str; access_str = strtok_r(NULL, " ", &iter_dash)) {
            lListElem *acc_elem = NULL;

            acc_elem = lCreateElem(US_Type);
            if (acc_elem != NULL) {
               lList *users = lCreateList("user", UE_Type);
               lSetString(acc_elem, US_name, access_str);
               lSetList(acc_elem, US_entries, users);
              
               lAppendElem(access_list, acc_elem);
              
               for (access_str = strtok_r(NULL, " ", &iter_dash); access_str; access_str = strtok_r(NULL, " ", &iter_dash)) {
                  lListElem *user = lCreateElem(UE_Type);

                  lSetString(user, UE_name, access_str);
                  lAppendElem(users, user);
               }

            }
         }
         if (access_cp != NULL) {
            free(access_cp);
            access_cp = NULL;
         }
      }
   }
   return access_list;
}

/****** test_category/test_create_request() ************************************
*  NAME
*     test_create_request() -- creats a request list from the request string
*
*  SYNOPSIS
*     lList* test_create_request(const char *requestStr, int count) 
*
*  INPUTS
*     const char *requestStr - request string
*     int count              - how many times the request string should be used
*                              (multiplyer, needs to between 1 and ...)
*
*  RESULT
*     lList* - NULL or request list
*
*  NOTES
*     MT-NOTE: test_create_request() is MT safe 
*
*******************************************************************************/
static lList *test_create_request(const char *requestStr, int count) 
{
   lList *requests = NULL;
   char *request_cp = NULL;
   char *iter_dash = NULL;

   requests = lCreateList("requests", CE_Type);

   if (requests != NULL) {
       int i;
       for (i = 0; i < count; i++) {
          char *request_str;

          request_cp = strdup(requestStr);
          
          for (request_str = strtok_r(request_cp, " ", &iter_dash); request_str; request_str = strtok_r(NULL, " ", &iter_dash)) {
            lListElem *request = NULL;
            
            request = lCreateElem(CE_Type);
            
            if (request != NULL) {
               lSetString(request, CE_name, request_str);
               lSetString(request, CE_stringval, strtok_r(NULL, " ", &iter_dash));
            }
            else {
               lFreeList(&requests);
               goto end;
            }
            lAppendElem(requests, request); 
          }
         if (request_cp != NULL) { 
            free(request_cp);
            request_cp = NULL;
         }
       }
   }
end:
   if (request_cp != NULL) {
      free(request_cp);
      request_cp = NULL;
   }
   return requests;
}

/****** test_category/test_create_queue() **************************************
*  NAME
*     test_create_queue() -- creates a request queue list from the queue string
*
*  SYNOPSIS
*     lList* test_create_queue(const char *queueStr, int count) 
*
*  INPUTS
*     const char *queueStr - the queue string used as a bases
*     int count            -  how many times the request string should be used
*                             (multiplyer, needs to between 1 and ...)
*
*  RESULT
*     lList* - NULL or valid queue request list
*
*  NOTES
*     MT-NOTE: test_create_queue() is MT safe 
*
*******************************************************************************/
static lList *test_create_queue(const char *queueStr, int count) 
{
   lList *queues = NULL;
   char *queue_cp = NULL;
   char *iter_dash = NULL;

   queues = lCreateList("queues", QR_Type);

   if (queues != NULL) {
       int i;
       for (i = 0; i < count; i++) {
          char *queues_str;
          queue_cp = strdup(queueStr);
          for (queues_str = strtok_r(queue_cp, " ", &iter_dash); queues_str; queues_str = strtok_r(NULL, " ", &iter_dash)) {
            lListElem *queue = NULL;
            
            queue = lCreateElem(QR_Type);
            
            if (queue != NULL) {
               lSetString(queue, QR_name, queues_str);
            }
            else {
               lFreeList(&queues);
               goto end;
            }
            lAppendElem(queues, queue); 
          }
          if (queue_cp != NULL) {
            free(queue_cp);
            queue_cp = NULL;
          }
       }
   }
end:
   if (queue_cp != NULL) {
      free(queue_cp); 
      queue_cp = NULL;
   }
   return queues;
}

/****** test_category/test_create_pe() *****************************************
*  NAME
*     test_create_pe() -- adds a pe object to the job
*
*  SYNOPSIS
*     void test_create_pe(const char *peStr, lListElem *job_elem) 
*
*  INPUTS
*     const char *peStr   - string representation of the pe object
*     lListElem *job_elem - job object
*
*  NOTES
*     MT-NOTE: test_create_pe() is MT safe 
*
*******************************************************************************/
static void test_create_pe(const char *peStr, lListElem *job_elem) 
{
   lList *range= NULL;
   char *pe_cp = strdup(peStr);
   char *iter_dash = NULL;
   char *pe_str;

    for (pe_str = strtok_r(pe_cp, " ", &iter_dash); pe_str; pe_str= strtok_r(NULL, " ", &iter_dash)) {

      lSetString(job_elem, JB_pe, pe_str);
      
      pe_str= strtok_r(NULL, " ", &iter_dash);
      range_list_parse_from_string(&range, NULL, pe_str, false, false, INF_ALLOWED);
      if (range != NULL) {
         lSetList(job_elem, JB_pe_range, range);             
      }
      else {
         lSetString(job_elem, JB_pe, NULL);
         printf("error generating pe object: %s\n", peStr);

      }

    }
   if (pe_cp != NULL) {
      free(pe_cp); 
      pe_cp = NULL;
   }
}


/****** test_category/test_create_job() ****************************************
*  NAME
*     test_create_job() -- creates a job object
*
*  SYNOPSIS
*     lListElem* test_create_job(data_entry_t *test, int count) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     data_entry_t *test - string representation of a job
*     int count          - multiplier for the requests
*
*  RESULT
*     lListElem* - NULL or valid job object
*
*  NOTES
*     MT-NOTE: test_create_job() is MT safe 
*
*******************************************************************************/
static lListElem *test_create_job(data_entry_t *test, int count) 
{
   lListElem *job = NULL;

   job = lCreateElem(JB_Type);

   if (job != NULL) {
      lSetUlong(job, JB_type, test->type);

      if (test->project != NULL) {
         lSetString(job, JB_project, test->project);
      }
      if (test->owner != NULL) {
         lSetString(job, JB_owner, test->owner);
      }
      if (test->group != NULL) {
         lSetString(job, JB_group, test->group);
      }
      if (test->checkpointing != NULL) {
         lSetString(job, JB_checkpoint_name, test->checkpointing);
      }
      if (test->hard_resource_list != NULL) {
         lList *requests = test_create_request(test->hard_resource_list, count);
         if (requests != NULL) {
            lSetList(job, JB_hard_resource_list, requests);
         }
         else {
            lFreeElem(&job);
            goto end;
         }
      }
      if (test->soft_resource_list != NULL) {
         lList *requests = test_create_request(test->soft_resource_list, count);
         if (requests != NULL) {
            lSetList(job, JB_soft_resource_list, requests);
         }
         else {
            lFreeElem(&job);
            goto end;
         }
      }
      if (test->hard_queue_list != NULL) {
         lList *queues = test_create_queue(test->hard_queue_list, count);
         if (queues != NULL) {
            lSetList(job, JB_hard_queue_list, queues);
         }
         else {
            lFreeElem(&job);
            goto end;
         }
      }
      if (test->hard_master_queue_list != NULL) {
         lList *queues = test_create_queue(test->hard_master_queue_list, count);
         if (queues != NULL) {
            lSetList(job, JB_master_hard_queue_list, queues);
         }
         else {
            lFreeElem(&job);
            goto end;
         }
      }
      if (test->pe != NULL) {
         test_create_pe(test->pe, job);
      }
   }
end:
   return job;
}

/****** test_category/test_performance() ***************************************
*  NAME
*     test_performance() -- messures and outputs the time neede for n category strings
*
*  SYNOPSIS
*     double test_performance(lListElem *job_elem, int max, lList* access_list) 
*
*  INPUTS
*     lListElem *job_elem - job object
*     int max             - number of generated category strings
*     lList* access_list  - access list or NULL
*
*  RESULT
*     double - time needed for the run
*
*  NOTES
*     MT-NOTE: test_performance() is MT safe 
*
*******************************************************************************/
static double test_performance(lListElem *job_elem, int max, lList* access_list) 
{
   int i;
   dstring category_str = DSTRING_INIT;
   struct timeval before;
   struct timeval after;
   double time_new;
   
   gettimeofday(&before, NULL); 
   for (i = 0; i < max; i++) {
      sge_build_job_category_dstring(&category_str, job_elem, access_list);
      sge_dstring_clear(&category_str);
   }
   gettimeofday(&after, NULL);
   sge_dstring_free(&category_str);

   time_new = after.tv_usec - before.tv_usec;
   time_new = after.tv_sec - before.tv_sec + (time_new/1000000);

   printf("tested %d category creations: new: %.2fs\n", max, time_new);

   return time_new;
}

/****** test_category/test() ***************************************************
*  NAME
*     test() -- executes one test including a performance test run
*
*  SYNOPSIS
*     int test(data_entry_t *test, char *result, int count) 
*
*  INPUTS
*     data_entry_t *test - one test setup
*     char *result       - expected category
*     int count          - test number
*
*  RESULT
*     int - 0 okay, 1 failed
*
*  NOTES
*     MT-NOTE: test() is MT safe 
*
*******************************************************************************/
static int test(data_entry_t *test, char *result, int count) 
{
   int ret = 0;
   lListElem *job_elem = NULL;
   lList *access_list = NULL;

   printf("\ntest %d:\n-------\n", test->test_nr);
   
   job_elem = test_create_job(test, 1);

   if (test->is_access_list == 1) {
      access_list = test_create_access();
   }

   if (job_elem != NULL) {
       dstring category_str = DSTRING_INIT;

       printf("expected: <%s>\n", result!=NULL? result:"<NULL>");

       sge_build_job_category_dstring(&category_str, job_elem, access_list);

       printf("got     : <%s>\n", sge_dstring_get_string(&category_str)!=NULL?sge_dstring_get_string(&category_str):"<NULL>");

       if (result != NULL && sge_dstring_get_string(&category_str) != NULL) {
         if (strcmp(result, sge_dstring_get_string(&category_str)) == 0) {
         }
         else {
            ret = 1;

         }
       }
       else if (result == NULL &&  sge_dstring_get_string(&category_str) == NULL) {
       }
       else {
         ret = 1;
       }
       
       if (ret == 0) {
         int i;
         int max = 10000;
         printf(" => category outputs match\n");
         lFreeElem(&job_elem);
         for (i = 1; i <= 500; i*=6) {
            printf("test with %dx :", i);
            job_elem = test_create_job(test, i);
            if (job_elem) {
               double time = test_performance(job_elem, max, access_list); 
               if (time > 5) {
                  max /= 10;
               }
            }
            else {
               printf("failed to create job\n");
               ret = 1;
               break;
            }
         }
       }
       else {
         printf(" => test failed\n");
       }
       
       sge_dstring_free(&category_str);
   }
   else {
      printf("failed to create job for test %d\n", count);
      ret = 1;
   }
   lFreeElem(&job_elem);
   return ret;
}

/****** test_sge_calendar/main() ***********************************************
*  NAME
*     main() -- calendar test
*
*  SYNOPSIS
*     int main(int argc, char* argv[]) 
*
*  FUNCTION
*     calendar test
*
*  INPUTS
*     int argc     - nr. of args 
*     char* argv[] - args
*
*  RESULT
*     int -  nr of failed tests
*
*******************************************************************************/
int main(int argc, char* argv[])
{
   int test_counter = 0;
   int failed = 0;

   sge_prof_setup();

   lInit(nmv);
   
   printf("==> category test <==\n");
   printf("---------------------\n");


   while (tests[test_counter].test_nr != -1) {
      if (test(&(tests[test_counter]), 
               result_category[test_counter], 
               test_counter) != 0) {
         failed++; 
      }   
      test_counter++;
   }

   if (failed == 0) {
      printf("\n==> All tests are okay <==\n");
   }
   else {
      printf("\n==> %d/%d test(s) failed <==\n", failed, test_counter);
   }
   
   return failed;
}
