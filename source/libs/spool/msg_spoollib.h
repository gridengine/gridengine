#ifndef __MSG_SPOOLLIB_H
#define __MSG_SPOOLLIB_H
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

#include "basis_types.h"


/* 
 * libs/spool/sge_spooling.c
 */
#define MSG_SPOOL_UNKNOWNOBJECTTYPEINCONTEXT_SS     _MESSAGE(59000, _("object type "SFQ" is not handled in spooling context "SFQ"\n")) 
#define MSG_SPOOL_NODEFAULTRULEFORTYPEINCONTEXT_SS  _MESSAGE(59001, _("no default rule for object type "SFQ" in spooling context "SFQ"\n"))
#define MSG_SPOOL_NORULESFORTYPEINCONTEXT_SS        _MESSAGE(59002, _("no rules for object type "SFQ" in spooling context "SFQ"\n"))
#define MSG_SPOOL_CORRUPTRULEINCONTEXT_SSS          _MESSAGE(59003, _("corrupt rule "SFQ" in spooling context "SFQ": "SFQ" is missing\n"))
#define MSG_SPOOL_CANNOTSPOOLFREEOBJECT             _MESSAGE(59004, _("cannot spool free object\n"))
#define MSG_SPOOL_RULEINCONTEXTFAILEDWRITING_SS     _MESSAGE(59005, _("rule "SFQ" in spooling context "SFQ" failed writing an object\n"))
/* #define MSG_SPOOL_INVALIDSPOOLEDOBJECTTYPE_D        _message(59006, _("invalid spooled object type %d\n")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_SPOOL_NODESCRIPTORFOROBJECTTYPEALL      _message(59007, _("no descriptor available for object type ALL\n")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_SPOOL_NOKEYFOROBJECTTYPEALL             _message(59008, _("no key field available for object type ALL\n")) __TS Removed automatically from testsuite!! TS__*/
#define MSG_SPOOL_NOVALIDCONTEXT_S                  _MESSAGE(59009, _("no valid spooling context passed to "SFQ"\n"))
#define MSG_SPOOL_CONTEXTNEEDSNAME                  _MESSAGE(59010, _("spooling context has to have a name\n"))
#define MSG_SPOOL_NOVALIDSPOOLTYPE_S                _MESSAGE(59011, _("no valid spool type passed to "SFQ"\n"))
#define MSG_SPOOL_NOVALIDRULE_S                     _MESSAGE(59012, _("no valid spooling rule passed to "SFQ"\n"))
#define MSG_SPOOL_CONTEXTCONTAINSNOTYPES_S          _MESSAGE(59013, _("spooling context "SFQ" does not contain any object type descriptions\n"))
#define MSG_SPOOL_CONTEXTCONTAINSNORULES_S          _MESSAGE(59014, _("spooling context "SFQ" does not contain any spooling\n"))
#define MSG_SPOOL_TYPECONTAINSNORULES_SS            _MESSAGE(59015, _("type definition "SFQ" in spooling context "SFQ" contains no references to rules\n"))
#define MSG_SPOOL_TYPEHASNODEFAULTRULE_SS           _MESSAGE(59016, _("type definition "SFQ" in spooling context "SFQ" has no default rule\n"))
#define MSG_SPOOL_TYPEHASMORETHANONEDEFAULTRULE_SS  _MESSAGE(59017, _("type definition "SFQ" in spooling context "SFQ" has more than one default rule\n"))
#define MSG_SPOOL_RULEALREADYEXISTS_SS              _MESSAGE(59018, _("a rule named "SFQ" already exists in spooling context "SFQ"\n"))
#define MSG_SPOOL_TYPEALREADYHASDEFAULTRULE_S       _MESSAGE(59019, _("spooling type "SFQ" already has a default rule, cannot add a second one\n"))

/* 
 * libs/spool/sge_spooling_utilities.c
 */
#define MSG_DONTKNOWHOWTOSPOOLSUBLIST_SS _MESSAGE(59100, _("don't know how to spool list attribute "SFQ" in function "SFQ"\n"))
#define MSG_UNKNOWNOBJECTTYPEFOR_SS _MESSAGE(59101, _("unknown object type for list attribute "SFQ" in function "SFQ"\n"))
#define MSG_FIELDDESCRIPTIONDOESNOTCONTAINNAME_S   _MESSAGE(59102, _("spooling field description for field "SFQ" does not contain name\n"))
#define MSG_NONAMEFORATTRIBUTE_D _MESSAGE(59103, _("no or invalid name for attribute %d\n"))
#endif /* __MSG_SPOOLLIB_H */
