#ifndef __SGE_SPOOLING_FLATFILE_H 
#define __SGE_SPOOLING_FLATFILE_H 
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

#include "sge_spooling_utilities.h"

/****** spool/utilities/--Spooling-Flatfile ************************************
*
*  NAME
*     Flat file spooling - spooling and output of data in flat files
*
*  FUNCTION
*     The module provides functions and a spooling framework instantiation
*     for data input/output in flat files.
*     It can be used for spooling of data, for information output (e.g. qstat)
*     and input/output as used by qconf.
*
*  SEE ALSO
****************************************************************************
*/

/****** spool/utilities/-Spooling-Flatfile-Typedefs ***************************
*
*  NAME
*     Typedefs -- type definitions for spooling utility functions
*
*  SYNOPSIS
*     
*  FUNCTION
*
*  NOTES
*     May not allow really comprehensive output in all possible variations,
*     but it seems to be sufficient for all spooling and output done in
*     Grid Engine.
*
*  SEE ALSO
****************************************************************************
*/

typedef enum {
   SP_DEST_STDOUT,
   SP_DEST_STDERR,
   SP_DEST_TMP,
   SP_DEST_SPOOL
} spool_flatfile_destination;

typedef enum {
   SP_FORM_ASCII,
   SP_FORM_XML,
   SP_FORM_CULL
} spool_flatfile_format;

/*
 * - output field name and value or only field name
 * - output a header that contains all field names of a sub list
 * - alignment: if field names are shown, align all values horizontally.
 * - delimiter for use between field name and value, if alignment is active,
 *   it may be repeated
 * - delimiter between fields
 * - delimiter between records
 * - for nested structures: record begin and record end sign
 *
 * parsing of such spooled data will accept any whitespaces between tokens
 */

typedef struct spool_flatfile_instruction {
   const spooling_instruction *spooling_instruction;
   bool show_field_names;
   bool show_field_header;
   bool align_names;
   bool align_data;
   const char *name_value_delimiter;
   const char *field_delimiter;
   const char *record_delimiter;
   const char *record_start;
   const char *record_end;
   const struct spool_flatfile_instruction *sub_instruction;
} spool_flatfile_instruction;

extern const spool_flatfile_instruction spool_flatfile_instruction_messages;
extern const spool_flatfile_instruction spool_flatfile_instruction_accounting;
extern const spool_flatfile_instruction spool_flatfile_instruction_config;
extern const spool_flatfile_instruction spool_flatfile_instruction_config_list;
extern const spool_flatfile_instruction spool_flatfile_instruction_complex;

const char *
spool_flatfile_write_object(lList **answer_list, const lListElem *object,
                            const spool_flatfile_instruction *instruction,
                            const spool_flatfile_destination destination,
                            const spool_flatfile_format format, 
                            const char *filepath);

const char 
*spool_flatfile_write_list(lList **answer_list,
                           const lList *list,
                           const spool_flatfile_instruction *instruction,
                           const spool_flatfile_destination destination,
                           const spool_flatfile_format format,
                           const char *filepath);
bool 
spool_flatfile_align_object(lList **answer_list, const lListElem *object, 
                            spooling_field *fields);

bool
spool_flatfile_align_list(lList **answer_list, const lList *list, 
                          spooling_field *fields);

#endif /* __SGE_SPOOLING_FLATFILE_H */    
