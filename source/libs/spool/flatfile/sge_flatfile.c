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

/* system */
#include <stdio.h>

#if defined(ALPHA)
   extern void flockfile(FILE *);
   extern void funlockfile(FILE *);
#endif

#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

/* rmon */
#include "sgermon.h"

/* uti */
#include "sge_log.h"
#include "sge_stdio.h"
#include "sge_string.h"
#include "sge_tmpnam.h"

/* sgeobj */
#include "config.h"
#include "sge_answer.h"
#include "sge_utility.h"
#include "sgeobj/sge_feature.h"

/* spool */
#include "spool/sge_spooling_utilities.h"

/* messages */
#include "msg_common.h"
#include "spool/msg_spoollib.h"
#include "spool/flatfile/msg_spoollib_flatfile.h"

/* local */
#include "spool/flatfile/sge_spooling_flatfile_scanner.h"
#include "spool/flatfile/sge_flatfile.h"

/* uti */
#include "uti/sge_spool.h"

static void spool_flatfile_add_line_breaks (dstring *buffer);
   
#ifdef DEBUG_FLATFILE
bool flatfile_debugging = true;

#define FF_DEBUG(msg) \
if (flatfile_debugging) {\
   debug_flatfile(msg, spool_line, *token, spool_text, end_token); \
}

static void debug_flatfile(const char *msg, int line, int token, 
                           const char *buffer, const char *end_token)
{
   const char *text;
   const char *et;

   DENTER(TOP_LAYER, "debug_flatfile");

   if (token == 0) {
      text = "<EOF>";
   } else if (*buffer == '\n') {
      text = "<NEWLINE>";
   } else {
      text = buffer;
   }

   if (end_token == NULL) {
      et = "<NULL>";
   } else {
      et = end_token;
   }
   
   DPRINTF(("%-20s: line %4d, token %2d, text "SFQ", end_token = "SFQ"\n",
            msg, line, token, text, et));

   DEXIT;
}
#else
#define FF_DEBUG(msg)
#endif



static const char *output_delimiter(const char c)
{
   static char buffer[2] = { '\0', '\0' };
   const char *ret;

   switch (c) {
      case '\n':
         ret = "<NEWLINE>";
         break;
      default:
         buffer[0] = c;
         ret = buffer;
         break;
   }

   return ret;
}

static char *get_end_token(char *buffer, int size, const char *end_token, 
                           const char new_end_token)
{
   char new_buffer[2] = { '\0', '\0' };

   if(end_token != NULL) {
      strncpy(buffer, end_token, size);
   } else {
      *buffer = '\0';
   }

   if (new_end_token != '\0') {
      new_buffer[0] = new_end_token;
   }

   strncat(buffer, new_buffer, size);

   return buffer;
}

static bool check_end_token(const char *end_token, const char act_char)
{
   bool ret = false;

   if (end_token !=  NULL && act_char != '\0') {
      if (strchr(end_token, act_char) != NULL) {
         ret = true;
      }
   }

   return ret;
}

static bool is_delimiter(int token)
{
   bool ret = false;
   
   if (token == SPFT_DELIMITER || token == SPFT_NEWLINE || 
       token == SPFT_WHITESPACE) {
       ret = true;
   }

   return ret;
}


static bool 
spool_flatfile_write_object_fields(lList **answer_list, const lListElem *object,
                                   dstring *buffer, 
                                   const spool_flatfile_instr *instr,
                                   const spooling_field *fields, bool recurse,
                                   bool root);

static bool
spool_flatfile_write_list_fields(lList **answer_list, const lList *list, 
                                 dstring *buffer, 
                                 const spool_flatfile_instr *instr,
                                 const spooling_field *fields, bool recurse);

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
                          const char *filepath, bool print_header);

static lListElem *
_spool_flatfile_read_object(lList **answer_list, const lDescr *descr, lListElem *root,
                            const spool_flatfile_instr *instr,
                            const spooling_field *fields, int fields_out[], int *token,
                            const char *end_token, bool parse_values);

static void
_spool_flatfile_read_live_object(lList **answer_list, lListElem **object,
                                 const lDescr *descr, lListElem *root,
                                 const spool_flatfile_instr *instr, 
                                 const spooling_field *fields, int fields_out[],
                                 int *token, const char *end_token,
                                 bool parse_values);

static lList *
_spool_flatfile_read_list(lList **answer_list, const lDescr *descr, 
                          const spool_flatfile_instr *instr,
                          const spooling_field *fields, int fields_out[], int *token,
                          const char *end_token, bool parse_values);

static spooling_field *get_recursion_field_list (const spool_flatfile_instr *instr);

/****** spool/flatfile/spool_flatfile_align_object() ********************
*  NAME
*     spool_flatfile_align_object() -- align object output
*
*  SYNOPSIS
*     bool 
*     spool_flatfile_align_object(lList **answer_list,
*                                 spooling_field *fields) 
*
*  FUNCTION
*     Computes the maximum length of the field names stored in <fields>
*     and sets the width value for all fields stored in <fields> to this 
*     maximum.
*
*  INPUTS
*     lList **answer_list     - answer list used to report errors
*     spooling_field *fields  - field description used for alignment
*
*  RESULT
*     bool - true on success, false if an error occured
*
*  SEE ALSO
*     spool/flatfile/spool_flatfile_align_list()
*******************************************************************************/
bool 
spool_flatfile_align_object(lList **answer_list, spooling_field *fields)
{
   int i;
   int width = 0;

   DENTER(TOP_LAYER, "spool_flatfile_align_object");

   SGE_CHECK_POINTER_FALSE(fields);

   for (i = 0; fields[i].nm != NoName; i++) {
      width = MAX(width, sge_strlen(fields[i].name));
   }

   for (i = 0; fields[i].nm != NoName; i++) {
      fields[i].width = width;
   }

   DEXIT;
   return true;
}


/****** spool/flatfile/spool_flatfile_align_list() **********************
*  NAME
*     spool_flatfile_align_list() -- align list data for table output
*
*  SYNOPSIS
*     bool 
*     spool_flatfile_align_list(lList **answer_list, const lList *list, 
*                               spooling_field *fields) 
*
*  FUNCTION
*     Computes the maximum width of field name and field contents for 
*     fields described in <fields> and data in <list>.
*     Stores the computed maxima in <fields>.
*
*  INPUTS
*     lList **answer_list    - answer list for error reporting
*     const lList *list      - list with data to align
*     spooling_field *fields - field description
*
*  RESULT
*     bool -  true on success, else false
*
*  NOTES
*     Sublists are not regarded.
*
*  SEE ALSO
*     spool/flatfile/spool_flatfile_align_object()
*******************************************************************************/
bool 
spool_flatfile_align_list(lList **answer_list, const lList *list, 
                          spooling_field *fields, int padding)
{
   dstring buffer = DSTRING_INIT;
   const lListElem *object;
   int i;

   DENTER(TOP_LAYER, "spool_flatfile_align_list");

   SGE_CHECK_POINTER_FALSE(list);
   SGE_CHECK_POINTER_FALSE(fields);

   for (i = 0; fields[i].nm != NoName; i++) {
      fields[i].width = sge_strlen(fields[i].name);
   }

   for_each (object, list) {
      for (i = 0; fields[i].nm != NoName; i++) {
         const char *value;
         
         sge_dstring_clear(&buffer);
         value = object_append_field_to_dstring(object, answer_list, 
                                                &buffer, fields[i].nm, '\0');
         fields[i].width = MAX(fields[i].width, (sge_strlen(value) + padding));
      }
   }

   DEXIT;
   return true;
}

/****** spool/flatfile/spool_flatfile_write_list() **********************
*  NAME
*     spool_flatfile_write_list() -- write (spool) a complete list
*
*  SYNOPSIS
*     const char* 
*     spool_flatfile_write_list(lList **answer_list, const lList *list, 
*                               const spooling_field *fields_in, 
*                               const spool_flatfile_instr *instr, 
*                               const spool_flatfile_destination destination, 
*                               const spool_flatfile_format format, 
*                               const char *filepath) 
*
*  FUNCTION
*     Writes all data of a list according to the directives given with the
*     parameters.
*
*     Which fields to write can either be passed to the function by setting
*     the parameter <fields_in>, or the function will generate this information
*     using the spooling instructions passed in <instr>.
*
*     <destination> defines the spooling destination, e.g. to stdout, to 
*     a temporary file, to a named file (name is passed in <filepath>.
*  
*     The data format to use (e.g. ASCII format) is passed in <format>.
*
*     On success, the function returns the name of the output file/stream.
*     It is in the responsibility of the caller to free the memory used
*     by the file/stream name.
*
*  INPUTS
*     lList **answer_list                          - for error reporting
*     const lList *list                            - list to write
*     const spooling_field *fields_in              - optional, field description
*     const spool_flatfile_instr *instr            - spooling instructions
*     const spool_flatfile_destination destination - destination
*     const spool_flatfile_format format           - format
*     const char *filepath                         - if destination == 
*                                                    SP_DEST_SPOOL, path to the 
*                                                    spool file
*
*  RESULT
*     const char* - on success the name of the spool file / stream, else NULL
*
*  SEE ALSO
*     spool/flatfile/spool_flatfile_write_object()
*******************************************************************************/
const char *
spool_flatfile_write_list(lList **answer_list,
                          const lList *list,
                          const spooling_field *fields_in,
                          const spool_flatfile_instr *instr,
                          const spool_flatfile_destination destination,
                          const spool_flatfile_format format,
                          const char *filepath, bool print_header)
{
   dstring char_buffer = DSTRING_INIT;
   const char *result = NULL;
   const void *data = NULL;
   size_t data_len  = 0;
   const spooling_field *fields = NULL;
   spooling_field *my_fields = NULL;

   DENTER(TOP_LAYER, "spool_flatfile_write_list");

   SGE_CHECK_POINTER_NULL(list);
   SGE_CHECK_POINTER_NULL(instr);

   /* if fields are passed, use them, else retrieve them from instructions */
   if (fields_in != NULL) {
      fields = fields_in;
   } else { 
      my_fields = spool_get_fields_to_spool(answer_list, lGetListDescr(list), 
                                            instr->spool_instr);
      if (my_fields == NULL) {
         /* message generated in spool_get_fields_to_spool */
         DEXIT;
         return NULL;
      }

      if (format == SP_FORM_ASCII) {
         if (instr->align_names || instr->align_data) {
            if (!spool_flatfile_align_list(answer_list, list, my_fields, 0)) {
               /* message generated in spool_flatfile_align_object */
               my_fields = spool_free_spooling_fields(my_fields);
               DEXIT;
               return NULL;
            }
         }
      }

      fields = my_fields;
   }

   switch (format) {
      case SP_FORM_ASCII:
         if (instr->show_field_header) {
            dstring tmp = DSTRING_INIT;
            int i = 0;
            int len = 0;
            
            sge_dstring_append_char (&char_buffer, COMMENT_CHAR);
            
            for (i = 0; fields[i].nm != NoName; i++) {
               sge_dstring_sprintf_append(&tmp, "%-*s",
                                          fields[i].width + (i?1:0),
                                          fields[i].name);
            }

            len = sge_dstring_strlen (&tmp);
            
            sge_dstring_append_dstring (&char_buffer, &tmp);
            sge_dstring_append_char (&char_buffer, '\n');
            sge_dstring_append_char (&char_buffer, COMMENT_CHAR);
            
            for (i = 0; i < len; i++) {
               sge_dstring_append_char (&char_buffer, '-');
            }
            
            sge_dstring_append_char (&char_buffer, '\n');
         }
         
         if(!spool_flatfile_write_list_fields(answer_list, list, &char_buffer, 
                                              instr, fields, false)) {
            /* in case of errors, messages are in answer_list,
             * clear data - we don't want to write erroneous data */
            sge_dstring_clear(&char_buffer); 
         }

         if (instr->record_end != '\n') {
            sge_dstring_append_char (&char_buffer, '\n');
         }
         
         if (instr->show_footer) {
            sge_dstring_sprintf_append (&char_buffer, "# "SFN, MSG_COMPLEX_STARTSCOMMENTBUTNOSAVE);
         }
         
         data     = sge_dstring_get_string(&char_buffer);
         data_len = sge_dstring_strlen(&char_buffer);
         break;
      case SP_FORM_XML:
      case SP_FORM_CULL:
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_NOTYETIMPLEMENTED_S, 
                                 "XML and CULL spooling");
         break;
   }      

   if (data == NULL || data_len == 0) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, MSG_FLATFILE_NODATATOSPOOL);
      sge_dstring_free(&char_buffer);
      if(my_fields != NULL) {
         my_fields = spool_free_spooling_fields(my_fields);
      }
      DEXIT;
      return NULL;
   }

   result = spool_flatfile_write_data(answer_list, data, data_len, destination, 
                                      filepath, print_header);

   /* cleanup */
   sge_dstring_free(&char_buffer);

   /* if we created our own fields */
   if (my_fields != NULL) {
      my_fields = spool_free_spooling_fields(my_fields);
   }

   DEXIT;
   return result;
}

/****** spool/flatfile/spool_flatfile_write_object() ********************
*  NAME
*     spool_flatfile_write_object() -- write (spool) an object 
*
*  SYNOPSIS
*     const char * 
*     spool_flatfile_write_object(lList **answer_list, const lListElem *object,
*                                 const spooling_field *fields_in, 
*                                 const spool_flatfile_instr *instr, 
*                                 const spool_flatfile_destination destination,
*                                 const spool_flatfile_format format, 
*                                 const char *filepath) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     lList **answer_list                          - ??? 
*     const lListElem *object                      - ??? 
*     const spooling_field *fields_in              - ??? 
*     const spool_flatfile_instr *instr            - ??? 
*     const spool_flatfile_destination destination - ??? 
*     const spool_flatfile_format format           - ??? 
*     const char *filepath                         - ??? 
*
*  RESULT
*     const char * - 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     ??? 
*
*  BUGS
*     ??? 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
const char * 
spool_flatfile_write_object(lList **answer_list, const lListElem *object,
                            bool is_root, const spooling_field *fields_in,
                            const spool_flatfile_instr *instr,
                            const spool_flatfile_destination destination,
                            const spool_flatfile_format format, 
                            const char *filepath, bool print_header)
{
   dstring char_buffer = DSTRING_INIT;
   const char *result = NULL;
   const void *data = NULL;
   size_t data_len  = 0;
   const spooling_field *fields = NULL;
   spooling_field *my_fields = NULL;

   DENTER(TOP_LAYER, "spool_flatfile_write_object");

   SGE_CHECK_POINTER_NULL(object);
   SGE_CHECK_POINTER_NULL(instr);

   /* if no fields are passed, retrieve them from instructions */
   if (fields_in != NULL) {
      fields = fields_in;
   } else { 
      my_fields = spool_get_fields_to_spool(answer_list, 
                                            object_get_type(object), 
                                            instr->spool_instr);
      if (my_fields == NULL) {
         /* message generated in spool_get_fields_to_spool */
         DEXIT;
         return NULL;
      }

      if(format == SP_FORM_ASCII) {
         if (instr->align_names) {
            if (!spool_flatfile_align_object(answer_list, my_fields)) {
               /* message generated in spool_flatfile_align_object */
               my_fields = spool_free_spooling_fields(my_fields);
               DEXIT;
               return NULL;
            }
         }
      }

      fields = my_fields;
   }   

   switch (format) {
      case SP_FORM_ASCII:
         if(!spool_flatfile_write_object_fields(answer_list, object, 
                                                &char_buffer, instr, fields,
                                                false, is_root)) {
            /* in case of errors, messages are in answer_list,
             * clear data - we don't want to write erroneous data */
            sge_dstring_clear(&char_buffer); 
         }
         
         sge_dstring_append_char (&char_buffer, '\n');

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
                              ANSWER_QUALITY_ERROR, MSG_FLATFILE_NODATATOSPOOL);
      sge_dstring_free(&char_buffer);
      if(my_fields != NULL) {
         my_fields = spool_free_spooling_fields(my_fields);
      }
      DEXIT;
      return NULL;
   }

   result = spool_flatfile_write_data(answer_list, data, data_len, destination, 
                                      filepath, print_header);

   /* cleanup */
   sge_dstring_free(&char_buffer);

   /* if we created our own fields */
   if (my_fields != NULL) {
      my_fields = spool_free_spooling_fields(my_fields);
   }

   DEXIT;
   return result;
}

/****** spool/flatfile/spool_flatfile_open_file() ***********************
*  NAME
*     spool_flatfile_open_file() -- open spooling file or stream
*
*  SYNOPSIS
*     static FILE * 
*     spool_flatfile_open_file(lList **answer_list, 
*                              const spool_flatfile_destination destination, 
*                              const char *filepath_in, 
*                              const char **filepath_out) 
*
*  FUNCTION
*     Opens a file or stream as described by <destination>.
*
*     Streams are locked to handle concurrent access by multiple threads.
*     
*     If <destination> is SP_DEST_TMP, a temporary file is opened.
*
*     If <destination> is SP_DEST_SPOOL, the file specified by 
*     <filepath_in> is opened.
*
*     The name of the file/stream opened is returned in <filepath_out>.
*     It is in the responsibility of the caller to free the memory allocated
*     by <filepath_out>.
*
*     spool_flatfile_close_file shall be used to close a file opened using
*     spool_flatfile_open_file.
*
*  INPUTS
*     lList **answer_list                          - for error reporting 
*     const spool_flatfile_destination destination - destination
*     const char *filepath_in                      - optional filename
*     const char **filepath_out                    - returned filename
*
*  RESULT
*     static FILE * - on success a file handle, else NULL
*
*  SEE ALSO
*     spool/flatfile/spool_flatfile_close_file()
*******************************************************************************/
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
            answer_list_add_sprintf(answer_list, STATUS_EDISK, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_STDFILEHANDLECLOSEDORCORRUPTED_S,
                                    "<stdout>");
            return NULL;
         }

#if !defined(DARWIN6)
         flockfile(file);
#endif
         fflush(file);
         *filepath_out = strdup("<stdout>");
         break;
      case SP_DEST_STDERR:
         file = stderr;

         /* check stderr file handle */
         if (!sge_check_stdout_stream(file, STDERR_FILENO)) {
            answer_list_add_sprintf(answer_list, STATUS_EDISK, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_STDFILEHANDLECLOSEDORCORRUPTED_S,
                                    "<stderr>");
            return NULL;
         }

#if !defined(AIX42) && !defined(DARWIN6)
         flockfile(file);
#endif
         fflush(file);
         *filepath_out = strdup("<stderr>");
         break;
      case SP_DEST_TMP:
         {
            char buffer[SGE_PATH_MAX];
            
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
               answer_list_add_sprintf(answer_list, STATUS_EDISK, 
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
            answer_list_add_sprintf(answer_list, STATUS_ESEMANTIC, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_INVALIDFILENAMENULLOREMPTY);
            return NULL;
         }
   
         /* open file */
         file = fopen(filepath_in, "w");
         if (file == NULL) {
            answer_list_add_sprintf(answer_list, STATUS_EDISK,
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

/****** spool/flatfile/spool_flatfile_close_file() **********************
*  NAME
*     spool_flatfile_close_file() -- close spool file / stream
*
*  SYNOPSIS
*     static bool 
*     spool_flatfile_close_file(lList **answer_list, FILE *file, 
*                               const char *filepath, 
*                               const spool_flatfile_destination destination) 
*
*  FUNCTION
*     Closes the given file or stream.
*     Streams (stdout, strerr) are not really closed, but just unlocked.
*
*  INPUTS
*     lList **answer_list                          - to return errors
*     FILE *file                                   - file handle to close
*     const char *filepath                         - filename
*     const spool_flatfile_destination destination - destination
*
*  RESULT
*     static bool - true on success, else false
*
*  SEE ALSO
*     spool/flatfile/spool_flatfile_open_file()
*******************************************************************************/
static bool 
spool_flatfile_close_file(lList **answer_list, FILE *file, const char *filepath,
                          const spool_flatfile_destination destination)
{
   if (destination == SP_DEST_STDOUT || destination == SP_DEST_STDERR) {
      fflush(file);
#if !defined(AIX42) && !defined(DARWIN6)
      funlockfile(file);
#endif
      return true;
   }

   if (fclose(file) != 0) {
      answer_list_add_sprintf(answer_list, STATUS_EDISK,
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
                          const char *filepath, bool print_header)
{
   FILE *file = NULL;
   const char *result = NULL;
   dstring ds = DSTRING_INIT;

   DENTER(TOP_LAYER, "spool_flatfile_write_data");

   SGE_CHECK_POINTER_NULL(data);

   /* open/get filehandle */
   file = spool_flatfile_open_file(answer_list, destination, filepath, &result);
   if (file == NULL) {
      /* message generated in spool_flatfile_open_file */
      DEXIT;
      return NULL;
   }

   if (print_header && (sge_spoolmsg_write(file, COMMENT_CHAR, 
         feature_get_product_name(FS_VERSION, &ds)) < 0)) {
      /* on error just don't print the header */
   }

   /* write data */
   if (fwrite(data, sizeof(char), data_len, file) != data_len) {
      answer_list_add_sprintf(answer_list, STATUS_EDISK,
                              ANSWER_QUALITY_ERROR, MSG_ERRORWRITINGFILE_SS,
                              result, strerror(errno));
      spool_flatfile_close_file(answer_list, file, result, destination);
      FREE(result);
      DEXIT;
      return NULL;
   }

   /* close file */
   if (!spool_flatfile_close_file(answer_list, file, result, destination)) {
      /* message generated in spool_flatfile_close_file */
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
                                   const spool_flatfile_instr *instr,
                                   const spooling_field *fields, bool recurse,
                                   bool root)
{
   int i, first_field;
   dstring field_buffer = DSTRING_INIT;
   dstring tmp_buffer = DSTRING_INIT;
   const lDescr *descr;

   DENTER(TOP_LAYER, "spool_flatfile_write_object_fields");

   SGE_CHECK_POINTER_FALSE(object);
   SGE_CHECK_POINTER_FALSE(buffer);
   SGE_CHECK_POINTER_FALSE(instr);
   SGE_CHECK_POINTER_FALSE(fields);
 
   descr = lGetElemDescr(object);
 
   /* loop over all fields */
   i = 0;
   first_field = true;

   for (i = 0; fields[i].nm != NoName; i++) {
      const char *value = NULL;
      int pos;

      /* If this isn't the root node, and this is the field we're supposed to
       * supress, skip it. */
      if (!root && (instr->recursion_info.supress_field == fields[i].nm)) {
         continue;
      }
      
      pos = lGetPosInDescr(descr, fields[i].nm);
      if (pos < 0) {
         answer_list_add_sprintf(answer_list, STATUS_ESEMANTIC,
                                 ANSWER_QUALITY_WARNING, 
                                 MSG_NMNOTINELEMENT_S,
                                 lNm2Str(fields[i].nm));
         continue;
      }

      sge_dstring_clear(&field_buffer);
         
      /* if not first field, output field_delimiter */
      if (!first_field || recurse) {
         sge_dstring_append_char(&field_buffer, instr->field_delimiter);
      } else {   
         first_field = false;
      }

      /* if show_field_names, output field name */
      if (instr->show_field_names && (fields[i].name != NULL)) {
         /* respect alignment */
         if (fields[i].width > 0) {
            sge_dstring_sprintf_append(&field_buffer, "%-*s", fields[0].width,
                                       fields[i].name);
         } else {
            sge_dstring_append(&field_buffer, fields[i].name);
         }

         /* output name-value delimiter */
         if (instr->name_value_delimiter != '\0') {
            sge_dstring_append_char(&field_buffer, instr->name_value_delimiter);
         } else {
            sge_dstring_append_char(&field_buffer, ' ');
         }
      }

      /* output value */
      if (fields[i].write_func != NULL) {
         fields[i].write_func (object, fields[i].nm, &field_buffer, answer_list);
      }
      else if (mt_get_type(descr[pos].mt) == lListT) {
         const spool_flatfile_instr *sub_instr = NULL;
         const bool recurse_field = (instr->recursion_info.recursion_field
                                                               == fields[i].nm);
         const spooling_field *sub_fields = NULL;
         
         if (!recurse_field) {
            sub_instr = (spool_flatfile_instr *)fields[i].clientdata;

            /* if no field specific sub_instr exists, use default from inst */
            if (sub_instr == NULL) {
               sub_instr = instr->sub_instr;
            }

            sub_fields = fields[i].sub_fields;
         }
         else {
            sub_instr = instr;
            sub_fields = fields;
         }

         if(sub_instr == NULL || sub_fields == NULL) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_WARNING, 
                                    MSG_DONTKNOWHOWTOSPOOLSUBLIST_SS,
                                    lNm2Str(fields[i].nm), SGE_FUNC);
            sge_dstring_append(&field_buffer, NONE_STR);
         } else {
            lList *sub_list = lGetList(object, fields[i].nm);      

            /* Bugfix: Issuezilla #1137
             * If a list field has no name and no value, it should be ignored
             * altogether.  I'm using this funny "if" arrangement to get the
             * desired effect with the minimum amount of overhead. */
            if ((sub_list == NULL) || (lGetNumberOfElem(sub_list) == 0)) {
               if (fields[i].name != NULL) {
                  sge_dstring_append(&field_buffer, NONE_STR);
               }
            } else {
               sge_dstring_clear(&tmp_buffer);
      
               if (spool_flatfile_write_list_fields(answer_list, sub_list, 
                                                    &tmp_buffer, sub_instr,
                                                    sub_fields,
                                                    recurse_field)) {
                  /* error handling has been done in spool_flatfile_write_list_fields */
                  sge_dstring_append_dstring(&field_buffer, &tmp_buffer);
               }
            }
         }
      }
      else {
         sge_dstring_clear(&tmp_buffer);
      
         value = object_append_field_to_dstring(object, answer_list, 
                                                &tmp_buffer, fields[i].nm, 
                                                '\0');
         
         /* If asked to align the data and this isn't the last field, pad with
          * spaces. Testing for i+1 is always valid because the last element
          * is NoName, and we can't get to here if the current element is
          * NoName, i.e. we're at the last element. */
         if (instr->align_data && (fields[i + 1].nm != NoName)) {
            sge_dstring_sprintf_append(&field_buffer, "%-*s", fields[i].width,
                                       value);
         } else {
            sge_dstring_append(&field_buffer, value);
         }
      }
      
      /* To save a lot of trouble, I'm making a big assumption here.  The
       * assumption is that any level where the data names are shown will be
       * field delimited by a newline.  This holds true in all current cases
       * (I think...), but it's still an assumption.  This is important
       * because I can only insert line breaks in fields that are full lines. */
      if (instr->show_field_names && (getenv("SGE_SINGLE_LINE") == NULL)) {
         spool_flatfile_add_line_breaks (&field_buffer);
      }
      
      sge_dstring_append_dstring (buffer, &field_buffer);
   }

   sge_dstring_free(&field_buffer);

   DEXIT;
   return true;
}

static bool
spool_flatfile_write_list_fields(lList **answer_list, const lList *list, 
                                 dstring *buffer, 
                                 const spool_flatfile_instr *instr,
                                 const spooling_field *fields, bool recurse)
{
   lListElem *ep;
   int first = true;
   dstring record_buffer = DSTRING_INIT;
   const spooling_field *my_fields = NULL;

   DENTER(TOP_LAYER, "spool_flatfile_write_list_fields");

   SGE_CHECK_POINTER_FALSE(list);
   SGE_CHECK_POINTER_FALSE(buffer);
   SGE_CHECK_POINTER_FALSE(instr);
   SGE_CHECK_POINTER_FALSE(fields);
  
   /* If recursion is enabled, only write out a single id field for each element
    * in the list.  Then recursively write out the entire element. */
   if (recurse) {
      my_fields = get_recursion_field_list (instr);
   }
   else {
      my_fields = fields;
   }
   
   for_each (ep, list) {
      /* from second record on write record delimiter */
      if (!first) {
         if (instr->record_delimiter != '\0') {
            sge_dstring_append_char(buffer, instr->record_delimiter);
         }
      } else {
         first = false;
      }

      /* if record_start, output record_start */
      if (instr->record_start != '\0') {
         sge_dstring_append_char(buffer, instr->record_start);
      }
         
      sge_dstring_clear (&record_buffer);

      if (!spool_flatfile_write_object_fields(answer_list, ep, &record_buffer, 
                                              instr, my_fields, false, false)) {
         /* error message generated in spool_flatfile_write_object_fields */
      } else {
         sge_dstring_append_dstring(buffer, &record_buffer);
      }

      /* if record_end, output record end, else record_delimiter */
      if (instr->record_end != '\0') {
         sge_dstring_append_char(buffer, instr->record_end);
      }
   }

   /* Recursively write out the sub objects. */
   if (recurse && (instr->recursion_info.recursion_field != NoName)) {
      for_each (ep, list) {         
         sge_dstring_clear (&record_buffer);

         if (!spool_flatfile_write_object_fields(answer_list, ep, &record_buffer, 
                                                 instr, fields, true, false)) {
            /* error message generated in spool_flatfile_write_object_fields */
         } else {
            sge_dstring_append_dstring(buffer, &record_buffer);
         }
      }
   }
   
   if (recurse) {
      FREE (my_fields);
   }
   
   sge_dstring_free(&record_buffer);

   DEXIT;
   return true;
}

/****** spool/flatfile/spool_flatfile_read_object() *********************
*  NAME
*     spool_flatfile_read_object() -- read an object from file / stream
*
*  SYNOPSIS
*     lListElem * 
*     spool_flatfile_read_object(lList **answer_list, const lDescr *descr, 
*                                const spooling_field *fields_in, 
*                                int fields_out[], 
*                                const spool_flatfile_instr *instr, 
*                                const spool_flatfile_format format, 
*                                FILE *file, const char *filepath) 
*
*  FUNCTION
*     Read an object of type <descr> from the stream <file> or a file described
*     by <filepath>.
*
*     <fields_in> names the fields that can be contained in the input.
*
*     The fields actually read are stored in <fields_out>.
*
*     <format> and <instr> describe the data format to expect.
*
*  INPUTS
*     lList **answer_list                - to report any errors
*     const lDescr *descr                - object type to read
*     const spooling_field *fields_in    - fields that can be contained in input
*     int fields_out[]                   - field actually read
*     const spool_flatfile_instr *instr  - spooling instruction
*     const spool_flatfile_format format - spooling format
*     FILE *file                         - filehandle to read from
*     const char *filepath               - if <file> == NULL, <filepath> is 
*                                          opened
*
*  RESULT
*     lListElem * - on success the read object, else NULL
*
*  SEE ALSO
*     spool/flatfile/spool_flatfile_write_object()
*     spool/flatfile/spool_flatfile_read_list()
*******************************************************************************/
lListElem * 
spool_flatfile_read_object(lList **answer_list, const lDescr *descr, 
                           lListElem *root, const spooling_field *fields_in,
                           int fields_out[], bool parse_values,
                           const spool_flatfile_instr *instr,
                           const spool_flatfile_format format,
                           FILE *file,
                           const char *filepath)
{
   bool file_opened = false;
   int token;
   lListElem *object = NULL;
   const spooling_field *fields = NULL;
   spooling_field *my_fields = NULL;

   DENTER(TOP_LAYER, "spool_flatfile_read_object");

   SGE_CHECK_POINTER_NULL(descr);
   SGE_CHECK_POINTER_NULL(instr);

   /* if no file handle is passed, try to open file for reading */
   if (file == NULL) {
      SGE_CHECK_POINTER_NULL(filepath);

      file = fopen(filepath, "r");
      if (file == NULL) {
         answer_list_add_sprintf(answer_list, STATUS_EDISK,
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_ERROROPENINGFILEFORREADING_SS,
                                 filepath, strerror(errno));
         DEXIT;
         return NULL;
      }

      file_opened = true;
   }

   /* initialize scanner */
   token = spool_scanner_initialize(file);

   /* if no fields are passed, retrieve them from instructions */
   if (fields_in != NULL) {
      fields = fields_in;
   } else {
      my_fields = spool_get_fields_to_spool(answer_list, descr, 
                                         instr->spool_instr);
      if (my_fields == NULL) {
         /* messages generated in spool_get_fields_to_spool */
         spool_scanner_shutdown();
         if (file_opened) {
            fclose (file);
         }
         DEXIT;
         return NULL;
      }

      fields = my_fields;
   }

   object = _spool_flatfile_read_object(answer_list, descr, root, instr, 
                                        fields, fields_out, &token, NULL,
                                        parse_values);

   spool_scanner_shutdown();

   /* if we opened the file, we also have to close it */
   if (file_opened) {
      fclose(file);
   }

   /* if we created our own fields */
   if (my_fields != NULL) {
      my_fields = spool_free_spooling_fields(my_fields);
   }

   DEXIT;
   return object;
}

static lListElem *
_spool_flatfile_read_object(lList **answer_list, const lDescr *descr, 
                            lListElem *root, const spool_flatfile_instr *instr, 
                            const spooling_field *fields, int fields_out[],
                            int *token, const char *end_token,
                            bool parse_values)
{
   lListElem *object = NULL;
   int *my_fields_out = NULL;
   
   /* BUGFIX: Issuezilla #732
    * If we're not given a fields_out array, create one for internal use. */
   if (fields_out != NULL) {
      my_fields_out = fields_out;
   }
   else {
      my_fields_out = (int *)malloc ((spool_get_number_of_fields (fields) + 1) *
                                                                  sizeof (int));
      my_fields_out[0] = NoName;
   }

   _spool_flatfile_read_live_object(answer_list, &object, descr, root, instr,
                                    fields, my_fields_out, token, end_token,
                                    parse_values);

   if (fields_out == NULL) {
      FREE (my_fields_out);
   }
   
   return object;
}

static void
_spool_flatfile_read_live_object(lList **answer_list, lListElem **object,
                                 const lDescr *descr, lListElem *root,
                                 const spool_flatfile_instr *instr, 
                                 const spooling_field *fields, int fields_out[],
                                 int *token, const char *end_token,
                                 bool parse_values)
{
   int field_index = -1;

   dstring buffer = DSTRING_INIT;
   bool stop = false;

   DENTER(TOP_LAYER, "_spool_flatfile_read_live_object");
   
FF_DEBUG("reading object");

   while (*token != 0 && !stop) {
      int nm = NoName;
      int pos, type;
      bool field_end  = false;
      bool record_end = false;
      bool field_has_name = false;
        
FF_DEBUG("reading field");

      /* check for list end condition */
      if (is_delimiter(*token) && check_end_token(end_token, *spool_text)) {
FF_DEBUG("detected end_token");
         stop = true;
         continue;
      }

      /* skip newlines */
      while (*token == SPFT_NEWLINE) {
FF_DEBUG("skip newline");
         /* DT: TODO: There is a potential problem here that just hasn't come up
          * yet.  If an object type with a NULL field name were to disable value
          * parsing, it wouldn't work because the field name is always expected
          * to be a WORD.  Therefore, with a NULL field name value parsing is
          * always assumed to be false.
          * To fix this, we would need to allow text to be pushed back onto the
          * spool_text so that we could read the name as a WORD, and if it turns
          * out to be a field with a NULL name with value parsing disabled we
          * could push the WORD back onto the spool_text and read it again with
          * value parsing disabled. */
         *token = spool_lex();
      }

      /* check for eof */
      if (*token == 0) {
FF_DEBUG("eof detected");
         continue;
      }

      if (*object == NULL) {
         *object = lCreateElem(descr);
         if (*object == NULL) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_ERRORCREATINGOBJECT);
            stop = true;
            continue;
         }
      }

      /* read field name from file or from field list */
      if (instr->show_field_names) {
         /* read field name from file */
FF_DEBUG("read field name");
         if (*token != SPFT_WORD) {
            answer_list_add_sprintf(answer_list, STATUS_ESYNTAX,
                                    ANSWER_QUALITY_ERROR,
                                    MSG_PARSINGOBJECTNOATTRIBUTE_D,
                                    spool_line);
            stop = true;
            continue;
         }
   
         /* search field name in field array */
         for(field_index = 0; fields[field_index].nm != NoName; field_index++) {
            if(sge_strnullcmp(spool_text, fields[field_index].name) == 0) {
               nm = fields[field_index].nm;
               break;
            }
         }

         /* Not found -> Search for a nameless field in the field array */
         if (nm == NoName) {
            field_has_name = false;
            
            for(field_index = 0; fields[field_index].nm != NoName; field_index++) {
               if (fields[field_index].name == NULL) {
                  nm = fields[field_index].nm;
                  break;
               }
            }
            
            /* not found -> error */
            if (nm == NoName) {
               answer_list_add_sprintf(answer_list, STATUS_ESYNTAX,
                                       ANSWER_QUALITY_ERROR,
                                       MSG_UNKNOWNATTRIBUTENAME_S, spool_text);
               stop = true;
               continue;
            }
         }
         else {
            field_has_name = true;
         }

         if (field_has_name) {
            if (isspace(instr->name_value_delimiter)) {
FF_DEBUG("return whitespace");
               spool_return_whitespace = true;
            }
            *token = spool_lex();
            spool_return_whitespace = false;

            /* do we have a special delimiter between attrib name and value? */
            if (instr->name_value_delimiter != '\0') {
FF_DEBUG("read name_value_delimiter");
               if (!is_delimiter(*token) || 
                   *spool_text != instr->name_value_delimiter) {
                  answer_list_add_sprintf(answer_list, STATUS_ESYNTAX,
                                          ANSWER_QUALITY_ERROR,
                                          MSG_PARSINGOBJECTNAMEVALUESEP_SD,
                                          output_delimiter(instr->name_value_delimiter),
                                          spool_line);
                  stop = true;
                  continue;
               }

               if (!parse_values) {
                  spool_finish_line = 1;
               }

               *token = spool_lex();
               spool_finish_line = 0;
            }
         }
      } else {
FF_DEBUG("eval next field");
         /* get next field from field array */   
         /* field_index starts at -1 so that this works on the first pass */
         nm = fields[++field_index].nm;

         /* last field reached */
         if (nm == NoName) {
            stop = true;
            continue;
         }
      }

FF_DEBUG(lNm2Str(nm));

      /* check if nm is an attribute of current object type */
      pos = lGetPosInDescr(descr, nm);
      if (pos < 0) {
         answer_list_add_sprintf(answer_list, STATUS_ESYNTAX,
                                 ANSWER_QUALITY_ERROR,
                                 MSG_ATTRIBUTENOTINOBJECT_S,
                                 (fields[field_index].name != NULL) ?
                                 fields[field_index].name : lNm2Str(nm));
         stop = true;
         continue;
      }

      /* if list of read fields is requested in fields_out, store this info */
      if (fields_out != NULL) {
         /* BUGFIX: Issuezilla #732
          * If add_nm_to_set returns -1, it means that the field already exists
          * in the field list.  In this case, return an error. */
         if (add_nm_to_set(fields_out, nm) == -1) {
            answer_list_add_sprintf(answer_list, STATUS_ESYNTAX,
                                    ANSWER_QUALITY_ERROR,
                                    MSG_FLATFILE_DUPLICATEATTRIB_S,
                                    (fields[field_index].name != NULL) ?
                                    fields[field_index].name : lNm2Str(nm));
            stop = true;
            continue;
         }
      }

      type = mt_get_type(descr[pos].mt);
      
      /* now read the data */
      if ((type != lListT) || (fields[field_index].read_func != NULL)) {
         bool found_value = false;
         
         /* read field data and append until field/record end */
         sge_dstring_clear(&buffer);
         spool_return_whitespace = true;

         while (*token != 0 && !field_end && !record_end) {
FF_DEBUG("reading value");
            if (is_delimiter(*token)) {
               /* check for field end */
               if (*spool_text == instr->field_delimiter) {
FF_DEBUG("detected field_delimiter");
                  field_end = true;
                  continue;
               }
               /* check for record end */
               if (*spool_text == instr->record_end) {
FF_DEBUG("detected record_end");
                  record_end = true;
                  continue;
               }
               /* check for record end */
               if (*spool_text == instr->record_delimiter) {
FF_DEBUG("detected record_delimiter");
                  record_end = true;
                  continue;
               }
               /* check for external end condition */
               if (check_end_token(end_token, *spool_text)) {
FF_DEBUG("detected end_token");
                  record_end = true;
                  continue;
               }
               /* if it's a space that doesn't have a special meaning,
                * ignore it */
               if (!found_value && (*spool_text == ' ')) {
                  *token = spool_lex();
               }
            }
            
            /* store data */
            sge_dstring_append(&buffer, spool_text);
            found_value = true;
            
            *token = spool_lex();
         }
         spool_return_whitespace = false;
         
         if (fields[field_index].read_func == NULL) {
            if (object_parse_field_from_string(*object, answer_list, nm, 
                                        sge_dstring_get_string(&buffer)) == 0) {
               stop = true;
               continue;
            }
         }
         else {
            if (fields[field_index].read_func (*object, nm,
                                               sge_dstring_get_string (&buffer),
                                               answer_list) == 0) {
               stop = true;
               continue;
            }
         }
      }
      else { /* if (type == lListT) */
         lList *list;
         const lDescr *sub_descr;

FF_DEBUG("reading list");
         /* check for empty sublist */
         if (*token == SPFT_WORD && 
             sge_strnullcasecmp(spool_text, NONE_STR) == 0) {
FF_DEBUG("empty list");
            *token = spool_lex();

            if (instr->recursion_info.recursion_field == nm) {
               record_end = true;
            }
            /* check for field end - we have to skip it later */
            else if (is_delimiter (*token) &&
                     (*spool_text == instr->field_delimiter)) {
               field_end = true;
            }
         } else {
            /* parse sublist - do we have necessary info */
            const spool_flatfile_instr *sub_instr = (spool_flatfile_instr *)fields[field_index].clientdata;
            /* if no field specific instr exists, take default sub_instr */
            if (sub_instr == NULL) {
               sub_instr = instr->sub_instr;
            }
            
            if (sub_instr == NULL || 
               fields[field_index].sub_fields == NULL) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                       ANSWER_QUALITY_ERROR,
                                       MSG_DONTKNOWHOWTOHANDLELIST_S, 
                                       lNm2Str(nm));
               stop = true;
               continue;
            }

            /* get / check type of sublist */
            sub_descr = object_get_subtype(nm);
            if (sub_descr == NULL)  {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                       ANSWER_QUALITY_ERROR,
                                       MSG_UNKNOWNOBJECTTYPEFOR_SS, 
                                       lNm2Str(nm), SGE_FUNC);
               stop = true;
               continue;
            }
           
            /* read sublist */
            {
               char new_end_token[MAX_STRING_SIZE];
               
               get_end_token(new_end_token, MAX_STRING_SIZE, end_token,
                             instr->field_delimiter);
               /* We're passing in NULL for the fields_out parameter
                * because we don't care about the fields read from sub
                * lists. */
               list = _spool_flatfile_read_list(answer_list, sub_descr, 
                                                sub_instr, 
                                                fields[field_index].sub_fields, 
                                                NULL, token, 
                                                new_end_token, parse_values);
            
               lSetPosList(*object, pos, list);
               
               if (instr->recursion_info.recursion_field == nm) {
                  lListElem *ep = NULL;
                  lListElem *rep = root;

                  if (rep == NULL) {
                     rep = *object;
                  }
            
                  if (instr->field_delimiter != '\0') {
                     if (!is_delimiter(*token) ||
                         *spool_text != instr->field_delimiter) {
                        answer_list_add_sprintf(answer_list, STATUS_ESYNTAX,
                                       ANSWER_QUALITY_ERROR,
                                       MSG_PARSINGOBJECTUNKNOWNTRAILER_DS,
                                       spool_line,
                                       token == 0 ? "<EOF>" : spool_text);
                        stop = true;
                        continue;
                     }
FF_DEBUG("skipping field delimiter");
                     *token = spool_lex();
                  }

                  /* Read in the full element for each element in the list */
                  for_each (ep, list) {
                     /* We're passing in NULL for the fields_out parameter
                      * because we don't care about the fields read from sub
                      * lists. */
                     _spool_flatfile_read_live_object(answer_list, &ep, descr,
                                                      rep, instr, fields, NULL,
                                                      token, end_token,
                                                      parse_values);
                  }
                  
                  record_end = true;
               }
            }
         }
      }
FF_DEBUG("after parsing value");

      /* check for eof */
      if (*token == 0) {
FF_DEBUG("eof detected");
         continue;
      }

      /* check for record_end while parsing value */
      if (record_end) {
         stop = true;
         continue;
      }

      /* check for record end */
      if (instr->record_end != '\0') {
         if (is_delimiter(*token) && *spool_text == instr->record_end) {
FF_DEBUG("detected record_end");
            stop = true;
            continue;
         }
      }

      /* check for record delimiter */
      if (instr->record_delimiter != '\0') {
         if (is_delimiter(*token) && *spool_text == instr->record_delimiter) {
FF_DEBUG("detected record_delimiter");
            stop = true;
            continue;
         }
      }

      /* if a field end has been detected while parsing a value, skip it 
       * else check for field end.
       */
      if (field_end) {
FF_DEBUG("skipping field delimiter");
         if (!field_has_name && !parse_values) {
            spool_finish_line = 1;
         }

         *token = spool_lex();
         
         spool_finish_line = 0;
      } else {
         if (instr->field_delimiter != '\0') {
            if (!is_delimiter(*token) ||
                *spool_text != instr->field_delimiter) {
               answer_list_add_sprintf(answer_list, STATUS_ESYNTAX,
                                       ANSWER_QUALITY_ERROR,
                                       MSG_PARSINGOBJECTUNKNOWNTRAILER_DS,
                                       spool_line,
                                       token == 0 ? "<EOF>" : spool_text);
               stop = true;
               continue;
            }
FF_DEBUG("skipping field delimiter");
            if (!field_has_name && !parse_values) {
               spool_finish_line = 1;
            }

            *token = spool_lex();

            spool_finish_line = 0;
         }
      }
   } /* while */

FF_DEBUG("after parsing object");

   /* cleanup */
   sge_dstring_free(&buffer);
   DEXIT;
   return;
}

/****** spool/flatfile/spool_flatfile_read_list() ***********************
*  NAME
*     spool_flatfile_read_list() -- read a list from file / stream
*
*  SYNOPSIS
*     lList * 
*     spool_flatfile_read_list(lList **answer_list, const lDescr *descr,
*                              const spooling_field *fields_in, 
*                              int fields_out[], 
*                              const spool_flatfile_instr *instr, 
*                              const spool_flatfile_format format, 
*                              FILE *file, const char *filepath) 
*
*  FUNCTION
*     Read a list of type <descr> from the stream <file> or a file described
*     by <filepath>.
*
*     <fields_in> names the fields that can be contained in the input.
*
*     The fields actually read are stored in <fields_out>.
*
*     <format> and <instr> describe the data format to expect.
*
*  INPUTS
*     lList **answer_list                - to report any errors
*     const lDescr *descr                - list type
*     const spooling_field *fields_in    - fields that can be contained in input
*     int fields_out[]                   - fields actually read
*     const spool_flatfile_instr *instr  - spooling instructions
*     const spool_flatfile_format format - data format
*     FILE *file                         - file to read or NULL if <filepath> 
*                                          shall be opened
*     const char *filepath               - file to open if <file> == NULL
*
*  RESULT
*     lList * - the list read on success, else NULL
*
*  SEE ALSO
*     spool/flatfile/spool_flatfile_write_list()
*     spool/flatfile/spool_flatfile_read_object()
*******************************************************************************/
lList * 
spool_flatfile_read_list(lList **answer_list, const lDescr *descr, 
                         const spooling_field *fields_in, int fields_out[],
                         bool parse_values, const spool_flatfile_instr *instr,
                         const spool_flatfile_format format,
                         FILE *file,
                         const char *filepath)
{
   bool file_opened = false;
   int token;
   lList *list = NULL;
   const spooling_field *fields = NULL;
   spooling_field *my_fields = NULL;

   DENTER(TOP_LAYER, "spool_flatfile_read_list");

   SGE_CHECK_POINTER_NULL(descr);
   SGE_CHECK_POINTER_NULL(instr);

   /* if no file handle is passed, try to open file for reading */
   if (file == NULL) {
      SGE_CHECK_POINTER_NULL(filepath);

      file = fopen(filepath, "r");
      if (file == NULL) {
         answer_list_add_sprintf(answer_list, STATUS_EDISK,
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_ERROROPENINGFILEFORREADING_SS,
                                 filepath, strerror(errno));
         DEXIT;
         return NULL;
      }

      file_opened = true;
   }

   /* initialize scanner */
   token = spool_scanner_initialize(file);

   /* if no fields are passed, retrieve them from instructions */
   if (fields_in != NULL) {
      fields = fields_in;
   } else {
      my_fields = spool_get_fields_to_spool(answer_list, descr, 
                                            instr->spool_instr);
      if (my_fields == NULL) {
         /* messages generated in spool_get_fields_to_spool */
         DEXIT;
         return NULL;
      }

      fields = my_fields;
   }

   list = _spool_flatfile_read_list(answer_list, descr, instr, 
                                    fields, fields_out, &token, NULL,
                                    parse_values);

   /* if we opened the file, we also have to close it */
   if (file_opened) {
      fclose(file);
   }

   /* if we created our own fields */
   if (my_fields != NULL) {
      my_fields = spool_free_spooling_fields(my_fields);
   }

   DEXIT;
   return list;
}

static lList *
_spool_flatfile_read_list(lList **answer_list, const lDescr *descr, 
                          const spool_flatfile_instr *instr, 
                          const spooling_field *fields, int fields_out[], 
                          int *token, const char *end_token, bool parse_values)
{
   bool stop = false;
   bool first_record = true;
   bool end_token_detected = false;
   char new_end_token[MAX_STRING_SIZE];
   lList *list;
   lListElem *object;

   DENTER(TOP_LAYER, "_spool_flatfile_read_list");

   list = lCreateList("list", descr);
   if (list == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                              ANSWER_QUALITY_ERROR,
                              MSG_ERRORCREATINGLIST);
      DEXIT;
      return NULL;
   }

   if (instr->record_end == '\0') {
      get_end_token(new_end_token, MAX_STRING_SIZE, end_token,
                    instr->record_delimiter);
   } else {
      /* we need no end token, as record_end character is 
       * an explicit end criterium 
       */
      new_end_token[0] = instr->record_end; 
      new_end_token[1] = '\0';
   }

FF_DEBUG("read list");
   /* parse all objects in list */
   while (*token != 0 && !stop) {
      /* check for list end condition */
      if (is_delimiter(*token) && check_end_token(end_token, *spool_text)) {
FF_DEBUG("detected end_token");
         stop = true;
         end_token_detected = true;
         continue;
      }
  
      /* for subsequent records check record_delimiter */
      if (!first_record) {
         if (instr->record_delimiter != '\0') {
            if (!is_delimiter(*token) || 
                *spool_text != instr->record_delimiter) {
               answer_list_add_sprintf(answer_list, STATUS_ESYNTAX,
                                       ANSWER_QUALITY_ERROR,
                                       MSG_PARSINGLISTBADRECORDSEP_DS,
                                       spool_line,
                                       output_delimiter(instr->record_delimiter));
               stop = true;
               continue;
            }
FF_DEBUG("detected record_delimiter");
            *token = spool_lex();
         }
      }

      /* check for record_start */
      if (instr->record_start != '\0') {
         if (!is_delimiter(*token) ||
            *spool_text != instr->record_start) {
            answer_list_add_sprintf(answer_list, STATUS_ESYNTAX,
                                    ANSWER_QUALITY_ERROR,
                                    MSG_PARSINGLISTBADRECORDSTART_DS,
                                    spool_line,
                                    output_delimiter(instr->record_start));
            stop = true;
            continue;
         }
FF_DEBUG("detected record_start");
         *token = spool_lex();
      }

      /* read an object */
      object = _spool_flatfile_read_object(answer_list, descr, NULL, instr,
                                           fields, fields_out, token, 
                                           new_end_token, parse_values);

      /* store object */
      if (object != NULL) {
         lAppendElem(list, object);
      } else {
         /* if no object was read due to an error, a message has been
          * created in _spool_flatfile_read_object
          */
         stop = true;
         continue;
      }
      
      /* check for record_end */
      if (instr->record_end != '\0') {
         if (!is_delimiter(*token) ||
            *spool_text != instr->record_end) {
            answer_list_add_sprintf(answer_list, STATUS_ESYNTAX,
                                    ANSWER_QUALITY_ERROR,
                                    MSG_PARSINGLISTBADRECORDEND_DS,
                                    spool_line,
                                    output_delimiter(instr->record_end));
            stop = true;
            continue;
         }
FF_DEBUG("detected record_end");
         *token = spool_lex();
      }

      first_record = false;
   } /* while */

   if (!end_token_detected) {
      *token = spool_lex();
   }

FF_DEBUG("after parsing list");

   /* if no objects could be read, we need no list */
   if (lGetNumberOfElem(list) == 0) {
      list = lFreeList(list);
   }

   return list;
}

static lListElem *search_for_tree_node(lListElem *ep, const char *id,
                                       int nm1, int nm2)
{
   lListElem *cep, *fep;
   lList *alp;
   dstring node_id = DSTRING_INIT;
   const char *node_id_str = NULL;

   DENTER(TOP_LAYER, "search_for_tree_node");

   if (!ep) {
      DEXIT;
      return NULL;
   }
   
   object_append_field_to_dstring (ep, &alp, &node_id, nm2, '\0');
   node_id_str = sge_dstring_get_string (&node_id);
   
   if (strcmp (id, node_id_str) == 0) {
      DEXIT;
      return ep;
   }

   for_each(cep, lGetList(ep, nm1)) {
      if ((fep = search_for_tree_node(cep, id, nm1, nm2))) {
         DEXIT;
         return fep;
      }
   }
      
   DEXIT;
   return NULL;
}

static spooling_field *get_recursion_field_list (const spool_flatfile_instr *instr)
{
   /* Only 2 entries in a recursion field list */
   spooling_field *fields = (spooling_field *)malloc (sizeof (spooling_field) * 2);
   fields[0].nm = instr->recursion_info.id_field;
   fields[0].width = 0;
   fields[0].name = NULL;
   fields[0].sub_fields = NULL;
   fields[0].read_func = NULL;
   fields[0].write_func = NULL;
   fields[1].nm = NoName;
   fields[1].width = 0;
   fields[1].name = NULL;
   fields[1].sub_fields = NULL;
   fields[1].read_func = NULL;
   fields[1].write_func = NULL;
   
   return fields;
}

void create_spooling_field (
   spooling_field *field,
   int nm, 
   int width, 
   const char *name, 
   struct spooling_field *sub_fields, 
   const void *clientdata, 
   int (*read_func) (lListElem *ep, int nm, const char *buffer, lList **alp), 
   int (*write_func) (const lListElem *ep, int nm, dstring *buffer, lList **alp)
)
{
   field->nm = nm;
   field->width = width;
   field->name = name;
   field->sub_fields = sub_fields;
   field->clientdata = clientdata;
   field->read_func = read_func;
   field->write_func = write_func;
}

int spool_get_unprocessed_field(spooling_field in[], int out[], lList **alpp)
{
   int count = 0;
   
   while (in[count].nm != NoName) {
      int counter = 0;
      
      while (out[counter] != NoName) {
         if (out[counter] == in[count].nm) {
            break;
         }
         
         counter++;
      }
      
      if (out[counter] == NoName) {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, 
                        MSG_FLATFILE_ATTRIBISMISSING_S,
                        (in[count].name == NULL) ?
                           lNm2Str(in[count].nm) :
                           in[count].name));
         answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, 
                         ANSWER_QUALITY_ERROR);
         return in[count].nm;
      }
      
      count++;
   }   
   
   return NoName;
}

int spool_get_number_of_fields(const spooling_field fields[])
{
   int count = 0;
   
   while (fields[count++].nm != NoName);
   
   return count;
}

/****** spool/flatfile/spool_flatfile_add_line_breaks() ************************
*  NAME
*     spool_flatfile_add_line_breaks() -- breaks up long lines by splitting on
*                                         commas and whitespace and inserting
*                                         backslashes at the ends of broken
*                                         lines
*
*  SYNOPSIS
*     static void spool_flatfile_add_line_breaks (dstring *buffer)
*
*  FUNCTION
*     Splits lines greater than MAX_LINE_LENGTH on commas and whitespace.  It
*     tries to choose the most convenient place to insert the break.  The break
*     consists of a space followed by a backslash followed by a new line
*     followed by enough spaces to indent the next line to the beginning of the
*     second word in the first line.  Lines are considered to be demarkated by
*     newlines.
*
*  INPUTS
*     dstring *buffer - The output to be split.  The dstring will be cleared,
*                       and the new output will be stored in it.
*
*******************************************************************************/
static void spool_flatfile_add_line_breaks (dstring *buffer)
{
   int index = 0;
   int word = 0;
   const char *tmp_orig = NULL;
   char *orig = NULL;
   char *strp = NULL;
   char str_buf[MAX_STRING_SIZE];
   char *indent_str = NULL;
   bool first_line = true;

   tmp_orig = sge_dstring_get_string (buffer);
   
   /* This happens when qconf -aconf is used. */
   if (tmp_orig == NULL) {
      return;
   }
   
   orig = strdup (tmp_orig);
   strp = orig;
   
   sge_dstring_clear (buffer);

   str_buf[0] = '\0';
   
   while (strlen (strp) > MAX_LINE_LENGTH - word) {
      /* Account for newlines */
      char *newlp = strchr (strp, '\n');
      index = (newlp - strp) / sizeof (char);
      
      if ((newlp != NULL) && (index <= MAX_LINE_LENGTH - word)) {
         strncpy (str_buf, strp, index + 1);
         str_buf[index + 1] = '\0';
         sge_dstring_append (buffer, str_buf);

         strp = newlp + 1;

         /* Reset to the first line */
         first_line = true;
         
         if (indent_str != NULL) {
            FREE (indent_str);
            word = 0;
         }
         
         continue;
      }
      
      /* Build the indent string by finding the beginning of the second word in
       * the line. */
      if (indent_str == NULL) {
         /* Word should always be 0 when we enter this function, but I'll set
          * it again, just to be sure. */
         word = 0;
         
         /* Find the first whitespace or equals */
         while (!isspace (strp[word]) && (strp[word] != '=')) {
            word++;
         }
         
         /* Now find the next non-whitespace, non-equals to mark the beginning
          * of the second word.  I am assuming this will always work because
          * with the current file formats it does always work. */
         while (isspace (strp[word]) || (strp[word] == '=')) {
            word++;
         }
         
         indent_str = (char *)malloc (word * sizeof (char) + 4);
         indent_str[0] = ' ';
         indent_str[1] = '\\';
         indent_str[2] = '\n';

         for (index = 0; index < word; index++) {
            indent_str[index + 3] = ' ';
         }

         indent_str[word + 3] = '\0';
      }
      
      /* Remove any leading spaces */
      if (!first_line) {
         while (isspace (*strp)) {
            strp++;
         }
      }
      
      /* Break on the last available whitespace or comma */
      /* We have to account for the fact that on all lines after the first, the
       * line length is decreased by the indentation. */
      /* On the first line, we have to start looking at the end of the line and
       * stop looking before we reach the delimiter before the second word. */
      if (first_line) {
         for (index = MAX_LINE_LENGTH - 1; index > word; index--) {
            if (isspace (strp[index]) || (strp[index] == ',')) {
               break;
            }
         }
      }
      /* On later lines, we start looking at the end of the line, adjusted for
       * the amount of space we will indent it, and we stop looking at the
       * second character on the line since having the first character as a
       * break point is useless. */
      else {
         for (index = MAX_LINE_LENGTH - word - 1; index >= 1; index--) {
            if (isspace (strp[index]) || (strp[index] == ',')) {
               break;
            }
         }
      }
      
      if (isspace (strp[index])) {
         strncpy (str_buf, strp, index);
         str_buf[index] = '\0';
         sge_dstring_append (buffer, str_buf);
         sge_dstring_append (buffer, indent_str);

         strp += index + 1;
         
         first_line = false;
         continue;
      }
      else if (strp[index] == ',') {
         strncpy (str_buf, strp, index + 1);
         str_buf[index + 1] = '\0';
         sge_dstring_append (buffer, str_buf);
         sge_dstring_append (buffer, indent_str);

         strp += index + 1;
         
         first_line = false;
         continue;
      }
      else {
         /* Break on the first whitespace past the end of the line */
         /* Break on the last available whitespace or comma */
         /* We have to account for the fact that on all lines after the first, the
          * line length is decreased by the indentation. */
         /* On the first line, we start looking at the end of the line. */
         if (first_line) {
            index = MAX_LINE_LENGTH;

            while ((index < MAX_STRING_SIZE - 1) && (strp[index] != '\0')) {
               if (isspace (strp[index]) || (strp[index] == ',') ||
                   (strp[index] == '\n')) {
                  break;
               }

               index++;
            }
         }
         /* On later lines, we start looking at the end of the line, adjusted for
          * the amount of space we will indent it. */
         else {
            index = MAX_LINE_LENGTH - word;

            while ((index < MAX_STRING_SIZE - 1) && (strp[index] != '\0')) {
               if (isspace (strp[index]) || (strp[index] == ',') ||
                   (strp[index] == '\n')) {
                  break;
               }

               index++;
            }
         }

         if (strp[index] == '\n') {
            strncpy (str_buf, strp, index + 1);
            str_buf[index + 1] = '\0';
            sge_dstring_append (buffer, str_buf);

            strp += index + 1;
            
            /* Reset to first line */
            first_line = true;
            word = 0;

            /* Release the indent string because each line may have different
             * indentation */
            FREE (indent_str);
            
            continue;
         }
         else if (isspace (strp[index])) {
            strncpy (str_buf, strp, index);
            str_buf[index] = '\0';
            sge_dstring_append (buffer, str_buf);
            sge_dstring_append (buffer, indent_str);

            strp += index + 1;
            
            first_line = false;
            continue;
         }
         else if (strp[index] == ',') {
            strncpy (str_buf, strp, index + 1);
            str_buf[index + 1] = '\0';
            sge_dstring_append (buffer, str_buf);
            sge_dstring_append (buffer, indent_str);

            strp += index + 1;

            first_line = false;
            continue;
         }
         else if (index == MAX_STRING_SIZE - 1) {
            strncpy (str_buf, strp, MAX_STRING_SIZE - 1);
            str_buf[MAX_STRING_SIZE - 1] = '\0';
            sge_dstring_append (buffer, str_buf);
            
            strp += MAX_STRING_SIZE - 1;
            
            continue;
         }
         else {
            /* There's no spaces or commas, and hence nothing we can do. */
            break; /* while */
         }
      } /* else */
   } /* while */

   sge_dstring_append (buffer, strp);
   
   if (indent_str != NULL) {
      FREE (indent_str);
   }
   
   FREE (orig);
   
   return;
}
