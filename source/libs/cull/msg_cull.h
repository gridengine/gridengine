#ifndef __MSG_CULL_H
#define __MSG_CULL_H 
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
** cull/src/cull_file.c
*/ 
#define MSG_CULL_NOPREFIXANDNOFILENAMEINWRITEELMTODISK   _("no prefix and no filename in lWriteElemToDisk")
#define MSG_CULL_NOPREFIXANDNOFILENAMEINREADELEMFROMDISK _("no prefix and no filename in lReadElemFromDisk")
#define MSG_CULL_NOTENOUGHMEMORYFORPACKINGXY_SS          _("not enough memory for packing "SFN" "SFQ"\n")
#define MSG_CULL_NOTENOUGHMEMORYFORUNPACKINGXY_SS        _("not enough memory for unpacking "SFN" "SFQ"\n")
#define MSG_CULL_FORMATERRORWHILEPACKINGXY_SS            _("format error while packing "SFN" "SFQ"\n")
#define MSG_CULL_UNEXPECTEDERRORWHILEPACKINGXY_SS        _("unexpected error while packing "SFN" "SFQ"\n")
#define MSG_CULL_UNEXPECTEDERRORWHILEUNPACKINGXY_SS      _("unexpected error while unpacking "SFN" "SFQ"\n")
#define MSG_CULL_FORMATERRORWHILEUNPACKINGXY_SS          _("format error while unpacking "SFN" "SFQ"\n")
#define MSG_CULL_CANTOPENXFORWRITINGOFYZ_SSS             _("can't open "SFN" for writing of "SFN": "SFN"")
#define MSG_CULL_CANTWRITEXTOFILEY_SS                    _("can't write "SFN" to file "SFN"")
#define MSG_CULL_CANTREADXFROMFILEY_SS                   _("can't read "SFN" from file "SFN"")
#define MSG_CULL_CANTGETFILESTATFORXFILEY_SS             _("can't get file stat for "SFN" file "SFQ"")
#define MSG_CULL_XFILEYHASZEROSIYE_SS                    _(""SFN" file "SFQ" has zero size")
#define MSG_CULL_ERRORREADINGXINFILEY_SS                 _("error reading "SFN" in file "SFN"")
#define MSG_CULL_BADARGUMENTWHILEUNPACKINGXY_SS          _("bad argument error while unpacking "SFN" "SFQ"\n")
#define MSG_CULL_FORMATERRORININITPACKBUFFER             _("format error in init_packbuffer")
#define MSG_CULL_NOTENOUGHMEMORY_D                       _("not enough memory to allocate %d bytes in init_packbuffer")


/* 
** cull/src/cull_list.c
*/ 
#define MSG_CULL_LISTXYNROFELEMENTSZ_SSI                 _("\n"SFN"List: <"SFN"> #Elements: %d\n")

/* 
** cull/src/cull_multitype.c
*/ 
#define MSG_CULL_POINTER_GETLISTREF_NULLELEMENTFORX_S    _("!!!!!!!!!! lGetListRef(): got NULL element for "SFN" !!!!!!!!!!\n")
#define MSG_CULL_POINTER_GETULONG_NULLELEMENTFORX_S      _("!!!!!!!!!! lGetUlong(): got NULL element for "SFN" !!!!!!!!!!\n")
#define MSG_CULL_POINTER_GETSTRING_NULLELEMENTFORX_S     _("!!!!!!!!!! lGetString(): got NULL element for "SFN" !!!!!!!!!!\n")
#define MSG_CULL_POINTER_GETHOST_NULLELEMENTFORX_S       _("!!!!!!!!!! lGetHost(): got NULL element for "SFN" !!!!!!!!!!\n")
#define MSG_CULL_POINTER_GETLIST_NULLELEMENTFORX_S       _("!!!!!!!!!! lGetList(): got NULL element for "SFN" !!!!!!!!!!\n")
#define MSG_CULL_GETLISTREF_XNOTFOUNDINELEMENT_S         _("!!!!!!!!!! lGetListRef(): "SFN" not found in element !!!!!!!!!!\n")
#define MSG_CULL_GETLIST_XNOTFOUNDINELEMENT_S            _("!!!!!!!!!! lGetList(): "SFN" not found in element !!!!!!!!!!\n")
#define MSG_CULL_GETINT_WRONGTYPEFORFIELDXY_SS           _("lGetInt: wrong type for field "SFN" ("SFN")\n")
#define MSG_CULL_GETULONG_NOSUCHNAMEXYINDESCRIPTOR_IS    _("lGetUlong: no such name (%d, "SFN") in descriptor\n")
#define MSG_CULL_GETSTRING_NOSUCHNAMEXYINDESCRIPTOR_IS   _("lGetString: no such name (%d, "SFN") in descriptor\n")
#define MSG_CULL_GETHOST_NOSUCHNAMEXYINDESCRIPTOR_IS     _("lGetHost: no such name (%d, "SFN") in descriptor\n")
#define MSG_CULL_GETULONG_WRONGTYPEFORFIELDXY_SS         _("lGetUlong: wrong type for field "SFN" ("SFN")\n")
#define MSG_CULL_GETSTRING_WRONGTYPEFORFILEDXY_SS        _("lGetString: wrong type for field "SFN" ("SFN")\n")
#define MSG_CULL_GETHOST_WRONGTYPEFORFILEDXY_SS          _("lGetHost: wrong type for field "SFN" ("SFN")\n")
#define MSG_CULL_GETLIST_WRONGTYPEFORFIELDXY_SS          _("lGetList: wrong type for field "SFN" ("SFN")\n")
#define MSG_CULL_GETFLOAT_WRONGTYPEFORFIELDXY_SS         _("lGetFloat: wrong type for field "SFN" ("SFN")\n")
#define MSG_CULL_GETDOUBLE_WRONGTYPEFORFIELDXY_SS        _("lGetDouble: wrong type for field "SFN" ("SFN")\n")
#define MSG_CULL_GETLONG_WRONGTYPEFORFIELDXY_SS          _("lGetLong: wrong type for field "SFN" ("SFN")\n")
#define MSG_CULL_GETCHAR_WRONGTYPEFORFIELDXY_SS          _("lGetChar: wrong type for field "SFN" ("SFN")\n")
#define MSG_CULL_GETREF_WRONGTYPEFORFIELDXY_SS           _("lGetRef: wrong type for field "SFN" ("SFN")\n")
#define MSG_CULL_SETINT_WRONGTYPEFORFIELDXY_SS           _("lSetInt: wrong type for field "SFN" ("SFN")\n")
#define MSG_CULL_SETULONG_WRONGTYPEFORFIELDXY_SS         _("lSetUlong: wrong type for field "SFN" ("SFN")\n")
#define MSG_CULL_SETSTRING_NOSUCHNAMEXYINDESCRIPTOR_IS   _("lSetString: no such name (%d, "SFN") in descriptor\n")
#define MSG_CULL_SETHOST_NOSUCHNAMEXYINDESCRIPTOR_IS     _("lSetHost: no such name (%d, "SFN") in descriptor\n")
#define MSG_CULL_SETSTRING_WRONGTYPEFORFIELDXY_SS        _("lSetString: wrong type for field "SFN" ("SFN")\n")
#define MSG_CULL_SETHOST_WRONGTYPEFORFIELDXY_SS          _("lSetHost: wrong type for field "SFN" ("SFN")\n")
#define MSG_CULL_XCHGLIST_WRONGTYPEFORFIELDXY_SS         _("lXchgList: wrong type for field "SFN" ("SFN")\n")
#define MSG_CULL_SETLIST_WRONGTYPEFORFIELDXY_SS          _("lSetList: wrong type for field "SFN" ("SFN")\n")
#define MSG_CULL_SETFLOAT_WRONGTYPEFORFIELDXY_SS         _("lSetFloat: wrong type for field "SFN" ("SFN")\n")
#define MSG_CULL_SETDOUBLE_WRONGTYPEFORFIELDXY_SS        _("lSetDouble: wrong type for field "SFN" ("SFN")\n")
#define MSG_CULL_SETLONG_WRONGTYPEFORFIELDXY_SS          _("lSetLong: wrong type for field "SFN" ("SFN")\n")
#define MSG_CULL_SETCHAR_WRONGTYPEFORFIELDXY_SS          _("lSetChar: wrong type for field "SFN" ("SFN")\n")
#define MSG_CULL_SETREF_WRONGTYPEFORFIELDXY_SS           _("lSetRef: wrong type for field "SFN" ("SFN")\n" )
#define MSG_CULL_ADDSUBSTRERRORXRUNTIMETYPE_S            _("error: lAddSubStr("SFN"): run time type error\n")
#define MSG_CULL_ADDSUBHOSTERRORXRUNTIMETYPE_S            _("error: lAddSubHost("SFN"): run time type error\n")
#define MSG_CULL_ADDELEMSTRERRORXRUNTIMETYPE_S           _("error: lAddElemStr("SFN"): run time type error\n")
#define MSG_CULL_ADDELEMHOSTERRORXRUNTIMETYPE_S           _("error: lAddElemHost("SFN"): run time type error\n")
#define MSG_CULL_DELSUBSTRERRORXRUNTIMETYPEERROR_S       _("error: lDelSubStr("SFN"): run time type error\n")
#define MSG_CULL_DELSUBHOSTERRORXRUNTIMETYPEERROR_S       _("error: lDelSubHost("SFN"): run time type error\n")
#define MSG_CULL_DELELEMSTRERRORXRUNTIMETYPEERROR_S      _("error: lDelElemStr("SFN"): run time type error\n")
#define MSG_CULL_DELELEMHOSTERRORXRUNTIMETYPEERROR_S      _("error: lDelElemHost("SFN"): run time type error\n")
#define MSG_CULL_GETSUBSTRERRORXRUNTIMETYPE_S            _("error: lGetSubStr("SFN"): run time type error\n")
#define MSG_CULL_GETELEMSTRERRORXRUNTIMETYPE_S           _("error: lGetElemStr("SFN"): run time type error\n")
#define MSG_CULL_GETELEMSTRERRORXRUNTIMETYPE_S           _("error: lGetElemStr("SFN"): run time type error\n")
#define MSG_CULL_ADDSUBULONGERRORXRUNTIMETYPE_S          _("error: lAddSubUlong("SFN"): run time type error\n")
#define MSG_CULL_ADDELEMULONGERRORXRUNTIMETYPE_S         _("error: lAddElemUlong("SFN"): run time type error\n")
#define MSG_CULL_DELSUBULONGERRORXRUNTIMETYPE_S          _("error: lDelSubUlong("SFN"): run time type error\n")
#define MSG_CULL_DELELEMULONGERRORXRUNTIMETYPE_S         _("error: lDelElemUlong("SFN"): run time type error\n")
#define MSG_CULL_GETSUBULONGERRORXRUNTIMETYPE_S          _("error: lGetSubUlong("SFN"): run time type error\n")
#define MSG_CULL_GETELEMULONGERRORXRUNTIMETYPE_S         _("error: lGetElemUlong("SFN"): run time type error\n")
#define MSG_CULL_ADDSUBCASESTRERRORXRUNTIMETYPE_S        _("error: lAddSubCaseStr("SFN"): run time type error\n")
#define MSG_CULL_ADDELEMCASESTRERRORXRUNTIMETYPE_S       _("error: lAddElemCaseStr("SFN"): run time type error\n")
#define MSG_CULL_DELSUBCASESTRERRORXRUNTIMETYPE_S        _("error: lDelSubCaseStr("SFN"): run time type error\n")
#define MSG_CULL_DELELEMCASESTRERRORXRUNTIMETYPE_S       _("error: lDelElemCaseStr("SFN"): run time type error\n")
#define MSG_CULL_DELELEMHOSTERRORXRUNTIMETYPE_S          _("error: lDelElemHost("SFN"): run time type error\n")
#define MSG_CULL_GETSUBCASESTRERRORXRUNTIMETYPE_S        _("error: lGetSubCaseStr("SFN"): run time type error\n")
#define MSG_CULL_GETELEMCASESTRERRORXRUNTIMETYPE_S       _("error: lGetElemCaseStr("SFN"): run time type error\n")
#define MSG_CULL_GETELEMCASEHOSTERRORXRUNTIMETYPE_S      _("error: lGetElemCaseHost("SFN"): run time type error\n")
#define MSG_CULL_GETELEMHOSTERRORXRUNTIMETYPE_S          _("error: lGetElemHost("SFN"): run time type error\n")
#define MSG_CULL_GETPOSULONG_GOTINVALIDPOSITION          _("!!!!!!!!!! lGetPosUlong() got an invalid pos !!!!!!!!!!!!\n")
#define MSG_CULL_GETPOSLIST_GOTANINVALIDPOS              _("!!!!!!!!!!!11 lGetPosUlong() got an invalid pos !!!!!!!!\n")

/* 
** cull/src/cull_where.c
*/ 
#define MSG_CULL_WHERE_SHOULDBEINTT                      _("lWhere (should be a lIntT)\n")
#define MSG_CULL_WHERE_SHOULDBESTRINGT                   _("lWhere (should be a lStringT or lHostT)\n")
#define MSG_CULL_WHERE_SHOULDBEULONGT                    _("lWhere (should be a lUlongT)\n")
#define MSG_CULL_WHERE_SHOULDBEFLOATT                    _("lWhere (should be a lFloatT)\n")
#define MSG_CULL_WHERE_SHOULDBEDOUBLET                   _("lWhere (should be a lDoubleT)\n")
#define MSG_CULL_WHERE_SHOULDBELONGT                     _("lWhere (should be a lLongT)\n")
#define MSG_CULL_WHERE_SHOULDBECHART                     _("lWhere (should be a lCharT)\n")
#define MSG_CULL_WHERE_SHOULDBEREFT                      _("lWhere (should be a lRefT)\n")
#define MSG_CULL_WHERE_OPERANDHITNOTOPERATORERROR        _("operand does not match to operator\n")

/* 
** cull/src/cull_lerrno.c
*/ 
#define MSG_CULL_LEMALLOC                _("malloc failure")
#define MSG_CULL_LEINCTYPE               _("incompatible type")
#define MSG_CULL_LEUNKTYPE               _("unknown type")
#define MSG_CULL_LEELEMNULL              _("element is NULL")
#define MSG_CULL_LENAMENOT               _("name not in descriptor contained")
#define MSG_CULL_LENAMEOUT               _("name out of namespaces")
#define MSG_CULL_LEDESCRNULL             _("descriptor is NULL, empty describtor")
#define MSG_CULL_LENEGPOS                _("negative position is not allowed")
#define MSG_CULL_LESTRDUP                _("strdup failure")
#define MSG_CULL_LEFILENULL              _("file pointer is NULL")
#define MSG_CULL_LEFGETBRA               _("fGetBra failed")
#define MSG_CULL_LEFGETKET               _("fGetKet failed")
#define MSG_CULL_LEFGETINT               _("fGetInt failed")
#define MSG_CULL_LEFGETDESCR             _("fGetDescr failed")
#define MSG_CULL_LELISTNULL              _("list is NULL")
#define MSG_CULL_LECREATEELEM            _("lCreateElem failure")
#define MSG_CULL_LECOUNTDESCR            _("lCountDescr failure")
#define MSG_CULL_LEFIELDREAD             _("reading field failure")
#define MSG_CULL_LEFGETSTRING            _("fGetString failure")
#define MSG_CULL_LECREATELIST            _("lCreateList failure")
#define MSG_CULL_LEUNDUMPELEM            _("lUndumpElem failure")
#define MSG_CULL_LESSCANF                _("sscanf failure")
#define MSG_CULL_LESYNTAX                _("syntax error")
#define MSG_CULL_LEFGETLINE              _("fGetLine failure")
#define MSG_CULL_LEFGETS                 _("fgets failure")
#define MSG_CULL_LESPACECOMMENT          _("space_comment failure")
#define MSG_CULL_LEUNDUMPLIST            _("lUndumpList failure")
#define MSG_CULL_LECOPYSWITCH            _("lCopySwitch failure")
#define MSG_CULL_LEENUMNULL              _("lEnumeration is NULL")
#define MSG_CULL_LECONDNULL              _("lCondition is NULL")
#define MSG_CULL_LENOLISTNAME            _("no listname specified")
#define MSG_CULL_LEDIFFDESCR             _("different list descriptors")
#define MSG_CULL_LEDECHAINELEM           _("lDechainElem failure")
#define MSG_CULL_LEAPPENDELEM            _("lAppendElem failure")
#define MSG_CULL_LENOFORMATSTR           _("format string is missing")
#define MSG_CULL_LEPARSESORTORD          _("lParseSortOrder failure")
#define MSG_CULL_LEGETNROFELEM           _("lgetNumberOfElem failure")
#define MSG_CULL_LESORTORDNULL           _("lSortOrder is NULL")
#define MSG_CULL_LESUM                   _("sum in where.c failure")
#define MSG_CULL_LEOPUNKNOWN             _("operator of lCondition struct unknown")
#define MSG_CULL_LECOPYELEMPART          _("lCopyElemPartial failure")
#define MSG_CULL_LENULLARGS              _("function argument is NULL")
#define MSG_CULL_LEFALSEFIELD            _("field is not allowed here")
#define MSG_CULL_LEJOINDESCR             _("lJoinDescr failure")
#define MSG_CULL_LEJOIN                  _("lJoin failure")
#define MSG_CULL_LEJOINCOPYELEM          _("lJoinCopyElem failure")
#define MSG_CULL_LEADDLIST               _("lAddList failure")
#define MSG_CULL_LECOUNTWHAT             _("lCountWhat failure")
#define MSG_CULL_LEPARTIALDESCR          _("lPartialDescr failure")
#define MSG_CULL_LEENUMDESCR             _("enumeration no subset of descriptor")
#define MSG_CULL_LEENUMBOTHNONE          _("at least one enumeration required")
#define MSG_CULL_LENULLSTRING            _("string NULL not allowed")
#define MSG_CULL_LEPARSECOND             _("parsing condition failed")
#define MSG_CULL_LEFORMAT                _("wrong packing format")
#define MSG_CULL_LEOPEN                  _("could not open file")

/* 
** cull/src/pack.c
*/ 
#define MSG_CULL_CANTPACKINTTOUNPACKBUFF   _("!!!!!!!! Cannot pack an integer into an unpacking buffer !!!!!!!!!!\n")
#define MSG_CULL_CANTPACKDOUBLETOUNPACKBUFF  _("!!!!!!!! Cannot pack a double into an unpacking buffer !!!!!!!!!!\n")
#define MSG_CULL_CANTPACKSTRINGTOUNPACKBUFF  _("!!!!!!!! Cannot pack a string into an unpacking buffer !!!!!!!!!!\n")
#define MSG_CULL_CANTPACKBYTEARRAYTOUNPACKBUFF  _("!!!!!!!! Cannot pack a bytearray into an unpacking buffer !!!!!!!!!!\n")
#define MSG_CULL_CANTUNPACKINTFROMPACKINGBUFF   _("!!!!!!!! Cannot unpack an integer from a packing buffer !!!!!!!!!!\n")
#define MSG_CULL_CANTUNPACKDOUBLEFROMPACKINGBUFF   _("!!!!!!!! Cannot unpack a double from a packing buffer !!!!!!!!!!\n")
#define MSG_CULL_CANTUNPACKSTRINGFROMPACKINGBUFF   _("!!!!!!!! Cannot unpack a string from a packing buffer !!!!!!!!!!\n")
#define MSG_CULL_CANTUNPACKBYTEARRAYFROMPACKINGBUFF   _("!!!!!!!! Cannot unpack a byte array from a packing buffer !!!!!!!!!!\n")


/*
** cull_hash.c
*/
#define MSG_CULL_HASHTABLEALREADYEXISTS_S    _("hash table already exists for field "SFQ"\n")




#endif /* __MSG_CULL_H */ 

