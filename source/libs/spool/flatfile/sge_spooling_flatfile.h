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

#include "spool/sge_spooling.h"
#include "spool/sge_spooling_utilities.h"

/****** spool/flatfile/--Spooling-Flatfile ************************************
*
*  NAME
*     Flat file spooling - spooling and output of data in flat files
*
*  FUNCTION
*     The module provides functions and a spooling framework instantiation
*     for data input/output in flat files.
*
*     It can be used for spooling of data, for information output (e.g. qstat)
*     and input/output as used by qconf.
*
*     The output format can be influenced by the use of a spool_flatfile_instr
*     structure.
*
*  SEE ALSO
*     spool/flatfile/-Spooling-Flatfile-Typedefs
*     spool/flatfile/spool_flatfile_write_object()
*     spool/flatfile/spool_flatfile_write_list()
*     spool/flatfile/spool_flatfile_read_object()
*     spool/flatfile/spool_flatfile_read_list()
*     spool/flatfile/spool_flatfile_align_object()
*     spool/flatfile/spool_flatfile_align_list()
****************************************************************************
*/

/****** spool/flatfile/-Spooling-Flatfile-Typedefs ***************************
*
*  NAME
*     Typedefs -- type definitions for spooling utility functions
*
*  SYNOPSIS
*     <to be documented after the module is finished>
*
*  FUNCTION
*     spool_flatfile_destination
*        Used to specify the destination of an output function, e.g. 
*        streams like stdin or stdout, temporary file or named file.
*
*     spool_flatfile_format
*        Format to use for output, e.g. ASCII, XML, CULL.
*
*     spool_flatfile_instr
*        Instruction for spooling. 
*        Describes which fields to spool, formatting, spooling of sublists ...
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

/* JG: TODO: we need a check function:
 * - delimiters may not contain whitespace, exception \n
 */

/*
 * spooling framework functions
 */

const char *
get_spooling_method(void);

lListElem *
spool_flatfile_create_context(lList **answer_list, const char *args);

bool 
spool_flatfile_default_startup_func(lList **answer_list, 
                                    const lListElem *rule);
bool 
spool_flatfile_common_startup_func(lList **answer_list, 
                                   const lListElem *rule);

bool 
spool_flatfile_default_list_func(lList **answer_list, 
                                 const lListElem *type, 
                                 const lListElem *rule,
                                 lList **list, 
                                 const sge_object_type event_type);
lListElem *
spool_flatfile_default_read_func(lList **answer_list, 
                                 const lListElem *type, 
                                 const lListElem *rule,
                                 const char *key, 
                                 const sge_object_type event_type);
bool 
spool_flatfile_default_write_func(lList **answer_list, 
                                  const lListElem *type, 
                                  const lListElem *rule, 
                                  const lListElem *object, 
                                  const char *key, 
                                  const sge_object_type event_type);
bool 
spool_flatfile_default_delete_func(lList **answer_list, 
                                   const lListElem *type, 
                                   const lListElem *rule,
                                   const char *key, 
                                   const sge_object_type event_type);
bool
spool_flatfile_default_verify_func(lList **answer_list, 
                                   const lListElem *type, 
                                   const lListElem *rule,
                                   lListElem *object,
                                   const sge_object_type event_type);
/*
 * base functions
 */

typedef struct spool_flatfile_instr {
   const spool_instr *spool_instr;
   bool show_field_names;
   bool show_field_header;
   bool align_names;
   bool align_data;
   const char name_value_delimiter;
   const char field_delimiter;
   const char record_delimiter;
   const char record_start;
   const char record_end;
   const struct spool_flatfile_instr *sub_instr;
} spool_flatfile_instr;

extern const spool_flatfile_instr spool_flatfile_instr_messages;
extern const spool_flatfile_instr spool_flatfile_instr_accounting;
extern const spool_flatfile_instr spool_flatfile_instr_conf;
extern const spool_flatfile_instr spool_flatfile_instr_config;
extern const spool_flatfile_instr spool_flatfile_instr_config_sublist;
extern const spool_flatfile_instr spool_flatfile_instr_config_list;
extern const spool_flatfile_instr spool_flatfile_instr_complex;
extern const spool_flatfile_instr spool_flatfile_instr_user;

const char *
spool_flatfile_write_object(lList **answer_list, const lListElem *object,
                            const spooling_field *fields,
                            const spool_flatfile_instr *instr,
                            const spool_flatfile_destination destination,
                            const spool_flatfile_format format, 
                            const char *filepath);

const char *
spool_flatfile_write_list(lList **answer_list,
                          const lList *list,
                          const spooling_field *fields,
                          const spool_flatfile_instr *instr,
                          const spool_flatfile_destination destination,
                          const spool_flatfile_format format,
                          const char *filepath);

lListElem *
spool_flatfile_read_object(lList **answer_list, const lDescr *descr, 
                           const spooling_field *fields_in, int fields_out[],
                           const spool_flatfile_instr *instr,
                           const spool_flatfile_format format,
                           FILE *file,
                           const char *filepath);
lList *
spool_flatfile_read_list(lList **answer_list, const lDescr *descr, 
                         const spooling_field *fields_in, int fields_out[],
                         const spool_flatfile_instr *instr,
                         const spool_flatfile_format format,
                         FILE *file,
                         const char *filepath);

bool 
spool_flatfile_align_object(lList **answer_list,
                            spooling_field *fields);

bool
spool_flatfile_align_list(lList **answer_list, const lList *list, 
                          spooling_field *fields);

#endif /* __SGE_SPOOLING_FLATFILE_H */    
