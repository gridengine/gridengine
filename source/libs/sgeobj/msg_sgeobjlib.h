#ifndef __MSG_SGEOBJLIB_H
#define __MSG_SGEOBJLIB_H
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

#define MSG_ANSWER_NOANSWERLIST     _MESSAGE(64000, _("no answer list - gdi request failed\n"))
#define MSG_CKPTREFINJOB_SU         _MESSAGE(64001, _("Checkpointing object "SFQ" is still referenced in job " U32CFormat".\n"))
#define MSG_SGETEXT_COMPLEXNOTUSERDEFINED_SSS      _MESSAGE(64002, _("denied: complex "SFQ" referenced in "SFN" "SFQ" is not a user complex\n"))
#define MSG_SGETEXT_UNKNOWNCOMPLEX_SSS             _MESSAGE(64003, _("denied: complex "SFQ" referenced in "SFN" "SFQ" does not exist\n"))
#define MSG_GDI_PRODUCTMODENOTSETFORFILE_S   _MESSAGE(64004, _("can't read "SFQ" - product mode not set."))
#define MSG_GDI_INVALIDPRODUCTMODESTRING_S   _MESSAGE(64005, _("invalid product mode string "SFQ"\n"))
#define MSG_GDI_CORRUPTPRODMODFILE_S         _MESSAGE(64006, _("product mode file "SFQ" is incorrect\n"))
#define MSG_GDI_SWITCHFROMTO_SS              _MESSAGE(64007, _("switching from "SFQ" to "SFQ" feature set\n"))
#define MSG_HOSTREFINQUEUE_SS                _MESSAGE(64008, _("Host object "SFQ" is still referenced in queue "SFQ".\n"))
#define MSG_SGETEXT_UNKNOWNQUEUE_SSSS        _MESSAGE(64009, _("denied: queue "SFQ" referenced in "SFN" of "SFN" "SFQ" does not exist\n"))
#define MSG_SGETEXT_QUEUEALLANDQUEUEARENOT_SSSS        _MESSAGE(64010, _("queuenames and keyword "SFQ" are not allowed in "SFQ" of "SFQ" "SFQ"\n"))
#define MSG_QUEUE_NULLPTR             _MESSAGE(64011, _("NULL ptr passed to sge_add_queue()\n"))
#define MSG_GDI_CONFIGNOARGUMENTGIVEN_S                  _MESSAGE(64012, _("no argument given in config option: "SFN"\n"))
#define MSG_GDI_CONFIGMISSINGARGUMENT_S                  _MESSAGE(64013, _("missing configuration attribute "SFQ""))
#define MSG_GDI_CONFIGADDLISTFAILED_S                    _MESSAGE(64014, _("can't add "SFQ" to list"))
#define MSG_GDI_CONFIGARGUMENTNOTINTEGER_SS              _MESSAGE(64015, _("value for attribute "SFN" "SFQ" is not an integer\n"))
#define MSG_GDI_CONFIGARGUMENTNOTDOUBLE_SS               _MESSAGE(64016, _("value for attribute "SFN" "SFQ" is not a double\n"))
#define MSG_GDI_CONFIGARGUMENTNOTTIME_SS                 _MESSAGE(64017, _("value for attribute "SFN" "SFQ" is not time\n"))
#define MSG_GDI_CONFIGARGUMENTNOMEMORY_SS                _MESSAGE(64018, _("value for attribute "SFN" "SFQ" is not memory\n"))
#define MSG_GDI_CONFIGINVALIDQUEUESPECIFIED              _MESSAGE(64019, _("reading conf file: invalid queue type specified\n"))
#define MSG_GDI_CONFIGREADFILEERRORNEAR_SS               _MESSAGE(64020, _("reading conf file: "SFN" error near "SFQ"\n"))
#define MSG_GDI_READCONFIGFILESPECGIVENTWICE_SS          _MESSAGE(64021, _("reading config file: specifier "SFQ" given twice for "SFQ"\n"))
#define MSG_GDI_READCONFIGFILEUNKNOWNSPEC_SS             _MESSAGE(64022, _("reading conf file: unknown specifier "SFQ" for "SFN"\n"))
#define MSG_GDI_READCONFIGFILEEMPTYENUMERATION_S         _MESSAGE(64023, _("reading conf file: empty enumeration for "SFQ"\n"))
#define MSG_JOB_XISINVALIDJOBTASKID_S                    _MESSAGE(64024, _("ERROR! "SFN" is a invalid job-task identifier\n"))

#define MSG_JOB_CANTFINDJOBFORUPDATEIN_SS          _MESSAGE(64025, _("can't find job "SFN" for update in function "SFN"\n"))
#define MSG_JOB_CANTFINDJATASKFORUPDATEIN_SS       _MESSAGE(64026, _("can't find array task "SFN" for update in function "SFN"\n"))
#define MSG_JOB_CANTFINDPETASKFORUPDATEIN_SS       _MESSAGE(64027, _("can't find parallel task "SFN" for update in function "SFN"\n"))
#define MSG_JOB_JLPPNULL                  _MESSAGE(64028, _("jlpp == NULL in job_add_job()\n"))                                                        
#define MSG_JOB_JEPNULL                   _MESSAGE(64029, _("jep == NULL in job_add_job()\n"))
#define MSG_JOB_JOBALREADYEXISTS_S        _MESSAGE(64030, _("can't add job "SFN" - job already exists\n") )
#define MSG_JOB_NULLNOTALLOWEDT           _MESSAGE(64031, _("job rejected: 0 is an invalid task id\n"))
#define MSG_JOB_NOIDNOTALLOWED            _MESSAGE(64032, _("job rejected: Job comprises no tasks in its id lists") )
#define MSG_JOB_JOB_ID_U                  _MESSAGE(64033, _(U32CFormat))
#define MSG_JOB_JOB_JATASK_ID_UU          _MESSAGE(64034, _(U32CFormat"."U32CFormat))
#define MSG_JOB_JOB_JATASK_PETASK_ID_UUS  _MESSAGE(64035, _(U32CFormat"."U32CFormat" task "SFN))
#define MSG_JOB_NODISPLAY_S               _MESSAGE(64036, _("no DISPLAY variable found with interactive job "SFN"\n"))
#define MSG_JOB_EMPTYDISPLAY_S            _MESSAGE(64037, _("empty DISPLAY variable delivered with interactive job "SFN"\n"))
#define MSG_JOB_LOCALDISPLAY_SS           _MESSAGE(64038, _("local DISPLAY variable "SFQ" delivered with interactive job "SFN"\n"))
#define MSG_COLONNOTALLOWED               _MESSAGE(64039, _("Colon (\':\') not allowed in account string\n") )
#define MSG_ERRORPARSINGVALUEFORNM_SS _MESSAGE(64040, _("error parsing value "SFQ" for attribute "SFQ"\n"))
#define MSG_PARSE_STARTTIMETOOLONG    _MESSAGE(64041, _("Starttime specifier field length exceeds maximum"))
#define MSG_PARSE_INVALIDSECONDS      _MESSAGE(64042, _("Invalid format of seconds field."))
#define MSG_PARSE_INVALIDHOURMIN      _MESSAGE(64043, _("Invalid format of date/hour-minute field."))
#define MSG_PARSE_INVALIDMONTH        _MESSAGE(64044, _("Invalid month specifica tion."))
#define MSG_PARSE_INVALIDDAY          _MESSAGE(64045, _("Invalid day specificati on."))
#define MSG_PARSE_INVALIDHOUR         _MESSAGE(64046, _("Invalid hour specification."))
#define MSG_PARSE_INVALIDMINUTE       _MESSAGE(64047, _("Invalid minute specification."))
#define MSG_PARSE_INVALIDSECOND       _MESSAGE(64048, _("Invalid seconds specification."))
#define MSG_PARSE_NODATEFROMINPUT     _MESSAGE(64049, _("Couldn't generate date from input. Perhaps a date before 1970 was specified."))
#define MSG_PARSE_NODATE                                _MESSAGE(64050, _("no date specified"))
#define MSG_GDI_VALUETHATCANBESETTOINF                  _MESSAGE(64051, _("value that can be set to infinity"))
#define MSG_GDI_UNRECOGNIZEDVALUETRAILER_SS             _MESSAGE(64052, _("Error! Unrecognized value-trailer '%20s' near '%20s'\nI expected multipliers k, K, m and M.\nThe value string is probably badly formed!\n" ))
#define MSG_GDI_UNEXPECTEDENDOFNUMERICALVALUE_SC        _MESSAGE(64053, _("Error! Unexpected end of numerical value near "SFN".\nExpected one of ',', '/' or '\\0'. Got '%c'\n" ))
#define MSG_GDI_NUMERICALVALUEFORHOUREXCEEDED_SS        _MESSAGE(64054, _("Error! numerical value near %20s for hour exceeded.\n'%20s' is no valid time specifier!\n"))
#define MSG_GDI_NUMERICALVALUEINVALID_SS                _MESSAGE(64055, _("Error! numerical value near %20s invalid.\n'%20s' is no valid time specifier!\n" ))
#define MSG_GDI_NUMERICALVALUEFORMINUTEEXCEEDED_SS      _MESSAGE(64056, _("Error! numerical value near %20s for minute exceeded.\n'%20s' is no valid time specifier!\n"))
#define MSG_GDI_NUMERICALVALUEINVALIDNONUMBER_SS        _MESSAGE(64057, _("Error! numerical value near %20s invalid.\n>%20s< contains no valid decimal or fixed float number\n"))
#define MSG_GDI_NUMERICALVALUEINVALIDNOHEXOCTNUMBER_SS  _MESSAGE(64058, _("Error! numerical value near "SFN" invalid.\n'"SFN"' contains no valid hex or octal number\n"))
#define MSG_PEREFINJOB_SU                               _MESSAGE(64059, _("Pe "SFQ" is still referenced in job "U32CFormat".\n"))
#define MSG_GDI_INITIALPORTIONSTRINGNODECIMAL_S          _MESSAGE(64060, _("Numerical value invalid!\nThe initial portion of string "SFQ" contains no decimal number\n"))
#define MSG_GDI_RANGESPECIFIERWITHUNKNOWNTRAILER_SS      _MESSAGE(64061, _("Range specifier "SFQ" has unknown trailer "SFQ"\n"))
#define MSG_GDI_UNEXPECTEDRANGEFOLLOWINGUNDEFINED        _MESSAGE(64062, _("unexpected range following \"UNDEFINED\"\n"))
#define MSG_GDI_UNEXPECTEDUNDEFINEDFOLLOWINGRANGE        _MESSAGE(64063, _("unexpected \"UNDEFINED\" following range\n"))
#define MSG_PARSE_NOALLOCREQRES       _MESSAGE(64064, _("unable to alloc space for requested resources\n"))
#define MSG_PARSE_NOALLOCATTRLIST     _MESSAGE(64065, _("unable to alloc space for attrib. list\n"))
#define MSG_PARSE_NOALLOCRESELEM      _MESSAGE(64066, _("unable to alloc space for resource element\n"))
#define MSG_PARSE_NOVALIDSLOTRANGE_S  _MESSAGE(64067, _(""SFQ" must be a valid slot range\n"))
#define MSG_PARSE_NOALLOCATTRELEM     _MESSAGE(64068, _("unable to alloc space for attrib. element\n"))
#define MSG_NONE_NOT_ALLOWED                    _MESSAGE(64079, _("The keyword \"none\" is not allowed in \"load_formula\"\n"))
#define MSG_NOTEXISTING_ATTRIBUTE_S             _MESSAGE(64080, _("\"load_formula\" references not existing complex attribute "SFQ"\n"))
#define MSG_WRONGTYPE_ATTRIBUTE_S               _MESSAGE(64081, _("String, CString or Host attributes are not allowed in \"load_formula\": " SFQ "\n"))
#define MSG_SGETEXT_UNKNOWNUSERSET_SSSS         _MESSAGE(64082, _("denied: userset "SFQ" referenced in "SFN" of "SFN" "SFQ" does not exist\n") )
#define MSG_US_INVALIDUSERNAME                  _MESSAGE(64083, _("userset contains invalid (null) user name"))

#endif /* __MSG_SGEOBJLIB_H */
