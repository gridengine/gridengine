#ifndef _EXEC_WRAPPER_H_
#define _EXEC_WRAPPER_H_
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


/* values for 'addr' of set_sgemode() */
enum { 
   CATCH_EXEC_MODE_REMOTE,
   CATCH_EXEC_MODE_VERBOSE,
   CATCH_EXEC_MODE_IMMEDIATE,
   CATCH_EXEC_MODE_FORCE_REMOTE
};

#if defined(__TCSH_SOURCE_CODE)
   /* prototypes: tcsh source code like */

   typedef void (*print_func_t) __P((char *fmt, ...));
   int sge_execv __P((const char *path, char *const argv[], const char *expath, int close_stdin));
   void sge_init __P((print_func_t ostream));
   void set_sgemode __P((int addr, int value)); 
   int get_sgemode __P((int addr)); 
#else
   /* prototypes: sge source code like */

   typedef void (*print_func_t)(char *fmt, ...);
   int sge_execv(char *path, char *argv[], char *expath, int close_stdin);
   void sge_init(print_func_t ostream);
   void set_sgemode(int addr, int value);
   int get_sgemode(int addr);
   char** sge_get_qtask_args(void *context, char *taskname, lList **answer_list);
#endif

#endif /* _EXEC_WRAPPER_H_ */
