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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#ifndef WIN32NATIVE
#	include <unistd.h>
#	include <sys/time.h>
#	include <sys/errno.h>
#else 
#	include <winsock2.h>
#	include "DebugC.h"
#endif


#if defined(AIX32) || defined(AIX41)
#   include <sys/select.h>
#endif

#include "sge_time.h"
#include "commd_io.h"
#include "sgermon.h"

#ifdef COMMLIB_ENABLE_DEBUG
int stored_errno = 0;
#endif

#ifndef MIN
#   define MIN(a,b) (((a)<(b))?(a):(b))       
#endif

/***********************************************************************/
/* (un)pack routines for communication over network                    */
/*                                                                     */
/* each routine gets a character pointer to a buffer to put stuff into */
/* or get stuff out and returns the incremented character pointer      */
/*                                                                     */
/* Data types:                                                         */
/*                                                                     */
/* ushort = 2 byte unsigned integer                                    */
/* ulong  = 4 byte unsigned integer                                    */
/* string = '0' terminated string (unlimited length)                   */
/***********************************************************************/
unsigned char *pack_ushort(u_short us, unsigned char *cp)
{
   *cp++ = (us >> 8) & 0xff;
   *cp++ = us & 0xff;
   return cp;
}

int pack_ushort_len(u_short us)
{
   return 2;
}

unsigned char *unpack_ushort(u_short *us, unsigned char *cp) 
{
   *us = (*cp++ << 8);
   (*us) += *cp++;
   return cp;
}

unsigned char *pack_ulong(u_long32 ul, unsigned char *cp) 
{
#ifndef WIN32NATIVE
   *cp++ = (ul >> 24) & 0xff;
   *cp++ = (ul >> 16) & 0xff;
   *cp++ = (ul >> 8) & 0xff;
   *cp++ = ul & 0xff;
#else 
   *cp++ = (unsigned char)(ul >> 24) & 0xff;
   *cp++ = (unsigned char)(ul >> 16) & 0xff;
   *cp++ = (unsigned char)(ul >> 8) & 0xff;
   *cp++ = (unsigned char)ul & 0xff;
#endif 
   return cp;
}

int pack_ulong_len(u_long ul) 
{
   return 4;
}

unsigned char *unpack_ulong(u_long32 *ul, unsigned char *cp) 
{
   *ul = *cp++ << 24;
   (*ul) += *cp++ << 16;
   (*ul) += *cp++ << 8;
   (*ul) += *cp++;
   return cp;
}

unsigned char *pack_string(const char *str, unsigned char *cp) 
{
   while (*str) {
      *cp++ = *str++;
   }
   *cp++ = '\0';
   return cp;
}

int pack_string_len(const char *str) 
{
   return strlen(str) + 1;
}

unsigned char *unpack_string(char *str, int len, unsigned char *cp) 
{
   while (len && *cp) {
      *str++ = *cp++;
      len--;
   }

   while (*cp)
      cp++;

   if (len)
      *str++ = '\0';
   else
      *(str - 1) = '\0';

   return cp + 1;
}

/**********************************************************************
  Read n bytes from a non blocking pipe.
  If n == -1 wait until s.th. comes in and then return immediately. Read
  at most 256 chars.

  This function must not need longer than 2*timeout seconds.
  If nothing comes in we only need timeout seconds.
  Why 2*timeout? Somebody does a 20s receive and just after 19 he gets
  a message. If we use timeout there is 1s left for receiving the message.

  Returns
     unread bytes or (0=all read) / (read bytes in case of n==-1)
     -1 if error or
     -2 on timeout
     -3 interrupted by signal

 **********************************************************************/
int readnbytes_nb(int sfd, char *ptr, int n, int timeout) 
{
   fd_set readfds;
   int res, j, errorcode, first, to;
   struct timeval timeleft;

   DENTER(COMMD_LAYER, "readnbytes_nb");

#ifdef COMMLIB_ENABLE_DEBUG
   stored_errno = 0;
#endif

   FD_ZERO(&readfds);
#ifndef WIN32NATIVE
   FD_SET(sfd, &readfds);
#else 
   FD_SET((u_int)sfd, &readfds);
#endif 

   first = 1;
   to = timeout;

   while (to > 0 && (n > 0 || n == -1)) {

      if (first)
         first = 0;
      else
         to = MAX(15, timeout);
   
      timeleft.tv_sec = to;
      timeleft.tv_usec = 0;

      res = select(FD_SETSIZE, &readfds, NULL, NULL, &timeleft);

#ifndef WIN32NATIVE
      errorcode = errno;
#else 
	  errorcode = WSAGetLastError();
#endif 

#ifdef COMMLIB_ENABLE_DEBUG
      stored_errno = errno;
#endif

      DPRINTF(("select returned %d errno=%s\n", res, strerror(errno)));

      if (res == -1) {
#ifndef WIN32NATIVE
         if (errorcode == EINTR)
#else 
         if (errorcode == WSAEINTR) 
#endif 
         {
            DEXIT;
            return -3;       /* interupted by signal */
         } 
         else {
            DEXIT;
            return -1;       /* interupted by other error */
         }   
      } 
      else if (res == 0) {   /* timeout in select */
         DEXIT;
         return -2;
      }


#ifndef WIN32NATIVE
      if (n == -1)
         j = read(sfd, ptr, 256);
      else
         j = read(sfd, ptr, n);

      errorcode = errno;
      	
      DPRINTF(("read returned %d %s\n", j, strerror(errno)));
#else /* WIN32NATIVE */
      if (n == -1)
         j = recv(sfd, ptr, 256, 0);
      else
         j = recv(sfd, ptr, n, 0);

      errorcode = WSAGetLastError();
      	
      DPRINTF(("recv returned %d\n", j));
#endif /* WIN32NATIVE */

#ifdef COMMLIB_ENABLE_DEBUG
      stored_errno = errno;
#endif

      if (n == -1) {         /* caller wants to get chunk immediately */
         DEXIT;
         return j;           /* return read bytes or -1 if error */
      }
      else if (j == 0) {     /* EOF on pipe - sender has closed connection */
         DEXIT;         
         return n;           /* return unread bytes */
      }
#ifndef WIN32NATIVE
      else if (j == -1 && !(errorcode == EWOULDBLOCK || errorcode == EAGAIN))
#else 
      else if (j == -1 && !(errorcode == WSAEWOULDBLOCK)) 
#endif 
      {
         DEXIT;
         return -1;          /* read error */
      }      

      if (j != -1) {         /* read error EWOULDBLOCK, EAGAIN */
         n -= j;
         ptr += j;
      }
   }

   if (n == 0) {
      DPRINTF(("all read\n"));
      DEXIT;
      return 0;
   }   
   else {
      DPRINTF(("timout\n"));
      DEXIT;
      return -2;
   }      
}

/*---------------------------------------------------------------------
 * writenbytes_nb
 *  Write n bytes to a non blocking pipe.
 *
 * Returns
 *    unwritten bytes or 0 = all written
 *    -1 select or read error
 *    -2 timeout
 *    -3 select interupted by signal
 *    -4 reading end of fd was closed
 *
 *--------------------------------------------------------------------*/
int writenbytes_nb(int sfd, const char *ptr, int n, int timeout) 
{
   fd_set writefds;
   int res, j, errorcode, to, first;
   struct timeval timeleft;

   DENTER(COMMD_LAYER, "writenbytes_nb");

   FD_ZERO(&writefds);

#ifndef WIN32NATIVE
   FD_SET(sfd, &writefds);
#else
   FD_SET((u_int)sfd, &writefds);
#endif

   first = 1;
   to = timeout;

   while (to > 0 && n > 0) {

   if (first)
      first = 0;
   else  
      to = MAX(15, timeout);
    
   timeleft.tv_sec = to;
   timeleft.tv_usec = 0;

      res = select(FD_SETSIZE, NULL, &writefds, NULL, &timeleft);

#ifndef WIN32NATIVE
      errorcode=errno;
#else 
      errorcode=WSAGetLastError();
#endif

#ifdef COMMLIB_ENABLE_DEBUG
      stored_errno = errno;
#endif 

      DPRINTF(("select returned %d errno=%s\n", res, strerror(errno)));

      if (res == -1) {
#ifndef WIN32NATIVE
         if (errorcode == EINTR)
#else 
         if (errorcode == WSAEINTR)
#endif 
         {
            DEXIT;
            return -3;       /* interupted by signal */
         } 
         else {
            DEXIT;
            return -1;       /* interupted by other error */
         }   
      } 
      else if (res == 0) {   /* timeout in select */
         DEXIT;
         return -2;
      }

#ifndef WIN32NATIVE
      j = write(sfd, ptr, n);
      errorcode = errno;

      DPRINTF(("write returned %d %s\n", j, strerror(errno)));
#else 
      j = send(sfd, ptr, n, 0);
      errorcode = WSAGetLastError();

      DPRINTF(("send returned %d\n", j));
#endif 

#ifdef COMMLIB_ENABLE_DEBUG
      stored_errno = errno;
#endif

      if (j == 0) {          /* EOF on pipe */
         DEXIT;
         return n;           /* return unwritten bytes */
      }
      else if (j == -1 && errorcode == EPIPE) {
         DEXIT;
         return -4; /* this causes COMMD_NACK_ENROLL */
      }
#ifndef WIN32NATIVE
      else if (j == -1 && !(errorcode == EWOULDBLOCK || errorcode == EAGAIN)) 
#else 
      else if (j == -1 && !(errorcode == WSAEWOULDBLOCK)) 
#endif 
      {
         DEXIT;
         return -1;          /* write error */
      } 

      if (j != -1) {         /* read error EWOULDBLOCK, EAGAIN */
         n -= j;
         ptr += j;
      }   
   }   

   if (n == 0) {
      DPRINTF(("all written\n"));
      DEXIT;
      return 0;
   }   
   else {
      DPRINTF(("timout\n"));
      DEXIT;
      return -2;
   }      
}
