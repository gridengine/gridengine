#ifndef __QUEUEL_H
#define __QUEUEL_H
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

#include "boundaries.h"
#include "cull.h"

enum {
   Q_name = Q_LOWERBOUND,    /* THIS IS VERY IMPORTANT */
        Q_hostname,
        Q_load,
        Q_status,
        Q_ownerlist,
        Q_complexname,
        Q_ref
};


LISTDEF( QueueT )
   SGE_XSTRING   ( Q_name )
   SGE_XSTRING   ( Q_hostname )
   SGE_XINT      ( Q_load )
   SGE_XULONG    ( Q_status )
   SGE_XLIST     ( Q_ownerlist, OwnerT )
   SGE_XLIST     ( Q_complexname, ComplexNameT )
   SGE_REF       ( Q_ref )
LISTEND

NAMEDEF( QueueN )
   NAME  ( "Q_name" )
   NAME  ( "Q_hostname" )
   NAME  ( "Q_load" )
   NAME  ( "Q_status" )
   NAME  ( "Q_ownerlist" )
   NAME  ( "Q_complexname" )
   NAME  ( "Q_ref" )
NAMEEND

NAMEDEF( QmonQueueN )
   NAME  ( "Warteschlangen Name" )
   NAME  ( "Gastgeber Name" )
   NAME  ( "Last" )
   NAME  ( "Status" )
   NAME  ( "Besitzer Liste" )
   NAME  ( "Komplexe" )
   NAME  ( "Referenz" )
NAMEEND

#define QueueS sizeof(QueueN)/sizeof(char*)
#define QmonQueueS sizeof(QmonQueueN)/sizeof(char*)

#endif /* __QUEUEL_H */

