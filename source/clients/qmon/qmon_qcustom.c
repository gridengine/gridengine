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
#include <Xm/List.h>

#include <Xmt/Xmt.h>
#include <Xmt/Dialog.h>
#include <Xmt/Create.h>
#include <Xmt/Layout.h>
#include <Xmt/Converters.h>
#include <Xmt/Procedures.h>
#include <Xmt/WidgetType.h>
#include <Xmt/Include.h>
#include <Xmt/InputField.h>

#include "Matrix.h" 

#include "qmon_rmon.h"
#include "qmon_cull.h"
#include "qmon_qcustom.h"
#include "qmon_queue.h"
#include "qmon_util.h"
#include "qmon_request.h"
#include "qmon_comm.h"
#include "qmon_appres.h"
#include "qmon_widgets.h"
#include "sge_sched.h"      
#include "commlib.h"
#include "sge_gdi_intern.h"
#include "sge_all_listsL.h"
#include "IconList.h"
#include "qmon_preferences.h"
#include "qmon_message.h"

/*-------------------------------------------------------------------------*/
/* Prototypes */
static void okCB(Widget w, XtPointer cld, XtPointer cad);
static void cancelCB(Widget w, XtPointer cld, XtPointer cad);
static void saveCB(Widget w, XtPointer cld, XtPointer cad);
static void qmonQFilterSet(Widget w, XtPointer cld, XtPointer cad);
static void qmonQFilterClear(Widget w, XtPointer cld, XtPointer cad);
static void qmonQFilterEditResource(Widget w, XtPointer cld, XtPointer cad);
static void qmonQFilterRemoveResource(Widget w, XtPointer cld, XtPointer cad);

/*
** this list restricts the selected qs, queues displayed
*/
static lList *qfilter_resources = NULL;

static Widget qcu = 0;
static Widget qfilter_ar = 0;
static Widget qfilter_sr = 0;

/*-------------------------------------------------------------------------*/
void qmonPopupQCU(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   DENTER(TOP_LAYER, "qmonPopupQCU");

   if (!qcu)
      qmonCreateQCU(w, NULL);

   xmui_manage(qcu);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void okCB(w, cld, cad)
Widget w;
XtPointer cld,cad;
{
   DENTER(GUI_LAYER, "okCB");

   updateQueueList();
   xmui_unmanage(qcu);

   DEXIT;
}
   
/*-------------------------------------------------------------------------*/
static void cancelCB(w, cld, cad)
Widget w;
XtPointer cld,cad;
{
   DENTER(GUI_LAYER, "cancelCB");
   
   xmui_unmanage(qcu);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void saveCB(w, cld, cad)
Widget w;
XtPointer cld,cad;
{
   lList *alp;

   DENTER(GUI_LAYER, "saveCB");
   
   okCB(w, NULL, NULL);

   if (!qmon_preferences)
      qmon_preferences = lCreateElem(PREF_Type);

   lSetList(qmon_preferences, PREF_queue_filter_resources, 
                     lCopyList("", qfilter_resources));

   alp = qmonWritePreferences();

   qmonMessageBox(w, alp, 0);

   alp = lFreeList(alp);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
void qmonCreateQCU(
Widget parent,
XtPointer cld 
) {

   Widget qcu_ok, qcu_cancel, qcu_folder, qfilter_clear,
          qcu_save;
   
   DENTER(GUI_LAYER, "qmonCreateQCU");

   /*
   ** the resource file for qc_dialog_shell must contain 
   ** qc_dialog as unmanaged XmtLayout child otherwise
   ** it is managed by XmtBuildQueryDialog and managed again
   ** in qmonPopupQCU
   */
   qcu = XmtBuildQueryDialog( parent, "qcu_shell",
                                 NULL, 0,
                                 "qcu_ok", &qcu_ok,
                                 "qcu_cancel", &qcu_cancel,
                                 "qcu_save", &qcu_save,
                                 "qcu_folder", &qcu_folder,
                                 "qfilter_ar", &qfilter_ar,
                                 "qfilter_sr", &qfilter_sr,
                                 "qfilter_clear", &qfilter_clear,
                                 NULL); 

   if (qmon_preferences) {
      qfilter_resources = lCopyList("", lGetList(qmon_preferences, 
                                          PREF_queue_filter_resources));
   }
      
   /*
   ** preset qfilter resources
   */
   qmonQFilterSet(qcu, NULL, NULL);

   XtAddCallback(qfilter_ar, XmNactivateCallback,
                        qmonQFilterEditResource, (XtPointer)0);
   XtAddCallback(qfilter_sr, XmNactivateCallback,
                        qmonQFilterEditResource, (XtPointer)1);
   XtAddCallback(qfilter_sr, XmNremoveCallback, 
                     qmonQFilterRemoveResource, NULL);
   XtAddCallback(qcu_ok, XmNactivateCallback,
                        okCB, NULL);
   XtAddCallback(qcu_cancel, XmNactivateCallback,
                        cancelCB, NULL);
   XtAddCallback(qcu_save, XmNactivateCallback,
                        saveCB, NULL);
   XtAddCallback(qfilter_clear, XmNactivateCallback,
                        qmonQFilterClear, NULL);
   XtAddCallback(qcu_folder, XmNvalueChangedCallback, 
                     qmonQFilterSet, NULL);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonQFilterClear(w, cld, cad)
Widget w;
XtPointer cld, cad;
{


   DENTER(GUI_LAYER, "qmonQFilterClear");

   qfilter_resources = lFreeList(qfilter_resources);
   qmonRequestDraw(qfilter_sr, qfilter_resources, 1);
   
   DEXIT;

}

/*-------------------------------------------------------------------------*/
static void qmonQFilterSet(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   lList *arl = NULL;
   lListElem *ep = NULL;
   lListElem *rp = NULL;


   DENTER(GUI_LAYER, "qmonQFilterSet");

   arl = qmonGetResources(qmonMirrorList(SGE_COMPLEX_LIST), ALL_RESOURCES);

   for_each (ep, qfilter_resources) {
      rp = lGetElemStr(arl, CE_name, lGetString(ep, CE_name));
      if (!rp)
         rp = lGetElemStr(arl, CE_shortcut, lGetString(ep, CE_name));
      if (rp) {
         lSetString(ep, CE_name, lGetString(rp, CE_name));
         lSetUlong(ep, CE_valtype, lGetUlong(rp, CE_valtype));
      }
   }
   /*
   ** do the drawing
   */
   qmonRequestDraw(qfilter_ar, arl, 0);
   qmonRequestDraw(qfilter_sr, qfilter_resources, 1);

   arl = lFreeList(arl);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonQFilterEditResource(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   XmIconListCallbackStruct *cbs = (XmIconListCallbackStruct*) cad;
   long how = (long)cld;
   lList *arl = NULL;
   int type;
   char stringval[MAXHOSTLEN];
   Boolean status = False;
   StringConst name, value, strval;
   Boolean found = False;
   lListElem *fill_in_request = NULL;
   

   DENTER(GUI_LAYER, "qmonQFilterEditResource");

   arl = qmonGetResources(qmonMirrorList(SGE_COMPLEX_LIST), ALL_RESOURCES);

   if (!how)
      fill_in_request = lGetElemStr(arl, CE_name, cbs->element->string[0]);
   else {
      for_each (fill_in_request, qfilter_resources) {
         name = lGetString(fill_in_request, CE_name);
         value = lGetString(fill_in_request, CE_stringval);
         if (cbs->element->string[0] && name && 
            !strcmp(cbs->element->string[0], name) &&
            cbs->element->string[2] && value &&
            !strcmp(cbs->element->string[2], value) ) {
               found = True;
               break;
         }
      }       
            
      if (!found) {
         fill_in_request = NULL; 
      }
   }

   if (!fill_in_request) {
      DEXIT;
      return;
   }

   type = lGetUlong(fill_in_request, CE_valtype);
   strval = lGetString(fill_in_request, CE_stringval);
   if (strval)
      strncpy(stringval, strval, MAXHOSTLEN-1);
   else
      strcpy(stringval, "");

   status = qmonRequestInput(w, type, cbs->element->string[0], 
                              stringval, sizeof(stringval));
   /* 
   ** put the value in the CE_Type elem 
   */
   if (status) {
      lSetString(fill_in_request, CE_stringval, stringval);
    
      /* put it in the request list if necessary */
      if (!how) {
         if (!qfilter_resources) {
            qfilter_resources = lCreateList("qfilter_resources", CE_Type);
         }
         if (!lGetElemStr(qfilter_resources, CE_name, cbs->element->string[0]))
            lAppendElem(qfilter_resources, lCopyElem(fill_in_request));
      }
      qmonRequestDraw(qfilter_sr, qfilter_resources, 1);
   }

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonQFilterRemoveResource(w, cld, cad)
Widget w;
XtPointer cld, cad;
{

   XmIconListCallbackStruct *cbs = (XmIconListCallbackStruct*) cad;
   lListElem *dep = NULL;
   Boolean found = False;
   StringConst name, value;

   DENTER(GUI_LAYER, "qmonQFilterRemoveResource");

   if (qfilter_resources) {
      for_each (dep, qfilter_resources) {
         name = lGetString(dep, CE_name);
         value = lGetString(dep, CE_stringval);
         if (cbs->element->string[0] && name && 
            !strcmp(cbs->element->string[0], name) &&
            cbs->element->string[2] && value &&
            !strcmp(cbs->element->string[2], value) ) {
               found = True;
               break;
         }
      }       
            
      if (found) {
         lRemoveElem(qfilter_resources, dep);
         if (lGetNumberOfElem(qfilter_resources) == 0)
            qfilter_resources = lFreeList(qfilter_resources);
         qmonRequestDraw(qfilter_sr, qfilter_resources, 1);
      }
   }
   
   DEXIT;
}

/*-------------------------------------------------------------------------*/
lList* qmonQFilterRequest(void)
{
   return qfilter_resources;
}

