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
#define MSG_PARSINGOBJECTEXPECTEDBUTGOT_SSD _MESSAGE(62001, _("reading object: expected "SFQ", but got "SFQ" in line %d\n"))
#define MSG_UNKNOWNATTRIBUTENAME_S _MESSAGE(62003, _("unknown attribute name "SFQ"\n"))
#define MSG_ATTRIBUTENOTINOBJECT_S _MESSAGE(62005, _("attribute "SFQ" is not part of current object\n"))
#define MSG_DONTKNOWHOWTOHANDLELIST_S  _MESSAGE(62006, _("don't know how to handle sublist "SFQ"\n"))
#endif /* __MSG_SPOOLLIB_FLATFILE_H */
