#ifndef __SGE_SPOOLL_H
#define __SGE_SPOOLL_H
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

/****** spool/--SPC_Type ***************************************
*
*  NAME
*     SPC_Type -- Spooling context
*
*  ELEMENTS
*     SGE_STRING(SPC_name, CULL_HASH | CULL_UNIQUE)
*        Unique name of the spooling context
*
*     SGE_LIST(SPC_rules, SPR_Type, CULL_DEFAULT)
*        List of spooling rules
*  
*     SGE_LIST(SPC_types, SPT_Type, CULL_DEFAULT)
*        List of spoolable object types with references to 
*        rules.
*
*  FUNCTION
*        A spooling context describes the way how objects
*        are spooled (read and written).
*
*        A spooling context contains one or multiple rules for 
*        spooling. A rule can for example describe a database 
*        connection.
*
*        It also contains a list of types that can be spooled.
*        A default entry for all types can be created.
*        A type references one or multiple rules which will 
*        be executed for writing or deleting data.
*        Exactly one rule can be defined to be the default rule
*        for reading objects.
*
*        +----------+       1:n       +----------+
*        | SPC_Type |----------------<| SPT_Type | 
*        +----------+                 +----------+
*             |                             |
*             |                             |
*             |                             |
*             | 1                           |
*             | :                           |
*             | n                           |
*             |                             |
*             ^                             |
*        +----------+   1:n, one is default |
*        | SPR_Type |>----------------------+
*        +----------+
*
*
*  SEE ALSO
*     spool/--Spooling
*
****************************************************************************
*/

enum {
   SPC_name = SPC_LOWERBOUND,  /* name of spooling context */
   SPC_rules,                  /* list of spooling rules */
   SPC_types                   /* list of object types to spool */
};

LISTDEF(SPC_Type)
   SGE_STRING(SPC_name, CULL_HASH | CULL_UNIQUE)
   SGE_LIST(SPC_rules, SPR_Type, CULL_DEFAULT)
   SGE_LIST(SPC_types, SPT_Type, CULL_DEFAULT)
LISTEND

NAMEDEF(SPCN)
   NAME("SPC_name")
   NAME("SPC_rules")
   NAME("SPC_types")
NAMEEND

#define SPCS sizeof(SPCN)/sizeof(char *)

enum {
   SPR_name = SPR_LOWERBOUND,        /* name of the rule */
   SPR_url,                          /* an url, e.g. base dir, database url */
   SPR_startup_func,                 /* function pointer: startup, e.g. connect to database */
   SPR_shutdown_func,                /* function pointer: shutdown, e.g. disconnect from db */
   SPR_list_func,                    /* function pointer: read master list */
   SPR_read_func,                    /* function pointer: read an object */
   SPR_write_func,                   /* function pointer: write an object */
   SPR_delete_func,                  /* function pointer: delete an object */
   SPR_clientdata                    /* any rule specific data */
};

LISTDEF(SPR_Type)
   SGE_STRING(SPR_name, CULL_HASH | CULL_UNIQUE)
   SGE_STRING(SPR_url, CULL_DEFAULT)
   SGE_REF(SPR_startup_func, CULL_ANY_SUBTYPE, CULL_DEFAULT)
   SGE_REF(SPR_shutdown_func, CULL_ANY_SUBTYPE, CULL_DEFAULT)
   SGE_REF(SPR_list_func, CULL_ANY_SUBTYPE, CULL_DEFAULT)
   SGE_REF(SPR_read_func, CULL_ANY_SUBTYPE, CULL_DEFAULT)
   SGE_REF(SPR_write_func, CULL_ANY_SUBTYPE, CULL_DEFAULT)
   SGE_REF(SPR_delete_func, CULL_ANY_SUBTYPE, CULL_DEFAULT)
   SGE_REF(SPR_clientdata, CULL_ANY_SUBTYPE, CULL_DEFAULT)
LISTEND

NAMEDEF(SPRN)
   NAME("SPR_name")
   NAME("SPR_url")
   NAME("SPR_startup_func")
   NAME("SPR_shutdown_func")
   NAME("SPR_list_func")
   NAME("SPR_read_func")
   NAME("SPR_write_func")
   NAME("SPR_delete_func")
   NAME("SPR_clientdata")
NAMEEND

#define SPRS sizeof(SPRN)/sizeof(char *)

enum {
   SPT_type = SPT_LOWERBOUND,      /* sge_event_type, SGE_EMT_ALL = default */
   SPT_name,                       /* name of the type, e.g. "JB_Type" */
   SPT_rules                       /* list of rules to spool this object type */
};

LISTDEF(SPT_Type)
   SGE_ULONG(SPT_type, CULL_HASH | CULL_UNIQUE)
   SGE_STRING(SPT_name, CULL_DEFAULT)
   SGE_LIST(SPT_rules, SPTR_Type, CULL_DEFAULT)
LISTEND

NAMEDEF(SPTN)
   NAME("SPT_type")
   NAME("SPT_name")
   NAME("SPT_rules")
NAMEEND

#define SPTS sizeof(SPTN)/sizeof(char *)


enum {
   SPTR_default = SPTR_LOWERBOUND,  /* is this the default rule for this object type? */
   SPTR_rule_name,                  /* name of the rule in context's rule list */
   SPTR_rule                        /* reference to rule for quicker access */
};

LISTDEF(SPTR_Type)
   SGE_BOOL(SPTR_default, CULL_DEFAULT)
   SGE_STRING(SPTR_rule_name, CULL_UNIQUE)
   SGE_REF(SPTR_rule, CULL_ANY_SUBTYPE, CULL_DEFAULT)
LISTEND

NAMEDEF(SPTRN)
   NAME("SPTR_default")
   NAME("SPTR_rule_name")
   NAME("SPTR_rule")
NAMEEND

#define SPTRS sizeof(SPTRN)/sizeof(char *)


/* *INDENT-ON* */

#ifdef  __cplusplus
}
#endif

#endif /* __SGE_SPOOLL_H */
