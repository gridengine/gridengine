#ifndef __COMPLEX_HISTORY_H
#define __COMPLEX_HISTORY_H
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



#include "sge_static_load.h"

/*
** flags for the by name functions
*/
#define FLG_BY_NAME_NOCASE 1

int init_history_subdirs(char *dirname, lList **pplist);
int init_history_files(lList *ldir, lListElem *lsubdir, lList **pplist);

int find_complex_version(lList *ldir, lListElem *version_subdir, unsigned long uldate, lListElem **ppelem);

int find_host_version(lList *ldir, lListElem *version_subdir, unsigned long uldate, lListElem **ppelem);

int find_complex_version_by_name(lList *ldir, char *version_subdir_name, unsigned long uldate, lListElem **ppelem, unsigned long flags);

int find_host_version_by_name(lList *ldir, char *version_subdir_name, unsigned long uldate, lListElem **ppelem, unsigned long flags);

int find_queue_version(lList *ldir, lListElem *version_subdir, unsigned long uldate, lListElem **ppelem);

int find_queue_version_by_name(lList *ldir, char *queue_name, unsigned long uldate, lListElem **ppelem, unsigned long flags);

int make_complex_list(lList *ldir, unsigned long uldate, lList **pplist);

int make_filename(char *buf, unsigned int max_len, const char *dir1, const char *dir2, const char *file);

time_t version_filename_to_t_time(char *timestr);
char *t_time_to_version_filename(char *buf, size_t buflen, time_t t_time);

int create_version_subdir(lList *ldir, char *dirname, lListElem **ppelem);

int find_version_subdir_by_name(lList *ldir, char *dirname, lListElem **ppelem, unsigned long flags);

int write_complex_host_version(lList *ldir, lListElem *version_subdir, unsigned long uldate, lList *cmplx, lListElem *host);

int write_queue_version(lList *ldir, lListElem *version_subdir, unsigned long uldate, lListElem *queue);

int sge_write_queue_version(lList *ldir, lListElem *version_subdir, unsigned long uldate, lListElem *qep);

int sge_write_queue_history(lListElem *qep);
int write_host_history(lListElem *host);
int write_complex_history(lListElem *complex);

int prepare_version_subdir(lList **psubdirs, char *sname, char *name, lListElem **pversion_subdir);

int is_object_in_history(char *sname, char *name);

#endif /* __COMPLEX_HISTORY_H */

