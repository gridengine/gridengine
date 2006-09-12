#___INFO__MARK_BEGIN_
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
#     qstat_xml_parse { output {param ""} }
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
#      param -  pass in param to qstat
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
#*******************************

proc qstat_xml_parse { output {param ""} } {

   global CHECK_OUTPUT

   catch { eval [package require tdom] } 

   upvar $output output_xml

   # Run now -xml command
   set XML [start_sge_bin  "qstat" "$param -xml" ]

   set doc  [dom parse $XML]

   set root [$doc documentElement]

   if { ($param == "-urg") } {
      set jobparam "urg"
   } elseif {  ($param == "-pri") } {
      set jobparam "pri"
   } elseif {  ($param == "-r") } {
      set jobparam "r"   
   } else {
      set jobparam "running"
   }
   
   # Parse the running jobs  using this node.
   set node [$root firstChild]   ; # <job-info/>
   set node1 [$node firstChild]  ; # <queue-info/>

   set result1 [qstat_xml_jobid $node1 $jobparam output_xml]

   # Parse the pending jobs info using this node. Need to start here
   # NOT at root.
   
   if { ($param == "-urg") } {
      set jobparam "urgpending"
   } elseif {  ($param == "-pri") } {
      set jobparam "pripending"
   } elseif {  ($param == "-r") } {
      set jobparam "rpending"
    } else {
      set jobparam "pending"
   }
   set node [$root firstChild]   ; # <job-info/>
   set node12 [$node nextSibling]  ; # <queue-info/>
   set node121 [$node12 firstChild]  ; # <qname/>

   set result2 [qstat_xml_jobid $node121 $jobparam output_xml]


   set_error 0 "ok"

}


#                                                             max. column:     |
#****** parser_xml/qstat_j_xml_parse() ******
#
#  NAME
#     qstat_j_xml_parse -- Generate XML output and return assoc array 
#
#  SYNOPSIS
#     qstat_j_xml_parse { output {param ""} }
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
#      param -  pass in param to qstat
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
#*******************************

proc qstat_j_xml_parse { output  } {

   global CHECK_OUTPUT

   catch { eval [package require tdom] } 

   upvar $output output_xml

   # Run now -xml command
   set XML [start_sge_bin  "qstat" "-j -xml" ]

   set doc  [dom parse $XML]

   set root [$doc documentElement]

   
   # Parse the running jobs  using this node.
   set node [$root firstChild]   ; # <qmaster_response/>

   set result1 [qstat_j_xml_jobid $node output_xml]


   set_error 0 "ok"

}

 




#                                                             max. column:     |
#****** parser_xml/qstat_j_JOB_NAME_xml_parse() ******
#
#  NAME
#     qstat_j_JOB_NAME_xml_parse -- Generate XML output and return assoc array 
#
#  SYNOPSIS
#     qstat_j_JOB_NAME_xml_parse { output {param ""} }
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
#      param -  pass in param to qstat
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
#*******************************

proc qstat_j_JOB_NAME_xml_parse { output {param ""} } {

   global CHECK_OUTPUT

   catch { eval [package require tdom] } 

   upvar $output output_xml

   # Run now -xml command
   set XML [start_sge_bin  "qstat" "-j $param -xml" ]

   set doc  [dom parse $XML]

   set root [$doc documentElement]
   
   # Parse the running jobs  using this node.
   set node [$root firstChild]   ; # <djob-info/>
   set node1 [$node firstChild]  ; # <qmaster_response/>

   set result1 [qstat_j_JOB_NAME_xml_jobid $node1 output_xml]


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
#      param - pass in "ext" or  "-ne" as params for qstat -f
#
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
#*******************************

proc qstat_f_xml_parse { output {param ""} } {

   global CHECK_OUTPUT

   catch { eval [package require tdom] } 

   upvar $output output_xml
   
   # Run now -xml command
   set XML [start_sge_bin  "qstat" "-f $param -xml" ]

   set doc  [dom parse $XML]

   set root [$doc documentElement]
   
   if { ($param == "-ext") } {
      set queueparam "fext"
   } elseif { ($param == "-r") } {
      set queueparam "fr"
   } elseif { ($param == "-urg") } {
      set queueparam "furg"   
   } else {
      set queueparam ""
   }
   
   # Parse the running jobs  using this node.
   set node [$root firstChild]   ; # <job-info/>
   set node1 [$node firstChild]  ; # <queue-info/>

   set result1 [qstat_xml_queue $node1 output_xml $queueparam]

   # Parse the pending jobs info using this node. Need to start here
   # NOT at root.
   
   if { ($param == "-ext") } {
      set jobparam "fextpending"
   } elseif { ($param == "-r") } {
      set jobparam "frpending"
   } elseif { ($param == "-urg") } {
      set jobparam "urgpending"   
   } else {
      set jobparam "full"
   }
   
   set node [$root firstChild]   ; # <job-info/>
   set node12 [$node nextSibling]  ; # <queue-info/>
   set node121 [$node12 firstChild]  ; # <qname/>

   set result2 [qstat_xml_jobid $node121 $jobparam output_xml]
   
   set_error 0 "ok"

}

 
#                                                             max. column:     |
#****** parser_xml/qstat_F_xml_parse() ******
#
#  NAME
#     qstat_F_xml_parse -- Generate XML output and return assoc array
#
#  SYNOPSIS
#     qstat_F_xml_parse { output {params ""} }
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
#      params  - args passed to the "qstat -F" command
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
#*******************************

proc qstat_F_xml_parse { output {params ""} } {

   global CHECK_OUTPUT

   catch { eval [package require tdom] } 

   upvar $output output_xml

   # Transform the params list into a comma separated list
   regsub " " $params "," arguments ;
   # Run now -xml command
   set XML [start_sge_bin  "qstat" "-F $arguments -xml" ]

   set doc  [dom parse $XML]

   set root [$doc documentElement]
   
   # Parse the running jobs  using this node.
   set node [$root firstChild]   ; # <job_info/>
   set node1 [$node firstChild]  ; # <queue-info/>

   # Transform the args list back into a ""  separated list
   regsub "," $arguments " " params ;
   set result1 [qstat_F_xml_queue $node1 output_xml $params]

   # Parse the pending jobs info using this node. Need to start here
   # NOT at root.
   
   set node [$root firstChild]   ; # <job-info/>
   set node12 [$node nextSibling]  ; # <queue-info/>
   set node121 [$node12 firstChild]  ; # <qname/>

   set result2 [qstat_xml_jobid $node121 full output_xml]
   
   set_error 0 "ok"

}



#                                                           max. column:     |
#****** parser_xml/qstat_j_xml_jobid() ******
#
#  NAME
#     qstat_j_xml_jobid -- Take XML node and return assoc array 
#
#  SYNOPSIS
#     qstat_j_xml_jobid -- Take XML node and return assoc array with
#                          entries jobid, message. 
#
#  FUNCTION
#     Return assoc array
#
#  INPUTS
#     
#     qstat_j_xml_jobid {node121  output} 
#
#     node121  -  node in XML doc where we start navigation
#    
#     output  -  asscoc array with the entries mentioned above.
#
#  NOTES
#     
#
#*******************************

proc qstat_j_xml_jobid { node1  output} {
   global CHECK_OUTPUT ts_config

   catch { eval [package require tdom] } 

   upvar $output output_xml_qstat
   
   set node121 [$node1 firstChild]  ; # <SME_message_list/>

   # If nothing, we have not started any jobs, so we return
   if { $node121 == "" } {
      return
   }
   set node1211 [$node121 firstChild]  ; # <element/>
   set node12111 [$node1211 firstChild]  ; # <SME_job_number_list/>
   set node124 [$node12111 firstChild] ; # <element/>
   
   set node125 [$node124 firstChild] ; # <ULNG/>
   set node126 [$node125 firstChild] ; # <elem/>
   set jobid [$node126 nodeValue]
                                
   set output_xml_qstat($jobid,jobid) $jobid
   lappend output_xml_qstat(jobid_list) $jobid

   puts $CHECK_OUTPUT "jobid is $jobid ....\n"
   
   set column_vars "job_number jobid_msg"
      

   foreach column $column_vars {
      set node21 [$node12111 nextSibling] ;
      if { $node21 == "" } {
         break
      }
      set node211 [$node21 firstChild] ; # <jobid info/

      if { $node211 == "" } { ; # we have hit the empty queue entry
         append output_xml_qstat($jobid,$column) ""
         set node12111 $node21
         continue
      }
      
      set xml_param [$node211 nodeValue]

      set output_xml_qstat($jobid,$column) $xml_param   
      set node12111 $node21
      
   }
  
   # The next list of jobs 
   set node1311 [$node1211 nextSibling]  ; # <next element/>
   set node13111 [$node1311 firstChild]  ; # <SME_job_number_list/>
   set node131111 [$node13111 firstChild] ; # <element/>
   
   set node135 [$node131111 firstChild] ; # <ULNG/>
   set node136 [$node135 firstChild] ; # <elem/>
   set jobid [$node136 nodeValue]
   
   set output_xml_qstat($jobid,jobid) $jobid
   lappend output_xml_qstat(jobid_list) $jobid
         
   puts $CHECK_OUTPUT "jobid is $jobid ....\n"
  
   foreach column $column_vars {
      set node31 [$node13111 nextSibling] ;
      if { $node31 == "" } {
         break
      }
      set node311 [$node31 firstChild] ; # <jobid info/

      if { $node311 == "" } { ; # we have hit the empty queue entry
         append output_xml_qstat($jobid,$column) ""
         set node13111 $node31
         continue
      }
      
      set xml_param [$node311 nodeValue]
      
      set output_xml_qstat($jobid,$column) $xml_param   
      set node13111 $node31
      
  }
 
  set_error 0 "ok"

}


#                                                           max. column:     |
#****** parser_xml/qstat_xml_jobid() ******
#
#  NAME
#     qstat_xml_jobid -- Take XML node and return assoc array 
#
#  SYNOPSIS
#     qstat_xml_jobid -- Take XML node and return assoc array with
#                        entries jobid, prior, name, user, state, total_time,
#                        queue slots and task_id if needed. 
#
#  FUNCTION
#     Return assoc array
#
#  INPUTS
#     
#     qstat_xml_jobid {node121 jobtype output} 
#
#     node121  -  node in XML doc where we start navigation
#     jobtype  -  "running" or "pending", "ext" or "extpending" which tells us which 
#                  fields to expect
#     output  -  asscoc array with the entries mentioned above.
#
#  NOTES
#     
#
#*******************************

proc qstat_xml_jobid { node121 jobtype output} {
   global CHECK_OUTPUT

   catch { eval [package require tdom] } 

   upvar $output output_xml_qstat
   

   # Add var to tell if doing a running job parse or pending job parse
   # jobtype = {runing, pending, full, ext, ext_pending}
   
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


   set output_xml_qstat($jobid,jobid) $jobid
   lappend output_xml_qstat(jobid_list) $jobid

   # Use the names from the parse_plain_qstat output :)
   # set column_vars "prior name user state time queue master"
   # As in parse_plain_qstat case, for pending jobs, queue and task_id entries
   # are set to blank.
   
   # For -ext, column order is: job-ID prior ntckts name  user
   # project department state cpu mem io tckts ovrts otckt ftckt stckt share queue ja-task-ID
   # The -ext_pending has missing data for: cpu mem io tckts ovrtcts queue ja-task-ID
   
   if { $jobtype == "running" } { ; # this is for listing of qstat running jobs 
      set column_vars "prior name user state time queue master task_id"
   } elseif { $jobtype == "pending" } { ; # this is for listing of qstat pending jobs
      set column_vars "prior name user state time queue master"
      set output_xml_qstat($jobid,task_id) ""
   }  elseif { $jobtype == "full" } { ; # this is for listing of qstat -F jobs
      set column_vars "prior name user state time slots task_id"   
   }
   if { $jobtype == "extpending" } { ; # this is for listing qstat -ext pending jobs
      set column_vars "prior ntckts name  user project department state tckts ovrts job_share otckt ftckt  \
      stckt share slots slots"
      append output_xml_qstat($jobid,cpu) " "
      append output_xml_qstat($jobid,mem) " "
      append output_xml_qstat($jobid,io) " "
      append output_xml_qstat($jobid,queue) " "      
      append output_xml_qstat($jobid,task_id) " "
   }
   
   if { $jobtype == "fextpending" } { ; # this is for listing qstat -f -ext pending jobs
      set column_vars "prior ntckts name user project department state tckts ovrts job_share \
      otckt ftckt stckt share slots"
      append output_xml_qstat($jobid,cpu) " "
      append output_xml_qstat($jobid,mem) " "
      append output_xml_qstat($jobid,io) " "
      append output_xml_qstat($jobid,task_id) " "
   }
   if { $jobtype == "ext" } { ; # this is for listing qstat -ext jobs
      set column_vars "prior ntckts name  user project department state cpu mem io tckts \
      ovrts job_share otckt ftckt stckt share queue slots task_id"
      append output_xml_qstat($jobid,task_id) " "
   }
   if { $jobtype == "fext" } { ; # this is for listing qstat -f -ext jobs
      set column_vars "prior ntckts name  user project department state cpu mem io tckts \
      ovrts job_share otckt ftckt stckt share slots task_id"
   }
   if { $jobtype == "urg" } { ; # this is for listing qstat -urg jobs
      set column_vars "prior nurg urg rrcontr wtcontr dlcontr name user state time  \
      queue slots task_id"
      append output_xml_qstat($jobid,deadline) " "
      append output_xml_qstat($jobid,task_id) " "
   }
   
   if { $jobtype == "furg" } { ; # this is for listing qstat -f -urg jobs. See IZ 2072.
      set column_vars "prior nurg urg rrcontr wtcontr dlcontr name user state time  \
      slots task_id"
      append output_xml_qstat($jobid,deadline) " "
      append output_xml_qstat($jobid,task_id) " "
   }
   
   if { $jobtype == "urgpending" } { ; # this is for listing qstat -urg jobs
      set column_vars "prior nurg urg rrcontr wtcontr dlcontr name user state time  \
      slots slots"
      append output_xml_qstat($jobid,deadline) " "
      append output_xml_qstat($jobid,queue) " "
      append output_xml_qstat($jobid,task_id) " "
   }
   
   if { $jobtype == "pri" } { ; # this is for listing qstat -pri jobs
      set column_vars "prior npprior ppri name  user state time queue slots task_id "
   }
   if { $jobtype == "pripending" } { ; # this is for listing qstat -pri pending jobs
      set column_vars "prior npprior ppri name  user state time slots slots"
      append output_xml_qstat($jobid,queue) ""
      append output_xml_qstat($jobid,task_id) ""
   }
   if { $jobtype == "r" } { ; # this is for listing qstat -r jobs
      set column_vars "prior  name  user state time queue slots task_id \
                       hard_req_queue hard_resource "
      append output_xml_qstat($jobid,full_jobname) ""
      append output_xml_qstat($jobid,master_queue) ""
      append output_xml_qstat($jobid,hard_resource) ""
      append output_xml_qstat($jobid,soft_resource) ""
      append output_xml_qstat($jobid,hard_req_queue) ""
      append output_xml_qstat($jobid,req_pe_value) ""
      append output_xml_qstat($jobid,granted_pe_value) ""
   }
   if { $jobtype == "rpending" } { ; # this is for listing qstat -r pending jobs
      set column_vars "prior  name  user state time slots slots hard_req_queue"
      append output_xml_qstat($jobid,queue) ""
      append output_xml_qstat($jobid,task_id) ""
      append output_xml_qstat($jobid,full_jobname) ""
      append output_xml_qstat($jobid,master_queue) ""
      append output_xml_qstat($jobid,hard_resource) ""
      append output_xml_qstat($jobid,soft_resource) ""
      append output_xml_qstat($jobid,hard_req_queue) ""
      append output_xml_qstat($jobid,req_pe_value) ""
      append output_xml_qstat($jobid,granted_pe_value) ""
   }
   
	if { $jobtype == "frpending" } { ; # this is for listing qstat -f -r pending jobs
      set column_vars "prior  name  user state time slots  hard_req_queue"
      append output_xml_qstat($jobid,queue) ""
      append output_xml_qstat($jobid,task_id) ""
      append output_xml_qstat($jobid,full_jobname) ""
      append output_xml_qstat($jobid,master_queue) ""
      append output_xml_qstat($jobid,hard_resource) ""
      append output_xml_qstat($jobid,soft_resource) ""
      append output_xml_qstat($jobid,hard_req_queue) ""
      append output_xml_qstat($jobid,req_pe_value) ""
      append output_xml_qstat($jobid,granted_pe_value) ""
   }
	
    if { $jobtype == "fr" } { ; # this is for listing qstat -f -r jobs; See IZ 2071.
      set column_vars "prior name user state time slots task_id \
                       hard_req_queue hard_resource "
      append output_xml_qstat($jobid,full_jobname) ""
      append output_xml_qstat($jobid,master_queue) ""
      append output_xml_qstat($jobid,hard_resource) ""
      append output_xml_qstat($jobid,soft_resource) ""
      append output_xml_qstat($jobid,hard_req_queue) ""
      append output_xml_qstat($jobid,req_pe_value) ""
      append output_xml_qstat($jobid,granted_pe_value) ""
   }

      
   #puts $CHECK_OUTPUT "$jobtype for jobid $jobid column_vars are $column_vars ... \n"
   puts  " "

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
      
      #puts $CHECK_OUTPUT " xml_param for $column is $xml_param ... \n"
      puts  " "

      # For time, need the UNIX value, to compare with plain output.
      if { ($column == "time") } {
         set xml_param  [transform_date_time $xml_param]
      }   
      
      # In the case of qstat -r, we get hard_req_queue after slots, not task_id
      if { ($jobtype == "r") && ($column == "task_id") && [regexp "\[a-zA-Z.\]"  $xml_param] } {
          set output_xml_qstat($jobid,hard_req_queue) $xml_param
          set node1211 $node21
          continue
      }
      
      if { ($jobtype == "rpending") && ($column == "hard_req_queue") && [regexp "\[0-9\]"  $xml_param] } {
          set output_xml_qstat($jobid,slots) $xml_param
          set node1211 $node21
          continue
      }
      
      # In the case of qstat -f -r, we get hard_req_queue after slots, not task_id
      if { ($jobtype == "fr") && ($column == "task_id") && [regexp "\[a-zA-Z.\]"  $xml_param] } {
          set output_xml_qstat($jobid,hard_req_queue) $xml_param
          set node1211 $node21
          continue
      }
      
      if { ($column == "hard_req_queue") && ($jobtype == "r") && [regexp "lx" $xml_param] || \
           [regexp "sol" $xml_param]} {
          set output_xml_qstat($jobid,hard_resource) "arch=$xml_param"
          #set output_xml_qstat($jobid,$column) ""
          set node1211 $node21
          continue
      }
      
      if { ($column == "hard_req_queue") && ($jobtype == "fr") && [regexp "lx" $xml_param] || \
           [regexp "sol" $xml_param]} {
          set output_xml_qstat($jobid,hard_resource) "arch=$xml_param"
          #puts $CHECK_OUTPUT "output_xml_qstat($jobid,hard_resource) is  $output_xml_qstat($jobid,hard_resource) ... \n"
          #set output_xml_qstat($jobid,$column) ""
          set node1211 $node21
          continue
      }
      
      # For colums "queue" and others we need to append rather than set
      # since we can have more than one entries.
      if { ($column == "queue") || ($column == "master") || ($column == "slots") || ($column == "task_id")} {
         append output_xml_qstat($jobid,$column) "$xml_param "
         set node1211 $node21
      } else {
         set output_xml_qstat($jobid,$column) $xml_param      
         set node1211 $node21
      }
     
      if { ($column == "hard_resource") } {
         set output_xml_qstat($jobid,hard_resource) "arch=$xml_param"
      }
      
   }

   while { 1 } {

      set node13  [$node121 nextSibling]  ; # <next jobid/>
      if { $node13 == "" } {
         break
      }

      set node122 [$node13 firstChild] ; #
      set node1212 [$node122 firstChild]  ; # <next jobid info/>

      set next_jobid [$node1212 nodeValue]

      set output_xml_qstat($next_jobid,jobid) $next_jobid
      lappend output_xml_qstat(jobid_list) $next_jobid
      
      #puts $CHECK_OUTPUT "$jobtype for jobid $jobid column_vars are $column_vars ... \n"   
      puts  " "
      
      set node121 $node13 ; # yes, node121, NOT node122...
      
      foreach next_column $column_vars {
         set node22 [$node122 nextSibling] ;
         if { $node22 == "" } {
            continue
         }
         set node221 [$node22 firstChild] ; # <jobid info/>

         if { $node221 == "" } { ; # we hit the empty queue entry
            append output_xml_qstat($next_jobid,$next_column) ""
            set node122 $node22
            continue        
         }

         set next_xml_param [$node221 nodeValue]

         # In the case of qstat -r, we get hard_req_queue after slots, not task_id
         if { ($jobtype == "r")  && ($next_column == "task_id") && [regexp "\[a-zA-Z.\]" $next_xml_param] } {
            set output_xml_qstat($next_jobid,hard_req_queue) $next_xml_param
            set node1211 $node21
            continue
         }
         
         # In the case of qstat -f -r, we get hard_req_queue after slots, not task_id
         if { ($jobtype == "fr")  && ($next_column == "task_id") && [regexp "\[a-zA-Z.\]" $next_xml_param] } {
            set output_xml_qstat($next_jobid,hard_req_queue) $next_xml_param
            set node1211 $node21
            continue
         }
         
         
         if { ($jobtype == "rpending") && ($next_column == "hard_req_queue") && [regexp "\[0-9\]"  $next_xml_param] } {
            set output_xml_qstat($next_jobid,slots) $next_xml_param
            set node1211 $node21
            continue
         }
         
         
         if { ($next_column == "hard_req_queue") && ($jobtype == "r") && [regexp "lx" $next_xml_param] || \
              [regexp "sol" $next_xml_param] } {
           set output_xml_qstat($next_jobid,hard_resource) "arch=$next_xml_param"
           #set output_xml_qstat($next_jobid,$next_column) ""
           set node1211 $node21
           continue
         }
         
         if { ($next_column == "hard_req_queue") && ($jobtype == "fr") && [regexp "lx" $next_xml_param] || \
              [regexp "sol" $next_xml_param] } {
           set output_xml_qstat($next_jobid,hard_resource) "arch=$next_xml_param"
           #set output_xml_qstat($next_jobid,$next_column) ""
           set node1211 $node21
           continue
         }

         #puts $CHECK_OUTPUT "next_xml_param for $next_column is $next_xml_param ... \n"
         puts  " "
         
         if { ($next_column == "time") } {
            set next_xml_param  [transform_date_time $next_xml_param]
         }   
            
         # For colums "queue", "master", "slots", "task_id" we need to append
         # rather than set since we can have more than one entries.
         if { ($next_column == "queue") || ($next_column == "master") || ($next_column == "slots") || ($next_column == "task_id")} {
            append output_xml_qstat($next_jobid,$next_column) "$next_xml_param "
            set node122 $node22
         } else {   
            set output_xml_qstat($next_jobid,$next_column) $next_xml_param
            set node122 $node22
         }
         
         if { ($next_column == "hard_resource") } {
            set output_xml_qstat($next_jobid,hard_resource) "arch=$next_xml_param"
         }
         
       }

    }

   set_error 0 "ok"

}



#                                                             max. column:     |
#****** parser_xml/qstat_j_JOB_NAME_xml_jobid() ******
#
#  NAME
#     qstat_j_JOB_NAME_xml_jobid -- Take XML node and return assoc array
#
#  SYNOPSIS
#     qstat_j_JOB_NAME_xml_jobid -- Take XML node and return assoc array with
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
#     qstat_j_JOB_NAME_xml_jobid {node121  output}
#
#     node121  -  node in XML doc where we start navigation
#
#     output  -  asscoc array with the entries mentioned above.
#
#  NOTES
#
#
#*******************************

proc qstat_j_JOB_NAME_xml_jobid { node121 output} {
   global CHECK_OUTPUT

   catch { eval [package require tdom] }

   upvar $output output_xml_qstat


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


   set output_xml_qstat($jobid,jobid) $jobid
   lappend output_xml_qstat(jobid_list) $jobid

   #puts $CHECK_OUTPUT "jobid in qstat_j_ERROR_xml_jobid is $jobid ... \n"
   set  column_vars "job_name version session department exec_file script_file \
                     script_size submission_time execution_time deadline owner uid group \
                     gid account notify type reserve priority jobshare shell_list verify \
                     env_list job_args checkpoint_attr checkpoint_object checkpoint_interval \
                     restart stdout_path_list merge_stderr hard_queue_list mail_options \
                     mail_list ja_structure ja_template ja_tasks host verify_suitable_queues \
                     nrunning soft_wallclock_gmt hard_wallclock_gmt override_tickets urg nurg \
                     nppri rrcont dlcontr wtcont"; # .... stop at </qmaster_response>

  
   # Here are sub-vars for some of the  vars above.
   set shell_list_vars "path host file_host file_staging"

   # sge_o_home_tag sge_o_home sge_o_logname_tag sge_o_logname ... then
   set env_list_vars "sge_o_path_tag sge_o_path sge_o_shell_tag sge_o_shell \
                      sge_o_mail_tag sge_o_mail sge_o_host_tag \
                      sge_o_host sge_o_workdir_tag sge_o_workdir"

   set job_args_vars "job_args_value"

   set stdout_path_list_vars "path host file_host file_staging"

   set hard_queue_list_args "hard_queue_list_value"

   set mail_list_vars "user host"

   set ja_structure_vars "min max step"

   set ja_template_vars "task_number status start_time end_time hold job_restarted stat pvm_ckpt_pid \
                         pending_signal pending_signal_delivery_time pid fshare tix oticket _fticket \
                         sticket share suitable pe_object next_pe_task_id stop_initiate_time prio ntix"

   set ja_tasks_vars "task_number status start_time end_time hold job_restarted state pvm_ckpt_pid pending_signal \
                      pending_signal_delivery_time pid usage_list fshare tix oticket fticket sticket share \
                      suitable previous_usage_list pe_object next_pe_task_id stop_initiate_time pri \
                      ntix message_list"

   set usage_list_vars "usage_list_submission_time usage_list_submission_time_value usage_list_priority \
                        usage_list_priority_value"

   set previous_usage_list_vars "previous_usage_list_submission_time previous_usage_list_submission_time_value \
                                 previous_usage_list_priority previous_usage_list_priority_value"

   set message_list_vars "type message"

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

       if { ($column == "env_list") } { ; # get the sge_o vars

         set node1311 [$node211  firstChild]

         foreach env_column "sge_o_home_tag sge_o_home"  {
            ##set node31 [$node1311 nextSibling] ; works
            set node31 $node1311
            if { $node31 == "" } {
               continue
            }
            set node311 [$node31 firstChild] ; # <jobid info/

            if { $node311 == "" } { ; # we have hit the empty queue entry
               append output_xml_qstat($jobid,$env_column) ""
               set node1311 [$node31 nextSibling]
               continue
            }

            set env_xml_param [$node311 nodeValue]
            #puts $CHECK_OUTPUT "env_xml_param is $env_xml_param ... \n"
            set output_xml_qstat($jobid,$env_column) $env_xml_param
            set node1311 [$node31 nextSibling] ; # works
          }

          set node1312 [$node211 nextSibling]
          set node1311 [$node1312 firstChild]

          foreach env_column "sge_o_log_name_tag sge_o_log_name" {
             set node31 $node1311
            if { $node31 == "" } {
               continue
            }
            set node311 [$node31 firstChild] ; # <jobid info/

            if { $node311 == "" } { ; # we have hit the empty queue entry
              append output_xml_qstat($jobid,$env_column) ""
              set node1311 [$node31 nextSibling]
             continue
            }

            set env_xml_param [$node311 nodeValue]
            #puts $CHECK_OUTPUT "env_xml_param2 is $env_xml_param ... \n"
            set output_xml_qstat($jobid,$env_column) $env_xml_param
            set node1311 [$node31 nextSibling]
          }

          set node1313 [$node1312 nextSibling]
          set node1311 [$node1313 firstChild]

          foreach env_column "sge_o_path_tag sge_o_path" {
             set node31 $node1311
             if { $node31 == "" } {
                #set node1313 [$node1312 nextSibling]
                #set node1311 [$node1313 firstChild]
                #set node1312 $node1313
                #puts $CHECK_OUTPUT "we are in the first continue for env_list_vars... \n"
                continue
             }
             set node311 [$node31 firstChild] ; # <jobid info/
          
               if { $node311 == "" } { ; # we have hit the empty queue entry
               append output_xml_qstat($jobid,$env_column) ""
               set node1311 [$node31 nextSibling]
               continue
            }

            set env_xml_param [$node311 nodeValue]
            #puts $CHECK_OUTPUT "env_xml_param3 is $env_xml_param ... \n"
            set output_xml_qstat($jobid,$env_column) $env_xml_param
            set node1311 [$node31 nextSibling]
          }

          set node1314 [$node1313 nextSibling]
          set node1311 [$node1314 firstChild]

          foreach env_column "sge_o_shell_tag sge_o_shell " {
             set node31 $node1311
             if { $node31 == "" } {
                continue
             }
             set node311 [$node31 firstChild] ; # <jobid info/

             if { $node311 == "" } { ; # we have hit the empty queue entry
                append output_xml_qstat($jobid,$env_column) ""
                set node1311 [$node31 nextSibling]
                continue
             }

             set env_xml_param [$node311 nodeValue]
             #puts $CHECK_OUTPUT "env_xml_param4 is $env_xml_param ... \n"
             set output_xml_qstat($jobid,$env_column) $env_xml_param
             set node1311 [$node31 nextSibling]
          }

          set node1315 [$node1314 nextSibling]
          set node1311 [$node1315 firstChild]

          foreach env_column "sge_o_mail_tag sge_o_mail" {
             set node31 $node1311
             if { $node31 == "" } {
                continue
             }
             set node311 [$node31 firstChild] ; # <jobid info/

             if { $node311 == "" } { ; # we have hit the empty queue entry
                append output_xml_qstat($jobid,$env_column) ""
                set node1311 [$node31 nextSibling]
                continue
             }

             set env_xml_param [$node311 nodeValue]
             #puts $CHECK_OUTPUT "env_xml_param5 is $env_xml_param ... \n"
             set output_xml_qstat($jobid,$env_column) $env_xml_param
             set node1311 [$node31 nextSibling]
          }

          # If we have time zone info, we are off by 1 set of
          # params. Need to do mail again

          if { ($output_xml_qstat($jobid,sge_o_mail_tag) == "__SGE_PREFIX__O_TZ") } {

             set node1315 [$node1315 nextSibling]
             set node1311 [$node1315 firstChild]

             foreach env_column "sge_o_mail_tag sge_o_mail" {
                set node31 $node1311
                if { $node31 == "" } {
                   continue
                }
                set node311 [$node31 firstChild] ; # <jobid info/

                if { $node311 == "" } { ; # we have hit the empty queue entry
                   append output_xml_qstat($jobid,$env_column) ""
                   set node1311 [$node31 nextSibling]
                   continue
                }

                set env_xml_param [$node311 nodeValue]
                #puts $CHECK_OUTPUT "env_xml_param5 is $env_xml_param ... \n"
                set output_xml_qstat($jobid,$env_column) $env_xml_param
                set node1311 [$node31 nextSibling]
             }

          }

          set node1316 [$node1315 nextSibling]
          set node1311 [$node1316 firstChild]

          foreach env_column "sge_o_host_tag sge_o_host" {
             set node31 $node1311
             if { $node31 == "" } {
                continue
              }
             set node311 [$node31 firstChild] ; # <jobid info/

             if { $node311 == "" } { ; # we have hit the empty queue entry
                append output_xml_qstat($jobid,$env_column) ""
                set node1311 [$node31 nextSibling]
                continue
             }

             set env_xml_param [$node311 nodeValue]
             #puts $CHECK_OUTPUT "env_xml_param6 is $env_xml_param ... \n"
             set output_xml_qstat($jobid,$env_column) $env_xml_param
             set node1311 [$node31 nextSibling]
          }

          set node1317 [$node1316 nextSibling]
          set node1311 [$node1317 firstChild]

          foreach env_column "sge_o_workdir_tag sge_o_workdir" {
             set node31 $node1311
             if { $node31 == "" } {
                continue
             }
             set node311 [$node31 firstChild] ; # <jobid info/

             if { $node311 == "" } { ; # we have hit the empty queue entry
                append output_xml_qstat($jobid,$env_column) ""
                set node1311 [$node31 nextSibling]
                continue
             }

             set env_xml_param [$node311 nodeValue]
             #puts $CHECK_OUTPUT "env_xml_param7 is $env_xml_param ... \n"
             set output_xml_qstat($jobid,$env_column) $env_xml_param
             set node1311 [$node31 nextSibling]
          }

     }

      #puts $CHECK_OUTPUT " xml_param for $column is $xml_param ... \n"

      set output_xml_qstat($jobid,$column) $xml_param
      set node1211 $node21

   }

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
#     qstat_xml_queue {node1 output {param ""} }
#
#     node1  -  node in XML doc where we start navigation
#     output  -  asscoc array with the entries mentioned above.
#     param  - pass in param ext for -ext output
#
#  NOTES
#
#
#*******************************

proc qstat_xml_queue { node1 output {param ""} } {
   global CHECK_OUTPUT

   upvar $output output_xml_qstat
   # Try this way to look at the data....

   # Queue info (except that jobid info might be in
   # here as well....

   set node11 [$node1 firstChild]   ; #  <Queue-list/>
   set node111 [$node11 firstChild]  ; # <qname/>

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
      set xml_param [$node122 nodeValue]
      set output_xml_qstat($queue,$column) $xml_param

      if { ($column == "load_avg") } {
            set output_xml_qstat($queue,$column) [format "%3.2f" $output_xml_qstat($queue,$column)]
      }

      set node11 $node12  ; # Shift to next paramter in the list

   }

   # Once we are done with the queue parameters, the next Sibling will be the
   # node pointing to job id information. We re-use queue_xml_jobid with
   # the  following flags:  "ext" for -ext; "fext" for -f -ext; and "full"
   # for -f.

   if { ($param == "ext") } {
      set jobparam "ext"
   } elseif { ($param == "fext") } {
      set jobparam "fext"
   } elseif { ($param == "fr") } {
      set jobparam "fr"
   } elseif { ($param == "furg") } {
      set jobparam "furg"   
   } else {
      set jobparam "full"
   }
   
   set result12 [qstat_xml_jobid $node11 $jobparam output_xml_qstat]
   set  node11 $node12


   while { 1 } {
      set node22 [$node1 nextSibling]  ; # <queue-info/>
      if { $node22 ==""} { ;  # Get out if at the end of tree
         break
      }

      set node222 [$node22 firstChild]  ; # <Queue-list/>
      set name [$node222 firstChild]    ; # <qname2>
      set node1 $node22

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
         set xml_param [$node221 nodeValue]
         set output_xml_qstat($queue,$column) $xml_param

         if { ($column == "load_avg") } {
            set output_xml_qstat($queue,$column) [format "%3.2f" $output_xml_qstat($queue,$column)]
         }

         set node222 $node2 ; # move to the next paramter

      }

      # Once we are done with the queue parameters, the next Sibling will be the
      # node pointing to job id information. We re-use queue_xml_jobid with
      # the  following flags: "extpending" for -ext; "fextpending" for -f -ext;
      # "full" for -f;

      if { ($param == "ext") } {
         set jobparam "ext"
      } elseif { ($param == "fext") } {
         set jobparam "fext"
      } elseif { ($param == "fr") } {
         set jobparam "fr"
      } elseif { ($param == "furg") } {
      set jobparam "furg"   
      } else {
         set jobparam "full"
      }
   
      set result222 [qstat_xml_jobid $node222 $jobparam output_xml_qstat]

    }

   set_error 0 "ok"

}

#                                                             max. column:     |
#****** parser_xml/qstat_F_xml_queue() ******
#
#  NAME
#     qstat_F_xml_queue -- Take XML node and return assoc array
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
#     qstat_F_xml_queue {node1 output {params ""} }
#
#     node1   -  node in XML doc where we start navigation
#     output  -  asscoc array with the entries mentioned above.
#     params  - args for qstat -F; "" for the whole set, or
#              "rerun h_vmem" for a subset.
#
#  NOTES    This parser only works for default complexes configuration
#
#
#*******************************

proc qstat_F_xml_queue { node1 output {params ""} } {
   global CHECK_OUTPUT
   
   upvar $output output_xml_qstat
   # Try this way to look at the data....

   # Queue info (except that jobid info might be in
   # here as well....

   set node11 [$node1 firstChild]   ; #  <Queue-list/>
   set node111 [$node11 firstChild]  ; # <qname/>

   set queue [$node111 nodeValue]
   set output_xml_qstat($queue,qname) $queue
   append output_xml_qstat($queue,state) ""
   lappend output_xml_qstat(queue_list) $queue
   
   if { $params == "" } {  
      set column_vars  "qtype used_slots total_slots load_avg arch \
                     hl:arch hl:num_proc hl:mem_total hl:swap_total hl:virtual_total \
                     hl:load_avg hl:load_short hl:load_medium hl:load_long hl:mem_free \
                     hl:swap_free hl:virtual_free hl:mem_used hl:swap_used hl:virtual_used \
                     hl:cpu hl:np_load_avg hl:np_load_short hl:np_load_medium hl:np_load_long \
                     qf:qname qf:hostname qc:slots qf:tmpdir qf:seq_no qf:rerun qf:calendar \
                     qf:s_rt qf:h_rt qf:s_cpu qf:h_cpu qf:s_fsize qf:h_fsize qf:s_data \
                     qf:h_data qf:s_stack qf:h_stack qf:s_core qf:h_core qf:s_rss \
                     qf:h_rss qf:s_vmem qf:h_vmem qf:min_cpu_interval"
      
   } elseif { $params == "rerun h_vmem" } {             
      set column_vars  "qtype used_slots total_slots load_avg arch \
                        qf:rerun qf:h_vmem"
   }
   
   foreach column $column_vars {

     set node12 [$node11 nextSibling]  ; # <queue name data/>

      if { $node12 == "" } { ;# Get out if at the end of tree
         break
      }
      set node122 [$node12 firstChild] ; # <parameters in queue listing/>
      set xml_param [$node122 nodeValue]
      set output_xml_qstat($queue,$column) $xml_param
      
      if { ($column == "load_avg") } {
            set output_xml_qstat($queue,$column) [format "%3.2f" $output_xml_qstat($queue,$column)]
         }           

      set node11 $node12  ; # Shift to next paramter in the list
      
   }
   
   # Once we are done with the queue parameters, the next Sibling will be the
   # node pointing to job id information. We re-user queue_xml_jobid with
   # the  "full" flag, to pick information as listed in "qstat -F"
   # We use a slightly different list of column vars, since it seems
   # that is what the XML schema does! See IZ 2049.
   
   if { $params == ""  } { 
      set column_vars  "qtype used_slots total_slots load_avg arch \
                     hl:load_avg hl:load_short hl:load_medium hl:load_long \
                     hl:arch hl:num_proc hl:mem_free hl:swap_free hl:virtual_free \
                     hl:mem_total hl:swap_total hl:virtual_total hl:mem_used \
                     hl:swap_used hl:virtual_used hl:cpu hl:np_load_avg \
                     hl:np_load_short hl:np_load_medium hl:np_load_long \
                     qf:qname qf:hostname qc:slots qf:tmpdir qf:seq_no qf:rerun \
                     qf:calendar qf:s_rt qf:h_rt qf:s_cpu qf:h_cpu qf:s_fsize \
                     qf:h_fsize qf:s_data qf:h_data qf:s_stack qf:h_stack \
                     qf:s_core qf:h_core qf:s_rss qf:h_rss qf:s_vmem \
                     qf:h_vmem qf:min_cpu_interval"
              
   } elseif { $params == "rerun h_vmem" } {             
      set column_vars  "qtype used_slots total_slots load_avg arch \
                        qf:rerun qf:h_vmem"
   }
   
   set node13 [$node11 nextSibling]
   set result12 [qstat_xml_jobid $node13 full output_xml_qstat]
   
   while { 1 } {
      set node22 [$node1 nextSibling]  ; # <queue-info/>
      if { $node22 ==""} { ;  # Get out if at the end of tree
         break
      }

      set node222 [$node22 firstChild]  ; # <Queue-list/>
      set name [$node222 firstChild]    ; # <qname2>
      set node1 $node22

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
         set xml_param [$node221 nodeValue]
         set output_xml_qstat($queue,$column) $xml_param
      
         if { ($column == "load_avg") } {
            set output_xml_qstat($queue,$column) [format "%3.2f" $output_xml_qstat($queue,$column)]
         }           

         set node222 $node2 ; # move to the next paramter

      }
      
      # Once we are done with the queue parameters, the next Sibling will be the
      # node pointing to job id information. We re-user queue_xml_jobid with
      # the  "full" flag, to pick information as listed in "qstat -F"
      
      set node223 [$node222 nextSibling]
      set result222 [qstat_xml_jobid $node223 full output_xml_qstat]

   }

   set_error 0 "ok"

}


#                                                             max. column:     |
#****** parser_xml/qstat_g_c_xml_parse() ******
#
#  NAME
#     qstat_g_c_xml_parse -- Generate XML output and return assoc array
#
#  SYNOPSIS
#     qstat_g_c_xml_parse { output }
#                     -- Generate XML output and return assoc array with
#                        entries clusterqueue, cqload, used, avail, total,
#                        aoACDS, cdsuE.
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
#*******************************

proc qstat_g_c_xml_parse { output } {

   global CHECK_OUTPUT

   catch { eval [package require tdom] } 
   
   upvar $output output_xml

   # Run now -xml command
   set XML [start_sge_bin  "qstat" "-g c -xml" ]

   set doc  [dom parse $XML]

   set root [$doc documentElement]
   
   # Parse the running jobs  using this node.
   set node [$root firstChild]   ; # <cluster-queue-info/>
   #set node1 [$node firstChild]  ; # 

   set result1 [qstat_g_c_xml_queue $node output_xml]
   
   set_error 0 "ok"

}

#                                                             max. column:     |
#****** parser_xml/qstat_g_c_xml_queue() ******
#
#  NAME
#     qstat_g_c_xml_queue -- Take XML node and return assoc array
#
#  SYNOPSIS
#     qstat_g_c_xml_queue -- Take XML node and return assoc array with
#     clusterqueue, cqload, used, avail, total,aoACDS, cdsuE.
#
#  FUNCTION
#     Return assoc array
#
#  INPUTS
#
#     qstat_g_c_xml_queue {node1 output}
#
#     node1  -  node in XML doc where we start navigation
#     output  -  asscoc array with the entries mentioned above.
#
#  NOTES
#
#
#*******************************

proc qstat_g_c_xml_queue { node output } {
   global CHECK_OUTPUT

   upvar $output output_xml_qstat
   # Try this way to look at the data....

   # Queue info (except that jobid info might be in
   # here as well....

   set node1 [$node firstChild]
   set node11 [$node1 firstChild]  ; # <cluster_queue_summary/>

   if { $node11 == "" } { ;# Get out if at the end of tree
      break
   }
   
   set queue [$node11 nodeValue]
   set output_xml_qstat($queue,clusterqueue) $queue
   lappend output_xml_qstat(queue_list) $queue

   set column_vars  "cqload used avail total aoACDS cdsuE"

   foreach column $column_vars {

      set node12 [$node1 nextSibling]  ; # <cluster queue data/>

      set node122 [$node12 firstChild] ; # <parameters in listing/>
      set xml_param [$node122 nodeValue]
      set output_xml_qstat($queue,$column) $xml_param
      # Format the cqload output, so we can compare it to the plain output
      if { ($column == "cqload") } {
         set output_xml_qstat($queue,$column) [format "%3.2f" $output_xml_qstat($queue,$column)]
      }

      set node1 $node12  ; # Shift to next paramter in the list

   }

   while { 1 } {
      set node22 [$node nextSibling]  ; # <cluster-queue-info/>
      if { $node22 ==""} { ;  # Get out if at the end of tree
         break
      }

      set node $node22
      set node222 [$node22 firstChild]  
      set node222 [$node22 firstChild]  
      set node2222 [$node222 firstChild]  
      set queue [$node2222 nodeValue]

      set output_xml_qstat($queue,clusterqueue) $queue
      lappend output_xml_qstat(queue_list) $queue

      foreach column $column_vars {
         set node2 [$node222 nextSibling]  ; # <queue name data/>
         if { $node2 == "" } { ; # continue if no more info
            continue
         }

         set node221 [$node2 firstChild] ; #
         set xml_param [$node221 nodeValue]
         set output_xml_qstat($queue,$column) $xml_param
         # Format the cqload output, so we can compare it to the plain output
         if { ($column == "cqload") } {
            set output_xml_qstat($queue,$column) [format "%3.2f" $output_xml_qstat($queue,$column)]
         }

         set node222 $node2 ; # move to the next paramter
      }

   }

   set_error 0 "ok"

}

 
#                                                             max. column:     |
#****** parser_xml/qstat_ext_xml_parse() ******
#
#  NAME
#     qstat_ext_xml_parse -- Generate XML output and return assoc array 
#
#  SYNOPSIS
#     qstat_ext_xml_parse { output }
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
#*******************************

proc qstat_ext_xml_parse { output } {

   global CHECK_OUTPUT

	catch { eval [package require tdom] } 

   upvar $output output_xml

   # Run now -xml command
   set XML [start_sge_bin  "qstat" "-ext -xml" ]
   #puts $CHECK_OUTPUT "Printing the xml result of qstat $option -xml... \n"
   #puts $CHECK_OUTPUT "$XML \n"

   set doc  [dom parse $XML]

   set root [$doc documentElement]

   # Parse the running jobs  using this node.
   set node [$root firstChild]   ; # <job-info/>
   set node1 [$node firstChild]  ; # <joblisting/>
  
   set job_type1 "ext"
   set result1 [qstat_xml_jobid $node1 $job_type1 output_xml]
   #puts $CHECK_OUTPUT "calling job_type is  $job_type1 ... \n"

   # Parse the pending jobs info using this node. Need to start here
   # NOT at root.
   set node [$root firstChild]   ; # <job-info/>
   set node12 [$node nextSibling]  ; # <queue-info/>
   set node121 [$node12 firstChild]  ; # <qname/>

   set job_type2 "extpending"
   set result2 [qstat_xml_jobid $node121 $job_type2 output_xml]
   #puts $CHECK_OUTPUT "second calling job_type is  $job_type2 ... \n"

   set_error 0 "ok"

}

