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
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "sgermon.h"
#include "sge_filecmp.h"
#include "sge_stat.h" 

/* -------------------------------------------------------

   compare two files wheater they are equal

DESCR

   they are equal 
   - if both of them have the same name 
   - if a stat() succeeds for both files and 
     i-node/device-id are equal
   
   we are not sure 
   - if stat() failes for at least one of the files 
     (It could be that both pathes direct to the same 
      file not existing)

RETURN
      0 files are identically
      1 files are not identically

*/
int filecmp(
const char *name0,
const char *name1 
) {
   SGE_STRUCT_STAT buf0, buf1;

   DENTER(TOP_LAYER, "filecmp");

   if (!strcmp(name0, name1)) {
      DEXIT;
      return 0;
   }

   if (SGE_STAT(name0, &buf0)<0) {
      DEXIT;
      return 1;
   }

   if (SGE_STAT(name1, &buf1)<0) {
      DEXIT;
      return 1;
   }

   if (buf0.st_ino == buf1.st_ino && buf0.st_dev == buf1.st_dev) {
      DEXIT;
      return 0;
   } else {
      DEXIT;
      return 1;
   }
}
