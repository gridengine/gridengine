#ifndef _sge_flatfile_obj_H
#define	_sge_flatfile_obj_H
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

#ifdef	__cplusplus
extern "C" {
#endif

#if 0
extern spooling_field *AMEM_sub_fields;
extern spooling_field *ATIME_sub_fields;
extern spooling_field *UP_sub_fields;
extern spooling_field *APRJLIST_sub_fields;
extern spooling_field *SO_sub_fields;
extern spooling_field *ASOLIST_sub_fields;
extern spooling_field *US_sub_fields;
extern spooling_field *AUSRLIST_sub_fields;
extern spooling_field *ABOOL_sub_fields;
extern spooling_field *ST_sub_fields;
extern spooling_field *ASTRLIST_sub_fields;
extern spooling_field *AQTLIST_sub_fields;
extern spooling_field *ASTR_sub_fields;
extern spooling_field *AINTER_sub_fields;
extern spooling_field *CE_sub_fields;
extern spooling_field *ACELIST_sub_fields;
extern spooling_field *AULNG_sub_fields;
extern spooling_field *CF_sub_fields;
extern spooling_field *STN_sub_fields;
extern spooling_field *HS_sub_fields;
extern spooling_field *RU_sub_fields;
extern spooling_field *HL_sub_fields;
extern spooling_field *STU_sub_fields;
extern spooling_field *UE_sub_fields;
extern spooling_field *UA_sub_fields;
extern spooling_field *UPP_sub_fields;
extern spooling_field *UPU_sub_fields;
extern spooling_field *HR_sub_fields;
#endif
extern spooling_field CAL_fields[];
extern spooling_field CK_fields[];
extern spooling_field CE_fields[];
extern spooling_field HGRP_fields[];
extern spooling_field US_fields[];
extern spooling_field SC_fields[];
extern spooling_field CQ_fields[];
extern spooling_field CU_fields[];

spooling_field *sge_build_UP_field_list (bool spool, bool user);
spooling_field *sge_build_STN_field_list (bool spool, bool recurse);
spooling_field *sge_build_PE_field_list (bool spool, bool to_stdout);
spooling_field *sge_build_EH_field_list (bool spool, bool to_stdout,
                                            bool history);
spooling_field *sge_build_CONF_field_list(bool spool_config);
spooling_field *sge_build_QU_field_list(bool to_stdout, bool to_file);

#ifdef	__cplusplus
}
#endif

#endif	/* _sge_flatfile_obj_H */
