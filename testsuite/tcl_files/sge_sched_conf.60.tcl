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

   set chgar(flush_submit_sec)        "0"
   set chgar(flush_finish_sec)        "0"
   set chgar(params)                  "none"
   set chgar(queue_sort_method)          "load"
   set chgar(reprioritize_interval)    "00:00:40"
   set chgar(share_override_tickets)        "true"
   set chgar(share_functional_shares)       "true"
   set chgar(max_functional_jobs_to_schedule) "200"
   set chgar(report_pjob_tickets)             "true"
   set chgar(max_pending_tasks_per_job)       "50"
   set chgar(halflife_decay_list)             "none"
   set chgar(policy_hierarchy)                "OFS"

   set chgar(weight_user)                "0.25"
   set chgar(weight_project)             "0.25"
   set chgar(weight_department)          "0.25"
   set chgar(weight_job)                 "0.25"
   set chgar(weight_ticket)                   "0.010000"
   set chgar(weight_waiting_time)             "0.000000"
   set chgar(weight_deadline)                 "3600000"
   set chgar(weight_urgency)                  "0.100000"
   set chgar(weight_priority)                  "1.000000"
   set chgar(max_reservation)                  "0"
   set chgar(default_duration)                 "0:10:0"
}
