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
#include <stdlib.h>
#include <time.h>

#include "sge_unistd.h"
#include "sge_gdi_intern.h"
#include "sge_all_listsL.h"
#include "usage.h"
#include "sig_handlers.h"
#include "commlib.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"
#include "msg_clients_common.h"
#include "sge_c_event.h"
#include "sge_event.h"

extern char **environ;

int main(int argc, char *argv[]);

static void dump_eventlist(lList *event_list)
{
   lListElem *event;
   for_each(event, event_list) {
      fprintf(stdout, event_text(event));
#if 0 /* EB: debug */
      lWriteElemTo(event, stdout); 
#endif
      switch(lGetUlong(event, ET_type)) {
         case sgeE_SHUTDOWN:
            shut_me_down = 1;
            break;
         case sgeE_QMASTER_GOES_DOWN:
            sleep(8);
            ec_mark4registration();
            break;
         default:
            break;
      }
   }
}

#define TEST

/************************************************************************/
int main(int argc, char **argv)
{
   int cl_err = 0;
   int ret;
   time_t last_heared = 0;

   DENTER_MAIN(TOP_LAYER, "qevent");

   sge_gdi_param(SET_MEWHO, QEVENT, NULL);
   if ((cl_err = sge_gdi_setup(prognames[QEVENT]))) {
      ERROR((SGE_EVENT, MSG_GDI_SGE_SETUP_FAILED_S, cl_errstr(cl_err)));
      SGE_EXIT(1);
   }

   sge_setup_sig_handlers(QEVENT);

   if (reresolve_me_qualified_hostname() != CL_OK) {
      SGE_EXIT(1);
   }   

   ec_prepare_registration(EV_ID_ANY, "qevent");
   
#ifndef TEST   
   ec_subscribe_all();
#endif

   while(!shut_me_down) {
      time_t now = time(0);
      
      lList *event_list = NULL;
      ret = ec_get(&event_list);

      if(ret == 0) {
         last_heared = now;
      } else {
         if(last_heared != 0 && (now > last_heared + ec_get_edtime() * 10)) {
            WARNING((SGE_EVENT, "qmaster alive timeout expired\n"));
            ec_mark4registration();
         }
      }
      
      if(event_list != NULL) {
         dump_eventlist(event_list);
         lFreeList(event_list);
      }

#ifdef TEST
      {
         static int event = sgeE_ALL_EVENTS + 1;
         static int subscribe = 1;
         if(subscribe) {
            if(event < sgeE_EVENTSIZE) {
               ec_subscribe(event++);
            } else {
               subscribe = 0;
               event--;
            }
         } else {
            if(event > sgeE_ALL_EVENTS) {
               ec_unsubscribe(event--);
            } else {
               subscribe = 1;
               event++;
            }   
         }
      }

      {
         if(ec_get_edtime() == DEFAULT_EVENT_DELIVERY_INTERVAL) {
            ec_set_edtime(DEFAULT_EVENT_DELIVERY_INTERVAL * 2);
         } else {
            ec_set_edtime(DEFAULT_EVENT_DELIVERY_INTERVAL);
         }
      }
#endif
      
   }

   ec_deregister();

   SGE_EXIT(0);
   return 0;
}
