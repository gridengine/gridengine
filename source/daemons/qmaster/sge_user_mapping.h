#ifndef _SGE_USER_MAPPING_H__
#define _SGE_USER_MAPPING_H__
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



#include "sge_c_gdi.h"



const char* sge_getUserNameForHost(lList *hostGroupList, lList *mapList, const char *hostName);

int   sge_map_gdi_request(lList *hostGroupList, lList *userMappingEntryList, sge_gdi_request *pApiRequest);

char* sge_malloc_map_out_going_username(lList *hostGroupList, lList *userMappingEntryList, const char *clusterName, const char *hostName);

int   sge_resolveMappingList(lList **alpp, lList *hostGroupList, lList *mapList);

int   sge_addMappingEntry(lList **alpp, lList *hostGroupList, lList *mapList, const char *actMapName, lList *actHostList, int doResolving);

int sge_removeOverstock(lList **alpp, lListElem *newListElem, lListElem *origListElem);

lListElem* sge_getElementFromMappingEntryList(lList *userMappingEntryList, const char *clusterName);

int sge_verifyMappingEntry(lList **alpp, lList *hostGroupList, lListElem *mapEntry, const char *filename, lList *userMappingEntryList);

int   sge_addHostToMappingList(lList *hostGroupList, lList *userMappingEntryList, char *clusterName, char *mapName, char *newHostName);

#endif /* _SGE_USER_MAPPING_H__ */


