#ifndef __MSG_SPOOLLIB_TEMPLATE_H
#define __MSG_SPOOLLIB_TEMPLATE_H
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
 * libs/spool/postgres/sge_spooling_postgres.c
 */
#define MSG_POSTGRES_INVALIDARGSTOCREATESPOOLINGCONTEXT  _MESSAGE(69000,_("invalid arguments passed to creation of postgres spooling context\n"))
#define MSG_POSTGRES_OPENFAILED_SS  _MESSAGE(69001, _("connecting to PostgreSQL Database ("SFN") failed: "SFN"\n"))
#define MSG_POSTGRES_OPENSUCCEEDED_S  _MESSAGE(69002, _("connected to PostgreSQL Database ("SFN")\n"))
#define MSG_POSTGRES_NOCONNECTIONTOCLOSE_S  _MESSAGE(69003, _("no database connection to close ("SFN")\n"))
#define MSG_POSTGRES_CLOSEDCONNECTION_S   _MESSAGE(69004, _("closed database connection ("SFN"\n"))
#define MSG_POSTGRES_CANNOTREADDBINFO_S   _MESSAGE(69005, _("cannot read initial data from database: "SFN"\n"))

#define MSG_POSTGRES_HISTORYDISABLED      _MESSAGE(69007, _("spooling without historical data\n"))
#define MSG_POSTGRES_HISTORYENABLED       _MESSAGE(69008, _("spooling with historical data\n"))
#define MSG_POSTGRES_COMMANDFAILED_SS     _MESSAGE(69009, _("failed executing sql command "SFQ": "SFN"\n"))

#endif /* __MSG_SPOOLLIB_TEMPLATE_H */
