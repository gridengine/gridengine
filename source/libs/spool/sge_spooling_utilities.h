#ifndef __SGE_SPOOLING_UTILITIES_H 
#define __SGE_SPOOLING_UTILITIES_H 
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

#include "cull.h"

#include "sge_dstring.h"

/****** spool/utilities/--Spooling-Utilities************************************
*
*  NAME
*     Spooling Utilities -- common data structures and functions for spooling
*
*  FUNCTION
*     The module provides utility functions used for spooling.
*
*  SEE ALSO
*     spool/utilities/-Spooling-Utilities-Typedefs
*     spool/utilities/spool_get_fields_to_spool()
****************************************************************************
*/

/****** spool/utilities/-Spooling-Utilities-Typedefs ***************************
*
*  NAME
*     Typedefs -- type definitions for spooling utility functions
*
*  SYNOPSIS
*     typedef struct spool_instr {
*        int selection;
*        bool copy_field_names;
*        bool strip_field_prefix;
*        const struct spool_instr *sub_instr;
*     } spool_instr;
*     
*     extern const spool_instr spool_config_instr;
*     
*     typedef struct spooling_field {
*        int nm;
*        int width;
*        const char *name;
*        const struct spooling_field *sub_fields;
*     } spooling_field;
*     
*  FUNCTION
*     spooling_instr
*     Describes how the fields to be spooled are selected.
*     The int field "selection" contains a bitmask that will be applied
*     to the mt field of a field descriptor to check, if a field has to be
*     spooled.
*     sub_instr points to a spool_instr that will be used
*     to spool elements in sublists.
*
*     spooling_field
*     An array of spooling_fields is provides the necessary information
*     for the formatted output of data.
*     It contains the names and types of attributes to spool, information
*     about field width (for formatted output), the attribute name that shall
*     be used in output.
*     For list fields, that shall be spooled, it contains an array of 
*     fields that shall be spooled in sublist objects.
*
*  NOTES
*     May not allow really comprehensive output in all possible variations,
*     but it seems to be sufficient for all spooling and output done in
*     Grid Engine.
*
*  SEE ALSO
*     spool/utilities/spool_get_fields_to_spool()
*     spool/utilities/spool_free_spooling_fields()
****************************************************************************
*/

typedef struct spool_instr {
   int selection;
   bool copy_field_names;
   bool strip_field_prefix;
   const struct spool_instr *sub_instr;
   const void *clientdata;
} spool_instr;

extern const spool_instr spool_config_instr;
extern const spool_instr spool_config_subinstr;

extern const spool_instr spool_complex_instr;
extern const spool_instr spool_complex_subinstr;

extern const spool_instr spool_user_instr;
extern const spool_instr spool_userprj_subinstr;

typedef struct spooling_field {
   int nm;
   int width;
   const char *name;
   struct spooling_field *sub_fields;
   const void *clientdata;
} spooling_field;

spooling_field *
spool_get_fields_to_spool(lList **answer_list, const lDescr *descr, 
                          const spool_instr *instr);

spooling_field *
spool_free_spooling_fields(spooling_field *fields);

#endif /* __SGE_SPOOLING_UTILITIES_H */
