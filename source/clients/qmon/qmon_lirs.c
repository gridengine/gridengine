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
#include <stdlib.h>

#include <Xm/Xm.h>
#include <Xm/Text.h>

#include <Xmt/Xmt.h>
#include <Xmt/MsgLine.h>
#include <Xmt/Dialogs.h>
#include <Xmt/Create.h>

#include "qmon_rmon.h"
#include "qmon_cull.h"
#include "qmon_message.h"
#include "qmon_file.h"
#include "qmon_appres.h"
#include "sge_answer.h"
#include "qmon_comm.h"
#include "qmon_timer.h"
#include "qmon_util.h"
#include "sge_limit_rule_qconf.h"
#include "sge_limit_ruleL.h"
#include "sge_io.h"
#include "spool/flatfile/sge_flatfile.h"
#include "spool/flatfile/sge_flatfile_obj.h"
#include "msg_common.h"
#include "msg_utilib.h"

#ifdef TEST_GDI2
#include "sge_gdi_ctx.h"
extern sge_gdi_ctx_class_t *ctx;
#endif

static Widget qmon_lirs = 0;
static Widget lirs_text = 0;

static void qmonLIRSOkay(Widget w, XtPointer cld, XtPointer cad);
static void qmonLIRSCancel(Widget w, XtPointer cld, XtPointer cad);
static void qmonLIRSSetText(Widget tw, lList *lirs_list, lList **alpp);
static void qmonLIRSGetText(Widget tw, lList *lirs_list, lList **alpp);

/*-------------------------------------------------------------------------*/
void qmonLIRSPopup(Widget w, XtPointer cld, XtPointer cad) 
{
   Widget shell, lirs_okay, lirs_cancel;
   lList *alp = NULL;
   lList *lirs_list = NULL;

   DENTER(GUI_LAYER, "qmonLIRSPopup");

   /* set busy cursor */
   XmtDisplayBusyCursor(w);

   if (!qmon_lirs) {
      shell = XmtGetTopLevelShell(w);
      qmon_lirs = XmtBuildQueryDialog(shell, "lirs_shell", 
                              NULL, 0,
                              "lirs_text", &lirs_text,
                              "lirs_okay", &lirs_okay,
                              "lirs_cancel", &lirs_cancel,
                              NULL);
      XtAddCallback(lirs_okay, XmNactivateCallback,
                     qmonLIRSOkay, NULL); 
      XtAddCallback(lirs_cancel, XmNactivateCallback, 
                     qmonLIRSCancel, NULL);
   }


   qmonMirrorMultiAnswer(LIRS_T, &alp);
   if (alp) {
      qmonMessageBox(w, alp, 0);
      lFreeList(&alp);
      /* set normal cursor */
      XmtDisplayDefaultCursor(w);
      DEXIT;
      return;
   }

   lirs_list = qmonMirrorList(SGE_LIRS_LIST);

   xmui_manage(qmon_lirs);

   qmonLIRSSetText(lirs_text, lirs_list, &alp);
   if (alp) {
      qmonMessageBox(w, alp, 0);
      lFreeList(&alp);
   }

   /* set default cursor */
   XmtDisplayDefaultCursor(w);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonLIRSOkay(Widget w, XtPointer cld, XtPointer cad)
{
   lList *alp = NULL;
   lList *lirs_list = NULL;

   DENTER(GUI_LAYER, "qmonLIRSOkay");
   
   lirs_list = qmonMirrorList(SGE_LIRS_LIST);
   qmonLIRSGetText(lirs_text, lirs_list, &alp);
   if (answer_list_has_error(&alp)) {
      qmonMessageBox(w, alp, 0);
      lFreeList(&alp);
      DEXIT;
      return;
   }

   xmui_unmanage(qmon_lirs);
   lFreeList(&alp);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonLIRSCancel(Widget w, XtPointer cld, XtPointer cad)
{

   DENTER(GUI_LAYER, "qmonLIRSCancel");
   
   xmui_unmanage(qmon_lirs);

   DEXIT;
}

static void qmonLIRSSetText(Widget tw, lList *lirs_list, lList **alpp)
{
   char *text = NULL;

   DENTER(GUI_LAYER, "qmonLIRSSetText");

   if (lirs_list != NULL) {
      const char *filename = NULL;
      spooling_field *fields = sge_build_LIRS_field_list(false, true);
      
      filename = spool_flatfile_write_list(alpp, lirs_list, fields,
                                           &qconf_limit_rule_set_sfi, 
                                           SP_DEST_TMP, SP_FORM_ASCII,
                                           filename, false);
      text = qmonReadText(filename, alpp);
      unlink(filename);
      FREE(filename);
      FREE(fields);
      if (text != NULL) {
         XmTextSetString(tw, text);
         XtFree(text);
      } else {
         XmTextSetString(tw, "");
      }   

   } 
   DEXIT;
}


static void qmonLIRSGetText(Widget tw, lList *lirs_list, lList **alpp)
{
   char *text = NULL;
   lList *new_lirs_list = NULL;
   char filename[SGE_PATH_MAX] = "";
   spooling_field *fields = sge_build_LIRS_field_list(false, true);
   size_t len = 0;
   bool ret = false;
   bool ignore_unchanged_message = false;

   DENTER(GUI_LAYER, "qmonLIRSGetText");

   if (sge_tmpnam(filename) == NULL) {
      answer_list_add(alpp, MSG_POINTER_NULLPARAMETER,
                         STATUS_ERROR1, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   /*
   ** allocates a string
   */
   text = XmTextGetString(tw);
   len = strlen(text);
   ret = sge_string2file(text, len, filename);
   XtFree(text);

   new_lirs_list = spool_flatfile_read_list(alpp, LIRS_Type, fields,
                                            NULL, true, 
                                            &qconf_limit_rule_set_sfi,
                                            SP_FORM_ASCII, NULL, filename);
   if (answer_list_output(alpp)) {
      lFreeList(&new_lirs_list);
   }

   if (new_lirs_list != NULL) {
      if (ignore_unchanged_message || object_list_has_differences(new_lirs_list, alpp, lirs_list, false)) {
         ret = true;
      } else {
         lFreeList(&new_lirs_list);
         answer_list_add(alpp, MSG_FILE_NOTCHANGED,
                         STATUS_ERROR1, ANSWER_QUALITY_ERROR);
      }
   } else {
      answer_list_add(alpp, MSG_FILE_ERRORREADINGINFILE,
                      STATUS_ERROR1, ANSWER_QUALITY_ERROR);
   }

   unlink(filename);
   FREE(fields);

   if (ret) {
#ifdef TEST_GDI2   
      ret = limit_rule_set_add_del_mod_via_gdi(ctx, new_lirs_list, alpp, SGE_GDI_REPLACE);
#else      
      ret = limit_rule_set_add_del_mod_via_gdi(NULL, new_lirs_list, alpp, SGE_GDI_REPLACE);
#endif
   }

   DEXIT;
}
