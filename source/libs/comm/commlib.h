#ifndef __COMMLIB_H
#define __COMMLIB_H
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

#include <syslog.h> 
#include <sys/types.h>

#ifdef WIN32NATIVE
#	include "win32nativetypes.h"
//#   undef LONG
//#   undef ULONG
#	include <winsock2.h>
//#   define LONG(name)
//#   define ULONG(name)
#else 
#	include <netinet/in.h>
#endif



#ifdef  __cplusplus
extern "C" {
#endif

#include "commd.h"
#include "commd_message_flags.h"
#include "pack.h"

#define SGE_COMMD_SERVICE   "sge_commd"

#define MAXTAG 0xffff

/* time we maximal wait in an synchronuous message receive and send */
#define TIMEOUT_SYNC_RCV (20)
#define TIMEOUT_SYNC_SND (60)

/* error codes returned by commlib functions 
   range ...-0xff correspond to acknowledges */
#define CL_OK               0
#define CL_RANGE            1
#define CL_CREATESOCKET     2
#define CL_RESOLVE          3
#define CL_CONNECT          4
#define CL_WRITE            5
#define CL_ALREADYDONE      6
#define CL_LOCALHOSTNAME    7
#define CL_NOTENROLLED      8
#define CL_SERVICE          9
#define CL_READ             10
#define CL_MALLOC           11
#define CL_UNKNOWN_PARAM    12
#define CL_INTR             13
#define CL_READ_TIMEOUT     14
#define CL_WRITE_TIMEOUT    15
#define CL_CHKSUM           16
#define CL_RRESVPORT        17 
#define SEC_SEND_FAILED     18
#define SEC_RECEIVE_FAILED  19
#define SEC_ANNOUNCE_FAILED 20 


#define CL_FIRST_FREE_EC    32
#define CL_LAST_FREE_EC     80

/* Parameters for set_commlib_param */
#define CL_P_RESERVED_PORT         1
#define CL_P_COMMDHOST             2
#define CL_P_TIMEOUT               3
#define CL_P_TIMEOUT_SRCV          4
#define CL_P_COMMDPORT             5
#define CL_P_OFFLINE_RECEIVE       6
#define CL_P_LT_HEARD_FROM_TIMEOUT 7
#define CL_P_TIMEOUT_SSND          8
#define CL_P_CLOSE_FD              9
#define CL_P_COMMDSERVICE         10
#define CL_P_NAME                 11
#define CL_P_ID                   13
#define CL_P_PRIO_LIST            12

typedef int (*sge_log_ftype)(int, const char*, const char*, const char*, int); 

#define COMMLIB_CRITICAL(x) \
   { \
      sge_log_ftype local_sge_log; \
      if ((local_sge_log = commlib_state_get_logging_function())) { \
         sprintf x; \
         local_sge_log(LOG_CRIT, SGE_EVENT, __FILE__,"commlib_function",__LINE__); \
      } \
   }     

#define COMMLIB_ERROR(x) \
   { \
      sge_log_ftype local_sge_log; \
      if ((local_sge_log = commlib_state_get_logging_function())) { \
         sprintf x; \
         local_sge_log(LOG_ERR, SGE_EVENT, __FILE__,"commlib_function",__LINE__); \
      } \
   } 

#define COMMLIB_WARNING(x) \
   { \
      sge_log_ftype local_sge_log; \
      if ((local_sge_log = commlib_state_get_logging_function())) { \
         sprintf x; \
         local_sge_log(LOG_WARNING, SGE_EVENT, __FILE__,"commlib_function",__LINE__); \
      } \
   }     

#define COMMLIB_NOTICE(x) \
   { \
      sge_log_ftype local_sge_log; \
      if ((local_sge_log = commlib_state_get_logging_function())) { \
         sprintf x; \
         local_sge_log(LOG_NOTICE, SGE_EVENT, __FILE__,"commlib_function",__LINE__); \
      } \
   }     

#define COMMLIB_INFO(x) \
   { \
      sge_log_ftype local_sge_log; \
      if ((local_sge_log = commlib_state_get_logging_function())) { \
         sprintf x; \
         local_sge_log(LOG_INFO, SGE_EVENT, __FILE__,"commlib_function",__LINE__); \
      } \
   }     

#define COMMLIB_DEBUG(x) \
   { \
      sge_log_ftype local_sge_log; \
      if ((local_sge_log = commlib_state_get_logging_function())) { \
         sprintf x; \
         local_sge_log(LOG_DEBUG, SGE_EVENT, __FILE__,"commlib_function",__LINE__); \
      } \
   }     

void commlib_mt_init(void);

int set_commlib_param(int param, int intval, const char *strval, int *intval_array);

int receive_message(char *fromcommproc, u_short *fromid, char *fromhost, int *tag, char **buffer, u_long32 *buflen, int synchron, u_short *compressed);

int send_message(int synchron, const char *tocomproc, int toid, const char *tohost, int tag, char *buffer, int buflen, u_long32 *mid, int compressed);

int leave_commd(void);

int cntl(u_short cntl_operation, u_long32 *arg, char *carg);

unsigned int ask_commproc(const char *host, const char *commprocname, u_short commprocid);

const char *cl_errstr(int n);

int getuniquehostname(const char *hostin, char *hostout, int refresh_aliases);
void generate_commd_port_and_service_status_message(int commlib_error, char* buffer);

int remove_pending_messages(char *fromcommproc, u_short fromid, char *fromhost, int tag);

u_long last_heard_from(const char *commproc, u_short *id, const char *host);

int set_last_heard_from(const char *commproc, u_short id, const char *host, u_long time);

int reset_last_heard(void);

int is_commd_alive(void);

int enroll(void);

/* set this flag and commlib will generate random errors */
#if RAND_ERROR
extern int rand_error;
#endif

/* struct entry, copied from commlib_last_heard.c */
typedef struct entry {
   char *commproc;
   char *host;
   u_short id;
   u_long time;  /* time (seconds since epoch we heard last from this guy) */
   struct entry *next;
} entry;

/* struct to store ALL state information of commlib */
/* makes commlib threadsafe */
struct commlib_state_t {
   int      enrolled;
   int      ever_enrolled;
   int      stored_tag_priority_list[10];
   char     componentname[MAXCOMPONENTLEN + 1];
   u_short  componentid;
   int      commdport;
   char     commdservice[32];
   int      commdaddr_length;
   struct in_addr commdaddr;
   int      sfd;
   u_long   lastmid;
   u_long   lastgc;
   int      reserved_port;         /* default: dont use reserved ports */
   char     commdhost[MAXHOSTLEN]; /*default: use local or $COMMD_HOST host */
   int      timeout;               /* Time we wait for the delivery acknowledge,
                                   if this time is exceeded we asume message
                                   lost */
   int      timeout_srcv;          /* Time we maximal wait in a
                                   synchron receive */
   int      timeout_ssnd;          /* Time we maximal wait in a
                                   synchron send */
   int      offline_receive;       /* close socket connection while waiting
                                   for a message */
   int      lt_heard_from_timeout;
   int      closefd;
   entry*   list;
   sge_log_ftype sge_log;           /* function which will be user in 
                                       COMMLIB_ERROR macro                */
   int      changed_flag;           /* 1 if reenroll is necessary before 
                                       next connection                    */
};

/* access functions for this struct */
int commlib_state_get_enrolled(void);
int commlib_state_get_ever_enrolled(void);
int* commlib_state_get_addr_stored_tag_priority_list(void);
int commlib_state_get_stored_tag_priority_list_i(int i);
char* commlib_state_get_componentname(void);
u_short commlib_state_get_componentid(void);
u_short* commlib_state_get_addr_componentid(void);
int commlib_state_get_commdport(void);
char* commlib_state_get_commdservice(void);
int commlib_state_get_commdaddr_length(void);
struct in_addr* commlib_state_get_addr_commdaddr(void);
int commlib_state_get_sfd(void);
u_long commlib_state_get_lastmid(void);
u_long commlib_state_get_lastgc(void);
int commlib_state_get_reserved_port(void);
char* commlib_state_get_commdhost(void);
int commlib_state_get_timeout(void);
int commlib_state_get_timeout_srcv(void);
int commlib_state_get_timeout_ssnd(void);
int commlib_state_get_offline_receive(void);
int commlib_state_get_lt_heard_from_timeout(void);
int commlib_state_get_closefd(void);
entry* commlib_state_get_list(void);
sge_log_ftype commlib_state_get_logging_function(void);
int commlib_state_get_changed_flag(void);

void commlib_state_set_enrolled(int state);
void commlib_state_set_ever_enrolled(int state);
void commlib_state_set_stored_tag_priority_list(int *state);
void commlib_state_set_componentname(const char *state);
void commlib_state_set_componentid(u_short state);
void commlib_state_set_commdport(int state);
void commlib_state_set_commdservice(const char *state);
void commlib_state_set_commdaddr_length(int state);
void commlib_state_set_sfd(int state);
void commlib_state_set_lastmid(u_long state);
void commlib_state_set_lastgc(u_long state);
void commlib_state_set_reserved_port(int state);
void commlib_state_set_commdhost(const char *state);
void commlib_state_set_timeout(int state);
void commlib_state_set_timeout_srcv(int state);
void commlib_state_set_timeout_ssnd(int state);
void commlib_state_set_offline_receive(int state);
void commlib_state_set_lt_heard_from_timeout(int state);
void commlib_state_set_closefd(int state);
void commlib_state_set_list(entry *state);
void inc_commlib_state_lastmid(void);
void clear_commlib_state_stored_tag_priority_list(void);
void commlib_state_set_logging_function(sge_log_ftype);
void commlib_state_set_changed_flag(int flag);

#ifdef  __cplusplus
}
#endif

#endif /* __COMMLIB_H */

