/* 
 * Motif Tools Library, Version 3.1
 * $Id: AssertClass.c,v 1.1 2001/07/18 11:06:01 root Exp $
 * 
 * Written by David Flanagan.
 * Copyright (c) 1992-2001 by David Flanagan.
 * All Rights Reserved.  See the file COPYRIGHT for details.
 * This is open source software.  See the file LICENSE for details.
 * There is no warranty for this software.  See NO_WARRANTY for details.
 *
 * $Log: AssertClass.c,v $
 * Revision 1.1  2001/07/18 11:06:01  root
 * Initial revision
 *
 * Revision 1.1  2001/06/12 15:00:21  andre
 * AA-2001-06-12-0: replaced Xmt212 by Xmt310
 *                  (http://sourceforge.net/projects/motiftools) with
 *                  our xmt212 patches applied
 *
 *
 */

#include <Xmt/Xmt.h>
#include <X11/IntrinsicP.h>

/*
 * If NDEBUG is defined, then XmtAssertWidgetClass() will be replaced
 * with an empty macro definition, so it won't be called.  But we do
 * always want the function to appear in the library, even when the
 * library is compiled with NDEBUG.  So we've got to undefine
 * any redefinition of XmtAssertWidgetClass
 */
#ifdef XmtAssertWidgetClass
#undef XmtAssertWidgetClass
#endif

#if NeedFunctionPrototypes
void XmtAssertWidgetClass(Widget w, WidgetClass c, String procname)
#else
void XmtAssertWidgetClass(w, c, procname)
Widget w;
WidgetClass c;
String procname;
#endif
{
    if (!XtIsSubclass(w, c)) {
	XmtWarningMsg("XmtAssertWidgetClass", "typeMismatch",
		      "\n\tA widget of class %s was passed to procedure %s.\n\tA widget of class %s was expected.\n\tAborting.",
		      XtClass(w)->core_class.class_name,
		      procname,
		      c->core_class.class_name);
	(void) abort();
    }
}
