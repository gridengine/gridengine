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
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "sgermon.h"
#include "sge_io.h"

/************************************************************************/

int sge_readnbytes(
register int sfd,
register char *ptr,
register int n 

/*
   sge_readnbytes - read n bytes from file descriptor 
 */

) {

   int i;                       /* number of bytes read */
   int nleft = n;               /* number of bytes still to read */

   DENTER(BASIS_LAYER, "sge_readnbytes");
   DPRINTF(("TOTAL BYTES TO BE READ %d\n", n));

   /* Read n bytes */
   while (nleft > 0) {
      i = read(sfd, ptr, nleft);
      DPRINTF(("read %d bytes on fd %d\n", i, sfd));

      if (i < 0) {
         DPRINTF(("sge_readnbytes: returning %d\n", i));
         DEXIT;
         return (i);
      }
      else {
         if (i == 0)
            break;
      }                         /* if */
      nleft -= i;
      ptr += i;
   }                            /* while */

   DPRINTF(("sge_readnbytes: returning %d\n", nleft));
   DEXIT;
   return (n - nleft);

}                               /* sge_readnbytes */

/************************************************************************/
int sge_writenbytes(
register int sfd,
register char *ptr,
register int n 

/*
   sge_writenbytes - write n bytes to file descriptor
 */

) {

   int i;                       /* number of bytes written */
   int nleft = n;               /* number of bytes still to write */

   DENTER(BASIS_LAYER, "sge_writenbytes");

   /* Write n bytes */
   while (nleft > 0) {
      DTRACE;
      i = write(sfd, ptr, nleft);
      DPRINTF(("wrote %d bytes on fd %d\n", i, sfd));

      if (i <= 0) {
         DPRINTF(("sge_writenbytes: returning %d\n", i));
         DEXIT;
         return (i);
      }                         /* if */
      nleft -= i;
      ptr += i;
   }                            /* while */

   DEXIT;
   return (n);

}                               /* sge_writenbytes */
