#ifndef __SGE_PROGNAMES_H
#define __SGE_PROGNAMES_H
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

#include "sge.h" 
#include "basis_types.h"

#include "sge_arch.h"

#define SGE_PREFIX      "sge_"

#define SGE_COMMD       "sge_commd"

#define SGE_SHEPHERD    "sge_shepherd"

#define SGE_COSHEPHERD  "sge_coshepherd"

#define SGE_QMASTER  "sge_qmaster"

#define SGE_SHADOWD     "sge_shadowd"

#define PE_HOSTFILE     "pe_hostfile"

/* who - must match prognames[] in prognames.c */

enum {
 QALTER =  1 , 
 QCONF           ,       /* 2  */        
 QDEL            ,       /* 3 */
 QHOLD           ,       /* 4 */
 QIDLE           ,       /* 5 */
 QMASTER         ,       /* 6 */
 QMOD            ,       /* 7 */
 QMOVE           ,       /* 8 */
 QMSG            ,       /* 9 */
 QRESUB          ,       /* 10 */
 QRLS            ,       /* 11 */
 QSELECT         ,       /* 12 */
 QSH             ,       /* 13 */
 QRSH            ,       /* 14 */
 QLOGIN          ,       /* 15 */
 QSIG            ,       /* 16 */
 QSTAT           ,       /* 17 */
 QSUB            ,       /* 18 */
 EXECD           ,       /* 19 */
 MAX_ANCILLARY   = EXECD , /* 19 */
 QEVENT,                 /* 20 */
 QUSERDEFINED    ,       /* 21 */
 ALL_OPT         ,       /* 22 */

/* programs with numbers > ALL_OPT do not use the old parsing */

 QUSAGE          ,       /* 23 */
 DCMD            ,       /* 24 */
 DSH             ,       /* 25 */
 QMON            ,       /* 26 */
 SCHEDD          ,       /* 27 */
 QSCHED          ,       /* 28 */
 QACCT           ,       /* 29 */
 QSTD            ,       /* 30 */
 COMMD           ,       /* 31 */
 SHADOWD         ,       /* 32 */
 CW3             ,       /* 33 */
 PVM_TASKER      ,       /* 34 */
 QIDLD           ,       /* 35 */
 PVM_RMANAGER    ,       /* 36 */
 QHOST           ,       /* 37 */
 COMMDCNTL       ,       /* 38 */ 
 SPOOLDEFAULTS   ,       /* 39 */
 JAPI            ,       /* 40 */
 JAPI_EC                 /* 41 */
};


typedef void (*sge_exit_func_t)(int);

extern const char *prognames[];

void sge_getme(u_long32 sge_formal_prog_name);

const char *    uti_state_get_sge_formal_prog_name(void);
const char *    uti_state_get_qualified_hostname(void);
const char *    uti_state_get_unqualified_hostname(void);
u_long32        uti_state_get_mewho(void);
u_long32        uti_state_get_uid(void);
u_long32        uti_state_get_gid(void);
int             uti_state_get_daemonized(void);
const char *    uti_state_get_user_name(void);
const char *    uti_state_get_default_cell(void);
int             uti_state_get_exit_on_error(void);
sge_exit_func_t uti_state_get_exit_func(void);

void uti_state_set_qualified_hostname(const char *s);
void uti_state_set_daemonized(int daemonized);
void uti_state_set_mewho(u_long32 who);
void uti_state_set_exit_on_error(int i);
void uti_state_set_exit_func(sge_exit_func_t f);

#endif /* __SGE_PROGNAMES_H */


