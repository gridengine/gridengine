#ifndef __SGE_MESSAGEL_H
#define __SGE_MESSAGEL_H

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

#include "sge_boundaries.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* *INDENT-OFF* */ 

/*
 * Scheduler messages
 */
enum {
   SME_message_list = SME_LOWERBOUND,
   SME_global_message_list
};

LISTDEF(SME_Type)
   SGE_LIST(SME_message_list)         /* MES_Type */
   SGE_LIST(SME_global_message_list)  /* MES_Type */
LISTEND 

NAMEDEF(SMEN)
   NAME("SME_message_list")
   NAME("SME_global_message_list")
NAMEEND

#define SMES sizeof(SMEN)/sizeof(char*)

/*
 * Message element
 */
enum {
   MES_job_number_list = MES_LOWERBOUND,
   MES_message_number,
   MES_message
};

LISTDEF(MES_Type)
   SGE_LIST(MES_job_number_list)
   SGE_ULONG(MES_message_number)
   SGE_STRING(MES_message)
LISTEND 

NAMEDEF(MESN)
   NAME("MES_job_number_list")
   NAME("MES_message_number")
   NAME("MES_message")
NAMEEND

/* *INDENT-ON* */   

#define MESS sizeof(MESN)/sizeof(char*)
#ifdef  __cplusplus
}
#endif
#endif
