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
/****************************************************************
 Module containing code for last heard functionality of commlib.

 The idea is, that a commproc can ask the commlib for the last
 successful receive or the last successful synchron send to another
 commproc. This information can be used for assuming a communication
 partner dead or alive without explicitly asking him all the time.
 ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "commlib.h"
#include "commlib_util.h"
#include "sge_time.h"
#include "msg_commd.h"

extern int commlib_debug;

static void new_entry(char *commproc, u_short id, char *host, u_long time);
static entry *find_entry(char *commproc, u_short id, char *host, u_long now);

static int set_last_heard_from_(char *commproc, u_short id, char *host, u_long time);
static u_long last_heard_from_(char *commproc, u_short *id, char *host);

/*****************************************************************
 ask about last alive sign of other commproc *without* doing
 communication. The behavior of this function is influenced by the
 parameter CL_P_LT_HEARD_FROM_TIMEOUT. Information older than this
 time is dropped. It defaults to 5min.

 commproc
 id
 host           specify commproc we ask for

 as known from other commlib calls we allow wildcard arguments and
 fill arguments with actual values

 return value

 0 = do not know anything about this commproc
 else
    time (seconds since epoch) we've last heard from this commproc
 *****************************************************************/
u_long last_heard_from(
char *commproc,
u_short *id,
char *host 
) {
   u_long time;

   time = last_heard_from_(commproc, id, host);
   if (commlib_debug) {
      printf(MSG_COMMLIB_LAST_HEARD_USIS ,
         u32c (time), commproc ? commproc : "", (int) (id ? *id : 0) , host ? host : "");
   }
   return time;
}

static u_long last_heard_from_(
char *commproc,
u_short *id,
char *host 
) {
   entry *ptr = NULL;

   if (commproc)
      if (secure_strlen(commproc, MAXCOMPONENTLEN) >= MAXCOMPONENTLEN)
         return CL_RANGE;

   if (id)
      if ((*id < MINID || *id > MAXID) && *id != 0)
         return CL_RANGE;

   if (host)
      if (secure_strlen(host, MAXHOSTLEN) >= MAXHOSTLEN)
         return CL_RANGE;

#ifndef WIN32NATIVE
   if (!(ptr = find_entry(commproc, id ? *id : 0, host, 0)))
#else 
   if (!(ptr = find_entry(commproc, (u_short)(id ? *id : 0), host, 0)))
#endif 
      return 0;


   if (commproc && commproc[0] == '\0') {
      if (ptr->commproc)
         strcpy(commproc, ptr->commproc);
      else
         commproc[0] = '\0';
   }      

   if (id && *id == 0)
      *id = ptr->id;
   if (host && host[0] == '\0') {
      if (ptr->host)
         strcpy(host, ptr->host);
      else
         host[0] = '\0';
   }

       
 
   return ptr->time;
}

/*static u_long lastgc = 0;*/

/****************************************************************
 call to set information returned by last_heard_from.

 This function is used inside the commlib to set information.
 It can be used from outside the commlib to overrule this information.
 This is neccesary, because other components may have additional
 information on the status of a commproc.

 We store only complete information. Wildcards are dropped.

 Main application from outside commlib may be to reset a commproc to
 unheard.

 ****************************************************************/
int set_last_heard_from(char *commproc, u_short id, char *host, u_long time)
{
   int ret;

   ret = set_last_heard_from_(commproc, id, host, time);

   if (commlib_debug) {
      printf(MSG_COMMLIB_SET_LAST_HEARD_ISIU ,
             ret, commproc ? commproc : "", (int) id, host ? host : "", u32c (time));
   }
   return ret;
}

static int set_last_heard_from_(char *commproc, u_short id, char *host, u_long time)
{
   /* throw away old stuff every 5 minutes (if we are called) */
   if (time && (time - get_commlib_state_lastgc() > 5 * 60)) {
      find_entry("never_use_this_one", 99, "never_use_this_one", time);
      set_commlib_state_lastgc(time);
   }

   if (!commproc || commproc[0] == '\0' ||
       !id ||
       !host || host[0] == '\0')
      return CL_OK;

   if (secure_strlen(commproc, MAXCOMPONENTLEN) >= MAXCOMPONENTLEN)
      return CL_RANGE;

   if ((id < MINID || id > MAXID) && id != 0)
      return CL_RANGE;

   if (secure_strlen(host, MAXHOSTLEN) >= MAXHOSTLEN)
      return CL_RANGE;

   new_entry(commproc, id, host, time);
   return CL_OK;
}

/****************************************************************
 code handling the last heard from information list
 ****************************************************************/
/* static entry *list;*/

/*************************************************************
 find_entry() searches for an entry in the last heard from list.
 He throws away old stuff.
 *************************************************************/
static entry *find_entry(char *commproc, u_short id, char *host,
                         u_long now)
{
   entry *ptr = NULL, *last = NULL;
   ptr = get_commlib_state_list();

   if (!now)
      now = sge_get_gmt();

   while (ptr) {

#ifdef WIN32                    /* int cast */
      if ((int) (now - ptr->time) > get_commlib_state_lt_heard_from_timeout()) {
#else
      if (now - ptr->time > get_commlib_state_lt_heard_from_timeout()) {
#endif

         if (commlib_debug) {
            printf(MSG_COMMLIB_DROPPING_SISUU ,
                   ptr->commproc, (int) ptr->id, ptr->host, u32c (ptr->time), u32c(now));
         }

         /* to old -> drop */
         free(ptr->commproc);
         free(ptr->host);
         if (last) {
            last->next = ptr->next;
            free(ptr);
            ptr = last->next;
            continue;
         }
         else {
            set_commlib_state_list(ptr->next);
            free(ptr);
            ptr = get_commlib_state_list();
            continue;
         }
      }

      if ((commproc && commproc[0] != '\0') &&
          strcmp(commproc, ptr->commproc)) {
         last = ptr;
         ptr = ptr->next;
         continue;
      }

      if (id && id != ptr->id) {
         last = ptr;
         ptr = ptr->next;
         continue;
      }

      if ((host && host[0] != '\0') && 
#ifndef WIN32NATIVE
         strcasecmp(host, ptr->host)
#else 
         stricmp(host, ptr->host)
#endif 
         ) {
         last = ptr;
         ptr = ptr->next;
         continue;
      }

      return ptr;
   }
   return NULL;
}

/***************************************/
static void new_entry(char *commproc, u_short id, char *host,
                      u_long time)
{
   entry *new;

   /* first search for this commproc */
   new = find_entry(commproc, id, host, time);
   if (new) {
      new->time = time;
      return;
   }

   if (!(new = (entry *) malloc(sizeof(entry))))
      return;

   memset(new, 0, sizeof(entry));

   if (commproc) {
      if (!(new->commproc = strdup(commproc))) {
         free(new);
         return;
      }
   }
   if (host) {
      if (!(new->host = strdup(host))) {
         if (commproc)
            free(new->commproc);
         free(new);
         return;
      }
   }
   new->id = id;
   new->time = time;
   new->next = get_commlib_state_list();
   set_commlib_state_list(new);
}

/*************************************************************
 reset_last_heard() removes all entries in the last heard from list.
 Returns number of deleted entries.
 *************************************************************/
int reset_last_heard()
{
   entry *ptr;
   int n = 0;

   /* eat all */
   while ((ptr = get_commlib_state_list())) {
      if (commlib_debug)
         printf(MSG_COMMLIB_RESET_LAST_HEARD_SISU 
            , ptr->commproc, (int) ptr->id, ptr->host, u32c(ptr->time));

      /* to old -> drop */
      free(ptr->commproc);
      free(ptr->host);

      set_commlib_state_list(ptr->next);
      free(ptr);
      n++;
   }

   return n;
}
