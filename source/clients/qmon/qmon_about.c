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

#include <Xmt/Xmt.h>
#include <Xmt/Dialogs.h>
#include <Xmt/Pixmap.h>

#include "qmon_about.h"
#include "qmon_appres.h"
#include "sgermon.h"
#include "sge_feature.h"
#include "sge_dstring.h"
#include "sge_prog.h"
#include "version.h"

static char header[] = "@fBWelcome %s@@%s,@fR\n\nYou are using @fB%s@fR in cell @fB'%s'@fR.\n%s%s";
extern char SFLN_ELN[];

#ifdef ADD_SUN_COPYRIGHT
static char mailto[] = "\nFor further information and feedback use: @fBn1ge6beta-support@@.sun.com@fR\n\n";
#else
static char mailto[] = "For further information and feedback please subscribe to the \"dev\" mailing list:\n\n   http://gridengine.sunsource.net/project/gridengine/maillist.html\n\n";
#endif

void qmonAboutMsg(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   dstring ds;
   char buffer[256];

   DENTER(TOP_LAYER, "qmonAboutMsg");
   
   sge_dstring_init(&ds, buffer, sizeof(buffer));
   XmtDisplayMessage(w, "about_msg", "Help", header, 
                     "About Qmon", NULL, None, XmDIALOG_MODELESS,
                     XmDIALOG_INFORMATION, 
                     uti_state_get_user_name(), uti_state_get_qualified_hostname(), 
                     feature_get_product_name(FS_LONG_VERSION, &ds), 
                     uti_state_get_default_cell(), 
                     XmtLocalize(w, mailto, "mailto_msg"), SFLN_ELN); 
/*                      XmtLocalize(w, "copyright_msg", SFLN_ELN, NULL)); */
   DEXIT;
}
