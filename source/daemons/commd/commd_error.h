#ifndef __COMMD_ERROR_H
#define __COMMD_ERROR_H
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
/* logging levels */
#define CLOG_DEBUG     1
#define CLOG_EXTDEBUG  2
#define CLOG_ERROR     4
#define CLOG_SYSERROR  8
#define CLOG_INFO      16
#define CLOG_TERMINATE 32
#define CLOG_FD        64
#define CLOG_COMMPROC  128
#define CLOG_ALL       0xff



void error(char *str);
void LOG(char *str, int level);
void setlog(int level);

#define LOGTST(level) (level & log_level)

#ifndef COMMD_ERR_MODULE
extern int log_level;
#endif

#endif /* __COMMD_ERROR_H */
