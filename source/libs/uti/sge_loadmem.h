#ifndef __SGE_LOADMEM_H
#define __SGE_LOADMEM_H
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

#if defined(CRAY) || defined(SOLARIS) || defined(HP10) || defined(HP11) || defined(ALPHA) || defined(LINUX) || defined(IRIX6) || defined(NECSX4) || defined(NECSX5) || defined(DARWIN) || defined(FREEBSD) || defined(NETBSD)

#define SGE_LOADMEM

typedef struct {
   double mem_total;   /* total amount of memory     in megs    */
   double mem_free;    /* amount of free memory      in megs    */
   double swap_total;  /* total amount of swap space in megs    */
   double swap_free;   /* amount of free swap space  in megs    */
#ifdef IRIX6
   double swap_rsvd;   /* amount of reserved swap space in megs */
#endif
} sge_mem_info_t;

int loadmem(sge_mem_info_t *mem_info);
#endif /* SGE_LOADMEM */

#endif /* __SGE_LOADMEM_H */
