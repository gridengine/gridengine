#                                                             max. column:     |
#****** parser/overview ***************************************
#
#  NAME
#     Parsing Functions -- parsing and processing of different input formats
#
#  SYNOPSIS
#     source parser.tcl
#     # call parsing functions
#
#  FUNCTION
#     The tcl library file parser.tcl provides a set of functions for
#     parsing and processing of input data coming for example from the
#     execution of programs like ps, qstat, qacct etc.
#     
#     The parsing functions take the input, apply certain filtering and
#     processing steps, and provide as output a uniform representation
#     of the data in a TCL array.
#
#     The following filtering/processing steps can be done:
#        - Replacements:
#          By this mechanism certain defined field contents can be replaced 
#          by other values. This may be needed for later processing steps.
#          Example: Output of qstat -ext contains "NA" in the columns cpu,
#                   mem and io when online accounting information is not yet 
#                   available. To be able to do computations on such a column,
#                   the value "NA" can be automatically replaced by the value 
#                   "0" during the parsing step.
#                   
#        - Transformations:
#          Transformations can be performed on the data of certain defined 
#          columns to change the data representation of the values.
#          Example: The output of qstat -ext contains the values for cpu 
#                   usage in the format "days:hours:minutes:seconds". To be
#                   able to do computations on cpu values, it is necessary
#                   to transform the given representation to a numerical
#                   value in seconds.
#
#                   Date and Time is often given in a textual representation.
#                   To do computations on date/time values, e.g. compute
#                   the time period between a start and an end timestamp, it
#                   is usefull to transform the date/time data to a UNIX-
#                   timestamp.
#
#        - Rules to handle multiple records for one output unit:
#          Often one record in the output array is built out of different
#          records in the input data. In this case, data values have to be
#          combined following a certain rule.
#          Example: The information given by qacct for a parallel job shall
#                   be output in one record. The resource values (cpu, mem and io)
#                   shall be summed up, the involved queues shall be returned 
#                   as a list, ...
#
#  EXAMPLES
#     Examples are given in the documentation of the different parsing
#     functions.
#     Also the functions parse_qstat and parse_qacct are a good example 
#     for the usage of the parsing functions.
#
#  SEE ALSO
#     parser/parse_fixed_column_lines
#     parser/process_named_record
#     parser/process_output_array
#     parser/overview_parsing_replacements
#     parser/overview_parsing_transformations
#     parser/overview_parsing_rules
#
#***************************************************************************
#
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
#****** parser/parse_fixed_column_lines() ***************************************
#
#  NAME
#     parse_fixed_column_lines -- parse fixed size input table
#
#  SYNOPSIS
#     parse_fixed_column_lines input output position 
#                              [start_line] [replace] [transform] 
#
#  FUNCTION
#     Parses an input table given as string in variable input with the following
#     format:
#       - table rows are separated by newline (\n)
#       - table columns have fixed width
#     The result is stored in a TCL array, the indices have the form
#     <row>,<column>, e.g. "0,4"; the first row or column has number 0, so
#     table indicese range from "0,0" to "n,m".
#     Header lines may be stripped by specifying a start_line > 0.
#     Certain contents of cells can be replaced, e.g. if a numerical cell
#     is empty (string ""), it could be set to 0.
#     A transformation can be performed while parsing the input, e.g. formatted
#     date/time can be transformed to UNIX timestamp.
#     Rules for replacement and transformation can be set per column.
#     In addition to the table cells, two entries are set in the output array
#     describing the tables dimensions: output(rows) and output(cols).
#
#  INPUTS
#     The parameters input, output, position, replace and transform are
#     passed by reference.
#
#     input        - name of the string variable containing the input table
#     output       - name of the output variable in which to place the resulting
#                    TCL array 
#     position     - name of the TCL array containing the positioning information.
#                    Contains one entry per column of the input table in the format
#                    "<start_position> <end_position>" where start_position and 
#                    end position are valid index parameters to the TCL function
#                    "string range". Example: "0 5" or "70 end".
#                    The array is indexed by the column number starting at 0 for the 
#                    first column, e.g. set position(0) "0 5".
#     [start_line] - line from which to start reading the table (default 0 = first line)
#     [replace]    - name of the TCL array containing rules to replace certain
#                    cell contents - if parameter is not passed to function, no replacements
#                    will be made.
#                    The index of the array is build as <column_number>,<string_to_replace>,
#                    the arrays values are the strings that replace any occurence of
#                    string_to_replace in column column_number.
#                    Example: set replace(0,) -1 sets each empty cell in row 0 to -1
#                             set replace(0,NA) -1 sets each cell containing NA in row 0 to -1
#     [transform]  - name of the TCL array containing rules to transform the contents of
#                    certain cells - if parameter is not passed to function, no transformations
#                    will be made.
#                    The array is indexed by the column number starting at 0 for the 
#                    first column, e.g. set transform(2) transform_date_time.
#                    The value of an array entry is a tcl command that is called with 
#                    a cells value as parameter and returns the new value.
#
#  RESULT
#     output - The resulting TCL array is placed in the variable that is referenced by
#              the parameter output in the callers namespace.
#
#  EXAMPLE
#     
#     source parser.tcl
#
#     set input "id num date
#     a 1 10/30/2000
#     a 2 10/31/2000
#     b 5 11/17/2000
#     - 8 01/05/2000"
#     
#     set position(0) "0 0"
#     set position(1) "2 2"
#     set position(2) "4 13"
#     
#     set replace(0,-) ?
#     
#     set transform(2) transform_date_time
#     
#     parse_fixed_column_lines input output position 1 replace transform
#     
#     output_array output
#     
#     Result: 
#     a       1       972860400
#     a       2       972946800
#     b       5       974415600
#     ?       8       947026800
#
#  NOTES
#     The output of parse_fixed_column_lines will usually be postprocessed
#     by the function process_output_array.
#     The function repeat_columns can be used to fill in missing information
#     into the output table of parse_fixed_column_lines.
#
#  SEE ALSO
#     parser/repeat_columns
#     parser/process_output_array
#     parser/overview_parsing_replacements
#     parser/overview_parsing_transformations
#
#***************************************************************************
#
proc parse_fixed_column_lines {input output position {start_line 0} 
                                                     {replace variable_not_set} 
                                                     {transform variable_not_set}} {
   upvar $input     in
   upvar $output    out
   upvar $position  pos
   upvar $replace   rep
   upvar $transform tra

   # split output lines into TCL-List
   set tmp [split $in "\n"]
   
   # compute array dimensions
   set num_lines [llength $tmp]
   set num_cols [array size pos]

   # split columns and create TCL array
   for { set i $start_line } { $i < $num_lines } { incr i } {
      for { set j 0 } { $j < $num_cols } { incr j } {
         set idx "[expr $i - $start_line],$j"
         set out($idx)  [string trim \
                           [string range \
                              [lindex $tmp $i] \
                              [lindex $pos($j) 0] \
                              [lindex $pos($j) 1] \
                           ] \
                        ]

         if { [info exists rep($j,$out($idx))] } {
            set out($idx) $rep($j,$out($idx))
         }

         if { [info exists tra($j)] } {
            set out($idx) [eval $tra($j) \"$out($idx)\"]
         }
      }
   }

   set out(rows) [expr $num_lines - $start_line]
   set out(cols) $num_cols
}

#                                                             max. column:     |
#****** parser/process_named_record() ***************************************
#
#  NAME
#     process_named_record -- parse records with named elements
#
#  SYNOPSIS
#     process_named_record input output delimiter index \
#                          [id] [head_line] [tail_line] \
#                          [replace] [transform] [rules]
#     
#
#  FUNCTION
#     Parses input data in the form of records that
#       - contains a tuple <field_name><whitespace><field_value> in each line
#       - records are separated by a fixed record delimiter
#
#     The records are stored in an TCL associative array, from which record fields
#     the index is created can be specified in a parameter.
#
#     Records can be filtered by the contents of any fields contained in the index
#     field list.
#
#     Heading or trailing lines can be excluded from parsing.
#
#     Certain input field values can be replaced by specifying a replace rule 
#     per field name.
#
#     Input field values can be transformed by specifying a transformation rule
#     per field name, it is for example possible to convert formatted date/time
#     to UNIX timestamp during the parsing of the input.
#     
#     If multiple records exist for one index value, a rule can be specified how to
#     merge the values, e.g. sum, average, build a list etc.
#
#  INPUTS
#     The parameters input, output, replace, transform and rules are
#     passed by reference.
#
#     input       - name of a string variable containing the input
#     output      - name of a TCL array into which the output is written
#     delimiter   - record delimiter (one line)
#     index       - list of fieldnames building the index
#     [id]        - list of fieldvalues refering to the index. Only records
#                   containing these field values will be processed.
#     [head_line] - number of lines to skip at the beginning of input
#     [tail_line] - number of lines to skip at the end of input
#     [replace]   - name of the TCL array containing rules to replace certain
#                   field contents - if parameter is not passed to function, no replacements
#                   will be made.
#                   The index of the array is build as <field_name>,<string_to_replace>,
#                   the arrays values are the strings that replace any occurence of
#                   string_to_replace in column column_number.
#                   Example: set replace(jobname,) noname sets each empty field with name jobname to noname
#                            set replace(cpu,NA) 0 sets each field with name cpu containing NA to 0
#     [transform] - name of the TCL array containing rules to transform the contents of
#                   certain cells - if parameter is not passed to function, no transformations
#                   will be made.
#                   The array is indexed by the field name.
#                   The value of an array entry is a tcl command that is called with 
#                   a cells value as parameter and returns the new value.
#     [rules]     - name of a TCL array containing rules to apply to field values
#                   if multiple records have the same index.
#                   The value of an array entry is the name of a TCL function that
#                   is called and is passed as parameters the value of the corresponding
#                   entry in the output array and the new value in the actual record.
#                   If no rule is set for a field, a new value replaces the old one.
#
#  RESULT
#     output - Name of a TCL array in which to place the resulting records.
#
#  EXAMPLE
#     source parser.tcl
#     
#     proc output_result {output} {
#        upvar $output out
#     
#        puts [format "%8s %-12s %-12s %-25s %8s" jobid task(s) jobname queue(s) cpu]
#        if { $out(index) == "" } {
#           puts [format "%8d %-12s %-12s %-25s %8d" $out(jobid) $out(taskid) $out(jobname) $out(queue) $out(cpu)]
#        } else {
#           foreach i $out(index) {
#              
#              puts [format "%8d %-12s %-12s %-25s %8d" $out(${i}jobid) $out(${i}taskid) $out(${i}jobname) $out(${i}queue) $out(${i}cpu)]
#           }
#        }
#     }   
#     
#     set input "some header line
#     jobid    123
#     taskid   1
#     jobname  sleeper.sh
#     queue    balrog.q
#     cpu      0:00:00:02
#     -------
#     jobid    124
#     taskid   1
#     jobname  worker.sh
#     queue    sowa.q
#     cpu      0:00:01:00
#     -------
#     jobid    124
#     taskid   2
#     jobname  worker.sh
#     queue    elendil.q
#     cpu      0:00:00:55
#     -------
#     jobid    124
#     taskid   3
#     jobname  worker.sh
#     queue    balrog.q
#     cpu      NA
#     ==========================
#     some trailing garbage ... 
#     in multiple lines
#     "
#     
#     set replace(cpu,NA) "0:00:00:00"
#     set transform(cpu)  transform_cpu
#     set rules(taskid)    rule_list
#     set rules(queue)     rule_list
#     set rules(cpu)       rule_sum
#     
#     # show all jobs, one record per jobid (means: join taskid's)
#     unset output
#     process_named_record input output "-------" "jobid" "" 1 3 replace transform rules
#     output_result output
#     
#     Result:
#        jobid task(s)      jobname      queue(s)                       cpu
#          123 1            sleeper.sh   balrog.q                         2
#          124 1 2 3        worker.sh    sowa.q elendil.q balrog.q      115
#     
#     # show all jobs, one record for each taskid
#     unset output
#     process_named_record input output "-------" "jobid taskid" "" 1 3 replace transform rules
#     output_result output
#     
#     Result:
#        jobid task(s)      jobname      queue(s)                       cpu
#          123 1            sleeper.sh   balrog.q                         2
#          124 1            worker.sh    sowa.q                          60
#          124 2            worker.sh    elendil.q                       55
#          124 3            worker.sh    balrog.q                         0 
#     
#     # show job 123
#     unset output
#     process_named_record input output "-------" "jobid" "123" 1 3 replace transform rules
#     output_result output
#     
#     Result:
#        jobid task(s)      jobname      queue(s)                       cpu
#          123 1            sleeper.sh   balrog.q                         2
#     
#     # show job 124, task 2
#     unset output
#     process_named_record input output "-------" "jobid taskid" "124 2" 1 3 replace transform rules
#     output_result output
#     
#     Result:
#        jobid task(s)      jobname      queue(s)                       cpu
#          124 2            worker.sh    elendil.q                       55
#     
#     # show all jobs that ran in queue balrog.q, one record per jobid
#     unset output
#     process_named_record input output "-------" "queue jobid" "balrog.q" 1 3 replace transform rules
#     output_result output
#     
#     Result:
#        jobid task(s)      jobname      queue(s)                       cpu
#          123 1            sleeper.sh   balrog.q                         2
#          124 3            worker.sh    balrog.q                         0
#
#  SEE ALSO
#     parser/overview_parsing_replacements
#     parser/overview_parsing_transformations
#     parser/overview_parsing_rules
#
#***************************************************************************
#

proc process_named_record {input output delimiter index {id ""}
                                                        {head_line 0}
                                                        {tail_line 0}
                                                        {replace variable_not_set}
                                                        {transform variable_not_set}
                                                        {rules variable_not_set}
                                                        {field_delimiter ""} } {
   upvar $input      in
   upvar $output     out
   upvar $replace    rep
   upvar $transform  tra
   upvar $rules      rul

   # split output lines into TCL-List
   set tmp [split $in "\n"]

   set num_lines [expr [llength $tmp] - $tail_line]
   set last_line [expr $num_lines - 1]

   # cleanup previous runs
   if {[info exists record]} {
      unset record
   }

   set out(index) ""

   # loop over all relevant lines
   for { set i $head_line } { $i < $num_lines } { incr i } {
      set line [lindex $tmp $i]
     
      # record or input end?
      if { [string compare $line $delimiter] == 0 || $i == $last_line } {
         # eval index and filter records according to parameter id
         set idxlen [llength $index]
         set idx ""
         set parse_record 1
         for {set j 0} {$j < $idxlen && $parse_record == 1} {incr j} {
            set idxpart [lindex $index $j]
            set idpart  [lindex $id $j]
            if { $idpart == "" } {
               append idx "$record($idxpart),"
            } else {
               if {[string compare $idpart $record($idxpart)] != 0} {
                  set parse_record 0
               }
            }
         }
        
         # merge record to output array
         if { $parse_record } {
            if { [lsearch -exact $out(index) $idx] == -1} {
               lappend out(index) $idx
            }
            foreach k [array names record] {
               set ridx "$idx$k"

               # if multiple entries exist for one index: apply rule
               if {[info exists out($ridx)] && [info exists rul($k)]} {
                  set out($ridx) [eval $rul($k) \"$out($ridx)\" \"$record($k)\"]
               } else {
                  set out($ridx) $record($k)
               }
            }
         }

         unset record      
      } else {
         # read record element 
         if { $field_delimiter == "" } {
            set idx [string trim [lindex $line 0]]
            set value [string trim [lrange $line 1 end]]
         } else {
            set helplist [split $line $field_delimiter]
            set helplist2 [lrange $helplist 1 end]
            set idx   [string trim [lindex $helplist 0]]
            set value [string trim [join $helplist2 $field_delimiter]]
         }

         # replace or set contents
         if {[info exists rep($idx,$value)]} {
            set record($idx) $rep($idx,$value)
         } else {
            set record($idx) $value
         }

         # transform contents
         if {[info exists tra($idx)]} {
            set record($idx) [eval $tra($idx) \"$record($idx)\"]
         }
      }
   }
}


#                                                             max. column:     |
#****** parser/process_output_array() ***************************************
#
#  NAME
#     process_output_array -- postprocessing of tables
#
#  SYNOPSIS
#     process_output_array input output names [id] [rules]
#
#  FUNCTION
#     The function takes a input a TCL array containing a 
#     data table indexed by "row,column".
#     It applies filtering and rules for the combination of
#     multiple rows and outputs a TCL array indexed by the
#     first column of the input table (optionally) and the column names
#     given in the parameter "names".
#
#  INPUTS
#     The parameters input, output, names and rules are
#     passed by reference.
#
#     input   - name of a TCL array containing the input
#     output  - name of a TCL array for the output
#     names   - name of a TCL array containing the column names; it is indexed
#               by the column number starting with 0
#     [id]    - optional value of cells in column 0 by which filtering is done.
#               If it's value is != "", only rows that have the value $id in
#               the first column are processed.
#               If id is not passed or its value is a string of length 0, all
#               rows from the input array are processed, the indexes in the 
#               output array are prefixed by the contents of column 0 from the 
#               input array.
#     [rules] - Rules to apply on values of cells, if multiple rows exist 
#               with the same value in the index column 0. 
#               A rule is a TCL expression that gets two parameters: the present
#               value of the output array for the specific index and the new
#               value of the actually parsed row.
#               For each column of the input table a rule can be defined, identified
#               by the column number as index of the array rules.
#               If no rule is specified for a column, new values will replace the 
#               present values.
#
#  RESULT
#     output - The resulting TCL array is placed in the variable that is referenced by
#              the parameter output in the callers namespace.
#
#  EXAMPLE
#     # Take the result of example for function parse_fixed_column_lines
#     a       1       972860400
#     a       2       972946800
#     b       5       974415600
#     ?       8       947026800
#
#     proc output_result {output} {
#        upvar $output out
#     
#        puts [format "%-5s %-10s %s" "id" "task(s)" "date"]
#        foreach i $out(index) {
#           puts [format "%-5s %-10s %s" $out(${i}id) $out(${i}task) [clock format $out(${i}start_date)]]
#        }
#     }
#
#     set names(0) id
#     set names(1) task          ; set rules(1) rule_list
#     set names(2) start_date    ; set rules(2) rule_min
#
#     process_output_array output newoutput names "" rules
#     puts [array names newoutput] ; output_result newoutput
#     Result:
#     index a,task a,start_date b,id id ?,id b,task b,start_date a,id task start_date ?,start_date ?,task
#     id    task(s)    date
#     a     1 2        Mon Oct 30 00:00:00 MET 2000
#     b     5          Fri Nov 17 00:00:00 MET 2000
#     ?     8          Wed Jan 05 00:00:00 MET 2000
#     
#     process_output_array output newoutput names a rules
#     puts [array names newoutput] ; output_result newoutput
#     Result:
#     index id start_date task
#     id    task(s)    date
#     a     1 2        Mon Oct 30 00:00:00 MET 2000
#
#
#  SEE ALSO
#     parser/parse_fixed_column_lines
#     parser/overview_parsing_rules
#
#***************************************************************************
#

proc process_output_array {input output names {id ""} {rules rules}} {
   upvar $input  in
   upvar $output out
   upvar $names  nam
   upvar $rules  rul
  
   set out(index) ""
  
   for { set i 0 } { $i < $in(rows) } { incr i } {
      # special id selected?
      if { $id != ""} {
         if {[string compare $in($i,0) $id] != 0 } {
            continue
         } else {
            set idx ""
         }
      } else {
         set idx "$in($i,0),"
      }

      if { [lsearch -exact $out(index) $idx] == -1} {
         lappend out(index) $idx
      }

      for { set j 0 } { $j < $in(cols) } { incr j } {
         set ridx "$idx$nam($j)"

         if {[info exists out($ridx)] && [info exists rul($j)]} {
            set out($ridx) [eval $rul($j) \"$out($ridx)\" \"$in($i,$j)\"]
         } else {
            set out($ridx) $in($i,$j)
         }
      }
   }
}


#                                                             max. column:     |
#****** parser/repeat_column() ***************************************
#
#  NAME
#     repeat_column -- repeat column contents where missing
#
#  SYNOPSIS
#     repeat_column input [column]
#
#  FUNCTION
#     Processes a table stored in a TCL array (e.g. output from 
#     parse_fixed_column_lines) and repeats values of cells where
#     they are missing in the following rows.
#     Example: Qstat output for parallel jobs outputs the jobid
#     only for the first task of the job in a certain queue, the
#     following tasks of this job in the same queue are listed 
#     without jobid. For easier processing of the job table, 
#     it is necessary to fill in the missing jobid's.
#
#  INPUTS
#     input    - TCL array containing a table, array indexes have the
#                form "row,column", e.g. "10,5"
#     [column] - column number in which to repeat missing values,
#                default is column 0
#
#  RESULT
#     Table in TCL array input is changed
#
#  SEE ALSO
#     parser/parse_fixed_column_lines
#
#***************************************************************************
#
proc repeat_column {input {column 0}} {
   upvar $input  in

   set last_id "-1"

   for { set i 0 } { $i < $in(rows) } { incr i } {
      if { $in($i,$column) == "" } {
         set in($i,$column) $last_id
      } else {
         set last_id $in($i,$column)
      }
   }
}


#                                                             max. column:     |
#****** parser/overview_parsing_replacements ***************************************
#
#  NAME
#     Parsing Replacements -- automatic replacement of certain cell contents
#
#  SYNOPSIS
#     set replace(<column/field>,<contents>) value
#
#  FUNCTION
#     For processing of data tables or records, it is sometimes necessary
#     to replace certain contents or to add missing contents.
#
#     Parsing Functions of this module allow the specification of a TCL array
#     describing replacement rules that will be automatically evaluated
#     during the parsing of input data.
#
#     Example:
#        If a numerical value is not yet known, its value is reported as "NA".
#        The occurence of "NA" in a table cell prohibits doing calculations
#        including this cell. 
#        Therefor it shall be replaced by "0".
#
#  EXAMPLE
#     # Value NA in cells of column 1 shall be replaced by 0
#     set replace(1,NA) 0
#     
#     # Missing values for record field "location" shall be replaced by "unknown"
#     set replace(location,) unknown
#
#  SEE ALSO
#     parser/parse_fixed_column_lines
#     parser/process_named_record
#
#***************************************************************************
#

#                                                             max. column:     |
#****** parser/overview_parsing_transformations ***************************************
#
#  NAME
#     Parsing Transformations -- tranformation of contents to other format
#
#  SYNOPSIS
#     set transform(column/field) expression
#
#  FUNCTION
#     To be able to process field or table cell contents it is often necessary
#     to change the data representation of the contents.
#
#     Parsing Functions of this module allow the specification of a TCL array
#     describing transformation rules that will be automatically evaluated
#     during the parsing of input data.
#
#     The parsing functions process the following TCL expression:
#     eval $transform(column/field) value
#
#     The specified transformation expression must be prepared to accept
#     exactly one parameter and return the transformed value.
#
#     Example:
#        To do calculations on date/time values, it is usefull to transform
#        their data representation from text format to UNIX-Timestamp.
#
#     The following transformation functions are provided in this module:
#        transform_duration: 
#           Transform a duration given as days:hours:minutes:seconds
#           where hour, minutes, seconds are written with leading 0 where
#           necessary to an integer representing the duration in seconds.
#
#        transform_date_time:
#           Transform a textual representation of date/time to a 
#           UNIX timestamp (seconds since 01/01/1970).
#           The textual representation must follow the rules defined in the
#           manual pages for the TCL command "clock scan".
#
#  EXAMPLE
#     set transform(start_time) transform_date_time
#
#  SEE ALSO
#     parser/parse_fixed_column_lines
#     parser/process_named_record
#
#***************************************************************************
#
#                                                             max. column:     |
#****** parser/transform_cpu() ******
# 
#  NAME
#     transform_cpu -- ??? 
#
#  SYNOPSIS
#     transform_cpu { s_cpu } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     s_cpu - ??? 
#
#  RESULT
#     ??? 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     ???/???
#*******************************
proc transform_cpu { s_cpu } {
   catch {
      scan $s_cpu "%d:%02d:%02d:%02d" days hours minutes seconds
      set cpu  [expr $days * 86400 + $hours * 3600 + $minutes * 60 + $seconds]
   }

   return $cpu
}

#                                                             max. column:     |
#****** parser/transform_date_time() ******
# 
#  NAME
#     transform_date_time -- ??? 
#
#  SYNOPSIS
#     transform_date_time { value } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     value - ??? 
#
#  RESULT
#     ??? 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     ???/???
#*******************************
proc transform_date_time { value } {
   if { $value == "" } {
      return ""
   } else {
      return [clock scan $value]
   }
}


#                                                             max. column:     |
#****** parser/overview_parsing_rules ***************************************
#
#  NAME
#     Parsing Rules -- Rules to combine multiple values
#
#  SYNOPSIS
#     set rules(field/column) functionname
#
#  FUNCTION
#     If an input table contains multiple rows that shall be combined into
#     one row in the output table, the data must be combined following certain
#     rules.
#     Therefor the processing functions in this module allow the specification
#     of rules that are applied to cells of certain table columns or record
#     fields.
#
#     The processing functions evaluate the following TCL expression:
#     eval $rules(field/column) present_output_value new_output_value
#
#     The functions representing a rule must be prepared to accept
#     two input values and return one combined output value.
#
#     The following rules are contained in this module:
#        rule_list:
#           Return a list containing the elements of both input values.
#
#        rule_sum:
#           Calculate the sum of the two input values.
#  
#        rule_min:
#           Return the smaller of the two input values.
#
#        rule_max:
#           Return the greater of the two input values. 
#  
#
#  EXAMPLE
#     set rules(5) rule_sum
#     set rules(start_time) rule_min
#     set rules(taskid) rule_list
#
#  SEE ALSO
#     parser/process_output_array
#     parser/process_named_record
#
#***************************************************************************
#
#                                                             max. column:     |
#****** parser/rule_list() ******
# 
#  NAME
#     rule_list -- ??? 
#
#  SYNOPSIS
#     rule_list { a b } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     a - ??? 
#     b - ??? 
#
#  RESULT
#     ??? 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     ???/???
#*******************************
proc rule_list { a b } {
   if { $a == {} } {
      return [list $a $b]
   } else {
      lappend a $b
      return $a
   }   
}

#                                                             max. column:     |
#****** parser/rule_sum() ******
# 
#  NAME
#     rule_sum -- ??? 
#
#  SYNOPSIS
#     rule_sum { a b } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     a - ??? 
#     b - ??? 
#
#  RESULT
#     ??? 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     ???/???
#*******************************
proc rule_sum { a b } {
   return [expr $a + $b]
}

#                                                             max. column:     |
#****** parser/rule_min() ******
# 
#  NAME
#     rule_min -- ??? 
#
#  SYNOPSIS
#     rule_min { a b } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     a - ??? 
#     b - ??? 
#
#  RESULT
#     ??? 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     ???/???
#*******************************
proc rule_min { a b } {
   if { $a <= $b } { 
      return $a
   } else {
      return $b
   }
}

#                                                             max. column:     |
#****** parser/rule_max() ******
# 
#  NAME
#     rule_max -- ??? 
#
#  SYNOPSIS
#     rule_max { a b } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     a - ??? 
#     b - ??? 
#
#  RESULT
#     ??? 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     ???/???
#*******************************
proc rule_max { a b } {
   if { $a >= $b } { 
      return $a
   } else {
      return $b
   }
}


#                                                             max. column:     |
#****** parser/parse_qstat() ***************************************
#
#  NAME
#     parse_qstat -- parse output of a qstat [-ext] command
#
#  SYNOPSIS
#     parse_qstat input output [jobid] [ext]
#
#  FUNCTION
#     Parses the output of a qstat or (in SGEEE) qstat -ext command. 
#     If a certain jobid is specified, only the information for
#     this job is returned, otherwise information for all jobs.
#  
#     The following processing is applied to data:
#        - numerical information containing empty strings or NA
#          is set to 0
#        - durations and data/time strings are transformed to 
#          UNIX timestamp
#
#     The following rules are applied to the data, if multiple values
#     have to be combined into one:
#        - take the minimum of submit/start times
#        - sum up all sort of resource values, tickets etc.
#        - build lists from qnames, task category (MASTER/SLAVE)
#          and taskid's
#
#  INPUTS
#     input   - name of the input string with data from qstat command
#     output  - name of the array in which to return results
#     [jobid] - jobid for filtering a certain job
#     [ext]   - 0: qstat command, 1: qstat -ext command
#
#  RESULT
#     The TCL array output is filled with the processed data.
#     If a certain jobid is specified, the arrays index consists of 
#     the columnnames (e.g. id, prior), if no jobid is specified, 
#     the index has the form "jobid,columnname" (e.g. 182,id).
#
#***************************************************************************
#
proc parse_qstat {input output {jobid ""} {ext 0}} {
   global ts_config
   upvar $input  in
   upvar $output out

   if { $ext } {
      if { $ts_config(gridengine_version) == 53 } {
         set   position(0)  "0 6"               ; set    names(0)    id
         set   position(1)  "8 12"              ; set    names(1)    prior
         set   position(2)  "14 23"             ; set    names(2)    name
         set   position(3)  "25 36"             ; set    names(3)    user
         set   position(4)  "38 53"             ; set    names(4)    project
         set   position(5)  "55 64"             ; set    names(5)    department
         set   position(6)  "66 70"             ; set    names(6)    state
         set   position(7)  "72 90"             ; set    names(7)    time
         set  transform(7)  transform_date_time
         set      rules(7)  rule_min
         set   position(8)  "92 108"            ; set    names(8)    deadline
         set  transform(8)  transform_date_time
         set   position(9)  "112 121"           ; set    names(9)    cpu
         set      rules(9)  rule_sum
         set    replace(9,) 0:00:00:00          ; set    replace(9,NA) 0:00:00:00
         set  transform(9)  transform_cpu
         set  position(10)  "123 129"           ; set    names(10)    mem
         set   replace(10,) 0                   ; set    replace(10,NA) 0
         set     rules(10)  rule_sum
         set  position(11) "131 137"            ; set    names(11)    io
         set   replace(11,) 0                   ; set    replace(11,NA) 0
         set     rules(11)  rule_sum
         set  position(12)  "139 143"           ; set    names(12)    tckts
         set  position(13)  "145 149"           ; set    names(13)    ovrts
         set  position(14)  "151 155"           ; set    names(14)    otckt
         set  position(15)  "157 161"           ; set    names(15)    dtckt
         set  position(16)  "163 167"           ; set    names(16)    ftckt
         set  position(17)  "169 173"           ; set    names(17)    stckt
         set  position(18)  "175 178"           ; set    names(18)    share
         set  position(19)  "180 190"           ; set    names(19)    queue
         set     rules(19)  rule_list
         set  position(20)  "192 198"           ; set    names(20)    master
         set     rules(20)  rule_list
         set  position(21)  "200 end"           ; set    names(21)    jatask
         set     rules(21)  rule_list
      }  else {
         set   position(0)  "0 6"               ; set    names(0)    id
         set   position(1)  "8 14"              ; set    names(1)    prior
         set   position(2)  "16 22"             ; set    names(2)    ntckts 
         set   position(3)  "24 33"             ; set    names(3)    name
         set   position(4)  "35 46"             ; set    names(4)    user
         set   position(5)  "48 63"             ; set    names(5)    project
         set   position(6)  "65 74"             ; set    names(6)    department
         set   position(7)  "76 80"             ; set    names(7)    state
         set   position(8)  "82 91"             ; set    names(8)    cpu
         set      rules(8)  rule_sum
         set    replace(8,) 0:00:00:00          ; set    replace(8,NA) 0:00:00:00
         set  transform(8)  transform_cpu
         set  position(9)  "93 99"              ; set    names(9)    mem
         set   replace(9,) 0                    ; set    replace(9,NA) 0
         set     rules(9)  rule_sum
         set  position(9) "101 107"             ; set   names(9)    io
         set   replace(9,) 0                    ; set   replace(9,NA) 0
         set     rules(9)  rule_sum
         set  position(10)  "109 113"           ; set   names(10)    tckts
         set  position(11)  "115 119"           ; set   names(11)    ovrts
         set  position(12)  "121 125"           ; set   names(12)    otckt
         set  position(13)  "127 131"           ; set   names(13)    ftckt
         set  position(14)  "133 137"           ; set   names(14)    stckt
         set  position(15)  "139 143"           ; set   names(15)    share
         set  position(16)  "145 154"           ; set   names(16)    queue
         set     rules(16)  rule_list

         # if { $petask } {
         #   set  position(17)  "192 198"           ; set   names(17)    master
         #   set     rules(17)  rule_list
         #    ...
         #    ...
         # } else {

         set  position(17)  "156 161"           ; set   names(17)     slots
         set  position(18)  "163 end"           ; set   names(18)    jatask
         set     rules(18)  rule_list

         # }
      }
   } else {
      if { $ts_config(gridengine_version) == 53 } {
         set   position(0)  "0 6"               ; set    names(0)    id   
         set   position(1)  "8 12"              ; set    names(1)    prior
         set   position(2)  "14 23"             ; set    names(2)    name
         set   position(3)  "25 36"             ; set    names(3)    user
         set   position(4)  "38 42"             ; set    names(4)    state
         set   position(5)  "44 62"             ; set    names(5)    time
         set  transform(5)  transform_date_time
         set   position(6)  "64 73"             ; set    names(6)    queue
         set      rules(6)  rule_list
         set   position(7)  "75 81"             ; set    names(7)    master
         set      rules(7)  rule_list
         set   position(8)  "83 end"            ; set    names(8)    jatask
         set      rules(8)  rule_list
      } else {
         set   position(0)  "0 6"               ; set    names(0)    id   
         set   position(1)  "8 14"              ; set    names(1)    prior
         set   position(2)  "16 25"             ; set    names(2)    name
         set   position(3)  "27 38"             ; set    names(3)    user
         set   position(4)  "40 44"             ; set    names(4)    state
         set   position(5)  "46 64"             ; set    names(5)    time
         set  transform(5)  transform_date_time
         set   position(6)  "66 75"             ; set    names(6)    queue
         set      rules(6)  rule_list
         set   position(7)  "77 82"             ; set    names(7)    master
         set      rules(7)  rule_list
         set   position(8)  "84 end"            ; set    names(8)    jatask
         set      rules(8)  rule_list
      }
   }

   # split text output of qstat to Array (list of lists)
   parse_fixed_column_lines in tmp position 2 replace transform

   # insert job id for multiplied pe task lines
   repeat_column tmp

   # process Array to associative Array
   process_output_array tmp out names $jobid rules
}


#                                                             max. column:     |
#****** parser/parse_qacct() ***************************************
#
#  NAME
#     parse_qacct -- parse information from qacct command
#
#  SYNOPSIS
#     parse_qacct input output [jobid]
#
#  FUNCTION
#     The function parses the output given from a qacct -j <jobid> command
#     and returns the information in a TCL array indexed by the fieldnames.
#     The following processing is applied to the data:
#        - taskids "unknown" are replaced by "1"
#        - Date/Time is transformed to UNIX timestamp
#     If multiple records are combined into one output record
#        - queuenames, hostnames, stati and taskid's are appended as lists
#        - resource values are summed up
#        - submit and starttime are the minimum of all values
#        - end time is the maximum of all values
#
#  INPUTS
#     input   - name of a string variable containing the output of qacct
#     output  - TCL array in which to store the results
#     [jobid] - jobid that was used for qacct command
#
#  RESULT
#     The output array is filled with the processed data.
#     If a jobid was specified, the array is indexed by the fieldnames,
#     if not, the index is built as "jobid,fieldname".
#
#***************************************************************************
#
proc parse_qacct {input output {jobid 0}} {
   upvar $input  in
   upvar $output out

   set rules(qname)           rule_list
   set rules(hostname)        rule_list
   set rules(qsub_time)       rule_min
   set rules(start_time)      rule_min
   set rules(end_time)        rule_max
   set rules(slots)           rule_max
   set rules(failed)          rule_list 
   set rules(exit_status)     rule_list
   set rules(ru_wallclock)    rule_max
   set rules(ru_utime)        rule_sum
   set rules(ru_stime)        rule_sum
   set rules(ru_maxrss)       rule_max
   set rules(ru_idrss)        rule_sum
   set rules(ru_minflt)       rule_sum
   set rules(ru_majflt)       rule_sum
   set rules(ru_nswap)        rule_sum
   set rules(ru_inblock)      rule_sum
   set rules(ru_oublock)      rule_sum
   set rules(ru_msgsnd)       rule_sum
   set rules(ru_msgrcv)       rule_sum
   set rules(ru_nsignals)     rule_sum
   set rules(cpu)             rule_sum
   set rules(mem)             rule_sum
   set rules(io)              rule_sum
   set rules(iow)             rule_sum
   set rules(taskid)          rule_list
   
   set replace(taskid,undefined) 1

   set transform(qsub_time)   transform_date_time
   set transform(start_time)  transform_date_time
   set transform(end_time)    transform_date_time

   set delimiter "=============================================================="
   
   process_named_record in out $delimiter "jobnumber" $jobid 1 0 replace transform rules
}

#****** parser/parse_qstat_j() *************************************************
#  NAME
#     parse_qstat_j() -- parse information from from qstat -j command
#
#  SYNOPSIS
#     parse_qstat_j { input output {jobid 0} } 
#
#  FUNCTION
#     The function parses the output given from a qstat -j <jobid> command
#     and returns the information in a TCL array indexed by the fieldnames.
#
#  INPUTS
#     input     - name of a string variable containing the output of qstat -j
#     output    - TCL array in which to store the results
#     {jobid 0} - jobid that was used for qstat -j command
#
#  RESULT
#     The output array is filled with the processed data.
#     If a jobid was specified, the array is indexed by the fieldnames,
#     if not, the index is built as "jobid,fieldname".
#
#*******************************************************************************
proc parse_qstat_j {input output {jobid 0} } {
   upvar $input  in
   upvar $output out

   set transform(submission_time)  transform_date_time
   set transform(execution_time)   transform_date_time
   process_named_record in out "no_delemiter___" "job_number" $jobid 0 0 variable_not_set transform variable_not_set ":"
}


#****** parser/parse_qconf_se() ************************************************
#  NAME
#     parse_qconf_se() -- parse information from qconf -se command
#
#  SYNOPSIS
#     parse_qconf_se { input output hostname } 
#
#  FUNCTION
#     This procedure parses the output given from a qconf -se command and
#     returns the information in a TCL array indexed by the fieldnames.
#
#  INPUTS
#     input    - name of a string variable containing the output of qconf -se
#     output   - TCL array in which to store the results
#     hostname - hostname of execution host for qconf -se command
#
#  RESULT
#     The output array is filled with the processed data.
#
#*******************************************************************************
proc parse_qconf_se { input output hostname } {
   upvar $input  in
   upvar $output out
 
    process_named_record in out "no_delemiter___" "hostname" $hostname
}

#                                                             max. column:     |
#****** parser/output_array() ******
# 
#  NAME
#     output_array -- ??? 
#
#  SYNOPSIS
#     output_array { input } 
#
#  FUNCTION
#     ??? 
#
#  INPUTS
#     input - ??? 
#
#  RESULT
#     ??? 
#
#  EXAMPLE
#     ??? 
#
#  NOTES
#     ??? 
#
#  BUGS
#     ??? 
#
#  SEE ALSO
#     ???/???
#*******************************
proc output_array { input } {
   upvar $input in

   puts "Array hat Dimension $in(rows) * $in(cols)"

   for { set i 0 } { $i < $in(rows) } { incr i } {
      for { set j 0 } { $j < $in(cols) } { incr j } {
         puts -nonewline "$in($i,$j)\t"
      }
      puts ""
   }   
}


