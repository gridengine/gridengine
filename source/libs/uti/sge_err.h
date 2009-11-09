#ifndef __SGE_ERR_H
#define __SGE_ERR_H

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

enum _sge_err_t {
   SGE_ERR_SUCCESS = 0,
   SGE_ERR_MEMORY,
   SGE_ERR_PARAMETER,
   SGE_ERR_FILE_EXIST
};

typedef enum _sge_err_t sge_err_t;

void 
sge_err_init(void);

void
sge_err_set(sge_err_t id, const char *format, ...);

void
sge_err_get(u_long32 pos, sge_err_t *id, char *message, size_t size);

u_long32
sge_err_get_errors(void);

bool
sge_err_has_error(void);

void
sge_err_clear(void); 

#endif


