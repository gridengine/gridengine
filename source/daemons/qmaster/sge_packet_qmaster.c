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

#include "gdi/sge_gdi.h"
#include "gdi/sge_gdi2.h"
#include "gdi/sge_gdi_packet.h"

#include "sge_packet_qmaster.h"

/* EB: TODO: ST: move the funczion to gdi functions */

const char *
sge_gdi_task_get_operation_name(sge_gdi_task_class_t *task) 
{
   const char *ret = NULL;

   switch (task->command) {
      case SGE_GDI_GET:
         ret = "GET";
         break;
      case SGE_GDI_ADD:         
         ret = "ADD";
         break;
      case SGE_GDI_DEL:         
         ret = "DEL";
         break;
      case SGE_GDI_MOD:         
         ret = "MOD";
         break;
      case SGE_GDI_COPY:
         ret = "COPY";         
         break;
      case SGE_GDI_TRIGGER:         
         ret = "TRIGGER";
         break;
      case SGE_GDI_PERMCHECK:         
         ret = "PERMCHECK";
         break; 
      case SGE_GDI_REPLACE:         
         ret = "REPLACE";
         break;
      default: 
         ret = "???";    
         break;
   }
   return ret;
}
