#ifndef __MSG_SPOOLLIB_BERKELEYDB_H
#define __MSG_SPOOLLIB_BERKELEYDB_H
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

/* 
 * libs/spool/sge_spooling_berkeleydb.c
 */

#define MSG_BERKELEY_COULDNTCREATEDBHANDLE_S _MESSAGE(70000, _("couldn't create berkeley db database handle: "SFN"\n"))
#define MSG_BERKELEY_COULDNTOPENDB_SS _MESSAGE(70001, _("couldn't open berkeley database "SFQ": "SFN"\n"))
#define MSG_BERKELEY_COULDNTCLOSEDB_SS _MESSAGE(70002, _("couldn't close berkeley database "SFQ": "SFN"\n"))
#define MSG_BERKELEY_NOCONNECTIONOPEN_S _MESSAGE(70003, _("no connection open to berkeley database "SFQ"\n"))
#define MSG_BERKELEY_CLOSEDDB_S _MESSAGE(70004, _("closed berkeley database "SFQ"\n"))
#define MSG_BERKELEY_COULDNTCREATEDB_SS _MESSAGE(70005, _("couldn't open berkeley database "SFQ": "SFN"\n"))
#define MSG_BERKELEY_PUTERROR_SS _MESSAGE(70006, _("error writing object with key "SFQ" into berkeley database: "SFN"\n"))
#define MSG_BERKELEY_QUERYERROR_S _MESSAGE(70007, _("error retrieving data from berkeley database: "SFN"\n"))
#define MSG_BERKELEY_DELETEERROR_SS _MESSAGE(70008, _("error deleting record with key "SFQ" from berkeley database: "SFN"\n"))
#define MSG_BERKELEY_COULDNTSETCACHE_SS _MESSAGE(70009, _("couldn't set cache size in berkeley database "SFN": "SFN"\n"))

#endif /* __MSG_SPOOLLIB_BERKELEYDB_H */
