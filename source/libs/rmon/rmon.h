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


#ifdef  __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <sys/types.h>

#include "rmon_monitoring_level.h"

#define RMON_NONE    0
#define RMON_LOCAL   1
#define RMON_REMOTE  2

extern int LAYER;
extern monitoring_level DEBUG_ON;
extern u_long MLEVEL;
extern u_long MTYPE;
extern u_long DEBUG_TRACEID;

int rmon_condition(register int layer, register int debug_class);
void rmon_mopen(int *argc, char *argv[], char *programname);
void rmon_menter(const char *func);
void rmon_mtrace(const char *func, const char *file, int line);
void rmon_mjobtrace(u_long jobid, const char *fmt, ...);
void rmon_mprintf(const char *fmt, ...);
void rmon_mexit(const char *func, const char *file, int line);
void rmon_mexite(const char *func, const char *file, int line);
void rmon_mclose(void);
void rmon_d_monlev(monitoring_level *ml);
int rmon_mmayclose(int fd);
int rmon_mexeclp(char *file, ...);
int rmon_mexecvp(char *file, char *argv[]);
int rmon_mfork(void);
void rmon_mpush_layer(int new_layer);
int rmon_mpop_layer(void);

int rmon_d_writenbytes(register int sfd, register char *ptr, register int n);

#define __CONDITION(x) rmon_condition(LAYER, x)

#ifdef  __cplusplus
}
#endif

#endif /* __RMON_H */

