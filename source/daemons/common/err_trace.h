#ifndef __ERR_TRACE_H
#define __ERR_TRACE_H
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
 *  License at http://www.gridengine.sunsource.net/license.html
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



typedef int (*tShepherd_trace)(char *str);

void shepherd_error(char *str);
void shepherd_error_impl(char *str, int do_exit);
int shepherd_trace(char *str);
void err_trace_chown_files(int uid);
extern int foreground;      /* != 0 if we can write to stderr/out     */
void shepherd_log_as_admin_user(void);
int count_exit_status(void);

#define SHEPHERD_ERROR(x) (sprintf x, shepherd_error(err_str))
#define SHEPHERD_TRACE(x) (sprintf x, shepherd_trace(err_str))

#endif /* __ERR_TRACE_H */

