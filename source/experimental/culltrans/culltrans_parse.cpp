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
 *  License at http://www.gridengine.sunsource.net/license.html
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
// culltrans_parse.cpp
// functions that build up the meta-database

#include <map>
#include <set>
#include <string>
#include <iostream.h>
#include <fstream.h>

#include "culltrans_repository.h"
#include "culltrans.h"

extern "C"
{
#include "cull.h"
#include "cod_all_listsL.h"
}

// newList
// begin a new list definition
// type: the list's type (eg QU_Type)
// name: the list's name (eg Queue)
// interface: flag if interface or struct
void newList(const char* type, const char* name, const char* cod_list_type, const bool interface) {
   if(disthdr)
      fprintf(disthdr, "LISTDEF( %s )", type);

   pair<map<string, List>::iterator, bool> status;
   List  l(type, name, yyin_name, cod_list_type, interface);
   status = lists.insert(map<string, List>::value_type(type, l));
   if(status.second)
      active = status.first;
   else
      active = lists.end();
}

// endList
// end the current list definition
void endList() {
   if(disthdr)
      fprintf(disthdr, "LISTEND");

   active = lists.end();
}

// newElem
// add a new element to a list. only use for standard types
// type: the type of the element (eg lfloatT)
// name: the name of the element (eg JB_job_number)
// readonly: flag if readlonly attribute or not
// key: flag, identifying element of interface. can only be used
//      once for any one interface
void newElem(const int type, const char* name, const bool readonly, const bool idlonly, const bool key) {
   if(disthdr && !idlonly) {
      switch(type) {
         case lBoolT:
            fprintf(disthdr, "   COD_ULONG");
            break;
         case lFloatT:
            fprintf(disthdr, "   COD_FLOAT");
            break;
         case lDoubleT:
            fprintf(disthdr, "   COD_DOUBLE");
            break;
         case lUlongT:
            fprintf(disthdr, "   COD_ULONG");
            break;
         case lLongT:
            fprintf(disthdr, "   COD_LONG");
            break;
         case lCharT:
            fprintf(disthdr, "   COD_CHAR");
            break;
         case lIntT:
            fprintf(disthdr, "   COD_INT");
            break;
         case lStringT:
            fprintf(disthdr, "   COD_STRING");
            break;
         default:
            break;
      }

      fprintf(disthdr, "( %s )", name);
   }

   if(!idlonly)
      constants[lStr2Nm((char*)name)] = name;
   else
      constants[last_qidl_only++] = name;

   const char* ptr = strchr(name, '_');
   
   if(active != lists.end())
      active->second.elems.push_back(Elem(type, ptr?ptr+1:name, readonly, key, idlonly));
}

// newXElem
// a list element that will not appear in the idl code
// but in the dist headers (if necessary)
void newXElem(const char* type, const char* name) {
   if(disthdr)
      fprintf(disthdr, "   COD_%s( %s )", strchr(type, 'X')+1, name);
}

// newListElem
// add a new list reference to a list.
// name: the name of the reference (eg queue)
// type: the type of the reference (eg QU_Type)
// readonly: flag if readonly or not
void newListElem(const char* name, const char* type, const bool readonly, const bool idlonly) {
   if(disthdr && !idlonly)
      fprintf(disthdr, "   COD_LIST( %s )", name);

   if(!idlonly)
      constants[lStr2Nm((char*)name)] = name;
   else
      constants[last_qidl_only++] = name;

   const char* ptr = strchr(name, '_');

   if(active != lists.end())
      active->second.elems.push_back(Elem(ptr?ptr+1:name, type, readonly, false, idlonly));
}

// newObjElem
// add a new object reference to a list.
// name: the name of the reference (eg queue)
// type: the type of the reference (eg QU_Type)
// readonly: flag if readonly or not
void newObjElem(const char* name, const char* type, const bool readonly, const bool idlonly) {
   if(disthdr && !idlonly)
      fprintf(disthdr, "   COD_LIST( %s )", name);

   if(!idlonly)
      constants[lStr2Nm((char*)name)] = name;
   else
      constants[last_qidl_only++] = name;

   const char* ptr = strchr(name, '_');

   if(active != lists.end())
      active->second.elems.push_back(Elem(ptr?ptr+1:name, type, readonly, true, idlonly));
}

// newXList
// a list element that will not appear in the idl code
// but in the dist headers (if necessary)
void newXList(const char* name, const char* type, const bool object) {
   if(disthdr)
      fprintf(disthdr, "   COD_LIST( %s )", name);

}

// newIDL
// add a line to the verbose idl part of a list
// line: a string containg the idl text
void newIDL(const char* line) {
   if(active != lists.end())
      active->second.idl += line;
}

// newWord
// receives a word that is otherwise of no importance
// simply copy it to the dist header if necessary
void newWord(const char* word) {
   if(disthdr)
      fprintf(disthdr, "%s", word);
}

// newOther
// receives a character that is otherwise of no importance
// used to write to distheaders
void newOther(const char other) {
   if(disthdr)
      fprintf(disthdr, "%c", other);
}
