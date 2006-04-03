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
#      output  -  asscoc array with the entries mentioned above.
#                 Output array is similar to that of 
#                 parse_qstat {input output {jobid ""} {ext 0} {do_replace_NA 1 } }
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

   package require tdom

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

   package require tdom

   upvar $output output_xml_qstat
   # Add var to tell if doing a running job parse or pending job parse
   # job_type = {runing or pending}
   
   # If nothing, we have not started any jobs, so we return
   if { $node121 == "" } {
      return
   }
   set node1211 [$node121 firstChild]  ; # <qname1/>
   set node12111 [$node1211 firstChild]  ; # <jobid/
   set jobid [$node12111 nodeValue]


   #puts $CHECK_OUTPUT " jobid node is $jobid ....\n"

   set output_xml_qstat($jobid,jobid) $jobid

   # Use the names from the parse_plain_qstat output :)
   # set column_vars "prior name user state time queue master"
   # As in parse_plain_qstat case, for pending jobs, queue and task_id entries
   # are set to blank.
   if { $job_type == "running" } {
      set column_vars "prior name user state time queue master task_id"
   } elseif { $job_type == "pending" } {
      set column_vars "prior name user state time queue master"
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
      if { ($column == "queue") || ($column == "master") } {
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
         if { ($next_column == "queue") || ($next_column == "master") } {
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

