#ifndef __MSG_SPOOLLIB_FLATFILE_H
#define __MSG_SPOOLLIB_FLATFILE_H
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

/* 
 * libs/spool/sge_spooling_flatfile.c
 */
#define MSG_FLATFILE_NODATATOSPOOL  _MESSAGE(62000, _("no data available for spooling\n"))
#define MSG_UNKNOWNATTRIBUTENAME_S _MESSAGE(62003, _("unknown attribute name "SFQ"\n"))
#define MSG_DONTKNOWHOWTOHANDLELIST_S  _MESSAGE(62006, _("don't know how to handle sublist "SFQ"\n"))
#define MSG_FLATFILE_ATTRIBISMISSING_S  _MESSAGE(62008, _("required attribute "SFQ" is missing\n"))
#define MSG_FLATFILE_DUPLICATEATTRIB_S  _MESSAGE(62009, _("attribute "SFQ" appears more than once\n"))
#define MSG_PARSINGOBJECTNOATTRIBUTE_D  _MESSAGE(62010, _("line %d should begin with an attribute name\n"))
#define MSG_PARSINGOBJECTNAMEVALUESEP_SD  _MESSAGE(62011, _(SFQ" is the only character allowed between the attribute name and the value in line %d\n"))
#define MSG_PARSINGOBJECTUNKNOWNTRAILER_DS  _MESSAGE(62012, _("unrecognized characters after the attribute values in line %d: "SFQ"\n"))
#define MSG_PARSINGLISTBADRECORDSTART_DS  _MESSAGE(62013, _("each value in the attribute value list in line %d should begin with "SFQ"\n"))
#define MSG_PARSINGLISTBADRECORDEND_DS  _MESSAGE(62014, _("each value in the attribute value list in line %d should end with "SFQ"\n"))
#define MSG_PARSINGLISTBADRECORDSEP_DS  _MESSAGE(62015, _("values in the attribute value list in line %d should be separated by "SFQ"\n"))

#endif /* __MSG_SPOOLLIB_FLATFILE_H */
