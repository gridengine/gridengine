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
#include <fnmatch.h>

#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/ToggleB.h>

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
#include "qmon_ar.h"
#include "qmon_arcustom.h"
#include "qmon_util.h"
#include "qmon_request.h"
#include "qmon_comm.h"
#include "qmon_appres.h"
#include "qmon_widgets.h"
#include "qmon_matrix.h"
#include "sge.h"
#include "symbols.h"
#include "sge_sched.h"      
#include "commlib.h"
#include "sge_time.h"
#include "sge_all_listsL.h"
#include "IconList.h"
#include "sge_feature.h"
#include "sge_htable.h"
#include "sge_range.h"
#include "qmon_preferences.h"
#include "qmon_message.h"
#include "sge_range.h"
#include "sge_job.h"
#include "sge_host.h"
#include "sge_parse_num_par.h"
#include "sge_object.h"
#include "sge_ulong.h"
#include "sge_centry.h"
#include "sge_cqueue.h"
#include "sge_qinstance.h"
#include "sge_qinstance_state.h"
#include "qstat_printing.h"
#include "sge_cqueue_qstat.h"
#include "uti/sge_string.h"

/*-------------------------------------------------------------------------*/
/* Prototypes */
static void okCB(Widget w, XtPointer cld, XtPointer cad);
static void cancelCB(Widget w, XtPointer cld, XtPointer cad);
static void addToSelected(Widget w, XtPointer cld, XtPointer cad);
static void rmFromSelected(Widget w, XtPointer cld, XtPointer cad);

static String PrintUlong(lListElem *ep, int nm);
static String PrintDoubleOpti(lListElem *ep, int nm);
static String PrintString(lListElem *ep, int nm);
static String PrintTime(lListElem *ep, int nm);
static String PrintBool(lListElem *ep, int nm);
static String PrintGrantedQueue(lListElem *ep, int nm);
static String PrintState(lListElem *ep, int nm);
static String PrintType(lListElem *ep, int nm);
static String PrintNotImpl(lListElem *ep, int nm);
static String PrintPathList(lListElem *ep, int nm);
static String PrintMailList(lListElem *ep, int nm);
static String PrintMailOptions(lListElem *ep, int nm);
static String PrintPERange(lListElem *ep, int nm);
static void SetARLabels(Widget w);
static void qmonWhatSetItems(Widget list, int how);
static void qmonARFilterSet(Widget w, XtPointer cld, XtPointer cad);
static void qmonARFilterClear(Widget w, XtPointer cld, XtPointer cad);
static void qmonARFilterEditResource(Widget w, XtPointer cld, XtPointer cad);
static void qmonARFilterRemoveResource(Widget w, XtPointer cld, XtPointer cad);

static int is_owner_ok(lListElem *ar, lList *owner_list);
/*--------------------------------------------------------------------------------*/
/* selectable items, the names must match the names in sge_advance_reservationL.h */
/* there are fixed columns at the beginning of the natrix                         */
/*--------------------------------------------------------------------------------*/

static htable ARColumnPrintHashTable = NULL;
static htable NameMappingHashTable = NULL;

#define FIRST_FIELD     4

static tARField ar_items[] = {
   { 1, AR_id, "@{Id}", 12, 20, PrintUlong }, 
   { 1, AR_name, "@{Name}", 10, 50, PrintString },
   { 1, AR_owner, "@{Owner}", 10, 50, PrintString },
   { 1, AR_state, "@{Status}", 7, 30, PrintState },
   { 0, AR_type, "@{Type}", 7, 30, PrintType },
   { 0, AR_submission_time, "@{SubmitTime}", 19, 30, PrintTime },
   { 0, AR_start_time, "@{StartTime}", 19, 30, PrintTime },
   { 0, AR_end_time, "@{EndTime}", 19, 30, PrintTime },
   { 0, AR_duration, "@{Duration}", 19, 30, PrintTime },
   { 0, AR_account, "@{Account}", 15, 50, PrintString },
   { 0, AR_checkpoint_name, "@{Checkpoint Name}", 11, 30, PrintString },
   { 0, AR_resource_list, "@{ResourceList}", 11, 30, PrintNotImpl },
   { 0, AR_resource_utilization, "@{ResourceUtilization}", 15, 30, PrintNotImpl },
   { 0, AR_queue_list, "@{Queues}", 6, 30, PrintNotImpl },
   { 0, AR_granted_slots, "@{GrantedSlots}", 15, 50, PrintNotImpl },
   { 0, AR_mail_options, "@{MailOptions}", 11, 30, PrintMailOptions },
   { 0, AR_mail_list, "@{MailTo}", 15, 30, PrintMailList },
   { 0, AR_pe, "@{PE}", 10, 30, PrintString },
   { 0, AR_pe_range, "@{PERange}", 15, 30, PrintPERange },
   { 0, AR_acl_list, "@{ACL}", 12, 30, PrintNotImpl },
   { 0, AR_xacl_list, "@{XACL}", 12, 30, PrintNotImpl }
};

/*
** this list restricts the selected ars, queues displayed
*/
static lList *arfilter_resources = NULL;
static lList *arfilter_owners = NULL;
static lList *arfilter_fields = NULL;


static Widget arcu = 0;
static Widget arfield = 0;
static Widget arfilter_ar = 0;
static Widget arfilter_sr = 0;
static Widget arfilter_owner = 0;
static Widget arfield_available = 0;
static Widget arfield_selected = 0;

/*-------------------------------------------------------------------------*/
static void ar_get_type_string(char *buf, size_t buflen, u_long32 type)
{
   if (type != 0) {
      snprintf(buf, buflen-1, "-now");  
   } else {   
      strcat(buf, "");  
   }   
}   

/*-------------------------------------------------------------------------*/
static void ar_get_state_string(char *buf, size_t buflen, u_long32 state)
{
   switch(state) {
      case AR_WAITING:
         snprintf(buf, buflen-1, "AR_WAITING");  
         break;
      case AR_RUNNING:
         snprintf(buf, buflen-1, "AR_RUNNING");  
         break;
      case AR_EXITED:
         snprintf(buf, buflen-1, "AR_EXITED");  
         break;
      case AR_DELETED:
         snprintf(buf, buflen-1, "AR_DELETED");  
         break;
      case AR_ERROR:
         snprintf(buf, buflen-1, "AR_ERROR");  
         break;
      default:
         snprintf(buf, buflen-1, "unknown");  
         break;
   }
}   

/*-------------------------------------------------------------------------*/
static String PrintDoubleOpti(
lListElem *ep,
int nm 
) {
   char buf[BUFSIZ];
   String str;

   DENTER(GUI_LAYER, "PrintDoubleOpti");

#define OPTI_PRINT8(value) \
   if (value > 99999999 ) \
      sprintf(buf, "%8.3g ", value); \
   else  \
      sprintf(buf, "%8.0f ", value)

   OPTI_PRINT8(lGetDouble(ep, nm));
   str = XtNewString(buf);

#undef OPTI_PRINT8

   DEXIT;
   return str;
}

/*-------------------------------------------------------------------------*/
static String PrintBool(
lListElem *ep,
int nm 
) {
   String str;

   DENTER(GUI_LAYER, "PrintBool");

   if (lGetBool(ep, nm))
      str = XtNewString("True");
   else
      str = XtNewString("False");

   DEXIT;
   return str;
}

/*-------------------------------------------------------------------------*/
static String PrintPERange(
lListElem *ep,
int nm 
) {
   dstring range_string = DSTRING_INIT;
   String str;

   DENTER(GUI_LAYER, "PrintPERange");

   range_list_print_to_string(lGetList(ep, nm), &range_string, true, false, false);
   str = XtNewString(sge_dstring_get_string(&range_string));
   sge_dstring_free(&range_string);

   DEXIT;
   return str;
}

/*-------------------------------------------------------------------------*/
static String PrintPathList(
lListElem *ep,
int nm 
) {
   String str;
   lList *path_list = NULL;

   DENTER(GUI_LAYER, "PrintPathList");

   path_list = lGetList(ep, nm); 

   str = XtNewString(write_pair_list(path_list, PATH_TYPE));

   DEXIT;
   return str;
}

/*-------------------------------------------------------------------------*/
static String PrintMailList(
lListElem *ep,
int nm 
) {
   String str;
   lList *mail_list = NULL;

   DENTER(GUI_LAYER, "PrintMailList");

   mail_list = lGetList(ep, nm); 

   str = XtNewString(write_pair_list(mail_list, MAIL_TYPE));

   DEXIT;
   return str;
}

/*-------------------------------------------------------------------------*/
static String PrintMailOptions(
lListElem *ep,
int nm 
) {
   char buf[BUFSIZ];
   String str;
   u_long32 ca;
   static u_long32 mail_at[] = { MAIL_AT_BEGINNING, MAIL_AT_EXIT,
                                 MAIL_AT_ABORT, MAIL_AT_SUSPENSION};
   static char mailsym[] ="beas";
   int i;

   DENTER(GUI_LAYER, "PrintMailOptions");

   ca = lGetUlong(ep, nm);
   strcpy(buf, "");
   
   if (ca) {
      for (i=0; i<4; i++) {
         if (ca & mail_at[i])
            sprintf(buf, "%s%c", buf, mailsym[i]);
      }
   }
   else
      strcpy(buf, "None"); 
   
   str = XtNewString(buf);

   DEXIT;
   return str;
}
   
/*-------------------------------------------------------------------------*/
static String PrintUlong(
lListElem *ep,
int nm 
) {
   char buf[BUFSIZ];
   String str;

   DENTER(GUI_LAYER, "PrintUlong");

   sprintf(buf, sge_u32, lGetUlong(ep, nm));

   str = XtNewString(buf);

   DEXIT;
   return str;
}

/*-------------------------------------------------------------------------*/
static String PrintState(
lListElem *ep,
int nm 
) {
   char buf[128] = "";
   String str;

   DENTER(GUI_LAYER, "PrintState");

   /* write states into string */
   ar_get_state_string(buf, sizeof(buf), lGetUlong(ep, AR_state));

   str = XtNewString(buf);

   DEXIT;
   return str;
}
   
/*-------------------------------------------------------------------------*/
static String PrintType(
lListElem *ep,
int nm 
) {
   char buf[128] = "";
   String str;

   DENTER(GUI_LAYER, "PrintType");

   /* write states into string */
   ar_get_type_string(buf, sizeof(buf), lGetUlong(ep, AR_type));

   str = XtNewString(buf);

   DEXIT;
   return str;
}
   
/*-------------------------------------------------------------------------*/
static String PrintNotImpl(
lListElem *ep,
int nm 
) {
   String str;

   DENTER(GUI_LAYER, "PrintNotImpl");

   str = XtNewString("NotImpl");

   DEXIT;
   return str;
}
   
/*-------------------------------------------------------------------------*/
static String PrintString(
lListElem *ep,
int nm 
) {

   String str;
   StringConst temp;
   
   DENTER(GUI_LAYER, "PrintString");

   temp = lGetString(ep, nm); 
   
   str = XtNewString(temp?temp:"");

   DEXIT;
   return str;
}
   
/*-------------------------------------------------------------------------*/
static String PrintTime(
lListElem *ep,
int nm 
) {

   String str;
   dstring ds;
   char buffer[128];

   DENTER(GUI_LAYER, "PrintTime");

   sge_dstring_init(&ds, buffer, sizeof(buffer));

   if (lGetUlong(ep, nm))
      str = XtNewString(sge_ctime(lGetUlong(ep, nm), &ds));
   else
      str = XtNewString("");

   DEXIT;
   return str;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
String* PrintARField(
lListElem *ep,
int cols 
) {
   int i;
   String* row = NULL;
   int col;
   String entry;

   DENTER(GUI_LAYER, "PrintARField");

   /*
   ** check col against XtNumber(ar_items)
   ** if cols > XtNumber(ar_items) we screw the whole thing up
   */

   row = (String*)XtMalloc(sizeof(String)*cols);

   if (ep) {
      for (i=0, col=0; row && i<XtNumber(ar_items) && col<cols; i++) {
         if (ar_items[i].show) {
            if (ar_items[i].printARField &&
               (entry = ar_items[i].printARField(ep, ar_items[i].nm))) { 
               row[col] = entry;
            } else {
               row[col] = XtNewString("");
            }
            col++;
         }
      }
   } else {
      for (col=0; row && col<cols; col++) {
         row[col] = XtNewString("");
      }
   }

   DEXIT;
   return row;
}

/*-------------------------------------------------------------------------*/
static void okCB(Widget w, XtPointer cld, XtPointer cad)
{
   int nr_fields = 0;
   int i, j;
   XmString *strlist;
   Widget run_m;
   Widget pen_m;
   String owner_str = NULL;
   tARField *ar_item = NULL;
   
   DENTER(GUI_LAYER, "okCB");

   XtVaGetValues( arfield_selected, 
                  XmNitemCount, &nr_fields, 
                  XmNitems, &strlist,
                  NULL);

   /*
   ** reset show flag
   */ 
   for (j=FIRST_FIELD; j<XtNumber(ar_items); j++) {           
      ar_items[j].show = 0; 
   }



   for (i=0; i<nr_fields; i++) {
      if (strlist[i]) {
         String text;
         u_long32 temp;
         XmStringGetLtoR(strlist[i], XmFONTLIST_DEFAULT_TAG, &text);
         temp = XrmStringToQuark(text);
         if (sge_htable_lookup(NameMappingHashTable, 
                             &temp,
                             (const void **) &ar_item)) {
            ar_item->show = 1;
         }
         XtFree(text);
         DPRINTF(("ar_item[%d].name: <%s> show = %d\n",
                  i, ar_items[i].name, ar_items[i].show));
      }
   }        

   qmonARReturnMatrix(&run_m, &pen_m);
   SetARLabels(run_m);
   SetARLabels(pen_m);
   
   /*
   ** get the strings to do a wildmatch against
   */
   owner_str = XmtInputFieldGetString(arfilter_owner);
   lFreeList(&arfilter_owners);
   lString2List(owner_str, &arfilter_owners, ST_Type, ST_name, NULL); 

   updateARList();

   xmui_unmanage(arcu);

   DEXIT;
}
   
/*-------------------------------------------------------------------------*/
static void cancelCB(Widget w, XtPointer cld, XtPointer cad)
{
   DENTER(GUI_LAYER, "cancelCB");
   
   qmonWhatSetItems(arfield_selected, FILL_SELECTED);
   xmui_unmanage(arcu);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void saveCB(Widget w, XtPointer cld, XtPointer cad)
{
   DENTER(GUI_LAYER, "saveCB");
   
   okCB(w, NULL, NULL);

#if 0
   lSetList(qmonGetPreferences(), PREF_ar_filter_resources, 
                     lCopyList("", arfilter_resources));
   lSetList(qmonGetPreferences(), PREF_ar_filter_owners, 
                     lCopyList("", arfilter_owners));
   for (j=FIRST_FIELD; j<XtNumber(ar_items); j++) {           
      if (ar_items[j].show)
         lAddElemStr(&lp, ST_name, ar_items[j].name, ST_Type); 
   }
   lSetList(qmonGetPreferences(), PREF_ar_filter_fields, lp);

   alp = qmonWritePreferences();

   qmonMessageBox(w, alp, 0);

   lFreeList(&alp);
#else
   printf("Not yet implemented\n");
#endif

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void addToSelected(Widget w, XtPointer cld, XtPointer cad)
{
   XmString *items;
   Cardinal itemCount;
   int i;

   DENTER(GUI_LAYER, "addToSelected");

   XtVaGetValues( arfield_available,
                  XmNselectedItems, &items,
                  XmNselectedItemCount, &itemCount,
                  NULL);

   for (i=0; i<itemCount; i++) {
      if (! XmListItemExists(arfield_selected, items[i]))
         XmListAddItem(arfield_selected, items[i], 0); 
   }
                     
   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void rmFromSelected(Widget w, XtPointer cld, XtPointer cad)
{
   XmString *items;
   Cardinal itemCount;

   DENTER(GUI_LAYER, "rmFromSelected");

   XtVaGetValues( arfield_selected,
                  XmNselectedItems, &items,
                  XmNselectedItemCount, &itemCount,
                  NULL);
   
   if (itemCount)
      XmListDeleteItems(arfield_selected, items, itemCount);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void SetARLabels(
Widget w 
) {
   int cols;
   int col;
   int i;
   String *labels = NULL;
   short *widths = NULL;
   int *max_lengths = NULL;
   int additional = 0;
   Screen *screen = XtScreen(w);

   DENTER(GUI_LAYER, "SetARLabels");

   for (i=FIRST_FIELD; i<XtNumber(ar_items); i++) {
      if (ar_items[i].show)
         additional++;
   }

   XtVaGetValues( w,
                  XmNcolumns, &cols,
                  NULL);

   /*
   ** delete the additional columns
   */
   if (cols > FIRST_FIELD)
      XbaeMatrixDeleteColumns(w, FIRST_FIELD, cols - FIRST_FIELD);

   if (additional > 0) {
      labels = (String*)XtMalloc(sizeof(String)*additional);
      widths = (short*)XtMalloc(sizeof(short)*additional);
      max_lengths = (int*)XtMalloc(sizeof(int)*additional);
      for (i=FIRST_FIELD, col=0; i<XtNumber(ar_items) && col<additional; i++) {
         if (ar_items[i].show) {
            String str, s, category, tag, defaultstr, free_me = NULL;
            str = ar_items[i].name; 
            /* if this string has a tag, localize it first */
            if ((str[0] == '@') && (str[1] == '{')) {
               s = XtNewString(str);
               free_me = s;
               s += 2;
               category = NULL;
               tag = NULL;
               defaultstr = s;
               while(*s) {
                  if (*s == '.' && !category) {
                     *s = '\0';
                     category = defaultstr;
                     defaultstr = s+1;
                  }
                  if (*s == '.' && !tag) {
                     *s = '\0';
                     tag = defaultstr;
                     defaultstr = s+1;
                  }
                  if (*s == '}') {
                     *s = '\0';
                     s++;
                     break;
                  }
                  s++;
               }
               if (!tag)
                  tag = defaultstr;
/*                if (!tag[0]) goto error; */
/*                if (category && !category[0]) goto error; */
               s = _XmtLocalize(screen, defaultstr, category, tag);
/* printf("--> '%s'\n", s);                  */
            }
            else {
               s = (String)str;
            }
    
            labels[col] = XtNewString(s);
            widths[col] = ar_items[i].width;
            max_lengths[col] = ar_items[i].max_length;
            if (free_me) XtFree(free_me);
            col++;
         } 
      }
      XbaeMatrixAddColumns(w,
                           FIRST_FIELD,
                           NULL,
                           labels,
                           widths,
                           max_lengths,
                           NULL,
                           NULL,
                           NULL,
                           additional);
      for(i=0; i<additional; i++) {
         XtFree(labels[i]);
      }   
      XtFree((char*)labels);
      XtFree((char*)widths);
      XtFree((char*)max_lengths);
   }
                           

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonWhatSetItems(
Widget list,
int how 
) {
/*    XmString *items; */
/*    Cardinal itemCount = 0; */
/*    int i, j; */
   int num_ars, i;

   DENTER(GUI_LAYER, "qmonWhatSetItems");

   num_ars = XtNumber(ar_items);

#if 0
   if (how == FILL_ALL)
      itemCount = num_ars - FIRST_FIELD;
   else
      for (i=FIRST_FIELD; i<num_ars; i++) {
         if (ar_items[i].show)
            itemCount++;
      } 

   items = (XmString*)XtMalloc(sizeof(XmString)*itemCount);
   for (i=FIRST_FIELD,j=0; i<num_ars && j<itemCount; i++) {
      if (how == FILL_ALL) {
         items[j] = XmtCreateLocalizedXmString(list, ar_items[i].name); 
         j++;
      }
      else {
         if (ar_items[i].show) {
            items[j] = XmtCreateLocalizedXmString(list, ar_items[i].name);
            j++;
         }
      }
   }
         
   XtVaSetValues( list, 
                  XmNitems, items,
                  XmNitemCount, itemCount,
                  NULL);
   XmStringTableFree(items, itemCount);

#else   

   XtVaSetValues( list, 
                  XmNitems, NULL,
                  XmNitemCount, 0,
                  NULL);
   for (i=FIRST_FIELD; i<num_ars; i++) {
      if (how == FILL_ALL) {
         DPRINTF(("XmListAddItemUniqueSorted: '%s'\n", ar_items[i].name));
         XmListAddItemUniqueSorted(list, ar_items[i].name);
      }
      else {
         if (ar_items[i].show) {
            XmListAddItemUniqueSorted(list, ar_items[i].name);
         }
      }
   }
         
#endif

   DEXIT;
}


/*-------------------------------------------------------------------------*/
void qmonPopupARCU(Widget w, XtPointer cld, XtPointer cad)
{
   DENTER(TOP_LAYER, "qmonPopupARCU");

   if (!arcu)
      qmonCreateARCU(w, NULL);
   xmui_manage(arcu);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
void qmonCreateARCU(
Widget parent,
XtPointer cld 
) {
   Widget   arcu_ok, arcu_cancel, arcu_save, arcu_folder,
            arfield_add, arfield_remove, arfilter_clear;
   Widget run_m;
   Widget pen_m;
   int i;
 
   DENTER(GUI_LAYER, "qmonCreateARCU");

   /*
   ** the resource file for qc_dialog_shell must contain 
   ** qc_dialog as unmanaged XmtLayout child otherwise
   ** it is managed by XmtBuildQueryDialog and managed again
   ** in qmonPopupARCU
   */
   arcu = XmtBuildQueryDialog(parent, "arcu_shell",
                           NULL, 0,
                           "arcu_folder", &arcu_folder,
                           "arfield_selected", &arfield_selected,
                           "arfield_available", &arfield_available,
                           "arfield_add", &arfield_add,
                           "arfield_remove", &arfield_remove,
                           "arcu_ok", &arcu_ok,
                           "arcu_cancel", &arcu_cancel,
                           "arcu_save", &arcu_save,
                           "arfilter_ar", &arfilter_ar,
                           "arfilter_sr", &arfilter_sr,
                           "arfilter_owner", &arfilter_owner,
                           "arfilter_clear", &arfilter_clear,
                           NULL);
   /*
   ** create ARColumnPrintHashTable
   */
   ARColumnPrintHashTable = sge_htable_create(5, dup_func_u_long32, hash_func_u_long32, hash_compare_u_long32);
   for (i=0; i<sizeof(ar_items)/sizeof(tARField); i++) {
      u_long32 temp = XrmStringToQuark(ar_items[i].name);
      sge_htable_store(ARColumnPrintHashTable,
                     &temp,
                     (void *)&ar_items[i]);
   }

   /*
   ** create NameMappingHashTable
   */
   NameMappingHashTable = sge_htable_create(5, dup_func_u_long32, hash_func_u_long32, hash_compare_u_long32);
   for (i=0; i<sizeof(ar_items)/sizeof(tARField); i++) {
      String text;
      u_long32 temp;
      XmString xstr = XmtCreateLocalizedXmString(arcu, ar_items[i].name);
      XmStringGetLtoR(xstr, XmFONTLIST_DEFAULT_TAG, &text);
      temp = XrmStringToQuark(text);
      sge_htable_store(NameMappingHashTable,
                     &temp,
                     (void *)&ar_items[i]);
      XmStringFree(xstr);
      XtFree(text);
   }

#if 0
   /*
   ** reference to preferences
   */
   if (qmonGetPreferences()) {
      lListElem *field;
      lListElem *ep;
      tARField *ar_item = NULL;

      arfilter_resources = lCopyList("", lGetList(qmonGetPreferences(), 
                                          PREF_ar_filter_resources));
      arfilter_owners = lCopyList("", lGetList(qmonGetPreferences(), 
                                          PREF_ar_filter_owners));
      arfilter_fields = lCopyList("", lGetList(qmonGetPreferences(),
                                          PREF_ar_filter_fields));

/* lWriteListTo(arfilter_resources, stdout); */
/* lWriteListTo(arfilter_owners, stdout); */
/* lWriteListTo(arfilter_fields, stdout); */


      /*
      ** preset the owners
      */
      strcpy(buf, "");
      for_each(ep, arfilter_owners) {
         if (first_time) {
            first_time = 0;
            strcpy(buf, lGetString(ep, ST_name));
         }
         else
            sprintf(buf, "%s,%s", buf, lGetString(ep, ST_name));
      }
      XmtInputFieldSetString(arfilter_owner, buf);


      /*
      ** set the fields which shall be shown
      */
      for_each(field, arfilter_fields) {
         u_long32 temp = XrmStringToQuark(lGetString(field, ST_name));
         if (sge_htable_lookup(ARColumnPrintHashTable, 
             &temp,
             (const void **) &ar_item)) {
            ar_item->show = 1;
         }
      }
   }

#endif   

   /*
   ** preset the resources 
   */
   qmonARFilterSet(arcu, NULL, NULL);

   /*
   ** preset the fields
   */
   qmonWhatSetItems(arfield_available, FILL_ALL); 
   qmonWhatSetItems(arfield_selected, FILL_SELECTED); 

   /*
   ** preset the column labels
   */
   qmonARReturnMatrix(&run_m, &pen_m);
   SetARLabels(run_m);
   SetARLabels(pen_m);
   
                           
   XtAddCallback(arcu_ok, XmNactivateCallback,
                        okCB, NULL);
   XtAddCallback(arcu_cancel, XmNactivateCallback,
                        cancelCB, NULL);
   XtAddCallback(arcu_save, XmNactivateCallback,
                        saveCB, NULL);
   XtAddCallback(arfield_add, XmNactivateCallback,
                        addToSelected, (XtPointer) arfield);
   XtAddCallback(arfield_remove, XmNactivateCallback,
                        rmFromSelected, (XtPointer) arfield);

   /*
   ** arfilter callbacks
   */
   XtAddCallback(arfilter_ar, XmNactivateCallback,
                        qmonARFilterEditResource, (XtPointer)0);
   XtAddCallback(arfilter_sr, XmNactivateCallback,
                        qmonARFilterEditResource, (XtPointer)1);
   XtAddCallback(arfilter_sr, XmNremoveCallback, 
                     qmonARFilterRemoveResource, NULL);
   XtAddCallback(arfilter_clear, XmNactivateCallback,
                        qmonARFilterClear, NULL);
   XtAddCallback(arcu_folder, XmNvalueChangedCallback, 
                     qmonARFilterSet, NULL);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonARFilterClear(Widget w, XtPointer cld, XtPointer cad)
{


   DENTER(GUI_LAYER, "qmonARFilterClear");

   lFreeList(&arfilter_resources);
   qmonRequestDraw(arfilter_sr, arfilter_resources, 1);

   lFreeList(&arfilter_owners);
   XmtInputFieldSetString(arfilter_owner, "");
   
   DEXIT;

}

/*-------------------------------------------------------------------------*/
static void qmonARFilterSet(Widget w, XtPointer cld, XtPointer cad)
{
   lList *arl = NULL;
   lListElem *ep = NULL;
   lListElem *rp = NULL;

   DENTER(GUI_LAYER, "qmonARFilterSet");

   arl = qmonGetResources(qmonMirrorList(SGE_CENTRY_LIST), ALL_RESOURCES);

   for_each (ep, arfilter_resources) {
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
   qmonRequestDraw(arfilter_ar, arl, 0);
   qmonRequestDraw(arfilter_sr, arfilter_resources, 1);

   lFreeList(&arl);

   DEXIT;
}


/*-------------------------------------------------------------------------*/
static void qmonARFilterEditResource(Widget w, XtPointer cld, XtPointer cad)
{
   XmIconListCallbackStruct *cbs = (XmIconListCallbackStruct*) cad;
   long how = (long)cld;
   lList *arl = NULL;
   int type;
   char stringval[CL_MAXHOSTLEN];
   Boolean status = False;
   StringConst name, value, strval;
   Boolean found = False;
   lListElem *fill_in_request = NULL;
   lListElem *global_fill_in_request = NULL;
   

   DENTER(GUI_LAYER, "qmonARFilterEditResource");

   arl = qmonGetResources(qmonMirrorList(SGE_CENTRY_LIST), ALL_RESOURCES);


   if (!how) {
      global_fill_in_request = lGetElemStr(arl, CE_name, cbs->element->string[0]);
   }
   for_each (fill_in_request, arfilter_resources) {
      name = lGetString(fill_in_request, CE_name);
      value = lGetString(fill_in_request, CE_stringval);
      if (cbs->element->string[0] && name && 
         !strcmp(cbs->element->string[0], name)) {
            found = True;
            break;
      }
   }       
         
   if (!found) {
      fill_in_request = global_fill_in_request; 
   }

   if (!fill_in_request) {
      DEXIT;
      return;
   }

   type = lGetUlong(fill_in_request, CE_valtype);
   strval = lGetString(fill_in_request, CE_stringval);
   sge_strlcpy(stringval, strval, CL_MAXHOSTLEN);

   status = qmonRequestInput(w, type, cbs->element->string[0], 
                              stringval, sizeof(stringval));
   /* 
   ** put the value in the CE_Type elem 
   */
   if (status) {
      lListElem *ep = NULL;
      lSetString(fill_in_request, CE_stringval, stringval);
    
      /* put it in the hard or soft resource list if necessary */
      if (!arfilter_resources) {
         arfilter_resources = lCreateList("arfilter_resources", CE_Type);
      }
      if (!(ep = lGetElemStr(arfilter_resources, CE_name, cbs->element->string[0]))) {
         lAppendElem(arfilter_resources, lCopyElem(fill_in_request));
      } else {
         lSetString(ep, CE_stringval, stringval);
      }   
         
      qmonRequestDraw(arfilter_sr, arfilter_resources, 1);
   }
   lFreeList(&arl);

   DEXIT;
}

/*-------------------------------------------------------------------------*/
static void qmonARFilterRemoveResource(Widget w, XtPointer cld, XtPointer cad)
{

   XmIconListCallbackStruct *cbs = (XmIconListCallbackStruct*) cad;
   lListElem *dep = NULL;
   Boolean found = False;
   StringConst name, value;

   DENTER(GUI_LAYER, "qmonARFilterRemoveResource");

   if (arfilter_resources) {
      for_each (dep, arfilter_resources) {
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
         lRemoveElem(arfilter_resources, &dep);
         if (lGetNumberOfElem(arfilter_resources) == 0)
            lFreeList(&arfilter_resources);
         qmonRequestDraw(arfilter_sr, arfilter_resources, 1);
      }
   }
   
   DEXIT;
}

/*-------------------------------------------------------------------------*/
lList* qmonARFilterResources(void)
{
   return arfilter_resources;
}

/*-------------------------------------------------------------------------*/
lList* qmonARFilterOwners(void)
{
   return arfilter_owners;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/* this function destroys the delivered ar list partially               */
/*-------------------------------------------------------------------------*/
int match_ar(
lList **ar_list,
lList *owner_list,
lList *queue_list, 
lList *complex_list,
lList *exec_host_list,
lList *request_list 
) {
   lListElem *ar;
   lListElem *dep;
   int show;

   DENTER(GUI_LAYER, "match_ar");

   ar = lFirst(*ar_list);
   while (ar) {
      /*
      ** first of all we assume that the ar should be displayed
      */
      show = 1;
      
      /*
      ** check if ar fulfills user_list and set show flag
      */
      if (show && owner_list)
         show = is_owner_ok(ar, owner_list);


      if (show) {
         ar = lNext(ar);
      }
      else {
         dep = ar;
         ar = lNext(ar);
         lRemoveElem(*ar_list, &dep);
      }
   } 

   if (lGetNumberOfElem(*ar_list) == 0) {
      lFreeList(ar_list);
   }

   DEXIT;
   return True;
}
         
/*-------------------------------------------------------------------------*/
static int is_owner_ok(
lListElem *ar,
lList *owner_list 
) {
   lListElem *op;

   if (!owner_list)
      return True;

   for_each(op, owner_list) {
      if (!fnmatch(lGetString(op, ST_name), lGetString(ar, AR_owner), 0)) {
         return True;
      }
   }
   return False;
}

