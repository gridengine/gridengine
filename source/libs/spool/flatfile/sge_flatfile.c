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

/* spool */
#include "spool/sge_spooling_utilities.h"

/* messages */
#include "msg_common.h"
#include "spool/msg_spoollib.h"
#include "spool/flatfile/msg_spoollib_flatfile.h"

/* local */
#include "spool/flatfile/sge_spooling_flatfile_scanner.h"
#include "spool/flatfile/sge_flatfile.h"

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
                                   const spooling_field *fields);

static bool
spool_flatfile_write_list_fields(lList **answer_list, const lList *list, 
                                 dstring *buffer, 
                                 const spool_flatfile_instr *instr,
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

static lListElem *
_spool_flatfile_read_object(lList **answer_list, const lDescr *descr, 
                            const spool_flatfile_instr *instr, const spooling_field *fields, int fields_out[], int *token,
                            const char *end_token);

static lList *
_spool_flatfile_read_list(lList **answer_list, const lDescr *descr, 
                          const spool_flatfile_instr *instr, const spooling_field *fields, int fields_out[], int *token,
                          const char *end_token);

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
                          spooling_field *fields)
{
   dstring buffer = DSTRING_INIT;
   const lListElem *object;
   int i;

   DENTER(TOP_LAYER, "spool_flatfile_align_list");

   SGE_CHECK_POINTER_FALSE(list);
   SGE_CHECK_POINTER_FALSE(fields);

   for (i = 0; fields[i].nm != NoName; i++) {
      fields[i].width = sge_strlen(lNm2Str(fields[i].nm));
   }

   for_each (object, list) {
      for (i = 0; fields[i].nm != NoName; i++) {
         const char *value;
         
         sge_dstring_clear(&buffer);
         value = object_append_field_to_dstring(object, answer_list, 
                                                &buffer, fields[i].nm, '\0');
         fields[i].width = MAX(fields[i].width, sge_strlen(value));
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
                          const char *filepath)
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
            if (!spool_flatfile_align_list(answer_list, list, my_fields)) {
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
         if(!spool_flatfile_write_list_fields(answer_list, list, &char_buffer, 
                                              instr, fields)) {
            /* in case of errors, messages are in answer_list,
             * clear data - we don't want to write erroneous data */
            sge_dstring_clear(&char_buffer); 
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
                                      filepath);

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
                            const spooling_field *fields_in,
                            const spool_flatfile_instr *instr,
                            const spool_flatfile_destination destination,
                            const spool_flatfile_format format, 
                            const char *filepath)
{
   dstring         char_buffer = DSTRING_INIT;
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
                                                &char_buffer, instr, fields)) {
            /* in case of errors, messages are in answer_list,
             * clear data - we don't want to write erroneous data */
            sge_dstring_clear(&char_buffer); 
         }

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
                                      filepath);

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
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_STDFILEHANDLECLOSEDORCORRUPTED_S,
                                    "<stdout>");
            return NULL;
         }

#if !defined(AIX42) && !defined(DARWIN6)
         flockfile(file);
#endif
         fflush(file);
         *filepath_out = strdup("<stdout>");
         break;
      case SP_DEST_STDERR:
         file = stderr;

         /* check stderr file handle */
         if (!sge_check_stdout_stream(file, STDERR_FILENO)) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
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
      /* message generated in spool_flatfile_open_file */
      DEXIT;
      return NULL;
   }

   /* write data */
   if (fwrite(data, sizeof(char), data_len, file) != data_len) {
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
                                   const spooling_field *fields)
{
   int i, first_field;
   dstring field_buffer = DSTRING_INIT;
   const lDescr *descr;

   DENTER(TOP_LAYER, "spool_flatfile_write_object_fields");

   SGE_CHECK_POINTER_FALSE(object);
   SGE_CHECK_POINTER_FALSE(buffer);
   SGE_CHECK_POINTER_FALSE(instr);
   SGE_CHECK_POINTER_FALSE(fields);
 
   descr = lGetElemDescr(object);
 
   /* clear input buffer */
   sge_dstring_clear(buffer);

   /* loop over all fields */
   i = 0;
   first_field = true;

   for (i = 0; fields[i].nm != NoName; i++) {
      const char *value = NULL;
      int pos;

      pos = lGetPosInDescr(descr, fields[i].nm);
      if (pos < 0) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_WARNING, 
                                 MSG_NMNOTINELEMENT_S,
                                 lNm2Str(fields[i].nm));
         continue;
      }

      /* if not first field, output field_delimiter */
      if (!first_field) {
         sge_dstring_append_char(buffer, instr->field_delimiter);
      } else {   
         first_field = false;
      }

      /* if show_field_names, output field name */
      if (fields[i].name != NULL) {
         const char *name = fields[i].name;

         /* respect alignment */
         if (fields[i].width > 0) {
            sge_dstring_sprintf_append(buffer, "%-*s", fields[0].width, name);
         } else {
            sge_dstring_append(buffer, name);
         }

         /* output name-value delimiter */
         if (instr->name_value_delimiter != '\0') {
            sge_dstring_append_char(buffer, instr->name_value_delimiter);
         } else {
            sge_dstring_append_char(buffer, ' ');
         }
      }

      /* output value */
      if (mt_get_type(descr[pos].mt) == lListT) {
         const spool_flatfile_instr *sub_instr = (spool_flatfile_instr *)fields[i].clientdata;
         /* if no field specific sub_instr exists, use default from inst */
         if (sub_instr == NULL) {
            sub_instr = instr->sub_instr;
         }

         if(sub_instr == NULL || fields[i].sub_fields == NULL) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_WARNING, 
                                    MSG_DONTKNOWHOWTOSPOOLSUBLIST_SS,
                                    lNm2Str(fields[i].nm), SGE_FUNC);
            sge_dstring_append(buffer, NONE_STR);
         } else {
            lList *sub_list = lGetList(object, fields[i].nm);      

            if (sub_list == NULL || lGetNumberOfElem(sub_list) == 0) {
               sge_dstring_append(buffer, NONE_STR);
            } else {
               if (!spool_flatfile_write_list_fields(answer_list, sub_list, 
                                                     &field_buffer, 
                                                     sub_instr,
                                                     fields[i].sub_fields)) {
                  /* error handling has been done in spool_flatfile_write_list_fields */
               } else {
                  sge_dstring_append_dstring(buffer, &field_buffer);
               }
            }
         }
      } else {
         sge_dstring_clear(&field_buffer);
         value = object_append_field_to_dstring(object, answer_list, 
                                                &field_buffer, fields[i].nm, 
                                                '\0');
         if (instr->align_data) {
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
                                 const spool_flatfile_instr *instr,
                                 const spooling_field *fields)
{
   lListElem *ep;
   int first = true;
   dstring record_buffer = DSTRING_INIT;

   DENTER(TOP_LAYER, "spool_flatfile_write_list_fields");

   SGE_CHECK_POINTER_FALSE(list);
   SGE_CHECK_POINTER_FALSE(buffer);
   SGE_CHECK_POINTER_FALSE(instr);
   SGE_CHECK_POINTER_FALSE(fields);
  
   /* clear input buffer */
   sge_dstring_clear(buffer);
 
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

      if (!spool_flatfile_write_object_fields(answer_list, ep, &record_buffer, 
                                              instr, fields)) {
         /* error message generated in spool_flatfile_write_object_fields */
      } else {
         sge_dstring_append_dstring(buffer, &record_buffer);
      }

      /* if record_end, output record end, else record_delimiter */
      if (instr->record_end != '\0') {
         sge_dstring_append_char(buffer, instr->record_end);
      }
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
                           const spooling_field *fields_in,
                           int fields_out[],
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
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
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

   object = _spool_flatfile_read_object(answer_list, descr, instr, 
                                        fields, fields_out, &token, NULL);

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
                            const spool_flatfile_instr *instr, 
                            const spooling_field *fields, int fields_out[],
                            int *token, const char *end_token)
{
   int field_index = -1;
   lListElem *object = NULL;
   dstring buffer = DSTRING_INIT;
   bool stop = false;

   DENTER(TOP_LAYER, "_spool_flatfile_read_object");

FF_DEBUG("reading object");

   while (*token != 0 && !stop) {
      int nm = NoName;
      int pos, type;
      bool field_end  = false;
      bool record_end = false;
        
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
         *token = spool_lex();
      }

      /* check for eof */
      if (*token == 0) {
FF_DEBUG("eof detected");
         continue;
      }

      if (object == NULL) {
         object = lCreateElem(descr);
         if (object == NULL) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_ERRORCREATINGOBJECT);
            stop = true;
            continue;
         }
      }

      /* read field name from file or from field list */
      if (instr->show_field_names) {
         /* read field name from file */#
FF_DEBUG("read field name");
         if (*token != SPFT_WORD) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                    ANSWER_QUALITY_ERROR,
                                    MSG_PARSINGOBJECTEXPECTEDBUTGOT_DSSD,
                                    __LINE__,
                                    "<field name>",
                                    token == 0 ? "<EOF>" : spool_text,
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

         /* not found -> error */
         if (nm == NoName) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                    ANSWER_QUALITY_ERROR,
                                    MSG_UNKNOWNATTRIBUTENAME_S, spool_text);
            stop = true;
            continue;
         }
        
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
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                       ANSWER_QUALITY_ERROR,
                                       MSG_PARSINGOBJECTEXPECTEDBUTGOT_DSSD,
                                       __LINE__,
                                       output_delimiter(instr->name_value_delimiter),
                                       token == 0 ? "<EOF>" : spool_text,
                                       spool_line);
               stop = true;
               continue;
            }

            *token = spool_lex();
         }
      } else {
FF_DEBUG("eval next field");
         /* get next field from field array */   
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
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                 ANSWER_QUALITY_ERROR,
                                 MSG_ATTRIBUTENOTINOBJECT_S, lNm2Str(nm));
         stop = true;
         continue;
      }

      /* if list of read fields is requested in fields_out, store this info */
      if (fields_out != NULL) {
         add_nm_to_set(fields_out, nm);
      }

      type = mt_get_type(descr[pos].mt);
      
      /* now read the data */
      if (type == lListT) {
         lList *list;
         const lDescr *sub_descr;

FF_DEBUG("reading list");
         /* check for empty sublist */
         if (*token == SPFT_WORD && 
             sge_strnullcasecmp(spool_text, NONE_STR) == 0) {
FF_DEBUG("empty list");
            *token = spool_lex();

            /* check for field end - we have to skip it later */
            if (is_delimiter(*token) && *spool_text == instr->field_delimiter) {
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
               list = _spool_flatfile_read_list(answer_list, sub_descr, 
                                                sub_instr, 
                                                fields[field_index].sub_fields, 
                                                fields_out, token, 
                                                new_end_token);
            }
            lSetPosList(object, pos, list);
         }
      } else {
         /* read field data and append until field/record end */
         sge_dstring_clear(&buffer);
         spool_return_whitespace = true;

         while (*token != 0 && !field_end && !record_end) {
FF_DEBUG("reading value");
            if (is_delimiter(*token)) {
               /* check for external end condition */
               if (check_end_token(end_token, *spool_text)) {
FF_DEBUG("detected end_token");
                  record_end = true;
                  continue;
               }
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
            }
            
            /* store data */
            sge_dstring_append(&buffer, spool_text);
            *token = spool_lex();
         }
         spool_return_whitespace = false;
         object_parse_field_from_string(object, answer_list, nm, 
                                        sge_dstring_get_string(&buffer));
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
         *token = spool_lex();
      } else {
         if (instr->field_delimiter != '\0') {
            if (!is_delimiter(*token) ||
                *spool_text != instr->field_delimiter) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                       ANSWER_QUALITY_ERROR,
                                       MSG_PARSINGOBJECTEXPECTEDBUTGOT_DSSD,
                                       __LINE__,
                                       output_delimiter(instr->field_delimiter),
                                       token == 0 ? "<EOF>" : spool_text,
                                       spool_line);
               stop = true;
               continue;
            }
FF_DEBUG("skipping field delimiter");
            *token = spool_lex();
         }
      }
   }

FF_DEBUG("after parsing object");

   /* cleanup */
   sge_dstring_free(&buffer);
   DEXIT;
   return object;
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
                         const spool_flatfile_instr *instr,
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
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
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
                                    fields, fields_out, &token, NULL);

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
                          int *token, const char *end_token)
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
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                       ANSWER_QUALITY_ERROR,
                                       MSG_PARSINGLISTEXPECTEDBUTGOT_DSSD,
                                       __LINE__,
                                       output_delimiter(instr->record_delimiter),
                                       *token == 0 ? "<EOF>" : spool_text,
                                       spool_line);
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
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                    ANSWER_QUALITY_ERROR,
                                    MSG_PARSINGLISTEXPECTEDBUTGOT_DSSD,
                                    __LINE__,
                                    output_delimiter(instr->record_start),
                                    token == 0 ? "<EOF>" : spool_text,
                                    spool_line);
            stop = true;
            continue;
         }
FF_DEBUG("detected record_start");
         *token = spool_lex();
      }

      /* read an object */
      object = _spool_flatfile_read_object(answer_list, descr, instr, fields,
                                           fields_out, token, 
                                           new_end_token);

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
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                    ANSWER_QUALITY_ERROR,
                                    MSG_PARSINGLISTEXPECTEDBUTGOT_DSSD,
                                    __LINE__,
                                    output_delimiter(instr->record_end),
                                    token == 0 ? "<EOF>" : spool_text,
                                    spool_line);
            stop = true;
            continue;
         }
FF_DEBUG("detected record_end");
         *token = spool_lex();
      }

      first_record = false;
   }

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
