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
#****** parser_xml/qstat_xml_parse() ******
#
#  NAME
#     qstat_xml_parse -- Generate XML output and return assoc array 
#
#  SYNOPSIS
#     qstat_xml_parse { output }
#                     -- Generate XML output and return assoc array with
#                        entries jobid, prior, name, user, state, total_time,
#                        queue slots and task_id if needed. Pass XML info
#                        to proc qstat_xml_jobid which does the bulk of
#                        the work.
#
#      output  -  asscoc array with the entries mentioned above.#
#                 Output array is similar to that of 
#                 parse_qstat {input output {jobid ""} {ext 0} {do_replace_NA 1 } }
#
#  FUNCTION
#     Print out parsed xml output
#
#  INPUTS
#     None
#
#  NOTES
#     
#
##*******************************

proc qstat_xml_parse { output } {

   global CHECK_OUTPUT

   #package require tdom

   upvar $output output_xml

   # Run now -xml command
   set XML [start_sge_bin  "qstat" " -xml" ]
   #puts $CHECK_OUTPUT "Printing the xml result of qstat $option -xml... \n"
   #puts $CHECK_OUTPUT "$XML \n"
   #upvar $input input_qstat

   set doc  [dom parse $XML]

   set root [$doc documentElement]

   # Parse the running jobs  using this node.
   set node [$root firstChild]   ; # <job-info/>
   set node1 [$node firstChild]  ; # <queue-info/>

   set result1 [qstat_xml_jobid $node1 running output_xml]

   # Parse the pending jobs info using this node. Need to start here
   # NOT at root.
   set node [$root firstChild]   ; # <job-info/>
   set node12 [$node nextSibling]  ; # <queue-info/>
   set node121 [$node12 firstChild]  ; # <qname/>

   set result2 [qstat_xml_jobid $node121 pending output_xml]

   set_error 0 "ok"

}

#                                                             max. column:     |
#****** parser_xml/qstat_f_xml_parse() ******
#
#  NAME
#     qstat_f_xml_parse -- Generate XML output and return assoc array
#
#  SYNOPSIS
#     qstat_f_xml_parse { output }
#                     -- Generate XML output and return assoc array with
#                        entries jobid, prior, name, user, state, total_time,
#                        queue slots and task_id if needed. Pass XML info
#                        to proc qstat_xml_jobid which does the bulk of
#                        the work.
#
#      output  -  asscoc array with the entries mentioned above.#
#                 Output array is similar to that of
#                 parse_qstat {input output {jobid ""} {ext 0} {do_replace_NA 1 } }
#
#  FUNCTION
#     Print out parsed xml output
#
#  INPUTS
#     None
#
#  NOTES
#
#
##*******************************

proc qstat_f_xml_parse { output } {

   global CHECK_OUTPUT

   #package require tdom

   upvar $output output_xml

   # Run now -xml command
   set XML [start_sge_bin  "qstat" "-f -xml" ]

   set doc  [dom parse $XML]

   set root [$doc documentElement]
   
   # Parse the running jobs  using this node.
   set node [$root firstChild]   ; # <job-info/>
   set node1 [$node firstChild]  ; # <queue-info/>

   set result1 [qstat_xml_queue $node1 output_xml]

   # Parse the pending jobs info using this node. Need to start here
   # NOT at root.
   
   set node [$root firstChild]   ; # <job-info/>
   set node12 [$node nextSibling]  ; # <queue-info/>
   set node121 [$node12 firstChild]  ; # <qname/>

   set result2 [qstat_xml_jobid $node121 full output_xml]
   
   set_error 0 "ok"

}

#                                                             max. column:     |
#****** parser_xml/qstat_xml_jobid() ******
#
#  NAME
#     qstat_xml_jobid -- Take XML node and return assoc array 
#
#  SYNOPSIS
#     qstat_xml_jobid -- Take XML node and return assoc array with
#                        entries jobid, prior, name, user, state, total_time,
#                        queue slots and task_id if needed. Pass XML info
#                        to proc qstat_xml_jobid which does the bulk of
#                        the work.
#
#  FUNCTION
#     Return assoc array
#
#  INPUTS
#     
#     qstat_xml_jobid {node121 job_type output} 
#
#     node121  -  node in XML doc where we start navigation
#     job_type  -  "running" or "pending", which tells us which 
#                  fields to expect
#     output  -  asscoc array with the entries mentioned above.
#
#  NOTES
#     
#
##*******************************

proc qstat_xml_jobid { node121 job_type output} {
   global CHECK_OUTPUT

   #package require tdom

   upvar $output output_xml_qstat
   # Add var to tell if doing a running job parse or pending job parse
   # job_type = {runing, pending, full}
   
   # If nothing, we have not started any jobs, so we return
   if { $node121 == "" } {
      return
   }
   set node1211 [$node121 firstChild]  ; # <qname1/>
   set node12111 [$node1211 firstChild]  ; # <jobid/
   if { $node12111 == "" } {
      return
   }
   set jobid [$node12111 nodeValue]


   #puts $CHECK_OUTPUT " jobid node is $jobid ....\n"

   set output_xml_qstat($jobid,jobid) $jobid
   lappend output_xml_qstat(jobid_list) $jobid

   # Use the names from the parse_plain_qstat output :)
   # set column_vars "prior name user state time queue master"
   # As in parse_plain_qstat case, for pending jobs, queue and task_id entries
   # are set to blank.
   if { $job_type == "running" } { ; # this is for listing of qstat running jobs 
      set column_vars "prior name user state time queue master task_id"
   } elseif { $job_type == "pending" } { ; # this is for listing of qstat pending jobs
      set column_vars "prior name user state time queue master"
      set output_xml_qstat($jobid,task_id) ""
   }  elseif { $job_type == "full" } { ; # this is for listing of qstat -f jobs
      set column_vars "prior name user state time slots task_id"
   }
   
   foreach column $column_vars {
      set node21 [$node1211 nextSibling] ;
      if { $node21 == "" } {
         break
      }
      set node211 [$node21 firstChild] ; # <jobid info/

      if { $node211 == "" } { ; # we have hit the empty queue entry
         append output_xml_qstat($jobid,$column) ""
         set node1211 $node21
         continue
      }
      
      set xml_param [$node211 nodeValue]
      
      # For time, need the UNIX value, to compare with plain output.
      if { ($column == "time") } {
         set xml_param  [transform_date_time $xml_param]
      }   
      
      #puts $CHECK_OUTPUT " $column is $xml_param ....\n"
      
      # For colums "queue" and "master" we need to lappend rather than set
      # since we can have more than one entries.
      if { ($column == "queue") || ($column == "master") || ($column == "slots") || ($column == "task_id")} {
         append output_xml_qstat($jobid,$column) "$xml_param "
         set node1211 $node21
      } else {
         set output_xml_qstat($jobid,$column) $xml_param      
         set node1211 $node21
      }
   }

   #puts    $CHECK_OUTPUT " our output for xml and jobtype $job_type is this.... \n"
   #parray output_xml_qstat

   while { 1 } {

      set node13  [$node121 nextSibling]  ; # <next jobid/>
      if { $node13 == "" } {
         break
      }

      set node122 [$node13 firstChild] ; #
      set node1212 [$node122 firstChild]  ; # <next jobid info/>

      set next_jobid [$node1212 nodeValue]
      #puts $CHECK_OUTPUT " next jobid node is $next_jobid ....\n"

      set output_xml_qstat($next_jobid,jobid) $next_jobid
      lappend output_xml_qstat(jobid_list) $next_jobid


      set node121 $node13 ; # yes, node121, NOT node122...
      
      foreach next_column $column_vars {
         set node22 [$node122 nextSibling] ;
         if { $node22 == "" } {
            break
         }
         set node221 [$node22 firstChild] ; # <jobid info/>

         if { $node221 == "" } { ; # we hit the empty queue entry
            append output_xml_qstat($next_jobid,$next_column) ""
            set node122 $node22
            continue
         }

         #set next_qstat_param $input_qstat($next_jobid,$next_column)
         set next_xml_param [$node221 nodeValue]
         #puts $CHECK_OUTPUT " $next_column is $next_xml_param ....\n"

         if { ($next_column == "time") } {
            set next_xml_param  [transform_date_time $next_xml_param]
         }   
      
         #puts $CHECK_OUTPUT " $column is $xml_param ....\n"
      
         # For colums "queue" and "master" we need to lappend rather than set
         # since we can have more than one entries.
         if { ($next_column == "queue") || ($next_column == "master") || ($next_column == "slots") || ($next_column == "task_id")} {
            append output_xml_qstat($next_jobid,$next_column) "$next_xml_param "
            set node122 $node22
         } else {   
            set output_xml_qstat($next_jobid,$next_column) $next_xml_param
            set node122 $node22
         }

         #puts    $CHECK_OUTPUT " our final output for xml and jobtype $job_type is this.... \n"
         #parray output_xml_qstat
      
       }

    }

   set_error 0 "ok"

}


#                                                             max. column:     |
#****** parser_xml/qstat_xml_queue() ******
#
#  NAME
#     qstat_xml_queue -- Take XML node and return assoc array
#
#  SYNOPSIS
#     qstat_xml_queue -- Take XML node and return assoc array with
#                        queuename, qtype, used slots,total slots, load_avg, 
#                        arch, states.
#
#  FUNCTION
#     Return assoc array
#
#  INPUTS
#
#     qstat_xml_queue {node1 output}
#
#     node1  -  node in XML doc where we start navigation
#     output  -  asscoc array with the entries mentioned above.
#
#  NOTES
#
#
##*******************************

proc qstat_xml_queue { node1 output } {
   global CHECK_OUTPUT
   
   upvar $output output_xml_qstat
   # Try this way to look at the data....

   # Queue info (except that jobid info might be in
   # here as well....

   set node11 [$node1 firstChild]   ; #  <Queue-list/>
   set node111 [$node11 firstChild]  ; # <qname/>

   #puts $CHECK_OUTPUT " queue is [$node111 nodeValue] ....\n"
   set queue [$node111 nodeValue]
   set output_xml_qstat($queue,qname) $queue
   append output_xml_qstat($queue,state) ""
   lappend output_xml_qstat(queue_list) $queue

   set column_vars  "qtype used_slots total_slots load_avg arch state"

   foreach column $column_vars {

     set node12 [$node11 nextSibling]  ; # <queue name data/>

      if { $node12 == "" } { ;# Get out if at the end of tree
         break
      }
      set node122 [$node12 firstChild] ; # <parameters in queue listing/>
      #puts $CHECK_OUTPUT " param is [$node122 nodeValue] ....\n"
      set xml_param [$node122 nodeValue]
      set output_xml_qstat($queue,$column) $xml_param
      #puts $CHECK_OUTPUT "output_xml_qstat for ($queue,$column) is $xml_param ....\n"
      
      if { ($column == "load_avg") } {
            set output_xml_qstat($queue,$column) [format "%3.2f" $output_xml_qstat($queue,$column)]
         }           

      set node11 $node12  ; # Shift to next paramter in the list
      #puts $CHECK_OUTPUT "first queue_list_index is $queue_list_index ... \n"

   }

   #puts $CHECK_OUTPUT "queue info now.... \n"
   
   # Once we are done with the queue parameters, the next Sibling will be the
   # node pointing to job id information. We re-user queue_xml_jobid with
   # the  "full" flag, to pick information as listed in "qstat -f"
   
   set result12 [qstat_xml_jobid $node11 full output_xml_qstat]
   set  node11 $node12
   
  
   
   while { 1 } {
      set node22 [$node1 nextSibling]  ; # <queue-info/>
      if { $node22 ==""} { ;  # Get out if at the end of tree
         break
      }

      set node222 [$node22 firstChild]  ; # <Queue-list/>
      set name [$node222 firstChild]    ; # <qname2>
      set node1 $node22

      #puts $CHECK_OUTPUT " next queue is [$name nodeValue] ....\n"
      set queue [$name nodeValue]
      set output_xml_qstat($queue,qname) $queue
      append output_xml_qstat($queue,state) ""
      lappend output_xml_qstat(queue_list) $queue

      foreach column $column_vars {

         set node2 [$node222 nextSibling]  ; # <queue name data/>
         if { $node2 == "" } { ; # break if no more info
            continue
         }

         set node221 [$node2 firstChild] ; #
         #puts $CHECK_OUTPUT " next param is [$node221 nodeValue] ....\n"
         set xml_param [$node221 nodeValue]
         set output_xml_qstat($queue,$column) $xml_param
      
         if { ($column == "load_avg") } {
            set output_xml_qstat($queue,$column) [format "%3.2f" $output_xml_qstat($queue,$column)]
         }           

         set node222 $node2 ; # move to the next paramter

      }
      
      # Once we are done with the queue parameters, the next Sibling will be the
      # node pointing to job id information. We re-user queue_xml_jobid with
      # the  "full" flag, to pick information as listed in "qstat -f"
      
      set result222 [qstat_xml_jobid $node222 full output_xml_qstat]

   }

   set_error 0 "ok"

}
