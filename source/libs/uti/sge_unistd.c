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

#include <string.h>
#include <errno.h>
#include <sys/time.h> 

#include "sge_unistd.h"
#include "def.h"
#include "sgermon.h"
#include "sge_log.h"
#include "basis_types.h"
#include "msg_utilib.h"

/****** uti/unistd/sge_unlink() ***********************************************
*  NAME
*     sge_unlink() -- delete a name and possibly the file it refers to
*
*  SYNOPSIS
*     int sge_unlink(const char *prefix, const char *suffix) 
*
*  FUNCTION
*     Replacement for unlink(). 'prefix' and 'suffix' will be combined
*     to a filename. This file will be deleted. 'prefix' may be NULL.
*
*  INPUTS
*     const char *prefix - pathname or NULL
*     const char *suffix - filename 
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - Error
******************************************************************************/
int sge_unlink(const char *prefix, const char *suffix) 
{
   int status;
   stringT str;
 
   DENTER(TOP_LAYER, "sge_unlink");
 
   if (!suffix) {
      ERROR((SGE_EVENT, MSG_POINTER_SUFFIXISNULLINSGEUNLINK ));
      DEXIT;
      return -1;
   }
 
   if (prefix) {
      sprintf(str, "%s/%s", prefix, suffix);
   } else {
      sprintf(str, "%s", suffix);
   }
 
   DPRINTF(("file to unlink: \"%s\"\n", str));
   status = unlink(str);
 
   if (status) {
      ERROR((SGE_EVENT, "ERROR: "SFN"\n", strerror(errno)));
      DEXIT;
      return -1;
   } else {
      DEXIT;
      return 0;
   }
}  

/****** uti/unistd/sge_sleep() ************************************************
*  NAME
*     sge_sleep() -- sleep for x microseconds 
*
*  SYNOPSIS
*     void sge_sleep(int sec, int usec) 
*
*  FUNCTION
*     Delays the calling application for 'sec' seconds and 'usec'
*     microseconds 
*
*  INPUTS
*     int sec  - seconds 
*     int usec - microseconds 
******************************************************************************/
void sge_sleep(int sec, int usec) 
{
   static struct timeval timeout;
 
   timeout.tv_sec = sec;
   timeout.tv_usec = usec;
 
#if !(defined(HPUX) || defined(HP10_01) || defined(HPCONVEX))
   select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &timeout);
#else
   select(0, (int *) 0, (int *) 0, (int *) 0, &timeout);
#endif
}       
