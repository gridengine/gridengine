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
#include <signal.h>

#ifdef WIN32NATIVE
#	include "win32nativetypes.h"
#endif /* WIN32NATIVE */

#include "sge_gdi.h"
#include "sge_gdi_intern.h"
#include "commlib.h"
#include "sge_prog.h"
#include "sge_all_listsL.h"
#include "sig_handlers.h"
#include "sgermon.h"
#include "sge_unistd.h"
#include "qm_name.h"
#include "sge_security.h"

struct gdi_state_t {
   /* gdi request base */
   u_long32 request_id;     /* incremented with each GDI request to have a unique request ID
                               it is ensured that the request ID is also contained in answer */
   int daemon_first;
   int first_time;
   int commd_state;

   /* event client */
   int ec_config_changed;
   int ec_need_register;
   lListElem *ec;
   u_long32 ec_reg_id;

   /* event mirror */
/*    mirror_entry mirror_base[SGE_EMT_ALL]; */
/*    u_long32 last_heared; */
};

#if defined(SGE_MT)
#include <pthread.h>
static pthread_key_t   gdi_state_key;
static pthread_mutex_t gdi_setup_mutex = PTHREAD_MUTEX_INITIALIZER;
#else
static struct gdi_state_t gdi_state_opaque = {
  0, 1, 1, COMMD_UNKNOWN, 0, 1, NULL };
struct gdi_state_t *gdi_state = &gdi_state_opaque;
#endif

#if defined(SGE_MT)
static void gdi_state_destroy(void* state) {
   lFreeElem(((struct gdi_state_t*)state)->ec);
   free(state);
}

void gdi_init_mt() {
   pthread_key_create(&gdi_state_key, &gdi_state_destroy);
} 
  
static void gdi_state_init(struct gdi_state_t* state) {
   state->request_id = 0;
   state->daemon_first = 1;
   state->first_time = 1;
   state->commd_state = COMMD_UNKNOWN;

   state->ec_config_changed = 0;
   state->ec_need_register = 1;
   state->ec = NULL;
   state->ec_reg_id = 0;
}
#endif


static void default_exit_func(int i);

static sge_exit_func_t gdi_exit_func = default_exit_func;

static int made_setup = 0;
static int program_id = QUSERDEFINED;
static int isalive = 0;
static int exit_on_error = 1;


/****** libs/gdi/gdi_state_get_????() ************************************
*  NAME
*     gdi_state_get_????() - read access to gdilib global variables
*
*  FUNCTION
*     Provides access to either global variable or per thread global
*     variable.
*
******************************************************************************/


u_long32 gdi_state_get_request_id(void)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key);
   return gdi_state->request_id;
}

int gdi_state_get_daemon_first(void)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key);
   return gdi_state->daemon_first;
}

int gdi_state_get_first_time(void)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key);
   return gdi_state->first_time;
}

int gdi_state_get_commd_state(void)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key);
   return gdi_state->commd_state;
}

int gdi_state_get_ec_config_changed(void)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key);
   return gdi_state->ec_config_changed;
}

int gdi_state_get_ec_need_register(void)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key);
   return gdi_state->ec_need_register;
}

lListElem *gdi_state_get_ec(void)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key);
   return gdi_state->ec;
}

u_long32 gdi_state_get_ec_reg_id(void)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key);
   return gdi_state->ec_reg_id;
}

/****** libs/gdi/gdi_state_set_????() ************************************
*  NAME
*     gdi_state_set_????() - write access to gdilib global variables
*
*  FUNCTION
*     Provides access to either global variable or per thread global
*     variable.
*
******************************************************************************/

void gdi_state_set_request_id(u_long32 id)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key);
   gdi_state->request_id = id;
}

void gdi_state_set_daemon_first(int i)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key);
   gdi_state->daemon_first = i;
}

void gdi_state_set_first_time(int i)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key);
   gdi_state->first_time = i;
}

void gdi_state_set_commd_state(int i)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key);
   gdi_state->commd_state = i;
}

void gdi_state_set_ec_config_changed(int i)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key);
   gdi_state->ec_config_changed = i;
}

void gdi_state_set_ec_need_register(int i)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key);
   gdi_state->ec_need_register = i;
}

void gdi_state_set_ec(lListElem *ec)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key);
   lFreeElem(gdi_state->ec);
   gdi_state->ec = ec;
}

void gdi_state_set_ec_reg_id(u_long32 id)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key);
   gdi_state->ec_reg_id = id;
}

/****** gdi/setup/sge_gdi_setup() *********************************************
*  NAME
*     sge_gdi_setup() -- setup GDI 
*
*  SYNOPSIS
*     int sge_gdi_setup(char* programname) 
*
*  FUNCTION
*     This function initializes the Gridengine Database Interface (GDI)
*     and their underlaying communication mechanisms 
*
*  INPUTS
*     char* programname - Name of thye program without path information 
*
*  RESULT
*     AE_OK            - everything is fine
*     AE_QMASTER_DOWN  - the master is down 
*     AE_ALREADY_SETUP - sge_gdi_setup() was already called
******************************************************************************/
int sge_gdi_setup(const char *programname)
{
   lList *alp = NULL;

   DENTER(TOP_LAYER, "sge_gdi_setup");

#if defined(SGE_MT)
   pthread_mutex_lock(&gdi_setup_mutex);
#endif

   if (made_setup) {
#if defined(SGE_MT)
      pthread_mutex_unlock(&gdi_setup_mutex);
#endif
      DEXIT;
      return AE_ALREADY_SETUP;
   }

#ifdef __SGE_COMPILE_WITH_GETTEXT__  
   /* init language output for gettext() , it will use the right language */
   sge_init_language_func((gettext_func_type)        gettext,
                         (setlocale_func_type)      setlocale,
                         (bindtextdomain_func_type) bindtextdomain,
                         (textdomain_func_type)     textdomain);
   sge_init_language(NULL,NULL);   
#endif /* __SGE_COMPILE_WITH_GETTEXT__  */

   lInit(nmv);

   sge_setup(program_id, exit_on_error?NULL:&alp);
   if (alp) {
      alp = lFreeList(alp);
#if defined(SGE_MT)
      pthread_mutex_unlock(&gdi_setup_mutex);
#endif
      DEXIT;
      return AE_QMASTER_DOWN;
   }
   prepare_enroll(programname, 0, NULL);
   sge_install_exit_func(gdi_exit_func);


   /* check if master is alive */
   if (isalive) {
      made_setup = 1;
#if defined(SGE_MT)
      pthread_mutex_unlock(&gdi_setup_mutex);
#endif
      DEXIT;
      return check_isalive(sge_get_master(0));
   }

   /* register successfull setup */
   made_setup = 1;
#if defined(SGE_MT)
      pthread_mutex_unlock(&gdi_setup_mutex);
#endif

   DEXIT;
   return AE_OK;
}


/****** gdi/setup/sge_gdi_param() *********************************************
*  NAME
*     sge_gdi_param() -- add some additional params for sge_gdi_setup() 
*
*  SYNOPSIS
*     int sge_gdi_param(int param, int intval, char* strval) 
*
*  FUNCTION
*     This function makes it possible to pass additional parameters to
*     sge_gdi_setup(). It has to be called before sge_gdi_setup() 
*
*  INPUTS
*     int param    - constant identifying the parameter 
*                       SET_MEWHO - 
*                          intval will be the program id
*                       SET_LEAVE - 
*                          strval is a pointer to an exit function
*                       SET_EXIT_ON_ERROR - 
*                          intval is 0 or 1
*                       SET_ISALIVE - 0 or 1 -  
*                          do check whether master is alive
*     int intval   - integer value or 0 
*     char* strval - string value or NULL 
*
*  RESULT
*     AE_OK            - parameter was set successfully
*     AE_ALREADY_SETUP - sge_gdi_setup() was called beforie 
*                        sge_gdi_param() 
*     AE_UNKNOWN_PARAM - param is an unknown constant
*
******************************************************************************/
int sge_gdi_param(int param, int intval, char *strval) 
{
   DENTER(TOP_LAYER, "sge_gdi_param");

   if (made_setup) {
      DEXIT;
      return AE_ALREADY_SETUP;
   }

   switch (param) {
   case SET_MEWHO:
      program_id = intval;
      break;
   case SET_ISALIVE:
      isalive = 1;
      break;
   case SET_LEAVE:
      gdi_exit_func = (sge_exit_func_t) strval;
      break;
   case SET_EXIT_ON_ERROR:
      exit_on_error = intval;
      break;
   default:
      DEXIT;
      return AE_UNKNOWN_PARAM;
   }

   DEXIT;
   return AE_OK;
}

static void default_exit_func(int i) 
{
   sge_security_exit(i); 

   leave_commd();  /* tell commd we're going */
}

/****** gdi/setup/sge_gdi_shutdown() ******************************************
*  NAME
*     sge_gdi_shutdown() -- gdi shutdown.
*
*  SYNOPSIS
*     int sge_gdi_shutdown()
*
*  FUNCTION
*     This function has to be called before quitting the program. It 
*     cancels registration at commd.
******************************************************************************/  
int sge_gdi_shutdown()
{
   DENTER(TOP_LAYER, "sge_gdi_shutdown");

   gdi_exit_func(0);

   DEXIT;
   return 0;
}
