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

#                                                             max. column:     |
#****** sge_procedures/reset_schedd_config() ******
# 
#  NAME
#     reset_schedd_config -- set schedd configuration default values
#
#  SYNOPSIS
#     reset_schedd_config { } 
#
#  FUNCTION
#     This procedure will call set_schedd_config with default values 
#
#  RESULT
#       -1 : timeout error
#        0 : ok
#
#
#  NOTES
#     The default values are:
#     
#     SGE system:
#    
#     algorithm                   "default"
#     schedule_interval           "0:0:15"
#     maxujobs                    "0"
#     queue_sort_method           "load"
#     user_sort                   "false"
#     job_load_adjustments        "np_load_avg=0.50"
#     load_adjustment_decay_time  "0:7:30"
#     load_formula                "np_load_avg"
#     schedd_job_info             "true"
#     
#     
#     SGEEE differences:
#     queue_sort_method           "share"
#     user_sort                   "false"
#     reprioritize_interval       "00:01:00"
#     halftime                    "168"
#     usage_weight_list           "cpu=1,mem=0,io=0"
#     compensation_factor         "5"
#     weight_user                 "0.2"
#     weight_project              "0.2"
#     weight_jobclass             "0.2"
#     weight_department           "0.2"
#     weight_job                  "0.2"
#     weight_tickets_functional   "0"
#     weight_tickets_share        "0"
#     weight_tickets_deadline     "10000"
#
#  SEE ALSO
#     sge_procedures/set_schedd_config()
#*******************************
proc reset_schedd_config {} {
   global ts_config
 
   set default_array(algorithm)                  "default"
   set default_array(schedule_interval)          "0:0:10"
   set default_array(maxujobs)                   "0"
   set default_array(job_load_adjustments)       "np_load_avg=0.15"
   set default_array(load_adjustment_decay_time) "0:7:30"
   set default_array(load_formula)               "np_load_avg"
   set default_array(schedd_job_info)            "true"

# this is sgeee
   if { [string compare $ts_config(product_type) "sgeee"] == 0 } {
      set default_array(halftime)                   "168"
      set default_array(usage_weight_list)          "cpu=1,mem=0,io=0"
      set default_array(compensation_factor)        "5"
      set default_array(weight_tickets_functional)  "0"
      set default_array(weight_tickets_share)       "0"
   }

   vdep_set_sched_conf_defaults default_array

   set ret_value [ set_schedd_config default_array ]

   if { $ret_value != 0 } {
      add_proc_error "reset_schedd_config" $ret_value "error set_schedd_config - call"
   } 

   return $ret_value
}



#                                                             max. column:     |
#****** sge_procedures/set_schedd_config() ******
# 
#  NAME
#     set_schedd_config -- change scheduler configuration
#
#  SYNOPSIS
#     set_schedd_config { change_array } 
#
#  FUNCTION
#     Set the scheduler configuration corresponding to the content of the 
#     change_array.
#
#  INPUTS
#     change_array - name of an array variable that will be set by 
#                    set_schedd_config
#  RESULT
#     -1 : timeout
#      0 : ok
#
#  EXAMPLE
#     get_schedd_config myconfig
#     set myconfig(schedule_interval) "0:0:10"
#     set_schedd_config myconfig
#
#  NOTES
#     The array should be build like follows:
#   
#     set change_array(algorithm) default
#     set change_array(schedule_interval) 0:0:15
#     ....
#     (every value that is set will be changed)
#
#     Here the possible change_array values with some typical settings:
#     
#     algorithm                   "default"
#     schedule_interval           "0:0:15"
#     maxujobs                    "0"
#     queue_sort_method           "share"
#     user_sort                   "false"
#     job_load_adjustments        "np_load_avg=0.50"
#     load_adjustment_decay_time  "0:7:30"
#     load_formula                "np_load_avg"
#     schedd_job_info             "true"
#     
#     
#     In case of a SGEEE - System:
#     
#     reprioritize_interval       "00:01:00"
#     halftime                    "168"
#     usage_weight_list           "cpu=0.34,mem=0.33,io=0.33"
#     compensation_factor         "5"
#     weight_user                 "0"
#     weight_project              "0"
#     weight_jobclass             "0"
#     weight_department           "0"
#     weight_job                  "0"
#     weight_tickets_functional   "0"
#     weight_tickets_share        "0"
#     weight_tickets_deadline     "10000"
#     
#
#  SEE ALSO
#     sge_procedures/get_schedd_config()
#*******************************
proc set_schedd_config { change_array } {
  global ts_config
  global env CHECK_ARCH open_spawn_buffer
  global CHECK_OUTPUT CHECK_CORE_MASTER
  upvar $change_array chgar

  set vi_commands [build_vi_command chgar]
  set CHANGED_SCHEDD_CONFIG [translate $CHECK_CORE_MASTER 1 0 0 [sge_macro MSG_SCHEDD_CHANGEDSCHEDULERCONFIGURATION]]
  set result [ handle_vi_edit "$ts_config(product_root)/bin/$CHECK_ARCH/qconf" "-msconf" $vi_commands $CHANGED_SCHEDD_CONFIG ]  

  if { $result != 0 } {
     add_proc_error "set_schedd_config" -1 "error changing scheduler configuration"
  }
  
  return $result
}

#                                                             max. column:     |
#****** sge_procedures/get_schedd_config() ******
# 
#  NAME
#     get_schedd_config -- get scheduler configuration 
#
#  SYNOPSIS
#     get_schedd_config { change_array } 
#
#  FUNCTION
#     Get the current scheduler configuration     
#
#  INPUTS
#     change_array - name of an array variable that will get set by 
#                    get_schedd_config
#
#  EXAMPLE
#     get_schedd_config test
#     puts $test(schedule_interval)
#
#  NOTES
# 
#     The array is build like follows:
#   
#     set change_array(algorithm) default
#     set change_array(schedule_interval) 0:0:15
#     ....
#
#     Here the possible change_array values with some typical settings:
#     
#     algorithm                   "default"
#     schedule_interval           "0:0:15"
#     maxujobs                    "0"
#     queue_sort_method           "share"
#     user_sort                   "false"
#     job_load_adjustments        "np_load_avg=0.50"
#     load_adjustment_decay_time  "0:7:30"
#     load_formula                "np_load_avg"
#     schedd_job_info             "true"
#     
#     
#     In case of a SGEEE - System:
#     
#     reprioritize_interval       "00:01:00"
#     halftime                    "168"
#     usage_weight_list           "cpu=0.34,mem=0.33,io=0.33"
#     compensation_factor         "5"
#     weight_user                 "0"
#     weight_project              "0"
#     weight_jobclass             "0"
#     weight_department           "0"
#     weight_job                  "0"
#     weight_tickets_functional   "0"
#     weight_tickets_share        "0"
#     weight_tickets_deadline     "10000"
#
#  SEE ALSO
#     sge_procedures/set_schedd_config()
#*******************************
proc get_schedd_config { change_array } {
  global ts_config
  global CHECK_ARCH
  upvar $change_array chgar

  set catch_return [ catch {  eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf -ssconf" } result ]
  if { $catch_return != 0 } {
     add_proc_error "get_schedd_config" "-1" "qconf error or binary not found"
     return
  }

  # split each line as listelement
  set help [split $result "\n"]

  foreach elem $help {
     set id [lindex $elem 0]
     set value [lrange $elem 1 end]
     set chgar($id) $value
  }
}


