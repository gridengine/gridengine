#!/usr/local/bin/tclsh
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
#****** sge_procedures/set_queue() ******
# 
#  NAME
#     set_queue -- set or change queue configuration
#
#  SYNOPSIS
#     set_queue { q_name change_array } 
#
#  FUNCTION
#     Set a queue configuration corresponding to the content of the change_array.
#
#  INPUTS
#     q_name       - name of the queue to configure
#     change_array - name of an array variable that will be set by set_queue
#
#  RESULT
#     0  : ok
#     -1 : timeout
#
#  EXAMPLE
#     get_queue myqueue.q queue1
#     set queue1(load_thresholds) "np_load_avg=3.75" 
#     set_queue myqueue.q queue1
#
#  NOTES
#     the array should look like this:
#
#     set change_array(qname) MYHOST
#     set change_array(hostname) MYHOST.domain
#     ....
#     (every value that is set will be changed)
#
#     here is a list of all valid array names (template queue):
#
#     change_array(qname)                "template"
#     change_array(hostname)             "unknown" - This is wrong. It should say
#     change_array(hostlist)             "unknown"
#     change_array(seq_no)               "0"
#     change_array(load_thresholds)      "np_load_avg=1.75"
#     change_array(suspend_thresholds)   "NONE"
#     change_array(nsuspend)             "0"
#     change_array(suspend_interval)     "00:05:00"
#     change_array(priority)             "0"
#     change_array(min_cpu_interval)     "00:05:00"
#     change_array(processors)           "UNDEFINED"
#     change_array(qtype)                "BATCH INTERACTIVE" 
#     change_array(rerun)                "FALSE"
#     change_array(slots)                "1"
#     change_array(shell)                "/bin/csh"
#     change_array(shell_start_mode)     "NONE"
#     change_array(prolog)               "NONE"
#     change_array(epilog)               "NONE"
#     change_array(starter_method)       "NONE"
#     change_array(suspend_method)       "NONE"
#     change_array(resume_method)        "NONE"
#     change_array(terminate_method)     "NONE"
#     change_array(notify)               "00:00:60"
#     change_array(owner_list)           "NONE"
#     change_array(user_lists)           "NONE"
#     change_array(xuser_lists)          "NONE"
#     change_array(subordinate_list)     "NONE"
#     change_array(complex_list)         "NONE"
#     change_array(complex_values)       "NONE"
#     change_array(projects)             "NONE"
#     change_array(xprojects)            "NONE"
#     change_array(calendar)             "NONE"
#     change_array(initial_state)        "default"
#     change_array(s_rt)                 "INFINITY"
#     change_array(h_rt)                 "INFINITY"
#     change_array(s_cpu)                "INFINITY"
#     change_array(h_cpu)                "INFINITY"
#     change_array(s_fsize)              "INFINITY"
#     change_array(h_fsize)              "INFINITY"
#     change_array(s_data)               "INFINITY"
#     change_array(h_data)               "INFINITY"
#     change_array(s_stack)              "INFINITY"
#     change_array(h_stack)              "INFINITY"
#     change_array(s_core)               "INFINITY"
#     change_array(h_core)               "INFINITY"
#     change_array(s_rss)                "INFINITY"
#     change_array(h_rss)                "INFINITY"
#     change_array(s_vmem)               "INFINITY"
#     change_array(h_vmem)               "INFINITY"
#
#
#  SEE ALSO
#     sge_procedures/mqattr()
#     sge_procedures/set_queue()
#     sge_procedures/add_queue()
#     sge_procedures/del_queue()
#     sge_procedures/get_queue()
#     sge_procedures/suspend_queue()
#     sge_procedures/unsuspend_queue()
#     sge_procedures/disable_queue()
#     sge_procedures/enable_queue()
#*******************************

#                                                             max. column:     |
#****** sge_queue/add_queue() ******
# 
#  NAME
#     add_queue -- Add a new queue configuration object
#
#  SYNOPSIS
#     add_queue { change_array {fast_add 1} } 
#
#  FUNCTION
#     Add a new queue configuration object corresponding to the content of 
#     the change_array.
#
#  INPUTS
#     change_array - name of an array variable that will be set by get_config
#     {fast_add 1} - if not 0 the add_queue procedure will use a file for
#                    queue configuration. (faster) (qconf -Aq, not qconf -aq)
#
#  RESULT
#     -1   timeout error
#     -2   queue already exists
#      0   ok 
#
#  EXAMPLE
#     set new_queue(qname)    "new.q"
#     set new_queue(hostname) "expo1"
#     add_queue new_queue 
#
#  NOTES
#     the array should look like this:
#
#     set change_array(qname) MYHOST
#     set change_array(hostname) MYHOST.domain
#     ....
#     (every value that is set will be changed)
#
#     here is a list of all valid array names (template queue):
#
#     change_array(qname)                "template"
#     change_array(hostname)             "unknown"
#     change_array(seq_no)               "0"
#     change_array(load_thresholds)      "np_load_avg=1.75"
#     change_array(suspend_thresholds)   "NONE"
#     change_array(nsuspend)             "0"
#     change_array(suspend_interval)     "00:05:00"
#     change_array(priority)             "0"
#     change_array(min_cpu_interval)     "00:05:00"
#     change_array(processors)           "UNDEFINED"
#     change_array(qtype)                "BATCH INTERACTIVE" 
#     change_array(rerun)                "FALSE"
#     change_array(slots)                "1"
#     change_array(shell)                "/bin/csh"
#     change_array(shell_start_mode)     "NONE"
#     change_array(prolog)               "NONE"
#     change_array(epilog)               "NONE"
#     change_array(starter_method)       "NONE"
#     change_array(suspend_method)       "NONE"
#     change_array(resume_method)        "NONE"
#     change_array(terminate_method)     "NONE"
#     change_array(notify)               "00:00:60"
#     change_array(owner_list)           "NONE"
#     change_array(user_lists)           "NONE"
#     change_array(xuser_lists)          "NONE"
#     change_array(subordinate_list)     "NONE"
#     change_array(complex_list)         "NONE"
#     change_array(complex_values)       "NONE"
#     change_array(projects)             "NONE"
#     change_array(xprojects)            "NONE"
#     change_array(calendar)             "NONE"
#     change_array(initial_state)        "default"
#     change_array(s_rt)                 "INFINITY"
#     change_array(h_rt)                 "INFINITY"
#     change_array(s_cpu)                "INFINITY"
#     change_array(h_cpu)                "INFINITY"
#     change_array(s_fsize)              "INFINITY"
#     change_array(h_fsize)              "INFINITY"
#     change_array(s_data)               "INFINITY"
#     change_array(h_data)               "INFINITY"
#     change_array(s_stack)              "INFINITY"
#     change_array(h_stack)              "INFINITY"
#     change_array(s_core)               "INFINITY"
#     change_array(h_core)               "INFINITY"
#     change_array(s_rss)                "INFINITY"
#     change_array(h_rss)                "INFINITY"
#     change_array(s_vmem)               "INFINITY"
#     change_array(h_vmem)               "INFINITY"
#
#  SEE ALSO
#     sge_procedures/mqattr()
#     sge_procedures/set_queue()
#     sge_procedures/add_queue()
#     sge_procedures/del_queue()
#     sge_procedures/get_queue()
#     sge_procedures/suspend_queue()
#     sge_procedures/unsuspend_queue()
#     sge_procedures/disable_queue()
#     sge_procedures/enable_queue()
#*******************************
proc set_queue_defaults { change_array } {
   global ts_config

   upvar $change_array chgar

   set chgar(qname)                "queuename"
   set chgar(seq_no)               "0"
   set chgar(load_thresholds)      "np_load_avg=7.00"
   set chgar(suspend_thresholds)   "NONE"
   set chgar(nsuspend)             "1"
   set chgar(suspend_interval)     "00:05:00"
   set chgar(priority)             "0"
   set chgar(min_cpu_interval)     "00:05:00"
   set chgar(processors)           "UNDEFINED"
   set chgar(rerun)                "FALSE"
   set chgar(slots)                "10"
   set chgar(shell)                "/bin/csh"
   set chgar(shell_start_mode)     "posix_compliant"
   set chgar(prolog)               "NONE"
   set chgar(epilog)               "NONE"
   set chgar(starter_method)       "NONE"
   set chgar(suspend_method)       "NONE"
   set chgar(resume_method)        "NONE"
   set chgar(terminate_method)     "NONE"
   set chgar(notify)               "00:00:60"
   set chgar(owner_list)           "NONE"
   set chgar(user_lists)           "NONE"
   set chgar(xuser_lists)          "NONE"
   set chgar(subordinate_list)     "NONE"
   set chgar(complex_values)       "NONE"
   set chgar(calendar)             "NONE"
   set chgar(initial_state)        "default"
   set chgar(s_rt)                 "INFINITY"
   set chgar(h_rt)                 "INFINITY"
   set chgar(s_cpu)                "INFINITY"
   set chgar(h_cpu)                "INFINITY"
   set chgar(s_fsize)              "INFINITY"
   set chgar(h_fsize)              "INFINITY"
   set chgar(s_data)               "INFINITY"
   set chgar(h_data)               "INFINITY"
   set chgar(s_stack)              "INFINITY"
   set chgar(h_stack)              "INFINITY"
   set chgar(s_core)               "INFINITY"
   set chgar(h_core)               "INFINITY"
   set chgar(s_rss)                "INFINITY"
   set chgar(h_rss)                "INFINITY"
   set chgar(s_vmem)               "INFINITY"
   set chgar(h_vmem)               "INFINITY"
  
   if { $ts_config(product_type) == "sgeee" } {
      set chgar(projects)           "NONE"
      set chgar(xprojects)          "NONE"
   }

   vdep_set_queue_defaults chgar
}

#                                                             max. column:     |
#****** sge_queue/del_queue() ******
# 
#  NAME
#     del_queue -- delete a queue
#
#  SYNOPSIS
#     del_queue { q_name } 
#
#  FUNCTION
#     remove a queue from the qmaster configuration
#
#  INPUTS
#     q_name - name of the queue to delete
#
#  RESULT
#     0  : ok
#     -1 : timeout error
#
#  EXAMPLE
#     del_queue "my_own_queue.q"
#
#  NOTES
#
#  SEE ALSO
#     sge_procedures/mqattr()
#     sge_procedures/set_queue() 
#     sge_procedures/add_queue()
#     sge_procedures/del_queue()
#     sge_procedures/get_queue()
#     sge_procedures/suspend_queue()
#     sge_procedures/unsuspend_queue()
#     sge_procedures/disable_queue()
#     sge_procedures/enable_queue()
#*******************************
#                                                             max. column:     |
#****** sge_procedures/get_queue() ******
# 
#  NAME
#     get_queue -- get queue configuration information
#
#  SYNOPSIS
#     get_queue { q_name change_array } 
#
#  FUNCTION
#     Get the actual configuration settings for the named queue
#
#  INPUTS
#     q_name       - name of the queue
#     change_array - name of an array variable that will get set by get_config
#
#  EXAMPLE
#     get_queue "myqueue.q" qinfo
#     puts qinfo(seq_no) 
#
#  NOTES
#     the array should look like this:
#
#     set change_array(qname) MYHOST
#     set change_array(hostname) MYHOST.domain
#     ....
#     (every value that is set will be changed)
#
#     here is a list of all valid array names (template queue):
#
#     change_array(qname)                "template"
#     change_array(hostname)             "unknown"
#     change_array(seq_no)               "0"
#     change_array(load_thresholds)      "np_load_avg=1.75"
#     change_array(suspend_thresholds)   "NONE"
#     change_array(nsuspend)             "0"
#     change_array(suspend_interval)     "00:05:00"
#     change_array(priority)             "0"
#     change_array(min_cpu_interval)     "00:05:00"
#     change_array(processors)           "UNDEFINED"
#     change_array(qtype)                "BATCH INTERACTIVE" 
#     change_array(rerun)                "FALSE"
#     change_array(slots)                "1"
#     change_array(tmpdir)               "/tmp/testsuite_1234"
#     change_array(shell)                "/bin/csh"
#     change_array(shell_start_mode)     "NONE"
#     change_array(prolog)               "NONE"
#     change_array(epilog)               "NONE"
#     change_array(starter_method)       "NONE"
#     change_array(suspend_method)       "NONE"
#     change_array(resume_method)        "NONE"
#     change_array(terminate_method)     "NONE"
#     change_array(notify)               "00:00:60"
#     change_array(owner_list)           "NONE"
#     change_array(user_lists)           "NONE"
#     change_array(xuser_lists)          "NONE"
#     change_array(subordinate_list)     "NONE"
#     change_array(complex_list)         "NONE"
#     change_array(complex_values)       "NONE"
#     change_array(projects)             "NONE"
#     change_array(xprojects)            "NONE"
#     change_array(calendar)             "NONE"
#     change_array(initial_state)        "default"
#     change_array(fshare)               "0"
#     change_array(oticket)              "0"
#     change_array(s_rt)                 "INFINITY"
#     change_array(h_rt)                 "INFINITY"
#     change_array(s_cpu)                "INFINITY"
#     change_array(h_cpu)                "INFINITY"
#     change_array(s_fsize)              "INFINITY"
#     change_array(h_fsize)              "INFINITY"
#     change_array(s_data)               "INFINITY"
#     change_array(h_data)               "INFINITY"
#     change_array(s_stack)              "INFINITY"
#     change_array(h_stack)              "INFINITY"
#     change_array(s_core)               "INFINITY"
#     change_array(h_core)               "INFINITY"
#     change_array(s_rss)                "INFINITY"
#     change_array(h_rss)                "INFINITY"
#     change_array(s_vmem)               "INFINITY"
#     change_array(h_vmem)               "INFINITY"
#
#  SEE ALSO
#     sge_procedures/mqattr()
#     sge_procedures/set_queue() 
#     sge_procedures/add_queue()
#     sge_procedures/del_queue()
#     sge_procedures/get_queue()
#     sge_procedures/suspend_queue()
#     sge_procedures/unsuspend_queue()
#     sge_procedures/disable_queue()
#     sge_procedures/enable_queue()
#*******************************
proc get_queue { q_name change_array } {
  global ts_config


  global CHECK_ARCH CHECK_OUTPUT
  upvar $change_array chgar

  set catch_return [ catch {  eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qconf -sq ${q_name}" } result ]
  if { $catch_return != 0 } {
     add_proc_error "get_queue" "-1" "qconf error or binary not found"
     return
  }

  # split each line as listelement
  set help [split $result "\n"]

  foreach elem $help {
     set id [lindex $elem 0]
     set value [lrange $elem 1 end]
     set value [replace_string $value "{" ""]
     set value [replace_string $value "}" ""]
     
     if { $id != "" } {
        set chgar($id) $value
#        puts $CHECK_OUTPUT "queue($id) = $value"
     }
  }
}

#                                                             max. column:     |
#****** sge_queue/suspend_queue() ******
# 
#  NAME
#     suspend_queue -- set a queue in suspend mode
#
#  SYNOPSIS
#     suspend_queue { qname } 
#
#  FUNCTION
#     This procedure will set the given queue into suspend state
#
#  INPUTS
#     qname - name of the queue to suspend 
#
#  RESULT
#     0  - ok
#    -1  - error 
#
#  SEE ALSO
#     sge_procedures/mqattr()
#     sge_procedures/set_queue() 
#     sge_procedures/add_queue()
#     sge_procedures/del_queue()
#     sge_procedures/get_queue()
#     sge_procedures/suspend_queue()
#     sge_procedures/unsuspend_queue()
#     sge_procedures/disable_queue()
#     sge_procedures/enable_queue()
#*******************************
proc suspend_queue { qname } {
  global ts_config
 global CHECK_ARCH CHECK_HOST CHECK_USER
 global CHECK_OUTPUT
  log_user 0 
   if { $ts_config(gridengine_version) == 53 } {
      set WAS_SUSPENDED [translate $CHECK_HOST 1 0 0 [sge_macro MSG_QUEUE_SUSPENDQ_SSS] "*" "*" "*" ]
   } else {
      set WAS_SUSPENDED [translate $CHECK_HOST 1 0 0 [sge_macro MSG_QINSTANCE_SUSPENDED]]
   }

  
  # spawn process
  set program "$ts_config(product_root)/bin/$CHECK_ARCH/qmod"
  set sid [ open_remote_spawn_process $CHECK_HOST $CHECK_USER $program "-s $qname" ]
  set sp_id [ lindex $sid 1 ]
  set result -1	

  log_user 0
  set timeout 30
  expect {
     -i $sp_id full_buffer {
         set result -1
         add_proc_error "suspend_queue" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
     }
     -i $sp_id "was suspended" {
         set result 0
     }
      -i $sp_id "*${WAS_SUSPENDED}*" {
         set result 0
     }

	  -i $sp_id default {
         puts $CHECK_OUTPUT $expect_out(buffer)
	      set result -1
	  }
  }
  # close spawned process 
  close_spawn_process $sid
  log_user 1
  if { $result != 0 } {
     add_proc_error "suspend_queue" -1 "could not suspend queue \"$qname\""
  }

  return $result
}

#                                                             max. column:     |
#****** sge_queue/unsuspend_queue() ******
# 
#  NAME
#     unsuspend_queue -- set a queue in suspend mode
#
#  SYNOPSIS
#     unsuspend_queue { queue } 
#
#  FUNCTION
#     This procedure will set the given queue into unsuspend state
#
#  INPUTS
#     queue - name of the queue to set into unsuspend state
#
#  RESULT
#     0  - ok
#    -1  - error 
#
#  SEE ALSO
#     sge_procedures/mqattr()
#     sge_procedures/set_queue() 
#     sge_procedures/add_queue()
#     sge_procedures/del_queue()
#     sge_procedures/get_queue()
#     sge_procedures/suspend_queue()
#     sge_procedures/unsuspend_queue()
#     sge_procedures/disable_queue()
#     sge_procedures/enable_queue()
#*******************************
proc unsuspend_queue { queue } {
  global ts_config
   global CHECK_ARCH CHECK_HOST CHECK_USER
   global CHECK_OUTPUT

  set timeout 30
  log_user 0 
   
   if { $ts_config(gridengine_version) == 53 } {
      set UNSUSP_QUEUE [translate $CHECK_HOST 1 0 0 [sge_macro MSG_QUEUE_UNSUSPENDQ_SSS] "*" "*" "*" ]
   } else {
      set UNSUSP_QUEUE [translate $CHECK_HOST 1 0 0 [sge_macro MSG_QINSTANCE_NSUSPENDED]]
   }

  # spawn process
  set program "$ts_config(product_root)/bin/$CHECK_ARCH/qmod"
  set sid [ open_remote_spawn_process $CHECK_HOST $CHECK_USER $program "-us $queue" ]     
  set sp_id [ lindex $sid 1 ]
  set result -1	
  log_user 0 

  set timeout 30
  expect {
      -i $sp_id full_buffer {
         set result -1
         add_proc_error "unsuspend_queue" "-1" "buffer overflow please increment CHECK_EXPECT_MATCH_MAX_BUFFER value"
      }
      -i $sp_id "unsuspended queue" {
         set result 0 
      }
      -i $sp_id  "*${UNSUSP_QUEUE}*" {
         set result 0 
      }
      -i $sp_id default {
         puts $CHECK_OUTPUT $expect_out(buffer) 
         set result -1 
      }
  }
  # close spawned process 
  close_spawn_process $sid
  log_user 1   
  if { $result != 0 } {
     add_proc_error "unsuspend_queue" -1 "could not unsuspend queue \"$queue\""
  }
  return $result
}

#                                                             max. column:     |
#****** sge_queue/disable_queue() ******
# 
#  NAME
#     disable_queue -- disable queues
#
#  SYNOPSIS
#     disable_queue { queue } 
#
#  FUNCTION
#     Disable the given queue/queue list
#
#  INPUTS
#     queue - name of queues to disable
#
#  RESULT
#     0  - ok
#    -1  - error
#
#  SEE ALSO
#     sge_procedures/mqattr()
#     sge_procedures/set_queue() 
#     sge_procedures/add_queue()
#     sge_procedures/del_queue()
#     sge_procedures/get_queue()
#     sge_procedures/suspend_queue()
#     sge_procedures/unsuspend_queue()
#     sge_procedures/disable_queue()
#     sge_procedures/enable_queue()
#*******************************
proc disable_queue { queuelist } {
  global ts_config
 global CHECK_ARCH
  global CHECK_OUTPUT CHECK_HOST CHECK_USER
  global CHECK_CORE_MASTER CHECK_USER
  
  set return_value ""
  # spawn process

  set nr_of_queues 0
  set nr_disabled 0

  foreach elem $queuelist {
     set queue_name($nr_of_queues) $elem
     incr nr_of_queues 1
  }

  set queue_nr 0
  while { $queue_nr != $nr_of_queues } {
     log_user 0
     set queues ""
     set i 100  ;# maximum 100 queues at one time (= 2000 byte commandline with avg(len(qname)) = 20
     while { $i > 0 } {
        if { $queue_nr < $nr_of_queues } {
           append queues " $queue_name($queue_nr)"
           incr queue_nr 1
        }
        incr i -1
     }   
     
     set result ""
     set catch_return [ catch { eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qmod -d $queues" } result ]
     debug_puts $CHECK_OUTPUT "disable queue(s) $queues"
     set res_split [ split $result "\n" ]   
     foreach elem $res_split {
        if { [ string first "has been disabled" $result ] >= 0 } {
           incr nr_disabled 1 
        } else {
           # try to find localized output
           foreach q_name $queues {
              if { $ts_config(gridengine_version) == 53 } {
                set HAS_DISABLED [translate $CHECK_HOST 1 0 0 [sge_macro MSG_QUEUE_DISABLEQ_SSS] $q_name $CHECK_USER "*" ]
              } else {
                set HAS_DISABLED [translate $CHECK_HOST 1 0 0 [sge_macro MSG_QINSTANCE_DISABLED]]
              }

              if { [ string match "*${HAS_DISABLED}*" $elem ] } {
                 incr nr_disabled 1
                 break
              } 
           }
        }
     }
  }    

  if { $nr_of_queues != $nr_disabled } {
     add_proc_error "disable_queue" "-1" "could not disable all queues"
     return -1
  }
 
  return 0
}


#                                                             max. column:     |
#****** sge_queue/enable_queue() ******
# 
#  NAME
#     enable_queue -- enable queuelist
#
#  SYNOPSIS
#     enable_queue { queue } 
#
#  FUNCTION
#     This procedure enables a given queuelist by calling the qmod -e binary
#
#  INPUTS
#     queue - name of queues to enable (list)
#
#  RESULT
#     0  - ok
#    -1  - on error
#
#  SEE ALSO
#     sge_procedures/mqattr()
#     sge_procedures/set_queue() 
#     sge_procedures/add_queue()
#     sge_procedures/del_queue()
#     sge_procedures/get_queue()
#     sge_procedures/suspend_queue()
#     sge_procedures/unsuspend_queue()
#     sge_procedures/disable_queue()
#     sge_procedures/enable_queue()
#*******************************
proc enable_queue { queuelist } {
  global ts_config
  global CHECK_ARCH
  global CHECK_OUTPUT CHECK_HOST CHECK_USER CHECK_CORE_MASTER 
  
  set return_value ""
  # spawn process

  set nr_of_queues 0
  set nr_enabled 0

  foreach elem $queuelist {
     set queue_name($nr_of_queues) $elem
     incr nr_of_queues 1
  }

  set queue_nr 0
  while { $queue_nr != $nr_of_queues } {
     log_user 0
     set queues ""
     set i 100  ;# maximum 100 queues at one time (= 2000 byte commandline with avg(len(qname)) = 20
     while { $i > 0 } {
        if { $queue_nr < $nr_of_queues } {
           append queues " $queue_name($queue_nr)"
           incr queue_nr 1
        }
        incr i -1
     }   
     set result ""
     set catch_return [ catch { eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qmod -e $queues" } result ]
     debug_puts $CHECK_OUTPUT "enable queue(s) $queues"
     set res_split [ split $result "\n" ]   
     foreach elem $res_split {
        if { [ string first "has been enabled" $result ] >= 0 } {
           incr nr_enabled 1 
        } else {
           # try to find localized output
           foreach q_name $queues {
              if { $ts_config(gridengine_version) == 53 } {
                 set BEEN_ENABLED  [translate $CHECK_HOST 1 0 0 [sge_macro MSG_QUEUE_ENABLEQ_SSS] $q_name $CHECK_USER "*" ]
              } else {
                 set BEEN_ENABLED  [translate $CHECK_HOST 1 0 0 [sge_macro MSG_QINSTANCE_NDISABLED]]
              }
              if { [ string match "*${BEEN_ENABLED}*" $result ] } {
                 incr nr_enabled 1
                 break
              } 
           }
        }
     }
  }    

  if { $nr_of_queues != $nr_enabled } {
     add_proc_error "enable_queue" "-1" "could not enable all queues"
     return -1
  }
  return 0
}


#                                                             max. column:     |
#****** sge_queue/get_queue_state() ******
# 
#  NAME
#     get_queue_state -- get the state of a queue
#
#  SYNOPSIS
#     get_queue_state { queue } 
#
#  FUNCTION
#     This procedure returns the state of the queue by parsing output of qstat -f. 
#
#  INPUTS
#     queue - name of the queue
#
#  RESULT
#     The return value can contain more than one state. Here is a list of possible
#     states:
#
#     u(nknown)
#     a(larm)
#     A(larm)
#     C(alendar  suspended)
#     s(uspended)
#     S(ubordinate)
#     d(isabled)
#     D(isabled)
#     E(rror)
#
#*******************************
proc get_queue_state { queue_name } {
  global ts_config

  global CHECK_ARCH

  # resolve the queue name
  set queue [resolve_queue $queue_name]
  set catch_return [ catch { 
     eval exec "$ts_config(product_root)/bin/$CHECK_ARCH/qstat -f -q $queue" 
  } result ]

  if { $catch_return != 0 } {
     add_proc_error "get_queue_state" "-1" "qstat error or binary not found"
     return ""
  }

  # split each line as listelement
  set back ""
  set help [split $result "\n"]
  foreach line $help { 
      if { [ string compare [lindex $line 0] $queue ] == 0 } {
         set back [lindex $line 5 ]
         return $back
      }
  }

  add_proc_error "get_queue_state" -1 "queue \"$queue\" not found" 
  return ""
}

#****** sge_queue/clear_queue() *****************************************
#  NAME
#     clear_queue() -- clear queue $queue
#
#  SYNOPSIS
#     clear_queue { queue {output_var result} {on_host ""} {as_user ""} {raise_error 1}  }
#
#  FUNCTION
#     Calls qconf -cq $queue to clear queue $queue
#
#  INPUTS
#     output_var      - result will be placed here
#     queue           - queue to be cleared
#     {on_host ""}    - execute qconf on this host, default is master host
#     {as_user ""}    - execute qconf as this user, default is $CHECK_USER
#     {raise_error 1} - raise an error condition on error (default), or just
#                       output the error message to stdout
#
#  RESULT
#     0 on success, an error code on error.
#     For a list of error codes, see sge_procedures/get_sge_error().
#
#  SEE ALSO
#     sge_calendar/get_calendar()
#     sge_calendar/get_calendar_error()
#*******************************************************************************
proc clear_queue {queue {output_var result}  {on_host ""} {as_user ""} {raise_error 1}} {

   upvar $output_var out

   # clear output variable
   if {[info exists out]} {
      unset out
   }

   set ret 0
   set result [start_sge_bin "qconf" "-cq $queue" $on_host $as_user]

   # parse output or raise error
   if {$prg_exit_state == 0} {
      parse_simple_record result out
   } else {
      set ret [clear_queue_error $result $queue $raise_error]
   }

   return $ret

}
#****** sge_queue/clear_queue_error() ***************************************
#  NAME
#     clear_queue_error() -- error handling for clear_queue
#
#  SYNOPSIS
#     clear_queue_error { result queue raise_error }
#
#  FUNCTION
#     Does the error handling for clear_queue.
#     Translates possible error messages of qconf -cq,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     clear_queue. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     queue       - queue for which qconf -cq has been called
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for clear_queue function:
#      -1:  invalid queue or job "queue"
#     -99: other error
#
#  SEE ALSO
#     sge_calendar/get_calendar
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc clear_queue_error {result queue raise_error} {

   # recognize certain error messages and return special return code
   set messages(index) "-1 "
   set messages(-1) [translate_macro MSG_QUEUE_INVALIDQORJOB_S $queue]

   # we might have version dependent, calendar specific error messages
   get_clear_queue_error_vdep messages $queue

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "get_calendar" "qconf -cq $queue" $result messages $raise_error]

   return $ret
}

#****** sge_queue/get_queue_list() ***************************************
#  NAME
#     get_queue_list() -- get a list of all queues
#
#  SYNOPSIS
#     get_queue_list { {output_var result} {on_host ""} {as_user ""} {raise_error 1}
#
#  FUNCTION
#     Calls qconf -scall to retrieve all calendars
#
#  INPUTS
#     output_var      - result will be placed here
#     {on_host ""}    - execute qconf on this host, default is master host
#     {as_user ""}    - execute qconf as this user, default is $CHECK_USER
#     {raise_error 1} - raise an error condition on error (default), or just
#                       output the error message to stdout
#
#  RESULT
#     0 on success, an error code on error.
#     For a list of error codes, see sge_procedures/get_sge_error().
#
#  SEE ALSO
#     sge_procedures/get_sge_error()
#     sge_procedures/get_qconf_list()
#*******************************************************************************
proc get_queue_list {{output_var result} {on_host ""} {as_user ""} {raise_error 1}} {
   upvar $output_var out

   return [get_qconf_list "get_queue_list" "-sql" out $on_host $as_user $raise_error]

}

