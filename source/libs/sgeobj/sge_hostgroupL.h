#ifndef __SGE_HOSTGROUPL_H
#define __SGE_HOSTGROUPL_H

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

/****** sgeobj/sge/GRP_LOWERBOUND **********************************
*
*  NAME
*     GRP_LOWERBOUND -- Group List 
*
*  SYNOPSIS
*
*     
*     enum {
*        GRP_group_name = GRP_LOWERBOUND,    groupname in sge/sge system 
*        GRP_member_list,                    list of hostnames in group 
*        GRP_subgroup_list,                  list of subgroup names  
*        GRP_supergroup                      groupname of supergroup 
*     };
*     
*     LISTDEF( GRP_Type )
*        SGE_STRING ( GRP_group_name      )  individual Name of group
* 
*        SGE_LIST   ( GRP_member_list     )  ST_Type - list 
*                                            (list of eg. hostnames)
*
*        SGE_LIST   ( GRP_subgroup_list   )  ST_Type - list 
*                                            (list of group names which 
*                                             are subgroups of this)
*
*        SGE_STRING  ( GRP_supergroup )      name of supergroup 
*     LISTEND
*
*  FUNCTION
*
*
*   GRP_Type list element
*   |
*   *---GRP_group_name      (SGE_STRING)
*   |
*   *---GRP_member_list       (SGE_LIST)    
*   |          |
*   |          |
*   |          *----STR  (SGE_STRING)  String list (ST_Type)
*   |
*   |
*   *---GRP_subgroup_list   (SGE_LIST)    
*   |          |
*   |          |
*   |          *----STR  (SGE_STRING)  String list (ST_Type)
*   |
*   |
*   *---GRP_supergroup (SGE_STRING)    
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

enum {
   GRP_group_name = GRP_LOWERBOUND,  /* groupname in sge/sge system */
   GRP_member_list,          /* list of hostnames in group */
   GRP_subgroup_list,        /* list of subgroup names */
   GRP_supergroup            /* supergroup name */
};

LISTDEF(GRP_Type)
   SGE_STRING(GRP_group_name, CULL_DEFAULT)     /* individual Name of group */
   SGE_LIST(GRP_member_list, ST_Type, CULL_DEFAULT)      /* ST_Type - list */
   SGE_LIST(GRP_subgroup_list, ST_Type, CULL_DEFAULT)    /* ST_Type - list */
   SGE_STRING(GRP_supergroup, CULL_DEFAULT)     /* name of supergroup */
LISTEND 

NAMEDEF(GRPN)
   NAME("GRP_group_name")
   NAME("GRP_member_list")
   NAME("GRP_subgroup_list")
   NAME("GRP_supergroup")
NAMEEND

#define GRPS sizeof(GRPN)/sizeof(char*)

enum {
   HGRP_name = HGRP_LOWERBOUND,
   HGRP_host_list
};

LISTDEF(HGRP_Type)
   SGE_HOST(HGRP_name, CULL_HASH | CULL_UNIQUE | CULL_SPOOL)
   SGE_LIST(HGRP_host_list, HR_TYPE, CULL_SPOOL) 
LISTEND

NAMEDEF(HGRPN)
   NAME("HGRP_name")
   NAME("HGRP_host_list")
NAMEEND

#define HGRPS sizeof(HGRPN)/sizeof(char*)

/* *INDENT-ON* */

#ifdef  __cplusplus
}
#endif

#endif /* __SGE_HOSTGROUPL_H */
