#ifndef __SGE_QSIL_H
#define __SGE_QSIL_H

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
#include "cull.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* *INDENT-OFF* */

/* 
 * this data structures describes a qsi command set element
 */
enum {
   CS_qs_name = CS_LOWERBOUND,
   CS_transfer_queue,
   CS_commands,
   CS_load_values
};

SLISTDEF(CS_Type, QueueingSystem)
   SGE_STRING(CS_qs_name)        /* Foreign QS name */
   SGE_STRING(CS_transfer_queue)      /* name of transfer queue */
   SGE_TLIST(CS_commands, CO_Type)    /* list of supported commands */
   SGE_LIST(CS_load_values)      /* load value list */
LISTEND 

NAMEDEF(CSN)
   NAME("CS_qs_name")
   NAME("CS_transfer_queue")
   NAME("CS_commands")
   NAME("CS_load_values")
NAMEEND

#define CSS sizeof(CSN)/sizeof(char*)

enum {
   CO_name,
   CO_command
};

SLISTDEF(CO_Type, QSCommand)
   SGE_STRING(CO_name)           /* name */
   SGE_STRING(CO_command)        /* command string */
LISTEND 

NAMEDEF(CON)
   NAME("CO_name")
   NAME("CO_command")
NAMEEND

/* *INDENT-ON* */

#define COS sizeof(CON)/sizeof(char*)
#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_QSIL_H */
