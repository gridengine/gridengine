#ifndef __SGE_USERMAPL_H
#define __SGE_USERMAPL_H

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

#ifdef  __cplusplus
extern "C" {
#endif

/****** gdi/usermap/UME_Type **************************************************
*  NAME
*     UME_Type -- CULL user mapping entry element
*
*  SYNOPSIS
*     UME_Type
*     +--- UME_cluster_user: username in SGE/EE system
*     +--- UME_mapping_list: user specific mapping list
*          +--- (UM_mapped_user)
*          +--- (UM_host_list)
*                +--- (STR)
*
*  FUNCTION
*     CULL User mapping entry element
*****************************************************************************/

/* *INDENT-OFF* */  

enum {
   UME_cluster_user = UME_LOWERBOUND,        /* username in sge/sge system */
   UME_mapping_list          /* user specific mapping list */
};

LISTDEF(UME_Type)
   SGE_STRING(UME_cluster_user, CULL_DEFAULT)
   SGE_LIST(UME_mapping_list, UM_Type, CULL_DEFAULT) /* UM_Type - list */
LISTEND 

NAMEDEF(UMEN)
   NAME("UME_cluster_user")
   NAME("UME_mapping_list")
NAMEEND

#define UMES sizeof(UMEN)/sizeof(char*)

/* *INDENT-ON* */   

/****** gdi/usermap/UM_Type ***************************************************
*  NAME
*     UM_Type -- CULL user mapping element 
*
*  SYNOPSIS
*     UM_Type
*     +--- UM_mapped_user: username @foreignhost
*     +--- UM_host_list: list of hosts
*
*  FUNCTION
*     CULL user mapping
******************************************************************************/

/* *INDENT-OFF* */  

enum {
   UM_mapped_user = UM_LOWERBOUND,   /* username @foreignhost */
   UM_host_list                      /* list of hosts */
};

LISTDEF(UM_Type)
   SGE_STRING(UM_mapped_user, CULL_DEFAULT)
   SGE_LIST(UM_host_list, ST_Type, CULL_DEFAULT)            /* ST_Type - List */
LISTEND 

NAMEDEF(UMN)
   NAME("UM_mapped_user")
   NAME("UM_host_list")
NAMEEND

#define UMS sizeof(UMN)/sizeof(char*)

/* *INDENT-ON* */  

#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_USERMAPL_H */
