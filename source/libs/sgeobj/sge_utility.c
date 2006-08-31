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

#include <ctype.h>
#include <string.h>

#include "sgermon.h"
#include "sge_log.h"
#include "cull_list.h"
#include "parse.h"
#include "sge_str.h"
#include "sge_idL.h"
#include "sge_string.h"
#include "sge_answer.h"
#include "sge_utility.h"

#include "cl_communication.h" /* CL_MAXHOSTNAMELEN_LENGTH */

#include "msg_common.h"
#include "msg_qmaster.h"
#include "msg_sgeobjlib.h"

an_status_t verify_str_key(lList **alpp, const char *str, size_t str_length, const char *name) 
{
   static char begin_chars[3] = { '.', '#', 0 };
   static const char *begin_strings[3];

   static const char mid_characters[19] = { '\n', '\t', '\r', ' ', '/', ':', '\'',
      '\"', '\\', '[', ']', '{', '}', '|', '(', ')', '@', '%' , 0};
   static const char *mid_strings[19];

   static const char* keyword[] = { "NONE", "ALL", "TEMPLATE", NULL };
   static const char* keyword_strings[4];

   static int initialized = 0;
   char forbidden_char;
   const char* forbidden_string;
   int i;

   if (!initialized) {
      begin_strings[0] = MSG_GDI_KEYSTR_DOT;
      begin_strings[1] = MSG_GDI_KEYSTR_HASH;
      begin_strings[2] = NULL;
      mid_strings[0] = MSG_GDI_KEYSTR_RETURN;
      mid_strings[1] = MSG_GDI_KEYSTR_TABULATOR;
      mid_strings[2] = MSG_GDI_KEYSTR_CARRIAGERET;
      mid_strings[3] = MSG_GDI_KEYSTR_SPACE;
      mid_strings[4] = MSG_GDI_KEYSTR_SLASH;
      mid_strings[5] = MSG_GDI_KEYSTR_COLON;
      mid_strings[6] = MSG_GDI_KEYSTR_QUOTE;
      mid_strings[7] = MSG_GDI_KEYSTR_DBLQUOTE;
      mid_strings[8] = MSG_GDI_KEYSTR_BACKSLASH;
      mid_strings[9] = MSG_GDI_KEYSTR_BRACKETS;
      mid_strings[10] = MSG_GDI_KEYSTR_BRACKETS;
      mid_strings[11] = MSG_GDI_KEYSTR_BRACES;
      mid_strings[12] = MSG_GDI_KEYSTR_BRACES;
      mid_strings[13] = MSG_GDI_KEYSTR_PIPE;
      mid_strings[14] = MSG_GDI_KEYSTR_PARENTHESIS;
      mid_strings[15] = MSG_GDI_KEYSTR_PARENTHESIS;
      mid_strings[16] = MSG_GDI_KEYSTR_AT;
      mid_strings[17] = MSG_GDI_KEYSTR_PERCENT;
      mid_strings[18] = NULL;
      keyword_strings[0] = MSG_GDI_KEYSTR_KEYWORD;
      keyword_strings[1] = MSG_GDI_KEYSTR_KEYWORD;
      keyword_strings[2] = MSG_GDI_KEYSTR_KEYWORD;
      keyword_strings[3] = NULL;
      initialized = 1;
   }

   if (str == NULL) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_KEYSTR_NULL_S, name));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      return STATUS_EUNKNOWN;
   }

   /* check string length first, if too long -> error */
   if (strlen(str) > str_length) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_KEYSTR_LENGTH_U, sge_u32c(str_length)));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      return STATUS_EUNKNOWN;
   }

   /* check first character */
   i = -1;
   while ((forbidden_char = begin_chars[++i])) {
      if (str[0] == forbidden_char) {
         if (isprint((int) forbidden_char)) {
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_KEYSTR_FIRSTCHAR_SC,
                           begin_strings[i], begin_chars[i]));
         } else {
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_KEYSTR_FIRSTCHAR_S, 
                           begin_strings[i]));
         }
         answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         return STATUS_EUNKNOWN;
      }
   }

   /* check all characters in str */
   i = -1;
   while ((forbidden_char = mid_characters[++i])) {
      if (strchr(str, forbidden_char)) {
         if (isprint((int) forbidden_char)) {
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_KEYSTR_MIDCHAR_SC,
                           mid_strings[i], mid_characters[i]));
         } else {
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_KEYSTR_MIDCHAR_S, 
                           mid_strings[i]));
         }
         answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         return STATUS_EUNKNOWN;
      }
   }

   /* reject invalid keywords */
   i = -1;
   while ((forbidden_string = keyword[++i])) {
      if (!strcasecmp(str, forbidden_string)) {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_KEYSTR_KEYWORD_SS, 
                        keyword_strings[i],
            forbidden_string));
         answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         return STATUS_EUNKNOWN;
      }
   }

   return STATUS_OK;
}

/****** sge_utility/verify_host_name() *****************************************
*  NAME
*     verify_host_name() -- verify a hostname
*
*  SYNOPSIS
*     bool 
*     verify_host_name(lList **answer_list, const char *host_name) 
*
*  FUNCTION
*     Verifies if a hostname is correct (regarding maximum length etc.).
*
*  INPUTS
*     lList **answer_list   - answer list to pass back error messages
*     const char *host_name - the hostname to verify
*
*  RESULT
*     bool - true on success,
*            false on error with error message in answer_list
*
*  NOTES
*     MT-NOTE: verify_host_name() is MT safe 
*******************************************************************************/
bool verify_host_name(lList **answer_list, const char *host_name)
{
   bool ret = true;

   if (host_name == NULL || *host_name == '\0') {
      answer_list_add_sprintf(answer_list, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR, 
                              MSG_HOSTNAME_NOT_EMPTY);
      ret = false;
   }

   if (ret) {
      if (strlen(host_name) > CL_MAXHOSTNAMELEN_LENGTH) {
         answer_list_add_sprintf(answer_list, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR, 
                                 MSG_HOSTNAME_NOT_EMPTY);
      }
   }

   /* TODO: further verification (e.g. character set) */

   return ret;
}
