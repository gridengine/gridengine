#ifndef __SGE_CONFL_H
#define __SGE_CONFL_H

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

#ifdef  __cplusplus
extern "C" {
#endif

/* *INDENT-OFF* */ 

enum {
   CONF_hname = CONF_LOWERBOUND,
   CONF_version,
   CONF_entries                 /* Points to CF_Type list */
};

ILISTDEF(CONF_Type, Configuration, SGE_CONFIG_LIST)
   SGE_KSTRINGHU(CONF_hname)
   SGE_XULONG(CONF_version)
   SGE_TLIST(CONF_entries, CF_Type)
LISTEND 

NAMEDEF(CONFN)
   NAME("CONF_hname")
   NAME("CONF_version")
   NAME("CONF_entries")
NAMEEND

#define CONFS sizeof(CONFN)/sizeof(char*)

/*
 * configuration list 
 */
enum {
   CF_name = CF_LOWERBOUND,  /* name of configuration element */
   CF_value,                 /* value of configuration element */
   CF_sublist,               /* sub-list of type CF_Type */
   CF_local                  /* global value can be overridden */
};

SLISTDEF(CF_Type, ConfigEntry)
   SGE_STRINGHU(CF_name)
   SGE_STRING(CF_value)
   SGE_LIST(CF_sublist)
   SGE_XULONG(CF_local)
LISTEND 

NAMEDEF(CFN)
   NAME("CF_name")
   NAME("CF_value")
   NAME("CF_sublist")
   NAME("CF_local")
NAMEEND

/* *INDENT-ON* */

#define CFS sizeof(CFN)/sizeof(char*)
#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_CONFL_H */
