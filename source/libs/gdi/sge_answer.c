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

#include <stdarg.h>
#include "sge.h"
#include "sgermon.h"
#include "cull.h"

#include "sge_unistd.h"
#include "sge_dstring.h"
#include "sge_log.h"

#include "msg_gdilib.h"

#include "sge_answer.h"

/****** gdi/answer/-AnswerList ************************************************
*  NAME
*     AnswerList - Object used to return errors/warning/infos
*
*  FUNCTION
*     Answer elements and lists are used to exchange information
*     about the level of success of an operation between caller
*     and callee. Especially in GDI answer element and lists are
*     used to transfer error information from server components
*     to clients.
*
*     Example:
*
*        void caller(void) {
*           lList *answer_list = NULL; 
*
*           callee(&answer_list);
*           if (answer_list_has_error(&answer_list)) {
*              try_to_handle_error();
*              answer_list = lFreeList(answer_list);
*           }
*        } 
*
*        void callee(lList **answer_list) {
*           char *s = malloc(256);
*
*           if (s == NULL) {
*              answer_list_add(answer_list, "no memory", 
*                              STATUS_ERROR, ANSWER_QUALITY_ERROR);
*              return;
*           }
*        }
*
*  SEE ALSO
*     gdi/answer/answer_has_quality() 
*     gdi/answer/answer_is_recoverable() 
*     gdi/answer/answer_exit_if_not_recoverable() 
*     gdi/answer/answer_get_quality_text() 
*     gdi/answer/answer_get_status() 
*     gdi/answer/answer_print_text() 
*     gdi/answer/answer_list_add() 
*     gdi/answer/answer_list_add_sprintf() 
*     gdi/answer/answer_list_has_quality() 
*     gdi/answer/answer_list_has_error() 
*     gdi/answer/answer_list_on_error_print_or_exit() 
******************************************************************************/

/****** gdi/answer/answer_has_quality() ***************************************
*  NAME
*     answer_has_quality() -- Check for certain answer quality 
*
*  SYNOPSIS
*     int answer_has_quality(const lListElem *answer, 
*                            answer_quality_t quality) 
*
*  FUNCTION
*     Return true (1) if "answer" has the given "quality" 
*
*  INPUTS
*     const lListElem *answer  - AN_Type element 
*     answer_quality_t quality - Quality id 
*
*  RESULT
*     int - true or false 
*******************************************************************************/
int answer_has_quality(const lListElem *answer, answer_quality_t quality) 
{
   return (lGetUlong(answer, AN_quality) ==  quality) ? 1 : 0;
}

/****** gdi/answer/answer_is_recoverable() ************************************
*  NAME
*     answer_is_recoverable() -- Check for recoverable error 
*
*  SYNOPSIS
*     int answer_is_recoverable(const lListElem *answer) 
*
*  FUNCTION
*     This function return true (1) if "answer" is an error where the
*     calling function application may recover from.
*     Following error are handeled as nonrecoverable:
*
*        STATUS_NOQMASTER
*        STATUS_NOCOMMD
*        STATUS_ENOKEY 
*
*  INPUTS
*     const lListElem *answer - AN_Type element 
*
*  RESULT
*     int - true or false
*******************************************************************************/
int answer_is_recoverable(const lListElem *answer)
{
   int ret = 1;

   if (answer != NULL) {
      const int max_non_recoverable = 3;
      const u_long32 non_recoverable[] = {
         STATUS_NOQMASTER,
         STATUS_NOCOMMD,
         STATUS_ENOKEY
      };
      u_long32 status = lGetUlong(answer, AN_status);
      int i;

      for (i = 0; i < max_non_recoverable; i++) {
         if (status == non_recoverable[i]) {
            ret = 0;
            break;
         }
      }
   }
   return ret;
}

/****** gdi/answer/answer_exit_if_not_recoverable() ***************************
*  NAME
*     answer_exit_if_not_recoverable() -- Exit on certain errors 
*
*  SYNOPSIS
*     void answer_exit_if_not_recoverable(const lListElem *answer) 
*
*  FUNCTION
*     This function checks if "answer" is an unrecoverable error.
*     (answer_is_recoverable() is used to check this.) The error
*     text will then be printed to stderr and the calling
*     process will be terminated with exit code 1. 
*
*  INPUTS
*     const lListElem *answer - AN_Type element 
*
*  NOTES
*     This function may never return. 
*******************************************************************************/
void answer_exit_if_not_recoverable(const lListElem *answer)
{
   DENTER(GDI_LAYER, "answer_exit_if_not_recoverable");

   if (!answer_is_recoverable(answer)) {
      fprintf(stderr, "%s %s", answer_get_quality_text(answer),
              lGetString(answer, AN_text));
      DEXIT;
      SGE_EXIT(1);
   }
}

/****** gdi/answer/answer_get_quality_text() **********************************
*  NAME
*     answer_get_quality_text() -- Get quality text 
*
*  SYNOPSIS
*     const char* answer_get_quality_text(const lListElem *answer) 
*
*  FUNCTION
*     Returns a string representation for the quality of the "answer" 
*
*  INPUTS
*     const lListElem *answer - AN_Type list 
*
*  RESULT
*     const char* - String
*******************************************************************************/
const char *answer_get_quality_text(const lListElem *answer) 
{
   const char *quality_text[] = {
      "ERROR",
      "WARNING",
      "INFO"
   };
   u_long32 quality = lGetUlong(answer, AN_quality);

   if (quality > 2) {
      quality = 0;
   }
   return quality_text[quality];
}

/****** gdi/answer/answer_get_status() ****************************************
*  NAME
*     answer_get_status() -- Return the error status.
*
*  SYNOPSIS
*     u_long32 answer_get_status(const lListElem *answer) 
*
*  FUNCTION
*     Return the error status of "answer". 
*
*  INPUTS
*     const lListElem *answer - AN_Type element 
*
*  RESULT
*     u_long32 - error status
*******************************************************************************/
u_long32 answer_get_status(const lListElem *answer) 
{
   return lGetUlong(answer, AN_status);
}

/****** gdi/answer/answer_print_text() ****************************************
*  NAME
*     answer_print_text() -- Prints error text 
*
*  SYNOPSIS
*     void answer_print_text(const lListElem *answer, 
*                            FILE *stream
*                            const char *prefix,
*                            const char *suffix) 
*
*  FUNCTION
*     Prints "prefix", the error text of "answer" and "suffix" 
*     to "stream".
*
*  INPUTS
*     const lListElem *answer - AN_Type element 
*     FILE *stream            - Output stream 
*     const char *prefix      - Introductional message
*     const char *prefix      - Final message
*******************************************************************************/
void answer_print_text(const lListElem *answer, 
                       FILE *stream, 
                       const char *prefix,
                       const char *suffix)
{
   const char *text = lGetString(answer, AN_text);

   if (prefix != NULL) {
      fprintf(stream, "%s", prefix);
   }
   if (text != NULL) {
      fprintf(stream, "%s", text);
   }
   if (suffix != NULL) {
      fprintf(stream, "%s", suffix);
   }
}

/****** gdi/answer/answer_list_add() ******************************************
*  NAME
*     answer_list_add() -- Add an answer to an answer list 
*
*  SYNOPSIS
*     int answer_list_add(lList **answer_list, 
*                         const char *text, 
*                         u_long32 status, 
*                         answer_quality_t quality) 
*
*  FUNCTION
*     This function creates a new answer element (using "quality",
*     "status" and "text"). The new element will be appended to
*     "answer_list" 
*
*     If "answer_list" is NULL, no action is performed.
*
*     If the list pointed to by "answer_list" is NULL, a new list will be
*     created.
*
*  INPUTS
*     lList **answer_list      - AN_Type list 
*     const char *text         - answer text 
*     u_long32 status          - answer status 
*     answer_quality_t quality - answer quality 
*
*  RESULT
*     int - error state
*        true  - OK
*        false - error occured
*
*  SEE ALSO
*     gdi/answer/answer_list_add_sprintf()
*******************************************************************************/
bool
answer_list_add(lList **answer_list, const char *text, 
                u_long32 status, answer_quality_t quality) 
{
   int ret = false;

   DENTER(GDI_LAYER, "answer_list_add");

   if (answer_list != NULL) {
      lListElem *answer = lCreateElem(AN_Type); 

      if (answer != NULL) {
         lSetString(answer, AN_text, text);
         lSetUlong(answer, AN_status, status);
         lSetUlong(answer, AN_quality, quality);
         
         if (*answer_list == NULL) {
            *answer_list = lCreateList("", AN_Type);
         }

         if (*answer_list != NULL) {
            lAppendElem(*answer_list, answer);
            ret = true;
         }
      }

      if (!ret) {
         answer = lFreeElem(answer);
      }
   }

   DEXIT;
   return ret;
}

/****** gdi/answer/answer_list_add_sprintf() ***********************************
*  NAME
*     answer_list_add_sprintf() -- Format add an answer to an answer list
*
*  SYNOPSIS
*     bool answer_list_add_sprintf(lList **answer_list, u_long32 status, 
*                                  answer_quality_t quality, const char *fmt, 
*                                  ...) 
*
*  FUNCTION
*     This function creates a new answer element having the properties
*     "status", "quality", and a message text created from fmt and the 
*     following variable argument list.
*
*     The new element will be appended to "answer_list".
*     
*     If "answer_list" is NULL, no action is performed.
*
*     If the list pointed to by "answer_list" is NULL, a new list will be
*     created.
*
*  INPUTS
*     lList **answer_list      - AN_Type list
*     u_long32 status          - answer status
*     answer_quality_t quality - answer quality
*     const char *fmt          - format string to create message (printf)
*     ...                      - arguments used for formatting message
*
*  RESULT
*     bool - true on success, else false
*
*  SEE ALSO
*     gdi/answer/answer_list_add()
*******************************************************************************/
bool 
answer_list_add_sprintf(lList **answer_list, u_long32 status, 
                        answer_quality_t quality, const char *fmt, ...)
{
   bool ret = false;

   DENTER(GDI_LAYER, "answer_list_add");
   
   if (answer_list != NULL) {
      dstring buffer = DSTRING_INIT;
      const char *message;
      va_list ap;

      va_start(ap, fmt);
      message = sge_dstring_vsprintf(&buffer, fmt, ap);

      if (message != NULL) {
         ret = answer_list_add(answer_list, message, status, quality);
      }

      sge_dstring_clear(&buffer);
   }

   DEXIT;
   return ret;
}

/****** gdi/answer/answer_list_has_quality() **********************************
*  NAME
*     answer_list_has_quality() -- Contains list  
*
*  SYNOPSIS
*     int answer_list_has_quality(lList **answer_list, 
*                                 answer_quality_t quality) 
*
*  FUNCTION
*     The function returns true (1) if the "answer_list" containes
*     at least one answer element with the given "quality". 
*
*  INPUTS
*     lList **answer_list      - AN_Type list 
*     answer_quality_t quality - quality value 
*
*  RESULT
*     int - true or false
*******************************************************************************/
int answer_list_has_quality(lList **answer_list, answer_quality_t quality)
{
   int ret = 0;

   if (answer_list != NULL) {
      lListElem *answer;   /* AN_Type */

      for_each(answer, *answer_list) {
         if (answer_has_quality(answer, quality)) {
            ret = 1;
            break;
         }
      }
   }
   return ret;
}

/****** gdi/answer/answer_list_has_error() ************************************
*  NAME
*     answer_list_has_error() -- Is an "error " in the list 
*
*  SYNOPSIS
*     int answer_list_has_error(lList **answer_list) 
*
*  FUNCTION
*     The function returns true (1) if the "answer_list" containes
*     at least one error answer element 
*
*  INPUTS
*     lList **answer_list - AN_Type list 
*
*  RESULT
*     int - true or false
*******************************************************************************/
int answer_list_has_error(lList **answer_list)
{
   int ret;

   DENTER(TOP_LAYER, "answer_list_has_error");
   ret = answer_list_has_quality(answer_list, ANSWER_QUALITY_ERROR);
   DEXIT;
   return ret;
}               

/****** gdi/answer/answer_list_on_error_print_or_exit() ***********************
*  NAME
*     answer_list_on_error_print_or_exit() -- Print and/or exit 
*
*  SYNOPSIS
*     void answer_list_on_error_print_or_exit(lList **answer_list, 
*                                             FILE *stream) 
*
*  FUNCTION
*     The error texts of all answer elements within "answer_list" are
*     printed to "stream". If an unrecoverable error is detected
*     the calling process will be terminated. 
*
*  INPUTS
*     lList **answer_list - AN_Type list 
*     FILE *stream        - output stream 
******************************************************************************/
void answer_list_on_error_print_or_exit(lList **answer_list, FILE *stream)
{
   lListElem *answer;   /* AN_Type */

   for_each(answer, *answer_list) {
      answer_exit_if_not_recoverable(answer);
      answer_print_text(answer, stream, NULL, NULL);
   }
}

/****** gdi/answer/answer_list_print_err_warn() *******************************
*  NAME
*     answer_list_print_err_warn() -- Print and/or exit 
*
*  SYNOPSIS
*     int answer_list_print_err_warn(lList **answer_list, 
*                                    const char *err_prefix, 
*                                    const char *warn_prefix) 
*
*  FUNCTION
*     Prints all messages contained in "answer_list". All error
*     messages will be printed to stderr with an an initial "err_prefix".
*     All warning and info messages will be printed to stdout with
*     the prefix "warn_prefix". 
*
*     If the "answer_list" contains at least one error then this
*     function will return with a positive return value. This value
*     is the errror status of the first error message.
*
*     If there is no error contained in 'answer_list' than this function 
*     will return with a value of 0. 
*
*     "*answer_list" will be freed.
*
*  INPUTS
*     lList **answer_list     - AN_Type list 
*     const char *err_prefix  - e.g. "qsub: "
*     const char *warn_prefix - e.g. MSG_WARNING
******************************************************************************/
int answer_list_print_err_warn(lList **answer_list, 
                               const char *err_prefix,
                               const char *warn_prefix)
{
   int do_exit = 0;
   lListElem *answer;   /* AN_Type */
   u_long32 status = 0;

   DENTER(TOP_LAYER, "answer_list_print_err_warn");
   for_each(answer, *answer_list) {
      if (answer_has_quality(answer, ANSWER_QUALITY_ERROR)) {
         answer_print_text(answer, stderr, err_prefix, NULL);
         if (do_exit == 0) {
            status = answer_get_status(answer);
            do_exit = 1;
         }
      } else {
         answer_print_text(answer, stdout, warn_prefix, NULL);
      }
   }
   *answer_list = lFreeList(*answer_list);
   DEXIT;
   return (int)status;
}

/****** gdi/answer/answer_list_handle_request_answer_list() ********************
*  NAME
*     answer_list_handle_request_answer_list() -- handle result of a gdi request
*
*  SYNOPSIS
*     int answer_list_handle_request_answer_list(lList **answer_list, 
*                                                FILE *stream) 
*
*  FUNCTION
*     Processes the answer list that results from a gdi request
*     (sge_gdi or sge_gdi_multi).
*     Outputs and errors and warnings and returns the first error
*     or warning status code.
*     The answer list is freed.
*
*  INPUTS
*     lList **answer_list - answer list to process
*     FILE *stream        - output stream
*
*  RESULT
*     int - first error or warning status code or STATUS_OK
*******************************************************************************/
int answer_list_handle_request_answer_list(lList **answer_list, FILE *stream) {
   lListElem *answer;
   int first_error = STATUS_OK;

   if(answer_list == NULL || *answer_list == NULL) {
      fprintf(stream, MSG_ANSWER_NOANSWERLIST);
      return STATUS_EUNKNOWN;
   }

   for_each(answer, *answer_list) {
      if(answer_has_quality(answer, ANSWER_QUALITY_ERROR) ||
         answer_has_quality(answer, ANSWER_QUALITY_WARNING)) {
         answer_print_text(answer, stream, NULL, NULL);
         if(first_error == STATUS_OK) {
            first_error = lGetUlong(answer, AN_status);
         }
      }
   }

   *answer_list = lFreeList(*answer_list);

   return first_error;
}
