#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>

#include "cl_lists.h"

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


/* this global is used to set the thread configuration data for each thread */
static pthread_mutex_t global_thread_config_key_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_key_t global_thread_config_key;
static int global_thread_config_key_done = 0;



#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_thread_create_thread_condition()"
int cl_thread_create_thread_condition(cl_thread_condition_t** condition ) {
   cl_thread_condition_t* new_condition = NULL;
   int ret_val;
   if (condition == NULL) {
      /* no condition pointer pointer */
      return CL_RETVAL_PARAMS;
   }
   if (*condition != NULL) {
      /* pointer pointer is allready initialized (not free) */
      return CL_RETVAL_PARAMS;
   }

   new_condition =  (cl_thread_condition_t*)malloc(sizeof(cl_thread_condition_t));
   if ( new_condition == NULL) {
      return CL_RETVAL_MALLOC;
   }

   new_condition->thread_mutex_lock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
   if ( new_condition->thread_mutex_lock == NULL) {
      free(new_condition);
      return CL_RETVAL_MALLOC;
   }

   new_condition->trigger_count_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
   if ( new_condition->trigger_count_mutex == NULL) {
      free(new_condition->thread_mutex_lock);
      free(new_condition);
      return CL_RETVAL_MALLOC;
   }
   new_condition->trigger_count = 0;


   new_condition->thread_cond_var = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
   if (new_condition->thread_cond_var == NULL) {
      free(new_condition->trigger_count_mutex);
      free(new_condition->thread_mutex_lock);
      free(new_condition);
      return CL_RETVAL_MALLOC;
   }

   if (pthread_mutex_init(new_condition->thread_mutex_lock, NULL) != 0) {
      free(new_condition->trigger_count_mutex);
      free(new_condition->thread_mutex_lock);
      free(new_condition->thread_cond_var);
      free(new_condition);
      return CL_RETVAL_MUTEX_ERROR;
   }

   if (pthread_mutex_init(new_condition->trigger_count_mutex, NULL) != 0) {
      ret_val = pthread_mutex_destroy(new_condition->thread_mutex_lock);
      if (ret_val == EBUSY) {
         return CL_RETVAL_MUTEX_CLEANUP_ERROR;
      }
      free(new_condition->trigger_count_mutex);
      free(new_condition->thread_mutex_lock);
      free(new_condition->thread_cond_var);
      free(new_condition);
      return CL_RETVAL_MUTEX_ERROR;
   }

   if (pthread_cond_init(new_condition->thread_cond_var,NULL) != 0) {
      ret_val = pthread_mutex_destroy(new_condition->thread_mutex_lock);
      if (ret_val == EBUSY) {
         return CL_RETVAL_MUTEX_CLEANUP_ERROR;
      }
      ret_val = pthread_mutex_destroy(new_condition->trigger_count_mutex);
      if (ret_val == EBUSY) {
         return CL_RETVAL_MUTEX_CLEANUP_ERROR;
      }
      free(new_condition->trigger_count_mutex);
      free(new_condition->thread_mutex_lock);
      free(new_condition->thread_cond_var);
      free(new_condition);
      return CL_RETVAL_CONDITION_ERROR;
   }

   *condition = new_condition;
   return CL_RETVAL_OK;
}
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_thread_delete_thread_condition()"
int cl_thread_delete_thread_condition(cl_thread_condition_t** condition ) {
   int ret_val;
   if (condition == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*condition == NULL) {
      return CL_RETVAL_PARAMS;
   }

   if ( (*condition)->thread_mutex_lock != NULL) {
      ret_val = pthread_mutex_destroy((*condition)->thread_mutex_lock);
      if (ret_val == EBUSY) {
         return CL_RETVAL_MUTEX_CLEANUP_ERROR;
      }
   }

   if ( (*condition)->trigger_count_mutex != NULL) {
      ret_val = pthread_mutex_destroy((*condition)->trigger_count_mutex);
      if (ret_val == EBUSY) {
         return CL_RETVAL_MUTEX_CLEANUP_ERROR;
      }
   }

   if ( (*condition)->thread_cond_var != NULL) {
      ret_val = pthread_cond_destroy((*condition)->thread_cond_var);
      if (ret_val == EBUSY) {
         return CL_RETVAL_CONDITION_CLEANUP_ERROR;
      }
   }

   if ( (*condition)->thread_mutex_lock != NULL) {
      free((*condition)->thread_mutex_lock);
   }
 
   if ( (*condition)->trigger_count_mutex != NULL) {
      free((*condition)->trigger_count_mutex);
   }

   if ( (*condition)->thread_cond_var != NULL) {
      free((*condition)->thread_cond_var);
   }

   free(*condition);
   *condition = NULL;
   return CL_RETVAL_OK;
}
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_thread_wait_for_thread_condition()"
int cl_thread_wait_for_thread_condition(cl_thread_condition_t* condition, long sec, long micro_sec) {
   int retcode;
   int ret_val = CL_RETVAL_OK;
   struct timeval now;
   struct timespec timeout;

   long sec_now;
   long micro_sec_now;


   if (condition == NULL) {
      return CL_RETVAL_PARAMS;
   }


   /* lock condition mutex */
   if (pthread_mutex_lock(condition->thread_mutex_lock) != 0) {
      return CL_RETVAL_MUTEX_LOCK_ERROR;
   } 


   pthread_mutex_lock(condition->trigger_count_mutex);
   CL_LOG_INT(CL_LOG_INFO,"Trigger count:", (int)condition->trigger_count );

   if ( condition->trigger_count == 0 ) {
      /* trigger count is zero, wait for trigger */

      pthread_mutex_unlock(condition->trigger_count_mutex);

      if (sec <= 0 && micro_sec <= 0) {
         /* do a not timed wait */
         if (pthread_cond_wait(condition->thread_cond_var,condition->thread_mutex_lock) != 0) {
            ret_val = CL_RETVAL_CONDITION_ERROR;
            pthread_mutex_lock(condition->trigger_count_mutex);
            condition->trigger_count = 0;
            pthread_mutex_unlock(condition->trigger_count_mutex);
         } else {
            /* triggered, do not count the awake trigger */
            pthread_mutex_lock(condition->trigger_count_mutex);
            if ( condition->trigger_count > 0) {
               condition->trigger_count = condition->trigger_count - 1;
            }
            pthread_mutex_unlock(condition->trigger_count_mutex);
         }
      } else {
         gettimeofday(&now,NULL);
         sec_now =  now.tv_sec;
         micro_sec_now = now.tv_usec;
   
         sec = sec + (micro_sec / 1000000);
         micro_sec = micro_sec % 1000000;
   
         micro_sec_now = micro_sec_now + micro_sec;
         sec_now = sec_now + sec;
         
         if (micro_sec_now >= 1000000) {
            micro_sec_now = micro_sec_now - 1000000; 
            sec_now++;
         }
   
         timeout.tv_sec = sec_now;
         timeout.tv_nsec = (micro_sec_now * 1000 );
   
         retcode = pthread_cond_timedwait(condition->thread_cond_var,condition->thread_mutex_lock , &timeout);
   
         if (retcode == ETIMEDOUT) {
            ret_val = CL_RETVAL_CONDITION_WAIT_TIMEOUT;   /* timeout */
         } else if (retcode != 0) {
            ret_val = CL_RETVAL_CONDITION_ERROR;
            pthread_mutex_lock(condition->trigger_count_mutex);
            condition->trigger_count = 0;
            pthread_mutex_unlock(condition->trigger_count_mutex);
         }

         if (ret_val != CL_RETVAL_CONDITION_WAIT_TIMEOUT && ret_val != CL_RETVAL_CONDITION_ERROR) {
            /* triggered, do not count the awake trigger */
            pthread_mutex_lock(condition->trigger_count_mutex);
            if ( condition->trigger_count > 0) {
               condition->trigger_count = condition->trigger_count - 1;
            }
            pthread_mutex_unlock(condition->trigger_count_mutex);
         }
      }
   } else {
      /* trigger count is > zero, do not trigger */
      CL_LOG(CL_LOG_INFO,"Thread was triggerd before wait - continue");
      CL_LOG_INT(CL_LOG_INFO,"Trigger count:", (int)condition->trigger_count );

      condition->trigger_count = condition->trigger_count - 1;
      pthread_mutex_unlock(condition->trigger_count_mutex);
   }


   /* unlock condition mutex */
   if (pthread_mutex_unlock(condition->thread_mutex_lock) != 0) {
      if (ret_val == CL_RETVAL_OK) {
         ret_val = CL_RETVAL_MUTEX_UNLOCK_ERROR;
      }
   }

   return ret_val;
}



#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_thread_clear_triggered_conditions()"
int cl_thread_clear_triggered_conditions(cl_thread_condition_t* condition) {
   if (condition == NULL) {
      return CL_RETVAL_PARAMS;
   }
   /* increase trigger count */
   if (pthread_mutex_lock(condition->trigger_count_mutex) == 0) {
      condition->trigger_count = 0;
      CL_LOG(CL_LOG_INFO,"cleared trigger count");
      if (pthread_mutex_unlock(condition->trigger_count_mutex) != 0) {
         CL_LOG(CL_LOG_ERROR,"could not unlock trigger_count_mutex");
         return CL_RETVAL_MUTEX_UNLOCK_ERROR;
      }
   } else {
      CL_LOG(CL_LOG_ERROR,"could not lock trigger_count_mutex");
      return CL_RETVAL_MUTEX_LOCK_ERROR;
   }
   return CL_RETVAL_OK;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_thread_trigger_thread_condition()"
int cl_thread_trigger_thread_condition(cl_thread_condition_t* condition, int do_broadcast) {
   int ret_val = CL_RETVAL_OK;

   if (condition == NULL) {
      return CL_RETVAL_PARAMS;
   }

   /* first lock condition mutex */
   if (pthread_mutex_lock(condition->thread_mutex_lock) != 0) {
      return CL_RETVAL_MUTEX_LOCK_ERROR;
   }

   /* increase trigger count */
   if (pthread_mutex_lock(condition->trigger_count_mutex) == 0) {
      condition->trigger_count = condition->trigger_count + 1;
      if (pthread_mutex_unlock(condition->trigger_count_mutex) != 0) {
         CL_LOG(CL_LOG_ERROR,"could not unlock trigger_count_mutex");
      }
   } else {
      CL_LOG(CL_LOG_ERROR,"could not lock trigger_count_mutex");
   }

   if (do_broadcast != 0) {
      /* signal condition with broadcast */
      if (pthread_cond_broadcast(condition->thread_cond_var) != 0) {
         ret_val = CL_RETVAL_CONDITION_SIGNAL_ERROR;
      }
   } else {
      /* signal condition with signal */
      if (pthread_cond_signal(condition->thread_cond_var) != 0) {
         ret_val = CL_RETVAL_CONDITION_SIGNAL_ERROR;
      }
   }
   
   /* unlock condition mutex */
   if (pthread_mutex_unlock(condition->thread_mutex_lock) != 0) {
      if (ret_val == CL_RETVAL_OK) {
         ret_val = CL_RETVAL_MUTEX_UNLOCK_ERROR;
      }
   }
   return ret_val;
}




/* if no start_routine is given (=NULL) the cl_thread_settings_t struct is
   filled, but no thread is started */
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_thread_setup()"
int cl_thread_setup(cl_thread_settings_t* thread_config, cl_raw_list_t* log_list, const char* name, int id, void * (*start_routine)(void *)) {

   int retry = 0; 
   int ret_val;
   
   if ( thread_config == NULL || name == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   /* clean thread structure */
   memset(thread_config, 0, sizeof(cl_thread_settings_t));

   thread_config->thread_name = strdup(name);    /* malloc */
   if (thread_config->thread_name == NULL) {
      return CL_RETVAL_MALLOC;
   }

   thread_config->thread_log_list = log_list;

   thread_config->thread_id = id;

   ret_val = cl_thread_create_thread_condition(&(thread_config->thread_event_condition));
   if ( ret_val != CL_RETVAL_OK ) {
      return ret_val;
   }

   thread_config->thread_state = CL_THREAD_STARTING;

   if (start_routine != NULL) {
      thread_config->thread_pointer = (pthread_t*)malloc(sizeof(pthread_t));
      if (thread_config->thread_pointer == NULL) {
         return CL_RETVAL_MALLOC;
      }
   } else {
      thread_config->thread_pointer = NULL;
   }


   ret_val = cl_thread_create_thread_condition(&(thread_config->thread_startup_condition));
   if ( ret_val != CL_RETVAL_OK ) {
      return ret_val;
   }

   pthread_mutex_lock(&global_thread_config_key_mutex);
   if (global_thread_config_key_done == 0) {
      pthread_key_create(&global_thread_config_key, NULL);
      global_thread_config_key_done = 1;
   }
   pthread_mutex_unlock(&global_thread_config_key_mutex);

   if (start_routine != NULL) {
      /* startup thread */
      if (pthread_create(thread_config->thread_pointer,NULL,start_routine,thread_config ) != 0) {
         return CL_RETVAL_THREAD_CREATE_ERROR;
      }
      

      /* wait for thread startup condition variable set */
      while (thread_config->thread_state == CL_THREAD_STARTING) {
         cl_thread_wait_for_thread_condition(thread_config->thread_startup_condition,0,100 * 1000);
         retry++;
         if (retry > 60) {
            return CL_RETVAL_THREAD_START_TIMEOUT;
         }
      }
   } else {
      /* this is creator thread setting */
      thread_config->thread_state = CL_THREAD_CREATOR;
      if (cl_thread_set_thread_config(thread_config) != CL_RETVAL_OK) {
         CL_LOG_STR(CL_LOG_ERROR, "cl_thread_set_thre_config() error for thread ->",thread_config->thread_name); 
      }
   }

   CL_LOG_STR(CL_LOG_DEBUG, "setup complete for thread ->",thread_config->thread_name); 
   return CL_RETVAL_OK;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_thread_join()"
int cl_thread_join(cl_thread_settings_t* thread_config) {
   
   if (thread_config == NULL) {
      return CL_RETVAL_PARAMS;
   }

   cl_thread_trigger_event(thread_config);

   CL_LOG(CL_LOG_DEBUG,"cl_thread_join() waiting for thread ...");

   /* wait for thread's end of life */
   if (pthread_join(*(thread_config->thread_pointer),NULL) != 0) {
      return CL_RETVAL_THREAD_JOIN_ERROR;
   }
   CL_LOG(CL_LOG_DEBUG,"cl_thread_join() done");
   return CL_RETVAL_OK;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_thread_get_thread_config()"
cl_thread_settings_t* cl_thread_get_thread_config(void) {
   /* cl_thread_setup  will set the thread specific data */
   cl_thread_settings_t* settings = NULL;
   pthread_mutex_lock(&global_thread_config_key_mutex);
   if (global_thread_config_key_done != 0) {
      settings = (cl_thread_settings_t*) pthread_getspecific(global_thread_config_key);
      pthread_mutex_unlock(&global_thread_config_key_mutex);
      return settings;
   }
   pthread_mutex_unlock(&global_thread_config_key_mutex);
   return NULL;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_thread_cleanup()"
int cl_thread_cleanup(cl_thread_settings_t* thread_config) {
   /* free all malloc()'ed pointers in cl_thread_settings_t structure */
   int ret_val;
   
   /* on CL_RETVAL_MUTEX_CLEANUP_ERROR or CL_RETVAL_CONDITION_CLEANUP_ERROR 
      the struct is NOT freed  !!! */

   if (thread_config == NULL) {
      return CL_RETVAL_PARAMS;
   }

   
   if (thread_config->thread_event_condition != NULL) {
      ret_val = cl_thread_delete_thread_condition(&(thread_config->thread_event_condition));
      if (ret_val != CL_RETVAL_OK) {
         return ret_val;
      }
   }
   
   if (thread_config->thread_startup_condition != NULL) {
      ret_val = cl_thread_delete_thread_condition(&(thread_config->thread_startup_condition));
      if (ret_val != CL_RETVAL_OK) {
         return ret_val;
      }
   }

   if (thread_config->thread_name) {
      CL_LOG_STR(CL_LOG_DEBUG,"cleanup for thread ->", thread_config->thread_name);
   }
   

   /* destroy thread name */
   if (thread_config->thread_name) {
      free(thread_config->thread_name);
      thread_config->thread_name = NULL;
   }
 
   /* destroy thread_pointer */
   if (thread_config->thread_pointer) {
      free(thread_config->thread_pointer);
      thread_config->thread_pointer = NULL;
   }

   return CL_RETVAL_OK;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_thread_shutdown()"
int cl_thread_shutdown(cl_thread_settings_t* thread_config) {
   int ret_val;

   if (thread_config == NULL) {
      return CL_RETVAL_PARAMS;
   }

   ret_val = pthread_cancel(*(thread_config->thread_pointer)); 

   switch(ret_val) {
       case 0:
          return CL_RETVAL_OK;
       case ESRCH:
          return CL_RETVAL_THREAD_NOT_FOUND;
       default:
          return CL_RETVAL_UNKNOWN; 
   }
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_thread_wait_for_event()"
int cl_thread_wait_for_event(cl_thread_settings_t *thread_config, long sec, long micro_sec) {

   int ret = CL_RETVAL_OK;
    
   if (thread_config == NULL) {
      return CL_RETVAL_PARAMS;
   }

   thread_config->thread_event_count = thread_config->thread_event_count + 1;
   thread_config->thread_state = CL_THREAD_WAITING;
   CL_LOG(CL_LOG_DEBUG, "cl_thread_wait_for_event() start waiting ...");

   
   ret = cl_thread_wait_for_thread_condition(thread_config->thread_event_condition, sec, micro_sec);

   thread_config->thread_state = CL_THREAD_RUNNING;
   CL_LOG(CL_LOG_DEBUG, "cl_thread_wait_for_event() wake up");


   return ret;
}



#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_thread_get_state()"
const char* cl_thread_get_state(cl_thread_settings_t* thread_config) {

   if ( thread_config == NULL ) {
      return "got no thread config";
   }

   return cl_thread_convert_state_id(thread_config->thread_state);
}
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_thread_convert_state_id()"
const char* cl_thread_convert_state_id(int thread_state) {


   switch (thread_state) {
      case CL_THREAD_RUNNING: 
         return "running";
      case CL_THREAD_WAITING:
         return "waiting";
      case CL_THREAD_EXIT:
         return "done";
      case CL_THREAD_STARTING:
         return "starting";
      case CL_THREAD_CANCELED:
         return "canceled";
      case CL_THREAD_CREATOR:
         return "creator";
      default:
         return "undefined";
   }
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_thread_clear_events()"
int cl_thread_clear_events(cl_thread_settings_t *thread_config) {
   if (thread_config == NULL) {
      return CL_RETVAL_PARAMS;
   }
   return cl_thread_clear_triggered_conditions(thread_config->thread_event_condition);
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_thread_trigger_event()"
int cl_thread_trigger_event(cl_thread_settings_t *thread_config) {
   int ret_val;
   if (thread_config == NULL) {
      return CL_RETVAL_PARAMS;
   }
   
   ret_val = cl_thread_trigger_thread_condition(thread_config->thread_event_condition, 0);

   CL_LOG(CL_LOG_DEBUG, "cl_thread_trigger_event() called");
   return ret_val;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_thread_func_testcancel()"
int cl_thread_func_testcancel(cl_thread_settings_t* thread_config) {
   int ret_val = 0;
   int execute_pop = 0;
   CL_LOG(CL_LOG_INFO, "checking for cancelation ...");


   /* push default cleanup function */
   /* (void(*)(void*)) */
   pthread_cleanup_push( (void(*)(void*)) cl_thread_default_cleanup_function, thread_config );

   ret_val = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);

   if ( ret_val == 0 ) {
      pthread_testcancel();
      ret_val = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
   }

   /* remove cleanup function from stack without execution */
   pthread_cleanup_pop(execute_pop);  /* client_thread_cleanup */

   if ( ret_val != 0) {
      return CL_RETVAL_THREAD_CANCELSTATE_ERROR;
   }
   return CL_RETVAL_OK;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_thread_func_startup()"
int cl_thread_func_startup(cl_thread_settings_t* thread_config) {
   int ret_val = CL_RETVAL_OK;
   if (thread_config == NULL) {
      return CL_RETVAL_PARAMS;
   }
   pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);

   /* set thread config data */
   if (cl_thread_set_thread_config(thread_config) != CL_RETVAL_OK) {
      printf("cl_thread_set_thread_config() error\n");
   }
   thread_config->thread_event_count = 0;

   ret_val = cl_thread_trigger_thread_condition(thread_config->thread_startup_condition,0);

   if (ret_val == CL_RETVAL_OK) {
      thread_config->thread_state = CL_THREAD_RUNNING;
   }
   CL_LOG(CL_LOG_DEBUG, "cl_thread_func_startup() done");
   return ret_val;
}
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_thread_set_thread_config()"
int cl_thread_set_thread_config(cl_thread_settings_t* thread_config) {

   pthread_mutex_lock(&global_thread_config_key_mutex);
   if (global_thread_config_key_done != 0) {
      if (pthread_setspecific(global_thread_config_key, thread_config) != 0) {
         pthread_mutex_unlock(&global_thread_config_key_mutex);
         return CL_RETVAL_THREAD_SETSPECIFIC_ERROR;
      }
      pthread_mutex_unlock(&global_thread_config_key_mutex);
      return CL_RETVAL_OK;
   }
   pthread_mutex_unlock(&global_thread_config_key_mutex);
   return CL_RETVAL_NOT_THREAD_SPECIFIC_INIT;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_thread_unset_thread_config()"
int cl_thread_unset_thread_config(void) {

   pthread_mutex_lock(&global_thread_config_key_mutex);
   if (global_thread_config_key_done != 0) {
      if (pthread_setspecific(global_thread_config_key, NULL) != 0) {
         pthread_mutex_unlock(&global_thread_config_key_mutex);
         return CL_RETVAL_THREAD_SETSPECIFIC_ERROR;
      }
      pthread_mutex_unlock(&global_thread_config_key_mutex);
      return CL_RETVAL_OK;
   } 
   pthread_mutex_unlock(&global_thread_config_key_mutex);
   return CL_RETVAL_NOT_THREAD_SPECIFIC_INIT;
}



#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_thread_func_cleanup()"
int cl_thread_func_cleanup(cl_thread_settings_t* thread_config) {
   if (thread_config == NULL) {
      return CL_RETVAL_PARAMS;
   }
   thread_config->thread_state = CL_THREAD_EXIT;
   CL_LOG(CL_LOG_DEBUG, "cl_thread_func_cleanup() called");
   cl_thread_unset_thread_config();
   return CL_RETVAL_OK;
}
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_thread_default_cleanup_function()"
void cl_thread_default_cleanup_function(cl_thread_settings_t* thread_config) {
   if (thread_config != NULL) {
      thread_config->thread_state = CL_THREAD_CANCELED;
      CL_LOG(CL_LOG_INFO,  "cl_thread_default_cleanup_function() called");
      /*  There is no need to unset thread config - This can result in
       *  unexpected cl_log_list - logging output.
       */ 
#if 0
      cl_thread_unset_thread_config();
#endif
   }
}


