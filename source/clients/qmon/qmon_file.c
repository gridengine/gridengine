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

#include <Xm/Xm.h>

#include "qmon_rmon.h"
#include "qmon_cull.h"
#include "qmon_browser.h"
#include "qmon_file.h"
#include "sge_unistd.h"
#include "sge_gdi_intern.h"
#include "sge_answer.h"

lList* qmonReadFile(
char *filename 
) {
   char *text;
   SGE_STRUCT_STAT statb;
   FILE *fp;
   lList *answer = NULL;
   char buf[BUFSIZ];

   DENTER(GUI_LAYER, "qmonReadFile");

   if (!filename) {
      answer_list_add(&answer, "No filename specified", 
                      STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      DEXIT;
      return answer;
   }

   /* 
   ** make sure the file is a regular text file and open it 
   */
   if (SGE_STAT(filename, &statb) == -1 || (statb.st_mode & S_IFMT) != S_IFREG ||
            !(fp = fopen(filename, "r"))) {
      sprintf(buf, "Cant open file '%s' for reading !", filename);
      answer_list_add(&answer, buf, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      DEXIT;
      return answer;
   }

   /* 
   ** put the contents of the file in the Text widget by allocating
   ** enough space for the entire file, reading the file into the
   ** allocated space, and using XmTextFieldSetString() to show the file.
   */
   if (!(text = XtMalloc((unsigned)(statb.st_size + 1)))) {
      sprintf(buf, "Cant alloc enough space for %s", filename);
      answer_list_add(&answer, buf, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      fclose(fp);
      DEXIT;
      return answer;
   }

   if (!fread(text, sizeof (char), statb.st_size + 1, fp)) {
      sprintf(buf, "May not have read entire file!\n");
      answer_list_add(&answer, buf, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
   }

   text[statb.st_size] = 0; /* be sure to NULL-terminate */

   /* insert file contents in browser */
   qmonBrowserShow(text);

   /* free all allocated space and close */
   XtFree(text);
   fclose(fp);

   DEXIT;
   return answer;
}
