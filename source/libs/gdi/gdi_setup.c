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
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>

#ifdef WIN32NATIVE
#	include "win32nativetypes.h"
#endif /* WIN32NATIVE */

#include "sge_log.h"
#include "sge_gdi.h"
#include "setup.h"
#include "gdi_setup.h"
#include "sge_any_request.h"
#include "sge_gdiP.h"
#include "commlib.h"
#include "cull_list.h"
#include "sge_prog.h"
#include "sge_all_listsL.h"
#include "sig_handlers.h"
#include "sgermon.h"
#include "sge_unistd.h"
#include "qm_name.h"
#include "sge_security.h"
#include "sge_answer.h"
#include "sge_uidgid.h"
#include "setup_path.h"
#include "sge_feature.h"
#include "sge_bootstrap.h"

static void default_exit_func(int i);
static void gdi_init_mt(void);

struct gdi_state_t {
   /* gdi request base */
   u_long32 request_id;     /* incremented with each GDI request to have a unique request ID
                               it is ensured that the request ID is also contained in answer */
   int      daemon_first;
   int      first_time;
   int      commd_state;

   int      made_setup;
   int      isalive;
   int      reread_qmaster_file;

   char     cached_master_name[MAXHOSTLEN];
};

static pthread_key_t   gdi_state_key;

static pthread_once_t gdi_once_control = PTHREAD_ONCE_INIT;

static void gdi_state_destroy(void* state) {
   free(state);
}

static void gdi_init_mt(void) {
   pthread_key_create(&gdi_state_key, &gdi_state_destroy);
} 
  
void gdi_once_init(void) {
   /* uti */
   uidgid_mt_init();

   bootstrap_mt_init();
   feature_mt_init();

   /* sec */
#ifdef SECURE
   sec_mt_init();
#endif

   /* gdi */
   gdi_init_mt();
   path_mt_init();
}

static void gdi_state_init(struct gdi_state_t* state) {
   state->request_id = 0;
   state->daemon_first = 1;
   state->first_time = 1;
   state->commd_state = COMMD_UNKNOWN;

   state->made_setup = 0;
   state->isalive = 0;
   state->reread_qmaster_file = 0;
   strcpy(state->cached_master_name, "");
}

/****** gid/gdi_setup/gdi_mt_init() ************************************************
*  NAME
*     gdi_mt_init() -- Initialize GDI state for multi threading use.
*
*  SYNOPSIS
*     void gdi_mt_init(void) 
*
*  FUNCTION
*     Set up GDI. This function must be called at least once before any of the
*     GDI functions is used. This function is idempotent, i.e. it is safe to
*     call it multiple times.
*
*     Thread local storage for the GDI state information is reserved. 
*
*  INPUTS
*     void - NONE 
*
*  RESULT
*     void - NONE
*
*  NOTES
*     MT-NOTE: gdi_mt_init() is MT safe 
*
*******************************************************************************/
void gdi_mt_init(void)
{
   pthread_once(&gdi_once_control, gdi_once_init);
}

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
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key, "gdi_state_get_request_id");
   return gdi_state->request_id;
}

int gdi_state_get_daemon_first(void)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key, "gdi_state_get_daemon_first");
   return gdi_state->daemon_first;
}

int gdi_state_get_first_time(void)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key, "gdi_state_get_first_time");
   return gdi_state->first_time;
}

int gdi_state_get_commd_state(void)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key, "gdi_state_get_commd_state");
   return gdi_state->commd_state;
}

int gdi_state_get_made_setup(void)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key, "gdi_state_get_made_setup");
   return gdi_state->made_setup;
}

int gdi_state_get_isalive(void)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key, "gdi_state_get_isalive");
   return gdi_state->isalive;
}

int gdi_state_get_reread_qmaster_file(void)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key, "gdi_state_get_reread_qmaster_file");
   return gdi_state->reread_qmaster_file;
}

char *gdi_state_get_cached_master_name(void)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key, "gdi_state_get_cached_master_name");
   return gdi_state->cached_master_name;
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
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key, "gdi_state_set_request_id");
   gdi_state->request_id = id;
}

void gdi_state_set_daemon_first(int i)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key, "gdi_state_set_daemon_first");
   gdi_state->daemon_first = i;
}

void gdi_state_set_first_time(int i)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key, "gdi_state_set_first_time");
   gdi_state->first_time = i;
}

void gdi_state_set_commd_state(int i)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key, "gdi_state_set_commd_state");
   gdi_state->commd_state = i;
}

void gdi_state_set_made_setup(int i)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key, "gdi_state_set_made_setup");
   gdi_state->made_setup = i;
}

void gdi_state_set_isalive(int i)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key, "gdi_state_set_isalive");
   gdi_state->isalive = i;
}

void gdi_state_set_reread_qmaster_file(int i)
{
   GET_SPECIFIC(struct gdi_state_t, gdi_state, gdi_state_init, gdi_state_key, "gdi_state_set_reread_qmaster_file");
   gdi_state->reread_qmaster_file = i;
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
*  OUTPUT
*     lList **alpp - If the GDI setup fails and alpp is non-NULL an answer 
*                    list is returned containing diagnosis information about
*                    the problem during setup.
*
*  RESULT
*     AE_OK            - everything is fine
*     AE_QMASTER_DOWN  - the master is down 
*     AE_ALREADY_SETUP - sge_gdi_setup() was already called
*
*  NOTES
*     MT-NOTES: sge_gdi_setup() is MT safe
*     MT-NOTES: In a mulit threaded program each thread must call 
*     MT-NOTES: before sge_gdi() and other calls can be used
******************************************************************************/
int sge_gdi_setup(const char *programname, lList **alpp)
{

   DENTER(TOP_LAYER, "sge_gdi_setup");

   /* initialize libraries */
   pthread_once(&gdi_once_control, gdi_once_init);
   if (gdi_state_get_made_setup()) {
      answer_list_add_sprintf(alpp, STATUS_EEXIST, ANSWER_QUALITY_WARNING, "GDI already setup\n");
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

   if (sge_setup(uti_state_get_mewho(), alpp)) {
      DEXIT;
      return AE_QMASTER_DOWN;
   }

   prepare_enroll(programname, 0, NULL);

   /* ensure gdi default exit func is used if no-one has been specified */
   if (!uti_state_get_exit_func())
      uti_state_set_exit_func(default_exit_func);   

   gdi_state_set_made_setup(1);

   /* check if master is alive */
   if (gdi_state_get_isalive()) {
      DEXIT;  /* TODO: shall we rework the gdi function return values ? CR */
      if (check_isalive(sge_get_master(0)) != CL_RETVAL_OK) {
         return AE_QMASTER_DOWN;
      } else {
         return AE_OK;
      }
   }

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
*  NOTES
*     MT-NOTES: sge_gdi_setup() is MT safe
*     MT-NOTES: In a mulit threaded program each thread must call 
*     MT-NOTES: sge_gdi_param() to have these settings in effect
******************************************************************************/
int sge_gdi_param(int param, int intval, char *strval) 
{
   DENTER(TOP_LAYER, "sge_gdi_param");

/* initialize libraries */
   pthread_once(&gdi_once_control, gdi_once_init);
   if (gdi_state_get_made_setup()) {
      DEXIT;
      return AE_ALREADY_SETUP;
   }

   switch (param) {
   case SET_MEWHO:
      uti_state_set_mewho(intval);
      break;
   case SET_ISALIVE:
      gdi_state_set_isalive(intval);
      break;
   case SET_LEAVE:
      uti_state_set_exit_func((sge_exit_func_t) strval);
      break;
   case SET_EXIT_ON_ERROR:
      uti_state_set_exit_on_error(intval);
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
#ifdef ENABLE_NGC
   cl_com_cleanup_commlib();
#else
   leave_commd();  /* tell commd we're going */
#endif
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
*
*  NOTES
*     MT-NOTES: sge_gdi_setup() is MT safe
******************************************************************************/  
int sge_gdi_shutdown()
{
   DENTER(TOP_LAYER, "sge_gdi_shutdown");

   /* initialize libraries */
   pthread_once(&gdi_once_control, gdi_once_init);
   default_exit_func(0);

   DEXIT;
   return 0;
}
