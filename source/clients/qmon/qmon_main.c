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
#include <unistd.h>

#include <Xm/Xm.h>

#include <Xmt/Xmt.h>
#include <Xmt/Help.h>
#include <Xmt/Icon.h>
#include <Xmt/AppRes.h>
#include <Xmt/Include.h>

/*
** These include file contains the description of the dialogs and is 
** automatically generated. The resources get into the resource database
** after reading the app-defaults so they can't be overwritten.
** This is done with XrmPutLineResource. 
*/

#include "qmon_diares.h"
#define APP_NAME  "Qmon"

#include "qmon_rmon.h"
#include "qmon_signal.h"
#include "qmon_timer.h"
#include "qmon_start.h"
#include "qmon_init.h"
#include "qmon_comm.h"
#include "qmon_menus.h"
#include "qmon_appres.h"
#include "qmon_preferences.h"
#include "sge_feature.h"
#include "sge_prog.h"

#ifdef REPLAY_XT
#include "ReplayXt.h"
/************************************
 ************************************
 * start of replay resource data defs
 ************************************
 ************************************/
typedef struct {
  String replay_rules;
  String replay_data;
  Boolean verbose;
  Boolean record;
  String recordFile;
  Boolean safe;
} ReplayType, *ReplayTypePtr;

static ReplayType replay_resource_values;

static XrmOptionDescRec replay_options[] = {
  {"-rulesFile", "rulesFile", XrmoptionSepArg, "replay.rules"},
  {"-dataFile", "dataFile", XrmoptionSepArg, "replay.data"},
  {"-verbose", "Verbose", XrmoptionNoArg, (XtPointer) "on"},
  {"-record", "Record", XrmoptionNoArg, (XtPointer) "on"},
  {"-recordFile", "RecordFile", XrmoptionSepArg, "replay.data"},
  {"-safe", "Safe", XrmoptionNoArg, (XtPointer) "on"},
};

XtResource replay_resources[] =
{
  {"rulesFile",
   "RulesFile",
   XmRString,
   sizeof(String),
   XtOffset(ReplayTypePtr, replay_rules),
   XtRImmediate,
   (XtPointer) "replay.rules"
  },
  {"dataFile",
   "DataFile",
   XmRString,
   sizeof(String),
   XtOffset(ReplayTypePtr, replay_data),
   XtRImmediate,
   (XtPointer) "replay.data"
  },
  {"verbose",
   "Verbose",
   XmRBoolean,
   sizeof(Boolean),
   XtOffset(ReplayTypePtr, verbose),
   XtRImmediate,
   (XtPointer) FALSE
  },
  {"safe",
   "Safe",
   XmRBoolean,
   sizeof(Boolean),
   XtOffset(ReplayTypePtr, safe),
   XtRImmediate,
   (XtPointer) FALSE
  },
  {"record",
   "Record",
   XmRBoolean,
   sizeof(Boolean),
   XtOffset(ReplayTypePtr, record),
   XtRImmediate,
   (XtPointer) False
  },
  {"recordFile",
   "RecordFile",
   XmRString,
   sizeof(String),
   XtOffset(ReplayTypePtr, recordFile),
   XtRImmediate,
   (XtPointer) "replay.data"
  },
};
/**********************************
 **********************************
 * end of replay resource data defs
 **********************************
 **********************************/

#endif



#ifdef HAS_EDITRES
   extern void _XEditResCheckMessages();
#endif

int main(int argc, char **argv);
static void qmonUsage(Widget w);

static Widget  MainControl;
/*-------------------------------------------------------------------------*/
/* global variables                                                        */
/*-------------------------------------------------------------------------*/
const char           *SGE_ROOT;
XtAppContext   AppContext;
Widget         AppShell; 
GC             fg_gc, bg_gc, qb_gc, alarm_gc, suspend_gc,
               running_gc, error_gc, disable_gc, caldisable_gc, calsuspend_gc;
Pixel          WarningPixel, QueueSelectedPixel, JobSuspPixel, JobSosPixel,
               JobDelPixel, JobHoldPixel, JobErrPixel, TooltipForeground,
               TooltipBackground;
int            nologo;
int            helpset;


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
int main(
int argc,
char **argv 
) {
   Widget StartupWindow = 0;
   Arg  args[10];
   Cardinal ac = 0;
   char *lang;
   static char progname[256];
/*    static char app_name[1024]; */

   int i;
   XrmDatabase qmon_database;

   DENTER_MAIN(TOP_LAYER, "qmon_main");

   /* INSTALL SIGNAL HANDLER */
   qmonInstSignalHandler();

   strcpy(progname, argv[0]);
   
   /* GENERAL SGE SETUP */
   if (!(argc > 1 && !strcmp(argv[1], "-help")))
      qmonInitSge(progname);

   SGE_ROOT = sge_get_root_dir(0, NULL, 0);

   /*
   ** Attention !!! Change the XtMalloc() above if you add additional args
   */
   ac = 0;
   XtSetArg(args[ac], XmtNconfigDir, SGE_ROOT); ac++;
   XtSetArg(args[ac], XmtNconfigPath, "%R/locale/%L/%N%S:%R/locale/%l/%N%S:%R/locale/%l_%t.%c/%N%S:%R/qmon/%N%S"); ac++;
/*    XtSetArg(args[ac], XmtNpixmapFilePath, "%R/qmon/PIXMAPS/%N.xpm"); ac++; */
/*    XtSetArg(args[ac], XmtNcontextHelpFile, "qmon_help"); ac++; */
   XtSetArg(args[ac], XtNtitle, "QMON +++ Main Control"); ac++;
   
   /* 
   ** SETUP XMT, here qmon_version is checked, 
   ** so here an exit is possible 
   */
#ifdef REPLAY_XT
   AppShell = XmtInitialize( &AppContext, APP_NAME,
                             replay_options, XtNumber(replay_options),
                             &argc, argv, 
                             qmon_fallbacks,
                             args, ac);
  /******************************
   ******************************
   * Get the replay app resources
   ******************************
   ******************************/
   XtGetApplicationResources(AppShell, &replay_resource_values,
      replay_resources, XtNumber(replay_resources),
      NULL, 0);


   /****************************
    ****************************
    * Register the replay system
    ****************************
    ****************************/
   if (replay_resource_values.record)
      RXt_StartRecorder(XtWidgetToApplicationContext(AppShell),
                           replay_resource_values.recordFile);
   else
      RXt_RegisterPlayer(AppShell,
                     replay_resource_values.replay_rules,
                     replay_resource_values.replay_data,
                     replay_resource_values.verbose,
                     replay_resource_values.safe);
#else
   AppShell = XmtInitialize( &AppContext, APP_NAME,
                             NULL, 0,
                             &argc, argv, 
                             qmon_fallbacks,
                             args, ac);
#endif
   
#if 0
   /*
   ** protocoll the actions performed by qmon
   */
   XtAppAddActionHook(AppContext, TraceActions, NULL);
#endif

   /*
   ** Internationalization:
   ** The qmon_messages.ad file is installed under 
   ** $GRD_ROOT/qmon/locale/<LANG>/qmon_messages.ad
   ** Read in the _Messages_ catalogue
   */
   if (((lang = getenv("LC_MESSAGES")) || (lang = getenv("LC_ALL")) ||
         (lang = getenv("LANG"))) && lang && strcasecmp(lang, "POSIX") &&
         strcasecmp(lang, "C")) {
      DPRINTF(("lang: '%s'\n", lang));
      if (!strcasecmp(lang, "relabel"))   
         lang = "C";
      XmtLoadResourceFile(AppShell, "qmon_messages", False, True);
   }   
#if 0   
   strcpy(app_name, "QMON +++ Main Control");
   if (strcmp(uti_state_get_default_cell(), "default")) {
      strcat(app_name, " @ ");
      strncat(app_name, uti_state_get_default_cell(), 1000);
   }

   XtVaSetValues(AppShell, 
              XtNtitle, XmtLocalize(AppShell, app_name,
                                    "QMON +++ Main Control"), NULL);
#endif   
   XtVaSetValues(AppShell, 
              XtNtitle, XmtLocalize(AppShell, "QMON +++ Main Control",
                                    "QMON +++ Main Control"), NULL);
   
   /*
   ** we must shift the usage here for internationalization
   */
   if (helpset) {
      qmonUsage(AppShell);
      qmonExitFunc(0);
   }
   
   /* 
   ** get the dialog resource files, they override any settings from the
   ** Qmon app default file concerning dialogue descriptions
   */
   qmon_database = XtDatabase(XtDisplay(AppShell));
   for (i=0; qmon_dialogs[i]; i++) {
      XrmPutLineResource(&qmon_database, qmon_dialogs[i]);
   }
#if 0
   /*
   ** Debugging:
   ** write contents of Resource DB to file DB.TXT in cwd
   */
   XrmPutFileDatabase(qmon_database, "DB.TXT"); */
#endif   

   /* 
   ** read qmon preferences file ~/.qmon_preferences, it contains
   ** customization info for Queue and Job Control dialogues
   */
   qmonReadPreferences();
   
   /*
   ** display of startup screen ?
   */
   if (!nologo) {
      /* show the user we're starting up */
      StartupWindow = qmonStartupWindow(AppShell);
   }
   
   /* 
   ** INITIALIZE Graphics Contexts 
   */
   qmonCreateGC(AppShell);

   /* 
   ** Allocate Pixel values 
   */
   qmonAllocColor(AppShell);

   /* 
   ** Cache all Icons 
   */
   qmonLoadIcons();

   /* 
   ** set the close button callback 
   ** cause the close button to call the qmonExitCB() 
   ** set the icon and iconName after qmonLoadIcons()
   */
   XmtCreatePixmapIcon(AppShell, qmonGetIcon("mcicon"), None);
   XtVaSetValues(AppShell, XtNiconName, "qmon:Main Control", NULL);
   XmtAddDeleteCallback(AppShell, XmDO_NOTHING, qmonExitCB, NULL);

   /* 
   ** CREATE MainControl 
   */
   MainControl = qmonCreateMainControl(AppShell);

   /*
   ** create qcontrol it takes a long time
   */

#ifdef REPLAY_XT
   /****************************
    ****************************
    * Add another top widget, name is  menu_bar
    ****************************
    ****************************/
   RXt_RegisterTopWidget(MainControl);


#endif

   /* 
   ** install context help 
   */
   XmtHelpInstallContextHelp(AppShell, XmtHelpContextHelpCallback, NULL);
/*    XmtHelpParseFile(AppShell, "qmon_help"); */


   /* 
   ** initialize QmonMirrorList entries 
   */
   qmonMirrorListInit();
   
   /* 
   ** setup timers 
   */
   qmonStartPolling(AppContext);
   
#ifdef HAS_EDITRES
    /* 
    ** Plug in editres protocol handler 
    */
    XtAddEventHandler (AppShell, (EventMask)0, True,
        _XEditResCheckMessages, (XtPointer)NULL);
#endif


   /* 
   ** Popdown startup screen and destroy it
   */
   if (!nologo) {
      sleep(1);
      XtDestroyWidget(StartupWindow);
   }   


   XtRealizeWidget(AppShell);
   XtAppMainLoop(AppContext);

   return 0;
}

/*-------------------------------------------------------------------------*/
void qmonMainControlRaise(Widget w, XtPointer cld, XtPointer cad)
{
   DENTER(GUI_LAYER, "qmonMainControlRaise");

   XmtRaiseShell(MainControl);
 
   DEXIT;
}


#if 0

static void TraceActions(Widget w, XtPointer cld, String action_name, XEvent *event, String *params, Cardinal *num_params);

/*-------------------------------------------------------------------------*/
static void TraceActions(
Widget w,
XtPointer cld,
String action_name,
XEvent *event,
String *params,
Cardinal *num_params 
) {
   DENTER(GUI_LAYER, "TraceActions");

   fprintf(stderr, "Widget: %20.20s Action: %s\n", XtName(w), action_name);
 
   DEXIT;
}

#endif

/*-------------------------------------------------------------------------*/
static void qmonUsage(Widget w)
{
   DENTER(GUI_LAYER, "qmonUsage");

   printf("%s\n", feature_get_product_name(FS_SHORT_VERSION));
   printf(XmtLocalize2(w, "usage: qmon\n", "qmon_usage", "usageTitle"));
   printf("	[-cmap]                           ");
   printf(XmtLocalize2(w, "use own colormap\n", "qmon_usage", "cmapOption"));
   printf("	[-help]                           ");
   printf(XmtLocalize2(w, "show this information and exit\n", 
                           "qmon_usage", "helpOption"));
   printf("	[-fontFamily {big|medium|small}]  ");
   printf(XmtLocalize2(w, "use small/medium/big fonts\n", 
                           "qmon_usage", "fontFamilyOption"));
   printf("	[-nologo]                         ");
   printf(XmtLocalize2(w, "startup without logo\n",
                           "qmon_usage", "nologoOption"));
   printf(XmtLocalize2(w, "Additionally the default X commandline switches can be used.\nFor further information see the manual page X(1)\n", 
          "qmon_usage", "X11OptionInfo"));

   DEXIT;
}
