#ifndef PARSE_QSUB_H
#define PARSE_QSUB_H
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



#include "sge_boundaries.h"
#include "cull.h"

/*
** defines for pseudo-arguments
*/
#define STR_PSEUDO_JOBID       "jobid"
#define STR_PSEUDO_SCRIPT      "script"
#define STR_PSEUDO_JOBARG      "jobarg"
#define STR_PSEUDO_SCRIPTLEN   "scriptlen"
#define STR_PSEUDO_SCRIPTPTR   "scriptptr"

/*
** flags
*/
#define FLG_USE_PSEUDOS 1
#define FLG_QALTER      2


lList *cull_parse_cmdline(char **arg_list, char **envp, lList **pcmdline, u_long32 flags);

/************************************************************************/
int cull_parse_path_list(lList **lpp, char *path_str);
int cull_parse_jid_hold_list(lList **lpp, char *str);

int var_list_parse_from_string(lList **lpp, const char *variable_str,
                               int check_environment);

#endif /* PARSE_QSUBL_H */













