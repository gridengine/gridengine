#ifndef __SGE_READ_LIST_H 
#define __SGE_READ_LIST_H 
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

int sge_read_cal_list_from_disk(lList **list, const char *directory);
int sge_read_ckpt_list_from_disk(lList **list, const char *directory);
int sge_read_exechost_list_from_disk(lList **list, const char *directory);
int sge_read_adminhost_list_from_disk(lList **list, const char *directory);
int sge_read_submithost_list_from_disk(lList **list, const char *directory);
int sge_read_pe_list_from_disk(lList **list, const char *directory);
int sge_read_project_list_from_disk(lList **list, const char *directory);
int sge_read_queue_list_from_disk(void);
int sge_read_user_list_from_disk(lList **list, const char *directory);
int sge_read_userset_list_from_disk(lList **list, const char *directory);
int sge_read_host_group_entries_from_disk(void);
int sge_read_cqueue_list_from_disk(lList **list, const char *directory);
int read_all_centries(lList **list, const char *directory);

int read_all_configurations(lList **lpp,  
                            const char *global_config_file, 
                            const char *local_config_dir);

int sge_read_user_mapping_entries_from_disk(void);

int sge_read_qinstance_list_from_disk(lListElem *cqueue);

#endif /* __SGE_READ_LIST_H */    
