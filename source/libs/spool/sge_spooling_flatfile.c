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

#include <stdio.h>
#ifdef ALPHA
   extern void flockfile(FILE *);
   extern void funlockfile(FILE *);
#endif

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "sgermon.h"
#include "sge_log.h"

#include "sge_io.h"
#include "sge_stdio.h"
#include "sge_stdlib.h"
#include "sge_string.h"
#include "sge_dstring.h"

#include "gdi_utility.h"
#include "sge_object.h"

#include "msg_common.h"
#include "msg_spoollib_flatfile.h"

#include "sge_spooling_flatfile.h"

const spool_flatfile_instruction spool_flatfile_instruction_config_sublist = 
{
   NULL,
   false,
   false,
   false,
   false,
   NULL,
   "=",
   ",",
   NULL,
   NULL
};

const spool_flatfile_instruction spool_flatfile_instruction_config = 
{
   &spool_config_instruction,
   true,
   false,
   true,
   false,
   " ",
   "\n",
   "\n",
   NULL,
   NULL,
   &spool_flatfile_instruction_config_sublist
};

const spool_flatfile_instruction spool_flatfile_instruction_config_list = 
{
   &spool_config_instruction,
   false,
   true,
   true,
   true,
   NULL,
   " ",
   "\n",
   NULL,
   NULL,
   &spool_flatfile_instruction_config_sublist
};

static bool 
spool_flatfile_write_object_fields(lList **answer_list, const lListElem *object,
                                   dstring *buffer, 
                                   const spool_flatfile_instruction *instruction,
                                   const spooling_field *fields);

static bool
spool_flatfile_write_list_fields(lList **answer_list, const lList *list, 
                                 dstring *buffer, 
                                 const spool_flatfile_instruction *instruction,
                                 const spooling_field *fields);

static FILE *
spool_flatfile_open_file(lList **answer_list,
                         const spool_flatfile_destination destination,
                         const char *filepath_in,
                         const char **filepath_out);

static bool
spool_flatfile_close_file(lList **answer_list, FILE *file, const char *filepath,
                          const spool_flatfile_destination destination);

static const char *
spool_flatfile_write_data(lList **answer_list, const void *data, int data_len, 
                          const spool_flatfile_destination destination, 
                          const char *filepath);

/* field width is the length of the field name */
bool 
spool_flatfile_align_object(lList **answer_list, const lListElem *object, 
                            spooling_field *fields)
{
   int i;
   int width = 0;

   DENTER(TOP_LAYER, "spool_flatfile_align_object");

   SGE_CHECK_POINTER_FALSE(object);
   SGE_CHECK_POINTER_FALSE(fields);

   for (i = 0; fields[i].nm >= 0; i++) {
      width = MAX(width, sge_strlen(lNm2Str(fields[i].nm)));
   }

   for (i = 0; fields[i].nm >= 0; i++) {
      fields[i].width = width;
   }

   DEXIT;
   return true;
}


/* field width is maximum of field name (table header) and contents */
/* sublists are not yet counted! */
bool 
spool_flatfile_align_list(lList **answer_list, const lList *list, 
                          spooling_field *fields)
{
   dstring buffer = DSTRING_INIT;
   const lListElem *object;
   int i;

   DENTER(TOP_LAYER, "spool_flatfile_align_list");

   SGE_CHECK_POINTER_FALSE(list);
   SGE_CHECK_POINTER_FALSE(fields);

   for (i = 0; fields[i].nm >= 0; i++) {
      fields[i].width = sge_strlen(lNm2Str(fields[i].nm));
   }

   for_each (object, list) {
      for (i = 0; fields[i].nm >= 0; i++) {
         const char *value = object_get_field_contents(object, answer_list, &buffer,
                                                      fields[i].nm);
         fields[i].width = MAX(fields[i].width, sge_strlen(value));
      }
   }

   DEXIT;
   return true;
}

const char 
*spool_flatfile_write_list(lList **answer_list,
                           const lList *list,
                           const spool_flatfile_instruction *instruction,
                           const spool_flatfile_destination destination,
                           const spool_flatfile_format format,
                           const char *filepath)
{
   dstring         char_buffer = DSTRING_INIT;
   spooling_field *fields;
   const char *result = NULL;
   const void *data = NULL;
   size_t data_len  = 0;

   DENTER(TOP_LAYER, "spool_flatfile_write_list");

   SGE_CHECK_POINTER_NULL(list);
   SGE_CHECK_POINTER_NULL(instruction);

   fields = spool_get_fields_to_spool(answer_list, lFirst(list), 
                                      instruction->spooling_instruction);
   if (fields == NULL) {
      /* Error handling has been done in spool_get_fields_to_spool */
      DEXIT;
      return NULL;
   }

   switch (format) {
      case SP_FORM_ASCII:
         if (instruction->align_names || instruction->align_data) {
            if (!spool_flatfile_align_list(answer_list, list, fields)) {
               /* Error handling has been done in spool_flatfile_align_object */
               fields = spool_free_spooling_fields(fields);
               DEXIT;
               return NULL;
            }
         }

         spool_flatfile_write_list_fields(answer_list, list, &char_buffer, 
                                            instruction, fields);

         data     = sge_dstring_get_string(&char_buffer);
         data_len = sge_dstring_strlen(&char_buffer);
         break;
      case SP_FORM_XML:
      case SP_FORM_CULL:
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 "not yet implemented");
         break;
   }      

   if (data == NULL || data_len == 0) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, "no data to spool");
      DEXIT;
      return NULL;
   }

   result = spool_flatfile_write_data(answer_list, data, data_len, destination, 
                                      filepath);

   /* cleanup */
   sge_dstring_free(&char_buffer);
   fields = spool_free_spooling_fields(fields);

   DEXIT;
   return result;
}

const char *
spool_flatfile_write_object(lList **answer_list, const lListElem *object,
                            const spool_flatfile_instruction *instruction,
                            const spool_flatfile_destination destination,
                            const spool_flatfile_format format, 
                            const char *filepath)
{
   dstring         char_buffer = DSTRING_INIT;
   spooling_field *fields;
   const char *result = NULL;
   const void *data = NULL;
   size_t data_len  = 0;

   DENTER(TOP_LAYER, "spool_flatfile_write_object");

   SGE_CHECK_POINTER_NULL(object);
   SGE_CHECK_POINTER_NULL(instruction);

   fields = spool_get_fields_to_spool(answer_list, object, 
                                      instruction->spooling_instruction);
   if (fields == NULL) {
      /* Error handling has been done in spool_get_fields_to_spool */
      DEXIT;
      return NULL;
   }

   switch (format) {
      case SP_FORM_ASCII:
         if (instruction->align_names) {
            if (!spool_flatfile_align_object(answer_list, object, fields)) {
               /* Error handling has been done in spool_flatfile_align_object */
               fields = spool_free_spooling_fields(fields);
               DEXIT;
               return NULL;
            }
         }

         spool_flatfile_write_object_fields(answer_list, object, &char_buffer, 
                                            instruction, fields);

         data     = sge_dstring_get_string(&char_buffer);
         data_len = sge_dstring_strlen(&char_buffer);
         break;
      case SP_FORM_XML:
      case SP_FORM_CULL:
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, "not yet implemented");
         break;
   }      

   if (data == NULL || data_len == 0) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, "no data to spool");
      DEXIT;
      return NULL;
   }

   result = spool_flatfile_write_data(answer_list, data, data_len, destination, 
                                      filepath);

   /* cleanup */
   sge_dstring_free(&char_buffer);
   fields = spool_free_spooling_fields(fields);

   DEXIT;
   return result;
}

static FILE *
spool_flatfile_open_file(lList **answer_list,
                         const spool_flatfile_destination destination,
                         const char *filepath_in,
                         const char **filepath_out)
{
   FILE *file = NULL;
   *filepath_out = NULL;

   switch (destination) {
      case SP_DEST_STDOUT:
         file = stdout;

         /* check stdout file handle */
         if (!sge_check_stdout_stream(file, STDOUT_FILENO)) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_STDFILEHANDLECLOSEDORCORRUPTED_S,
                                    "<stdout>");
            return NULL;
         }

         flockfile(file);
         fflush(file);
         *filepath_out = strdup("<stdout>");
         break;
      case SP_DEST_STDERR:
         file = stderr;

         /* check stdout file handle */
         if (!sge_check_stdout_stream(file, STDERR_FILENO)) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_STDFILEHANDLECLOSEDORCORRUPTED_S,
                                    "<stderr>");
            return NULL;
         }

         flockfile(file);
         fflush(file);
         *filepath_out = strdup("<stderr>");
         break;
      case SP_DEST_TMP:
         {
            char buffer[L_tmpnam];
            
            /* get filename for temporary file, pass buffer to make it
             * thread safe.
             */
            filepath_in = sge_tmpnam(buffer);
            if (filepath_in == NULL) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                       ANSWER_QUALITY_ERROR, 
                                       MSG_ERRORGETTINGTMPNAM_S, 
                                       strerror(errno));
               return NULL;
            }
            
            /* open file */
            file = fopen(filepath_in, "w");
            if (file == NULL) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                       ANSWER_QUALITY_ERROR, 
                                       MSG_ERROROPENINGFILEFORWRITING_SS, 
                                       filepath_in, strerror(errno));
               return NULL;
            }

            *filepath_out = strdup(filepath_in);
         }   
         break;
      case SP_DEST_SPOOL:
         /* check file name */
         if (filepath_in == NULL || *filepath_in == 0) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_INVALIDFILENAMENULLOREMPTY);
            return NULL;
         }
   
         /* open file */
         file = fopen(filepath_in, "w");
         if (file == NULL) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_ERROROPENINGFILEFORWRITING_SS, 
                                    filepath_in, strerror(errno));
            return NULL;
         }

         *filepath_out = strdup(filepath_in);
         break;
   }

   return file;
}

static bool
spool_flatfile_close_file(lList **answer_list, FILE *file, const char *filepath,
                          const spool_flatfile_destination destination)
{
   if (destination == SP_DEST_STDOUT || destination == SP_DEST_STDERR) {
      fflush(file);
      funlockfile(file);
      return true;
   }

   if (fclose(file) != 0) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_ERRORCLOSINGFILE_SS, 
                              filepath != NULL ? filepath : "<null>", 
                              strerror(errno));
      return false;
   }

   return true;
}

static const char *
spool_flatfile_write_data(lList **answer_list, const void *data, int data_len, 
                          const spool_flatfile_destination destination, 
                          const char *filepath)
{
   FILE *file = NULL;
   const char *result = NULL;

   DENTER(TOP_LAYER, "spool_flatfile_write_data");

   SGE_CHECK_POINTER_NULL(data);

   /* open/get filehandle */
   file = spool_flatfile_open_file(answer_list, destination, filepath, &result);
   if (file == NULL) {
      DEXIT;
      return NULL;
   }

   /* write data */
   if (fwrite(data, sizeof(char), data_len, file) != data_len) {
/*    if (write(fileno(file), data, data_len) != data_len) { */
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, MSG_ERROR_WRITINGFILE_SS,
                              result, strerror(errno));
      spool_flatfile_close_file(answer_list, file, result, destination);
      FREE(result);
      DEXIT;
      return NULL;
   }

   /* close file */
   if (!spool_flatfile_close_file(answer_list, file, result, destination)) {
      FREE(result);
      DEXIT;
      return NULL;
   }

   DEXIT;
   return result;
}

static bool 
spool_flatfile_write_object_fields(lList **answer_list, const lListElem *object,
                                   dstring *buffer, 
                                   const spool_flatfile_instruction *instruction,
                                   const spooling_field *fields)
{
   int i, first_field;
   dstring field_buffer = DSTRING_INIT;

   DENTER(TOP_LAYER, "spool_flatfile_write_object_fields");

   SGE_CHECK_POINTER_FALSE(object);
   SGE_CHECK_POINTER_FALSE(buffer);
   SGE_CHECK_POINTER_FALSE(instruction);
   SGE_CHECK_POINTER_FALSE(fields);
  
   /* clear input buffer */
   sge_dstring_clear(buffer);
  
   /* loop over all fields */
   i = 0;
   first_field = true;

   for (i = 0; fields[i].nm >= 0; i++) {
      const char *value;

      /* if not first field, output field_delimiter */
      if (!first_field) {
         sge_dstring_append(buffer, instruction->field_delimiter);
      } else {   
         first_field = false;
      }

      /* if show_field_names, output field name */
      if (instruction->show_field_names) {
         if(instruction->align_names) {
            sge_dstring_sprintf_append(buffer, "%-*s", fields[0].width, 
                                       lNm2Str(fields[i].nm));
         } else {
            sge_dstring_append(buffer, lNm2Str(fields[i].nm));
         }
         sge_dstring_append(buffer, instruction->name_value_delimiter);
      }

      /* output value */
      if (mt_get_type(fields[i].mt) == lListT && 
          instruction->sub_instruction != NULL) {
         lList *sub_list = lGetList(object, fields[i].nm);      

         if (sub_list == NULL || lGetNumberOfElem(sub_list) == 0) {
            sge_dstring_append(buffer, "none");
         } else {
            if (!spool_flatfile_write_list_fields(answer_list, sub_list, 
                                                  &field_buffer, 
                                                  instruction->sub_instruction,
                                                  fields[i].sub_fields)) {
               /* error handling has been done in spool_flatfile_write_list_fields */
            } else {
               sge_dstring_append_dstring(buffer, &field_buffer);
            }
         }   
      } else {
         value = object_get_field_contents(object, answer_list, &field_buffer,
                                          fields[i].nm);
         if(instruction->align_data) {
            sge_dstring_sprintf_append(buffer, "%-*s", fields[i].width, value);
         } else {
            sge_dstring_append(buffer, value);
         }
      }
#if 0
      if (value == NULL) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, "blub");
         sge_dstring_free(&field_buffer);
         DEXIT;
         return false;
      }
#endif      
   }

   sge_dstring_free(&field_buffer);

   DEXIT;
   return true;
}

static bool
spool_flatfile_write_list_fields(lList **answer_list, const lList *list, 
                                 dstring *buffer, 
                                 const spool_flatfile_instruction *instruction,
                                 const spooling_field *fields)
{
   lListElem *ep;
   int first = true;
   dstring record_buffer = DSTRING_INIT;

   DENTER(TOP_LAYER, "spool_flatfile_write_list_fields");

   SGE_CHECK_POINTER_FALSE(list);
   SGE_CHECK_POINTER_FALSE(buffer);
   SGE_CHECK_POINTER_FALSE(instruction);
   SGE_CHECK_POINTER_FALSE(fields);
  
   /* clear input buffer */
   sge_dstring_clear(buffer);
 
   for_each (ep, list) {
      /* from second record on write record delimiter */
      if (!first) {
         if (instruction->record_delimiter != NULL) {
            sge_dstring_append(buffer, instruction->record_delimiter);
         }
      } else {
         first = false;
      }

      /* if record_start, output record_start */
      if (instruction->record_start != NULL) {
         sge_dstring_append(buffer, instruction->record_start);
      }

      if (!spool_flatfile_write_object_fields(answer_list, ep, &record_buffer, 
                                              instruction, fields)) {
         /* error handling has been done in spool_flatfile_write_object_fields */
      } else {
         sge_dstring_append_dstring(buffer, &record_buffer);
      }

      /* if record_end, output record end, else record_delimiter */
      if (instruction->record_end != NULL) {
         sge_dstring_append(buffer, instruction->record_end);
      }
   }

   sge_dstring_free(&record_buffer);

   DEXIT;
   return true;
}

#if 0
int spool_flatfile_read_object()
{
   /* if record_start, expect record_start */
   /* while not finished */
      /* if show_field_names */
         /* read field name */
         /* lookup field name */
         /* read name_value_delimiter */
      /* else get next field */   
      /* read field */
      /* read field delimiter, if not found: record end? */
   /* if record_end, read record_end, else record_delimiter */    
}

int spool_flatfile_read_field()
{
   /* if type is list, and we have sub_instruction: call read_object */
   /* else read value */
}
#endif
