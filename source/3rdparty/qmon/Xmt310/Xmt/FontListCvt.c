/* 
 * Motif Tools Library, Version 3.1
 * $Id: FontListCvt.c,v 1.1 2001/07/18 11:06:02 root Exp $
 * 
 * Written by David Flanagan.
 * Copyright (c) 1992-2001 by David Flanagan.
 * All Rights Reserved.  See the file COPYRIGHT for details.
 * This is open source software.  See the file LICENSE for details.
 * There is no warranty for this software.  See NO_WARRANTY for details.
 *
 * $Log: FontListCvt.c,v $
 * Revision 1.1  2001/07/18 11:06:02  root
 * Initial revision
 *
 * Revision 1.2  2001/06/12 16:25:28  andre
 * *** empty log message ***
 *
 *
 */

#include <ctype.h>
#include <Xmt/Xmt.h>
#include <Xmt/ConvertersP.h>
#include <Xmt/LookupP.h>

/* ARGSUSED */
#if NeedFunctionPrototypes
Boolean XmtConvertStringToXmFontList(Display *dpy,
				     XrmValue *args, Cardinal *num_args,
				     XrmValue *from, XrmValue *to,
				     XtPointer *converter_data)
#else
Boolean XmtConvertStringToXmFontList(dpy, args, num_args,
				     from, to, converter_data)
Display *dpy;
XrmValue *args;
Cardinal *num_args;
XrmValue *from;
XrmValue *to;
XtPointer *converter_data;
#endif
{
    Screen *screen = *(Screen **)args[0].addr;
    String copy = XtNewString((String) from->addr);
    String s;
    String next_entry;
    String tag;
    String tmp;
#if 0    
    XFontStruct *font;
    XFontSet fontset;
    char **missing_list;
    int missing_count;
    int i;
    char *def_string = NULL; 
#endif    
    XmFontListEntry entry;
    XmFontList fontlist = NULL;

    /*
     * This XmFontList converter does not handle XFontSets defined in
     * X11R5 and Motif 1.2.  This is to make it easier to write, and
     * for portablity to R4 and 1.1.  Also, this converter uses the 1.1
     * XmFontList API.  Anyone using multi-charset Asian languages will
     * not want to use this converter.
     * This converter also does not support the (undocumented?) features
     * of the Motif converter that allows quotes in font names.
     * The main reason to use the converter is because it handles
     * font list indirection.  And because it doesn't have the bug
     * that 1.2.3 does...
     *
     * The syntax is:
     *    font-name [ `=' tag ] { `,' font-name [ `=' tag ] }
     * 
     * and we extend this also to allow:
     *    `$' symbolic-font-list-name
     * A symbolic name like this will be looked up under _Fonts_,
     * depending on display resolution, and other factors.
     */
    
    s = copy;

    /* handle a symbolic font list name by recursing */
    if (s[0] == '$') {
	String value;
	XrmValue new_from;

	/* get symbolic name */
	s++;
	while(isspace(*s)) s++;
	
	/*
	 * lookup value of that symbolic name under:
	 *   _FontLists_.fontFamily.language.territory.codeset
	 * Note that we don't have to free the return value.
	 */
	value = _XmtLookupResource(screen, "Ffltc", s);
	
	/* if no value defined, warn and fail */
	if (!value) {
	    XmtWarningMsg("XmtConvertStringToFontList", "nosymbol",
			  "No font list with name '%s' defined in resource file under _FontLists_",
			  s);
	    goto fail;
	}

	/*
	 * Otherwise, recurse to convert the definition we've found 
	 * The programmer must be smart enough to avoid infinite recursion
	 */
	XtFree(copy);
	new_from.addr = (XPointer) value;
	new_from.size = strlen(value) + 1;
	return XmtConvertStringToXmFontList(dpy, args, num_args,
					    &new_from, to, converter_data);
    }
    
    /* Otherwise, it is not a symbolic font name */
    while(s && *s) {
	/* skip white-space */
	while(isspace(*s)) s++;
	if (!*s) break;

	/* isolate the first entry, and remember start of next entry */
	next_entry = strchr(s, ',');
        if (next_entry == NULL)
            next_entry = strchr(s, ':');
	if (next_entry) {
	    *next_entry = '\0';
	    next_entry++;
	}

	/*
	 * find the start of the font tag, if any
	 * and isolate the font name from it.
	 * remove whitespace from beginning and end of tag
	 */
	tag = strchr(s, '=');
	if (tag) {
	    *tag = '\0';
	    tag++;
	    while(isspace(*tag)) tag++;
	    for(tmp=tag; *tmp && !isspace(*tmp); tmp++)  ;
	    *tmp = '\0';
	}
	else {
#if (XmVersion < 1002)
	    tag = XmSTRING_DEFAULT_CHARSET;
#else
	    tag = XmFONTLIST_DEFAULT_TAG;
#endif	    
	}

	/* remove any whitespace from the end of the font name */
	tmp = s + strlen(s) - 1;
	while (isspace(*tmp)) tmp--;
	*(++tmp) = '\0';

#if 0
        /*
        ** fontset support it is a bit tricky, but needed for multibyte chars
        ** FIXME: figure out how to do it
        */

        fontset = XCreateFontSet(dpy, s, &missing_list, &missing_count,
                                        &def_string);
        for (i=0; i<missing_count; i++) {
            printf("missing_list[%d]: '%s'\n", i, missing_list[i]);
            XtFree(missing_list[i]);
        }
        XtFree((char *)missing_list);

#if !defined(SOLARIS) && !defined(SOLARIS64)
        entry = XmFontListEntryLoad(dpy, s, XmFONT_IS_FONT, tag);
#else
        entry = XmFontListEntryLoad(dpy, s, XmFONT_IS_FONTSET, tag);
#endif
#endif
        
        /*
        ** FIXME: replace the following line with the correct version
        **        for multibyte chars and fontsets
        */
        entry = XmFontListEntryLoad(dpy, s, XmFONT_IS_FONT, tag);

        fontlist = XmFontListAppendEntry(fontlist, entry);
        XmFontListEntryFree(&entry);
        

#if 0
	font = XLoadQueryFont(dpy, s);
	if (!font) {
	    XmtWarningMsg("XmtConvertStringToFontList", "badfont",
			  "unknown font '%s'.\n\tUsing default.",
			  s);
	    /* this default is from the X11R5 font converter */
	    font = XLoadQueryFont(dpy, "-*-*-*-R-*-*-*-120-*-*-*-*-ISO8859-1");

	    /* if still no font, then we fail */
	    if (!font) {
		XmtWarningMsg("XmtConvertStringToFontList", "nodefault",
			      "cannot load any default font.");
		goto fail;
	    }
	}

	/* and use this font and tag to create or append to the font list */
	if (!fontlist)
	    fontlist = XmFontListCreate(font, tag);
	else 
	    fontlist = XmFontListAdd(fontlist, font, tag);

#endif

	/* finally, move on to the next entry, and start the loop over */
	s = next_entry;
    }


    if (fontlist) {
	XtFree(copy);
	done(XmFontList, fontlist);  /* this macro returns */
    }

 fail:
    XtDisplayStringConversionWarning(dpy, copy, XmRFontList);
    XtFree(copy);
    return False;
}


/* ARGSUSED */
#if NeedFunctionPrototypes
static void FreeConvertedXmFontList(XtAppContext app, XrmValue *to,
				    XtPointer closure,
				    XrmValue *args, Cardinal *num_args)
#else
static void FreeConvertedXmFontList(app, to, closure, args, num_args)
XtAppContext app;
XrmValue *to;
XtPointer closure;
XrmValue *args;
Cardinal *num_args;
#endif
{
    XmFontList fontlist = *((XmFontList *) to->addr);
    Screen *screen = *(Screen **)args[0].addr;
    Display *dpy = DisplayOfScreen(screen);
    XmFontContext context;
    XFontStruct *font;
    XmStringCharSet tag;
    
    if (XmFontListInitFontContext(&context, fontlist)) {
	while(XmFontListGetNextFont(context, &tag, &font))
	    XFreeFont(dpy, font);
	XmFontListFreeFontContext(context);
    }
    XmFontListFree(fontlist);
}

#if NeedFunctionPrototypes
void XmtRegisterXmFontListConverter(void)
#else
void XmtRegisterXmFontListConverter()
#endif
{
    static Boolean registered = False;

    if (!registered) {
	XtSetTypeConverter(XtRString, XmRFontList,
			   XmtConvertStringToXmFontList,
			   (XtConvertArgRec *)screenConvertArg, (Cardinal) 1,
			   XtCacheByDisplay,
			   FreeConvertedXmFontList);
	registered = True;
    }
}


