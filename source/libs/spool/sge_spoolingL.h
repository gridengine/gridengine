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
*        A default entry for all types can be created; if type entries
*        for individual types exist, these entries will be used for spooling.
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
*     spool/--SPR_Type
*     spool/--SPT_Type
*     spool/--SPTR_Type
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

/****** spool/--SPR_Type ***************************************
*
*  NAME
*     SPR_Type -- Spooling rule
*
*  ELEMENTS
*     SGE_STRING(SPR_name, CULL_HASH | CULL_UNIQUE)
*        Unique name of the rule.
*
*     SGE_STRING(SPR_url, CULL_DEFAULT)
*        An url, e.g. a spool directory, a database url etc.
*
*     SGE_REF(SPR_startup_func, CULL_ANY_SUBTYPE, CULL_DEFAULT)
*        function pointer to a startup function, 
*        e.g. establishing a connection to a database.        
*
*     SGE_REF(SPR_shutdown_func, CULL_ANY_SUBTYPE, CULL_DEFAULT)
*        function pointer to a shutdown function,
*        e.g. disconnecting from a database or closing file handles.
*
*     SGE_REF(SPR_list_func, CULL_ANY_SUBTYPE, CULL_DEFAULT)
*        pointer to a function reading complete lists (master lists)
*        from the spooling data source.
*
*     SGE_REF(SPR_read_func, CULL_ANY_SUBTYPE, CULL_DEFAULT)
*        pointer to a function reading a single object from the
*        spooling data source.
*
*     SGE_REF(SPR_write_func, CULL_ANY_SUBTYPE, CULL_DEFAULT)
*        pointer to a function writing a single object.
*
*     SGE_REF(SPR_delete_func, CULL_ANY_SUBTYPE, CULL_DEFAULT)
*        pointer to a function deleting a single object.
*
*     SGE_REF(SPR_verify_func, CULL_ANY_SUBTYPE, CULL_DEFAULT)
*        pointer to a function verifying a single object.
*
*     SGE_REF(SPR_clientdata, CULL_ANY_SUBTYPE, CULL_DEFAULT)
*        clientdata; any pointer, can be used to store and
*        reference rule specific data, e.g. file or database handles.
*
*  FUNCTION
*     A spooling rule describes a certain way to store and retrieve
*     data from a defined storage facility.
*     
*     Spooling rules can implement spooling to files in a certain
*     directory or spooling into a database, to an LDAP repository
*     etc.
*  
*     A spooling context can contain multiple spooling rules.
*
*  SEE ALSO
*     spool/--Spooling
*     spool/--SPC_Type
****************************************************************************
*/

enum {
   SPR_name = SPR_LOWERBOUND,        /* name of the rule */
   SPR_url,                          /* an url, e.g. base dir, database url */
   SPR_startup_func,                 /* function pointer: startup, e.g. connect to database */
   SPR_shutdown_func,                /* function pointer: shutdown, e.g. disconnect from db */
   SPR_list_func,                    /* function pointer: read master list */
   SPR_read_func,                    /* function pointer: read an object */
   SPR_write_func,                   /* function pointer: write an object */
   SPR_delete_func,                  /* function pointer: delete an object */
   SPR_verify_func,                  /* function pointer: verify an object */
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
   SGE_REF(SPR_verify_func, CULL_ANY_SUBTYPE, CULL_DEFAULT)
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
   NAME("SPR_verify_func")
   NAME("SPR_clientdata")
NAMEEND

#define SPRS sizeof(SPRN)/sizeof(char *)

/****** spool/--SPT_Type ***************************************
*
*  NAME
*     SPT_Type -- Spooling object types
*
*  ELEMENTS
*     SGE_ULONG(SPT_type, CULL_HASH | CULL_UNIQUE)
*        Unique type identifier.
*        See enum sge_object_type in libs/gdi/sge_mirror.h
*        SGE_TYPE_ALL describes a default type entry for all
*        object types.
*
*     SGE_STRING(SPT_name, CULL_DEFAULT)
*        Name of the type - used for informational messages etc.
*
*     SGE_LIST(SPT_rules, SPTR_Type, CULL_DEFAULT)
*        List of rules that can be applied for a certain object type.
*        Does not reference the rules themselves, but contains mapping
*        objects mapping between type and rule.
*
*
*  FUNCTION
*     Objects to be spooled have a certain type that can be identified
*     by the sge_object_type enum.
*     A spooling context can contain information about individual
*     types and/or define a default behaviour for all (not individually
*     handled) types.
*
*     The spooling behaviour for a type is defined by a list of references
*     to rules in the spooling context.
*     One of the referenced spooling rules has to be made default rule
*     for reading objects.
*
*  NOTES
*     The type identifiers should not come from the mirroring interface,
*     but be part of a more general type information handling in libgdi.
*
*  SEE ALSO
*     spool/--Spooling
*     spool/--SPC_Type
*     spool/--SPTR_Type
****************************************************************************
*/

enum {
   SPT_type = SPT_LOWERBOUND,      /* sge_object_type, SGE_TYPE_ALL = default */
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


/****** spool/--SPTR_Type ***************************************
*
*  NAME
*     SPTR_Type -- references to rules for certain object types
*
*  ELEMENTS
*     SGE_BOOL(SPTR_default, CULL_DEFAULT)
*        Defines whether the referenced rule is the default rule
*        for reading the defined object type.
*
*     SGE_STRING(SPTR_rule_name, CULL_UNIQUE)
*        Name of the referenced rule.
*
*     SGE_REF(SPTR_rule, CULL_ANY_SUBTYPE, CULL_DEFAULT)
*        Pointer/reference to the rule to be used with the
*        defined object type.
*
*
*  FUNCTION
*     Elements of SPTR_Type define a mapping between object type (SPT_Type)
*     and spooling rules (SPR_Type).
*     One object type can be spooled (written) using multiple spooling rules.
*     One object type will be read using one (the default) spooling rule.
*     One spooling rule can be referenced by multiple object types.
*
*  SEE ALSO
*     spool/--Spooling
*     spool/--SPC_Type
*     spool/--SPT_Type
****************************************************************************
*/

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

enum {
   SPM_key = SPM_LOWERBOUND,     /* pointer to object */
   SPM_id,                       /* id of object in database */
   SPM_tag,                      /* tagging of keys to find deleted objects */
   SPM_sublist                   /* list of keys from subobjects */
};

LISTDEF(SPM_Type)
   SGE_STRING(SPM_key, CULL_HASH | CULL_UNIQUE)
   SGE_STRING(SPM_id, CULL_DEFAULT)
   SGE_BOOL(SPM_tag, CULL_DEFAULT)
   SGE_LIST(SPM_sublist, SPM_Type, CULL_DEFAULT)
LISTEND

NAMEDEF(SPMN)
   NAME("SPM_key")
   NAME("SPM_id")
   NAME("SPM_tag")
   NAME("SPM_sublist")
NAMEEND

#define SPMS sizeof(SPMN)/sizeof(char *)

#ifdef  __cplusplus
}
#endif

#endif /* __SGE_SPOOLL_H */
