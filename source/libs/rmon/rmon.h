#ifndef __RMON_H
#define __RMON_H
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

#include <stdarg.h>
#include <sys/types.h>

#include "rmon_monitoring_level.h"

extern monitoring_level DEBUG_ON;

int  rmon_condition(int layer, int debug_class);
int  rmon_is_enabled(void);
void rmon_mopen(int *argc, char *argv[], char *programname);
void rmon_menter(const char *func);
void rmon_mtrace(const char *func, const char *file, int line);
void rmon_mprintf(const char *fmt, ...);
void rmon_mexit(const char *func, const char *file, int line);

#define __CONDITION(x) rmon_condition(TOP_LAYER, x)

#endif /* __RMON_H */

