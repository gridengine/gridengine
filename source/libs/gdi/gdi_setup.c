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
//#	include "def.h"
#	include "win32nativetypes.h"
#endif /* WIN32NATIVE */

#include "sge_gdi.h"
#include "sge_gdi_intern.h"
#include "commlib.h"
#include "sge_prognames.h"
#include "sge_all_listsL.h"
#include "sig_handlers.h"
#include "sgermon.h"
#include "sge_exit.h"
#include "utility.h"
#include "qm_name.h"

static void default_exit_func(int i);

static exit_func_type gdi_exit_func = default_exit_func;

static int made_setup = 0;
static int program_id = QUSERDEFINED;
static int isalive = 0;
static int exit_on_error = 1;

/****** gdi_setup/sge_gdi_setup() **********************************************
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
*
********************************************************************************
*/
int sge_gdi_setup(char *programname)
{
   lList *alp = NULL;

   DENTER(TOP_LAYER, "sge_gdi_setup");

#ifdef __SGE_COMPILE_WITH_GETTEXT__  
   /* init language output for gettext() , it will use the right language */
   install_language_func((gettext_func_type)        gettext,
                         (setlocale_func_type)      setlocale,
                         (bindtextdomain_func_type) bindtextdomain,
                         (textdomain_func_type)     textdomain);
   sge_init_language(NULL,NULL);   
#endif /* __SGE_COMPILE_WITH_GETTEXT__  */

   lInit(nmv);

   sge_setup(program_id, exit_on_error?NULL:&alp);
   if (alp) {
      alp = lFreeList(alp);
/*       fprintf(stderr, "%s", lGetString(lFirst(alp), AN_text)); */
      DEXIT;
      return AE_QMASTER_DOWN;
   }
   prepare_enroll(programname, 0, NULL);
   install_exit_func(gdi_exit_func);


   /* check if master is alive */
   if (isalive) {
      DEXIT;
      return check_isalive(sge_get_master(0));
   }

/* standard signal handler should be replaceable */
/*    sge_setup_sig_handlers(me.who); */

   /* register successfull setup */
   made_setup = 1;

   DEXIT;
   return CL_OK;
}


/****** gdi_setup/sge_gdi_param() **********************************************
*  NAME
*     sge_gdi_param() -- add some additional parameters for sge_gdi_setup() 
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
*                       SET_MEWHO - intval will be the program id
*                       SET_LEAVE - strval is a pointer to an exit function
*                       SET_EXIT_ON_ERROR - intval is 0 or 1
*                       SET_ISALIVE - 0 or 1 -  do check whether master is alive
*     int intval   - integer value or 0 
*     char* strval - string value or NULL 
*
*  RESULT
*     AE_OK            - parameter was set successfully
*     AE_ALREADY_SETUP - sge_gdi_setup() was called befor sge_gdi_param() 
*     AE_UNKNOWN_PARAM - param is an unknown constant
*
********************************************************************************
*/
int sge_gdi_param(
int param,
int intval,
char *strval 
) {
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
      gdi_exit_func = (exit_func_type) strval;
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

static void default_exit_func(
int i 
) {
   leave_commd();  /* tell commd we're going */
}

/****** gdi_shutdown/sge_gdi_shutdown() ****************************************
*  NAME
*     sge_gdi_shutdown() -- gdi shutdown.
*
*  SYNOPSIS
*     int sge_gdi_shutdown()
*
*  FUNCTION
*     This function has to be called before quitting the program. It cancels
*     registration at commd.
*
*  INPUTS
*
*  RESULT
*
********************************************************************************
*/  
int sge_gdi_shutdown()
{
   DENTER(TOP_LAYER, "sge_gdi_shutdown");

   default_exit_func(0);

   DEXIT;
   return 0;
}
