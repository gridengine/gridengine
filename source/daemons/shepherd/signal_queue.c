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

#include "signal_queue.h"
#include "err_trace.h"

#define MAXSIG 100

/* ring buffer to queue signals */
static int sig_queue[MAXSIG];
static int n_sigs = 0;
static int next_sig = 0;
static int free_sig = 0; 

#define NEXT_INDEX(i) (((i+1)>MAXSIG-1)?0:(i+1))

#ifdef DEBUG
void report_signal_queue()
{
   char str[256];
   int n, i;
 
   if (n_sigs==0) {
      shepherd_trace("no signals in queue");
      return;
   }

   i=next_sig;

   for (n=n_sigs; n; n--) {
      sprintf(str, "%d. %d", i, sig_queue[i]); 
      shepherd_trace(str);
      i = NEXT_INDEX(i);
   }

   return;
}
#endif 

/* -------------------------------

   store signal in queue 

   if the buffer is full -1 is returned
   else 0 
*/
int add_signal(int signal)
{
   /* is still place in ring buffer? */
   if (n_sigs==MAXSIG)
      return 1;

   n_sigs++;

   /* put signal into ring buffer */
   sig_queue[free_sig] = signal;

   /* inc index of next free signal place */
   free_sig = NEXT_INDEX(free_sig);

   return 0;
}  

/* ----------------------------------

   get signal from queue 

   if there are no more signals 
   -1 is returned

*/
int get_signal()
{
   int signal;

   if (n_sigs==0)
      return -1;

   n_sigs--;

   signal = sig_queue[next_sig];

   /* inc index of next signal place */
   next_sig = NEXT_INDEX(next_sig);

   return signal;
}

/* ----------------------------------

   looks for given signal in queue

   returns 1 if found  
   0 if not found

*/
int pending_sig(
int sig 
) {
   int n, i;

   for (n=n_sigs, i=next_sig; n; n--, i=NEXT_INDEX(i)) {
      if (sig_queue[i]==sig)
         return 1;
   }

   return 0;
}

int get_n_sigs()
{
   return n_sigs; 
}
void clear_queued_signals()
{
   n_sigs = next_sig = free_sig = 0; 
}
