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

/****** gdilib/UME_LOWERBOUND **********************************
*
*  NAME
*     UME_LOWERBOUND -- User Mapping Entry List 
*
*  SYNOPSIS
*
*     #include "sge_usermapL.h"
*     #include <gdilib/sge_usermapL.h>
* 
*     enum {
*        UME_cluster_user = UME_LOWERBOUND,  / username in sge/sge system /
*        UME_mapping_list                    / user specific mapping list /
*     };
*     
*     LISTDEF( UME_Type )
*        SGE_STRING( UME_cluster_user )
*        SGE_LIST  ( UME_mapping_list )      / UM_Type - list /
*     LISTEND
*     
*     NAMEDEF( UMEN )
*        NAME( "UME_cluster_user" )
*        NAME( "UME_mapping_list" )
*     NAMEEND 
*       
*
*  FUNCTION
*
*   UME_Type list element
*   |
*   *---UME_cluster_user (SGE_STRING)
*   *---UME_mapping_list (SGE_LIST)
*             |
*             |
*             *----UM_mapped_user (SGE_STRING)
*             *----UM_host_list   (SGE_LIST)
*                       |
*                       |
*                       *----STR  (SGE_STRING)  String list (ST_Type)
*
*
*  INPUTS
*
*
*  RESULT
*
*
*  EXAMPLE
*
*
*  NOTES
*
*
*  BUGS
*     no bugs known
*
*
*  SEE ALSO
*     /()
*     
****************************************************************************
*/

/* *INDENT-OFF* */  

/* 
 * user mapping entry list 
 */
enum {
   UME_cluster_user = UME_LOWERBOUND,        /* username in sge/sge system */
   UME_mapping_list          /* user specific mapping list */
};

LISTDEF(UME_Type)
   SGE_STRING(UME_cluster_user)
   SGE_LIST(UME_mapping_list) /* UM_Type - list */
LISTEND 

NAMEDEF(UMEN)
   NAME("UME_cluster_user")
   NAME("UME_mapping_list")
NAMEEND

/* *INDENT-ON* */   

#define UMES sizeof(UMEN)/sizeof(char*)

/****** gdilib/UM_LOWERBOUND **********************************
*
*  NAME
*     UM_LOWERBOUND -- User Mapping List 
*
*  SYNOPSIS
*
*     #include "sge_usermapL.h"
*     #include <gdilib/sge_usermapL.h>
* 
*     enum {
*        UM_mapped_user = UM_LOWERBOUND,            / username @foreignhost /
*        UM_host_list                               / list of hosts /
*     };
*     
*     LISTDEF( UM_Type )
*        SGE_STRING( UM_mapped_user )
*        SGE_LIST  ( UM_host_list )                 / ST_Type  -  List / 
*     LISTEND
*     
*     NAMEDEF( UMN )
*        NAME( "UM_mapped_user" )
*        NAME( "UM_host_list" ) 
*     NAMEEND
*       
*
*  FUNCTION
*
*
*  INPUTS
*
*
*  RESULT
*
*
*  EXAMPLE
*
*
*  NOTES
*
*
*  BUGS
*     no bugs known
*
*
*  SEE ALSO
*     /()
*     
****************************************************************************
*/

/* -------------------- 
   user mapping list 
 -------------------- */

/* *INDENT-OFF* */  

enum {
   UM_mapped_user = UM_LOWERBOUND,   /* username @foreignhost */
   UM_host_list                      /* list of hosts */
};

LISTDEF(UM_Type)
   SGE_STRING(UM_mapped_user)
   SGE_LIST(UM_host_list)            /* ST_Type - List */
LISTEND 

NAMEDEF(UMN)
   NAME("UM_mapped_user")
   NAME("UM_host_list")
NAMEEND

/* *INDENT-ON* */  

#define UMS sizeof(UMN)/sizeof(char*)
#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_USERMAPL_H */
