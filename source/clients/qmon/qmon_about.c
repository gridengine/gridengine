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

#include "sge_gdi_intern.h"
#include "qmon_about.h"
#include "qmon_appres.h"
#include "sgermon.h"
#include "sge_feature.h"
#include "utility.h"
#include "sge_string_append.h"
#include "sge_me.h"

static char about[] = 
   "@fBWelcome %s@@%s to %s qmon\n\n(c) 2000 SUN Microsystems, Inc. All rights reserved.\nUse is subject to license terms. Third party software, including font\ntechnology, is copyrighted and licensed from Sun suppliers. Portions may be\nderived from Berkeley BSD systems, licensed from U. of CA. Sun, Sun\nMicrosystems, the Sun Logo, Solaris and Sun TM %s are trademarks or\nregistered trademarks of Sun Microsystems, Inc. in the U.S. and other\ncountries. Federal Acquisitions: Commercial software--Government Users\nSubject to Standard License Terms and Conditions.\n";

  

void qmonAboutMsg(w, cld, cad)
Widget w;
XtPointer cld, cad;
{
   int sge_mode; 

   DENTER(TOP_LAYER, "qmonAboutMsg");

   sge_mode = feature_is_enabled(FEATURE_SGEEE);

   XmtDisplayMessage(w, "about_msg", "Help", about, 
                     "About Qmon", NULL, None, XmDIALOG_MODELESS,
                     XmDIALOG_INFORMATION, me.user_name, me.qualified_hostname, 
                     feature_get_product_name(FS_SHORT_VERSION), 
                     feature_get_product_name(FS_SHORT));

   DEXIT;
}
