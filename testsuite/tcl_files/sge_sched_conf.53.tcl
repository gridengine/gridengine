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


proc vdep_set_sched_conf_defaults { change_array } {
   global ts_config
   upvar $change_array chgar

   set chgar(user_sort)                  "false"

   if { [string compare $ts_config(product_type) "sgeee"] == 0 } {
      set chgar(queue_sort_method)          "share"
      set chgar(sgeee_schedule_interval)    "00:00:40"
      set chgar(weight_tickets_deadline)    "10000"
      set chgar(weight_jobclass)            "0.2"
      set chgar(weight_user)                "0.2"
      set chgar(weight_project)             "0.2"
      set chgar(weight_department)          "0.2"
      set chgar(weight_job)                 "0.2"
   }
}
