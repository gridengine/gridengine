#ifndef __SGE_ANSWERL_H
#define __SGE_ANSWERL_H

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

#include "sge_boundaries.h"
#include "cull.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* *INDENT-OFF* */

/* 
 * valid values for AN_status 
 */
enum {
   STATUS_OK = 1,            /* everything was fine */
   STATUS_ESEMANTIC,         /* semantic error */
   STATUS_EEXIST,            /* elem does not exist OR it exists for a
                              * "add" request */
   STATUS_EUNKNOWN,          /* unknown error occured */
   STATUS_ENOIMP,            /* command not implemented for target */
   STATUS_ENOKEY,            /* missing key field in case of add,del,mod */
   STATUS_ESYNTAX,           /* syntax error parsing a_source field */
   STATUS_EDENIED2HOST,      /* operation denied to this host */
   STATUS_ENOMGR,            /* operation needs manager privileges */
   STATUS_ENOOPR,            /* operation needs operator privileges */
   STATUS_NOQMASTER,         /* failed to reach sge_qmaster */
   STATUS_NOCOMMD,           /* failed to reach commd */
   STATUS_EDISK,             /* disk operation failed */
   STATUS_ENOSUCHUSER,       /* can't resolve user */
   STATUS_NOSUCHGROUP,       /* can't resolve group */
   STATUS_EMALLOC,           /* can't allocate memory */
   STATUS_ENOTOWNER,         /* need to be owner for this operation */
   STATUS_ESUBHLIC,          /* too few submit host licenses */
   STATUS_DENIED,            /* not allowed to do whatever you try */
   STATUS_EVERSION,          /* qmaster GDI version differs from clients
                              * GDI version */
   STATUS_ERROR1,            /* general error 1 */
   STATUS_ERROR2,            /* general error 2 */
   STATUS_ERROR3,            /* general error 3 */
   STATUS_OK_DOAGAIN=24,     /* 
                              * everything was fine but transaction
                              * was not completly finished. 
                              */
   STATUS_NOTOK_DOAGAIN=25   /*
                              * transaction was rejected. Try again later 
                              * (value will be used as return value for
                              * qsub)
                              */
};

/*
 * definess for AN_quality
 */
#define NUM_AN_ERROR    0
#define NUM_AN_WARNING  1
#define NUM_AN_INFO     2

enum {
   AN_status = AN_LOWERBOUND,
   AN_text,
   AN_quality
};

LISTDEF(AN_Type)
   SGE_ULONG(AN_status)
   SGE_STRING(AN_text)
   SGE_ULONG(AN_quality)
LISTEND 

NAMEDEF(ANN)
   NAME("AN_status")
   NAME("AN_text")
   NAME("AN_quality")
NAMEEND

/* *INDENT-ON* */

#define ANS sizeof(ANN)/sizeof(char*)
#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_ANSWERL_H */
