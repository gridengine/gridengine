#ifndef __RMON_CONF_H
#define __RMON_CONF_H
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



#define RMON_ROOT                                               "../rmon/"
#define RMON_CONFIG_FILE                                "rmon_config"
#define RMON_RESTART_SPY_FILE                   "rmon_restart_spy"
#define RMON_RESTART_SPY_FILE_LOCK              "rmon_restart_spy_lock"

#define MAX_MESSAGES    1

#define RESERVED_PORT   0
#define RMOND_SERVICE   "rmond"
#define SPY_BASE_PORT   6543

#define ALARMS      3           /*   short timeout */
#define ALARM       4           /*   medium timeout */
#define ALARML      5           /*   long timeout */

void rmon_read_configuration(void);
int rmon_save_conf_file(void);
u_long rmon_make_addr(char *host);
int rmon_make_name(char *host, char *name);

#endif /* __RMON_CONF_H */



