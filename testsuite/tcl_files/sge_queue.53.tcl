#___INFO__MARK_BEGIN__
##########################################################################
#
#  The Contents of this file are made available subject to the terms of
#  the Sun Industry Standards Source License Version 1.2
#
#  Sun Microsystems Inc., March, 2001
#
#
#  Sun Industry Standards Source License Version 1.2
#  =================================================
#  The contents of this file are subject to the Sun Industry Standards
#  Source License Version 1.2 (the "License"); You may not use this file
#  except in compliance with the License. You may obtain a copy of the
#  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
#
#  Software provided under this License is provided on an "AS IS" basis,
#  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
#  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
#  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
#  See the License for the specific provisions governing your rights and
#  obligations concerning the Software.
#
#  The Initial Developer of the Original Code is: Sun Microsystems, Inc.
#
#  Copyright: 2001 by Sun Microsystems, Inc.
#
#  All Rights Reserved.
#
##########################################################################
#___INFO__MARK_END__

proc get_queue_instance {queue host} {
   return "${queue}_${host}"
}

#****** sge_procedures.53/queue/vdep_set_queue_defaults() **********************
#  NAME
#     vdep_set_queue_defaults() -- create version dependent queue settings
#
#  SYNOPSIS
#     vdep_set_queue_defaults { change_array } 
#
#  FUNCTION
#     Fills the array change_array with queue attributes specific for SGE 5.3
#
#  INPUTS
#     change_array - the resulting array
#
#  SEE ALSO
#     sge_procedures/queue/set_queue_defaults()
#*******************************************************************************
proc vdep_set_queue_defaults { change_array } {
   upvar $change_array chgar

   set chgar(hostname)             "hostname"
   set chgar(qtype)                "BATCH INTERACTIVE CHECKPOINTING PARALLEL"
   set chgar(complex_list)         "NONE"
   if { $ts_config(product_type) == "sgeee" } {
      set chgar(fshare)             "0"
      set chgar(oticket)            "0"
   }
}

