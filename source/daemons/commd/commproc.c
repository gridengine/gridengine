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
#include <stdlib.h>

#include "commproc.h"
#include "sge_time.h"
#include "commd_error.h"
#include "debug_malloc.h"
#include "sgermon.h"
#include "sge_log.h"
#include "msg_commd.h"
#include "message.h"
#include "rwfd.h"


extern void reset_messages_for_commproc(commproc *commp);
extern void trace(char *);

/* list of all known (local registered) communication processes */
commproc *commprocs = NULL;

/* Timeout value of commproc structures. If a commproc is not active for
   a longer time he is considered dead. */
static u_long commproc_timeout = COMMPROC_TIMEOUT;

/* search for a commproc in list of all known commprocs who matches the 
   address */
commproc *match_commproc(
commproc *to 
) {
   commproc *commp = commprocs;

   while (commp) {
      if (match2commprocs(to, commp))
         return commp;
      commp = commp->next;
   }
   return NULL;
}

/**********************************************************************
  match 2 commproc descriptions whether they could match the same commproc 
  return 0 -> no match
         1 -> matching 
 **********************************************************************/
int match2commprocs(
commproc *a,
commproc *b 
) {
   /* id == 0 -> match all commprocids */
   if (a->id && b->id && (a->id != b->id))
      return 0;

   /* host == NULL -> match all hosts */
   if (a->host && b->host && a->host != b->host)
      return 0;

   /* commprocname == "" matches any commprocname */
   if (a->name[0] && b->name[0] && strcmp(a->name, b->name))
      return 0;

   return 1;                    /* match ok */
}

/**********************************************************************
  make a new entry into the commproc list
 **********************************************************************/
commproc *add_commproc(
int fd,
int res_port,
struct in_addr *addr 
) {
   commproc *new;

   DENTER(TOP_LAYER, "add_commproc");

   new = (commproc *) malloc(sizeof(commproc));
   if (!new) {
      DEXIT;
      return NULL;
   }
   memset(new, '\0', sizeof(commproc));
   new->fd = fd;
   if(fd != -1) {
      new->reserved_port = res_port;
      if(!addr) {
         free(new);
         DEXIT; 
         return NULL;
      }
      memcpy(&new->fromaddr, addr, sizeof(struct in_addr));
   }
   new->w_fd = -1;              /* 0 may be a used fd */
   new->lastaction = sge_get_gmt(); /* needed for timeouts */
   new->next = commprocs;
   commprocs = new;

   DEXIT; 
   return new;
}

/**********************************************************************
  delete entry from the commproc list
 **********************************************************************/
void delete_commproc(
commproc *commp 
) {
   commproc *cop = commprocs, *last = NULL;

   DENTER(TOP_LAYER, "delete_commproc");

   if (!commp)
      return;

   while (cop && cop != commp) {
      last = cop;
      cop = cop->next;
   }

   if (cop) {
      if (last)
         last->next = cop->next;
      else
         commprocs = cop->next;
      if(cop->fd != -1) {
         shutdown(cop->fd, 2);
         close(cop->fd);
         DEBUG((SGE_EVENT, "closing fd=%d", cop->fd));
      }
      free(cop);
   }

   DEXIT;
}


/**********************************************************************
  a commproc closed the socket to indicate a receive timeout
 **********************************************************************/
void disconnect_commproc_using_fd(
int fd 
) {
   commproc *cop = commprocs;

   DENTER(TOP_LAYER, "disconnect_commproc_using_fd");

   if (!(cop = search_commproc_using_fd(fd))) {
      DEBUG((SGE_EVENT, "found no commproc on fd=%d to be disconnected", fd));
      DEXIT;
      return;
   }

   DEBUG((SGE_EVENT, "receive timeout disconnects commproc %s by closing fd=%d", 
      cop->name, fd));
   reset_messages_for_commproc(cop);
   shutdown(cop->fd, 2);
   close(cop->fd);
   cop->fd = -1;

   if (cop->w_fd != -1) {
      if (cop->w_fd != fd)
         fd_close(cop->w_fd, "disconnecting");
      cop->w_fd = -1;
   }

   DEXIT;
   return;
}



/**********************************************************************
  got a new connection to an already registered commproc
 **********************************************************************/
int reconnect_commproc_with_fd(
commproc *cop,
int fd,
char *str 
) {
   DENTER(TOP_LAYER, "reconnect_commproc_with_fd");

   if (cop->fd != -1) {
      disconnect_commproc_using_fd(cop->fd); 
      if (cop->fd != -1) {
         ERROR((SGE_EVENT, "commproc to be reconnected via %s fd=%d is still connected to fd=%d", str, fd, cop->fd));

         DEXIT;
         return -1;
      }
   }

   DEBUG((SGE_EVENT, "reconnecting commproc %s %s %d with fd=%d",
      cop->name, cop->host->mainname, cop->id, fd));

   cop->fd = fd;

   DEXIT;
   return 0;
}

/**********************************************************************
  delete entry from the commproc list with w_fd == fd
 **********************************************************************/
void delete_commproc_waiting_on_fd(
int fd,
char *name 
) {
   commproc *cop = commprocs;

   DENTER(TOP_LAYER, "delete_commproc_waiting_on_fd");

   if (!(cop = search_commproc_waiting_on_fd(fd))) {
      DEXIT;
      return;
   }

   strcpy(name, cop->name);  /* return information on removed commp */
   delete_commproc(cop);
   DEXIT;
   return;
}

/**********************************************************************
  delete entry from the commproc list with cop->fd == fd
  returns TRUE if there was a commproc, FALSE otherwise
 **********************************************************************/
int delete_commproc_using_fd(
int fd 
) {
   commproc *cop = commprocs;

   DENTER(TOP_LAYER, "delete_commproc_using_fd");

   if (!(cop = search_commproc_using_fd(fd))) {
      DEXIT;
      return 0;
   }

   reset_messages_for_commproc(cop);
   delete_commproc(cop);
   DEXIT;
   return 1;
}

/**********************************************************************
 search for unused commproc id and set new commproc to this id
 **********************************************************************/
int setcommprocid(commproc *new, u_short startvalue)
{
   commproc *commp;
   static u_short nextid = 1;

   if (startvalue)
      nextid = startvalue;
   else
      startvalue = nextid;

   while (1) {
      commp = commprocs;
      while (commp) {
         if (!strcasecmp(new->name, commp->name) &&
             new->host == commp->host &&
             nextid == commp->id) {
            nextid++;
            nextid = nextid ? nextid : 1;
            break;
         }
         commp = commp->next;
      }
      if (!commp) {
         new->id = nextid;
         nextid++;
         nextid = nextid ? nextid : 1;
         return 0;
      }
      if (nextid == startvalue) {       /* big s. we ran out of ids */
         new->id = 0;
         return 1;
      }
   }
}

/**********************************************************************/
commproc *search_commproc(char *h, char* name, u_short id)
{
   commproc *commp = commprocs;

   while (commp) {
      if ((!h || search_host(h, NULL) == commp->host) &&
          !strcmp(name, commp->name) &&
          (!id || id == commp->id))
         return commp;

      commp = commp->next;
   }
   return NULL;
}

/**********************************************************************
 look for commproc waiting on fd
 **********************************************************************/
commproc *search_commproc_waiting_on_fd(
int fd 
) {
   commproc *commp = commprocs;

   while (commp) {
      if (commp->w_fd == fd)
         return commp;
      commp = commp->next;
   }
   return NULL;
}

/**********************************************************************
 look for commproc using fd
 **********************************************************************/
commproc *search_commproc_using_fd(
int fd 
) {
   commproc *commp = commprocs;

   while (commp) {
      if (commp->fd == fd)
         return commp;
      commp = commp->next;
   }
   return NULL;
}

/**************************************************/
void print_commproc(
commproc *commp,
FILE *fp 
) {
   fprintf(fp, MSG_COMMPROC_NAMEANDID_SI , commp->name, (int) commp->id);
   fprintf(fp, MSG_COMMPROC_HOST_S , commp->host ? get_mainname(commp->host) : MSG_COMMPROC_NONE );
   fprintf(fp, MSG_COMMPROC_USING_FD_I , commp->fd);
   if (commp->w_fd != -1) {
      fprintf(fp, MSG_COMMPROC_WAITING_ON_FD_I , commp->w_fd);
      fprintf(fp, MSG_COMMPROC_WAITING_FOR_COMPONENT_ID_SI ,
              commp->w_name[0] ? commp->w_name : MSG_COMMPROC_ANY, (int) commp->w_id);
      fprintf(fp, MSG_COMMPROC_ON_HOST_S ,
              commp->w_host ? get_mainname(commp->w_host) : MSG_COMMPROC_ANY);
   }
}

/**************************************************/
void print_commprocs(
FILE *fp 
) {
   commproc *commp = commprocs;

   while (commp) {
      print_commproc(commp, fp);
      if (commp->next)
         fprintf(fp, "---------------\n");
      commp = commp->next;
   }
}

/**********************************************************************
  set touched time to now
 **********************************************************************/
void commproc_touch(
commproc *commp,
u_long now 
) {
   commp->lastaction = now;
}

/**********************************************************************
  look for timed out commproc structures
  
  - If a commproc had no action for a longer time we declare him dead.

  He is declared alive:

  - If he is hanging in a read (w_fd != -1) 
  
  - He sent a request in the past and "actual time - time of request
    send < timeout"

 **********************************************************************/
void look4timeouts(
u_long now 
) {
   commproc *commp = commprocs, *next;

   DENTER(TOP_LAYER, "look4timeouts");

   while (commp) {

      /* commproc waits on a fd. We would recognize if he dies so he
         sure is alive */
      if (commp->w_fd != -1) {
         commproc_touch(commp, now);
         commp = commp->next;
         continue;
      }

      if (now - commp->lastaction > commproc_timeout) {
         ERROR((SGE_EVENT, MSG_COMMPROC_INACTIVEFOR_SIU ,
                commp->name, (int) commp->id, u32c((now - commp->lastaction)) ));
         next = commp->next;
         delete_commproc(commp);
         commp = next;
         continue;
      }
      commp = commp->next;
   }

   DEXIT;
}
