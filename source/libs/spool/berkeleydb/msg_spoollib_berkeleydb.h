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
#define MSG_BERKELEY_QUERYERROR_SS _MESSAGE(70007, _("error retrieving data ("SFQ") from berkeley database: "SFN"\n"))
#define MSG_BERKELEY_DELETEERROR_SS _MESSAGE(70008, _("error deleting record with key "SFQ" from berkeley database: "SFN"\n"))
/* #define MSG_BERKELEY_COULDNTSETCACHE_SS _MESSAGE(70009, _("couldn't set cache size in berkeley database "SFN": "SFN"\n")) */
#define MSG_BERKELEY_NULLVALUEASKEY _MESSAGE(70010, _("null value given as object primary key\n"))
#define MSG_BERKELEY_TXNALREADYOPEN _MESSAGE(70017, _("cannot open new transaction: There is already one open\n"))
#define MSG_BERKELEY_ERRORSTARTINGTRANSACTION_S _MESSAGE(70018, _("error starting a transaction: "SFN"\n"))
#define MSG_BERKELEY_TXNNOTOPEN _MESSAGE(70019, _("cannot close transaction: There is no open transaction\n"))
#define MSG_BERKELEY_ABORTINGTRANSACTION _MESSAGE(70020, _("aborting transaction (rollback)\n"))
#define MSG_BERKELEY_ERRORENDINGTRANSACTION_S _MESSAGE(70021, _("error ending a transaction: "SFN"\n"))
#define MSG_BERKELEY_CANNOTRETRIEVELOGARCHIVE_S _MESSAGE(70022, _("error retrieving berkeley db log archive: "SFN"\n"))
#define MSG_BERKELEY_CANNOTCHECKPOINT_S _MESSAGE(70023, _("error checkpointing berkeley db: "SFN"\n"))
#define MSG_BERKELEY_TRANSACTIONEINVAL _MESSAGE(70024, _("invalid transaction command\n"))
#define MSG_BERKELEY_USINGBDBVERSION_S _MESSAGE(70025, _("using BerkeleyDB version "SFN"\n"))
#define MSG_BERKELEY_WRONGBDBVERSIONEXPECTING_SDD  _MESSAGE(70026, _("wrong BerkeleyDB version: Using "SFN", but exect major version = %d, minor version >= %d\n"))
#define MSG_BERKELEY_PACKERROR_SS _MESSAGE(70027, _("error packing object with key "SFQ": "SFN"\n"))
#define MSG_BERKELEY_PACKINITERROR_SS _MESSAGE(70028, _("error initializing packing buffer for object with key "SFQ": "SFN"\n"))
#define MSG_BERKELEY_UNPACKERROR_SS _MESSAGE(70029, _("error unpacking object with key "SFQ": "SFN"\n"))
#define MSG_BERKELEY_UNPACKINITERROR_SS _MESSAGE(70030, _("error initializing packing buffer while unpacking object with key "SFQ": "SFN"\n"))
#define MSG_BERKELEY_CANTSPOOLFREEELEM_S _MESSAGE(70031, _("can't spool free element with key "SFQ"\n"))

/*
 * sge_bdb.c
 */
#define MSG_BERKELEY_DATABASEDIRDOESNTEXIST_S _MESSAGE(70100, _("database directory "SFN" doesn't exist\n"))
#define MSG_BERKELEY_COULDNTCREATEENVIRONMENT_S _MESSAGE(70100, _("couldn't create database environment: "SFN"\n"))
#define MSG_BERKELEY_COULDNTOPENENVIRONMENT_SS _MESSAGE(70100, _("couldn't open database environment "SFQ": "SFN"\n"))
#define MSG_BERKELEY_COULDNTCLOSEENVIRONMENT_SS _MESSAGE(70100, _("couldn't close database environment: "SFN": "SFN"\n"))
#define MSG_BERKELEY_COULDNTESETUPLOCKDETECTION_S _MESSAGE(70100, _("couldn't setup deadlock detection: "SFN"\n"))
#define MSG_BERKELEY_COULDNTESETRPCSERVER_S _MESSAGE(70100, _("couldn't set rpc server in database environment: "SFN"\n"))

/*
 * sge_bdb_types.c
 */


#endif /* __MSG_SPOOLLIB_BERKELEYDB_H */
