#ifndef __SGE_CULL_XMLL_H
#define __SGE_CULL_XMLL_H
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

#include "sge_boundaries.h"
#include "cull.h"
#include "sge_strL.h"
#include "cull_list.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* *INDENT-OFF* */    

/**
 * This file contains some extra cull definition for XML cull output.
 * Such as the XML header structure and a XML element
 *
 */


/**************************************************
 *
 * XML attribute:
 *
 **************************************************/
enum {
   XMLA_Name = XMLA_LOWERBOUND,
   XMLA_Value
};

SLISTDEF(XMLA_Type, XML_Attribute)
   SGE_STRING(XMLA_Name, CULL_DEFAULT)
   SGE_STRING(XMLA_Value, CULL_DEFAULT)        
LISTEND        

NAMEDEF(XMLAN)
   NAME("XMLA_Name")
   NAME("XMLA_Value")
NAMEEND

#define XMLAS sizeof(XMLAN)/sizeof(char*)

/**************************************************
 *
 * XML schema:
 *
 **************************************************/
enum {
   XMLS_Name = XMLS_LOWERBOUND,
   XMLS_Value,
   XMLS_Version
};

SLISTDEF(XMLS_Type, XML_Stylesheet)
   SGE_STRING(XMLS_Name, CULL_DEFAULT)
   SGE_STRING(XMLS_Value, CULL_DEFAULT)        
   SGE_STRING(XMLS_Version, CULL_DEFAULT)
LISTEND        

NAMEDEF(XMLSN)
   NAME("XMLS_Name")
   NAME("XMLS_Value")
   NAME("XMLS_Version")
NAMEEND

#define XMLSS sizeof(XMLSN)/sizeof(char*)


/**************************************************
 *
 * XML documtent head
 *
 **************************************************/
enum {
   XMLH_Version = XMLH_LOWERBOUND,
   XMLH_Name,
   XMLH_Stylesheet,
   XMLH_Attribute,
   XMLH_Element
};

SLISTDEF(XMLH_Type, XML_Head)
   SGE_STRING(XMLH_Version, CULL_DEFAULT)
   SGE_STRING(XMLH_Name, CULL_DEFAULT)
   SGE_LIST(XMLH_Stylesheet, XMLS_Type, CULL_DEFAULT)
   SGE_LIST(XMLH_Attribute, XMLA_Type, CULL_DEFAULT)
   SGE_LIST(XMLH_Element, CULL_ANY_SUBTYPE, CULL_DEFAULT)   
LISTEND

NAMEDEF(XMLHN)
   NAME("XMLH_Version") 
   NAME("XMLH_Name")
   NAME("XMLH_Stylesheet")
   NAME("XMLH_Attribute")
   NAME("XMLH_Element")  
NAMEEND   

#define XMLHS sizeof(XMLHN)/sizeof(char*)

/**************************************************
 *
 * XML document entry
 *
 **************************************************/
enum {
   XMLE_Attribute = XMLE_LOWERBOUND,     
   XMLE_Print,
   XMLE_Element,
   XMLE_List
};

SLISTDEF(XMLE_Type, XML_Element)
   SGE_LIST(XMLE_Attribute, XMLA_Type, CULL_DEFAULT)
   SGE_BOOL(XMLE_Print, CULL_DEFAULT)
   SGE_OBJECT(XMLE_Element, CULL_ANY_SUBTYPE, CULL_DEFAULT)
   SGE_LIST(XMLE_List, CULL_ANY_SUBTYPE, CULL_DEFAULT)
LISTEND

NAMEDEF(XMLEN)
   NAME("XMLE_Attribute")     
   NAME("XMLE_Print")
   NAME("XMLE_Element")
   NAME("XMLE_List")
NAMEEND

#define XMLES sizeof (XMLEN)/sizeof(char*)

/* *INDENT-ON* */ 
        
#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_COMPLEXL_H */        
