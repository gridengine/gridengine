#ifndef __JB_NOW_H
#define __JB_NOW_H
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
/****** gdilib/jb_now ***************************************
*
*  NAME
*     jb_now -- macros to handle flag JB_now 
*
*  SYNOPSIS
*
*  FUNCTION
*
*  INPUTS
*
*  RESULT
*
*  EXAMPLE
*
*  NOTES
*
*  BUGS
*
*  SEE ALSO
*
****************************************************************************
*/

#define JB_NOW_IMMEDIATE  0x01L
#define JB_NOW_QSH        0x02L
#define JB_NOW_QLOGIN     0x04L
#define JB_NOW_QRSH       0x08L
#define JB_NOW_QRLOGIN    0x10L
#define JOB_TYPE_ARRAY    0x40L

#define JB_NOW_STR_IMMEDIATE  "IMMEDIATE"
#define JB_NOW_STR_QSH        "INTERACTIVE"
#define JB_NOW_STR_QLOGIN     "QLOGIN"
#define JB_NOW_STR_QRSH       "QRSH"
#define JB_NOW_STR_QRLOGIN    "QRLOGIN"

#define JB_NOW_CLEAR_IMMEDIATE(jb_now)    jb_now = jb_now & 0xF8L 

#define JB_NOW_SET_IMMEDIATE(jb_now)      jb_now =  jb_now | JB_NOW_IMMEDIATE
#define JB_NOW_SET_QSH(jb_now)            jb_now = (jb_now & JB_NOW_IMMEDIATE) | JB_NOW_QSH
#define JB_NOW_SET_QLOGIN(jb_now)         jb_now = (jb_now & JB_NOW_IMMEDIATE) | JB_NOW_QLOGIN
#define JB_NOW_SET_QRSH(jb_now)           jb_now = (jb_now & JB_NOW_IMMEDIATE) | JB_NOW_QRSH
#define JB_NOW_SET_QRLOGIN(jb_now)        jb_now = (jb_now & JB_NOW_IMMEDIATE) | JB_NOW_QRLOGIN
#define JOB_TYPE_SET_ARRAY(jb_now)        jb_now = jb_now | JOB_TYPE_ARRAY

#define JB_NOW_IS_IMMEDIATE(jb_now)       (jb_now & JB_NOW_IMMEDIATE)
#define JB_NOW_IS_QSH(jb_now)             (jb_now & JB_NOW_QSH)
#define JB_NOW_IS_QLOGIN(jb_now)          (jb_now & JB_NOW_QLOGIN)
#define JB_NOW_IS_QRSH(jb_now)            (jb_now & JB_NOW_QRSH)
#define JB_NOW_IS_QRLOGIN(jb_now)         (jb_now & JB_NOW_QRLOGIN)
#define JOB_TYPE_IS_ARRAY(jb_now)          (jb_now & JOB_TYPE_ARRAY)

#endif /* __JB_NOW_H */
