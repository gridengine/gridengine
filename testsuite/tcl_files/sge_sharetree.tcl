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

#****** sge_sharetree/add_sharetree_node() *******************************************
#  NAME
#     add_sharetree_node() -- add sharetree node project user1 shares1 user2 shares2
#
#  SYNOPSIS
#     add_sharetree_node { project user1 shares1 user2 shares2  {on_host ""} {as_user ""}  {raise_error 1}  }
#
#  FUNCTION
#     Calls qconf -astnode project to add user1, user2 to project with shares1,  shares2
#
#  INPUTS
#     project         - project for which  we wish to see sharetree; 
#     user1           - user1   for which  we wish to chage shares; 
#     shares1         - shares   for user1;
#     user2           - user2   for which  we wish to chage shares; 
#     shares2        - shares   for user12
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
proc add_sharetree_node {project user1 shares1 user2 shares2 {on_host ""} {as_user ""} {raise_error 1}} {

   set ret 0
   set result [start_sge_bin "qconf" "-astnode /$project=100,/$project/$user1=$shares1,/$project/$user2=$shares2" $on_host $as_user]

   # parse output or raise error
   if {$prg_exit_state == 0} {
      set ret  $result 
   } else {
      set ret [add_sharetree_node_error $prg_exit_state $project $user1 $shares1 $user2 $shares2 $raise_error]
   }

   return $ret

}

#****** sge_sharetree/add_sharetree_node_error() ***************************************
#  NAME
#     add_sharetree_node_error() -- error handling for add_sharetree_node
#
#  SYNOPSIS
#     add_sharetree_node_error { result project user1 shares1 user2 shares2 raise_error }
#
#  FUNCTION
#     Does the error handling for add_sharetree_node.
#     Translates possible error messages of qconf -astnode,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     add_sharetree_node. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     project     - project for which qconf -astnode has been called
#     user1           - user1   for which  we wish to chage shares;
#     shares1         - shares   for user1;
#     user2           - user2   for which  we wish to chage shares;
#     shares2        - shares   for user2
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for add_sharetree_node function:
#      -1: "wrong_calendar" is not a calendar
#     -99: other error
#
#  SEE ALSO
#     sge_calendar/get_calendar
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc add_sharetree_node_error {result project user1 shares1 user2 shares2 raise_error} {
 
   # recognize certain error messages and return special return code
   set messages(index) "-1"
   set messages(-1) [translate_macro MSG_CALENDAR_XISNOTACALENDAR_S $calendar]

   # we might have version dependent, calendar specific error messages
   get_sharetree_error_vdep messages $project

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "add_sharetree_node" "qconf -astnode /$project=100,/$project/$user1=$shares1,/$project/$user2=$shares2" $result messages $raise_error]

   return $ret
}

 
#****** sge_sharetree/get_sharetree_list() *****************************************
#  NAME
#     get_sharetree_list() -- get a list of all sharetrees
#
#  SYNOPSIS
#     get_sharetree_list { project {on_host ""} {as_user ""} {raise_error 1}  }
#
#  FUNCTION
#     Calls qconf -sstnode /$project to retrieve all sharetrees for project
#
#  INPUTS
#     project         - project for which we do  qconf -sstnode
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
proc get_sharetree_list { project {on_host ""} {as_user ""} {raise_error 1}} {

   return [get_qconf_list "get_sharetree_list" "-sstnode /$project" out $on_host $as_user $raise_error]

}

#****** sge_sharetree/mod_sharetree_node() *******************************************
#  NAME
#     mod_sharetree_node() -- modify sharetree node project user1 shares1 user2 shares2
#
#  SYNOPSIS
#     mod_sharetree_node { project user1 shares1 user2 shares2 {on_host ""} {as_user ""}  {raise_error 1}
#     }
#
#  FUNCTION
#     Calls qconf -mstnode project to modify shares of user1, user2 in  project
#
#  INPUTS
#     project         - project for which  we wish to see sharetree;
#     user1           - user1   for which  we wish to chage shares;
#     shares1         - shares   for user1;
#     user2           - user2   for which  we wish to chage shares;
#     shares2        - shares   for user12
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
proc mod_sharetree_node {project user1 shares1 user2 shares2 {on_host ""} {as_user ""} {raise_error 1}} {

   set ret 0
   set result [start_sge_bin "qconf" "-mstnode /$project=100,/$project/$user1=$shares1,/$project/$user2=$shares2" $on_host $as_user]

   # parse output or raise error
   if {$prg_exit_state == 0} {
      set ret $result 
   } else {
      set ret [mod_sharetree_node_error $prg_exit_state $project $user1 $shares1 $user2 $shares2 $raise_error]
   }

   return $ret

}

#****** sge_sharetree/mod_sharetree_node_error() ***************************************
#  NAME
#     mod_sharetree_node_error() -- error handling for mod_sharetree_node
#
#  SYNOPSIS
#     mod_sharetree_node_error { result project user1 shares1 user2 shares2 raise_error }
#
#  FUNCTION
#     Does the error handling for mod_sharetree_node.
#     Translates possible error messages of qconf -astnode,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     mod_sharetree_node. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     project     - project for which qconf -astnode has been called
#     user1           - user1   for which  we wish to chage shares;
#     shares1         - shares   for user1;
#     user2           - user2   for which  we wish to chage shares;
#     shares2        - shares   for user2
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for mod_sharetree_node function:
#      -1: "wrong_calendar" is not a calendar
#     -99: other error
#
#  SEE ALSO
#     sge_calendar/get_calendar
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc mod_sharetree_node_error {result project user1 shares1 user2 shares2 raise_error} {

   # recognize certain error messages and return special return code
   set messages(index) "-1"
   set messages(-1) [translate_macro MSG_CALENDAR_XISNOTACALENDAR_S $calendar]

   # we might have version dependent, calendar specific error messages
   get_sharetree_error_vdep messages $project

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "mod_sharetree_node" "qconf -mstnode /$project=100,/$project/$user1=$shares1,/$project/$user2=$shares2" $result messages $raise_error]

   return $ret
}
#****** sge_sharetree/del_sharetree_node() *******************************************
#  NAME
#     del_sharetree_node() -- delete sharetree node project user
#
#  SYNOPSIS
#     del_sharetree_node { project user {on_host ""} {as_user ""}  {raise_error 1}
#     }
#
#  FUNCTION
#     Calls qconf -dstnode project to delete user in  project
#
#  INPUTS
#     project         - project for which  we wish to see sharetree;
#     user            - user   which  we wish to delete
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
proc del_sharetree_node {project user {on_host ""} {as_user ""} {raise_error 1}} {

   set ret 0
   set result [start_sge_bin "qconf" "-dstnode /$project/$user" $on_host $as_user]

   # parse output or raise error
   if {$prg_exit_state == 0} {
      set ret  $result 
   } else {
      set ret [del_sharetree_node_error $prg_exit_state $project $user $raise_error]
   }

   return $ret

}

#****** sge_sharetree/del_sharetree_node_error() ***************************************
#  NAME
#     del_sharetree_node_error() -- error handling for del_sharetree_node
#
#  SYNOPSIS
#     del_sharetree_node_error { result project user raise_error }
#
#  FUNCTION
#     Does the error handling for del_sharetree_node.
#     Translates possible error messages of qconf -dstnode,
#     builds the datastructure required for the handle_sge_errors
#     function call.
#
#     The error handling function has been intentionally separated from
#     del_sharetree_node. While the qconf call and parsing the result is
#     version independent, the error messages (macros) usually are version
#     dependent.
#
#  INPUTS
#     result      - qconf output
#     project     - project for which qconf -astnode has been called
#     user        - user   which  we wish to delete
#     raise_error - do add_proc_error in case of errors
#
#  RESULT
#     Returncode for del_sharetree_node function:
#      -1: "wrong_user" is not a user
#     -99: other error
#
#  SEE ALSO
#     sge_calendar/get_calendar
#     sge_procedures/handle_sge_errors
#*******************************************************************************
proc del_sharetree_node_error {result project user raise_error} {
   # recognize certain error messages and return special return code
   set messages(index) "-1"
   set messages(-1) [translate_macro MSG_CALENDAR_XISNOTACALENDAR_S $calendar]

   # we might have version dependent, calendar specific error messages
   get_sharetree_error_vdep messages $project

   set ret 0
   # now evaluate return code and raise errors
   set ret [handle_sge_errors "del_sharetree_node" "qconf -dstnode /$project/$user" $result messages $raise_error]

   return $ret
}

#****** sge_sharetree/del_sharetree() ******************************************
#  NAME
#     del_sharetree() -- delete the sharetree
#
#  SYNOPSIS
#     del_sharetree { {on_host ""} {as_user ""} {raise_error 1} } 
#
#  FUNCTION
#     Deletes the sharetree.
#
#  INPUTS
#     {on_host ""}    - execute qconf on this host (default: qmaster host)
#     {as_user ""}    - execute qconf as this user (default: CHECK_USER)
#     {raise_error 1} - raise error condition in case of errors?
#
#  RESULT
#       0: a sharetree existed and has been deleted
#       1: there was no sharetree to delete
#     < 0: error, see sge_procedures/get_sge_error()
#
#  SEE ALSO
#     sge_procedures/get_sge_error()
#*******************************************************************************
proc del_sharetree {{on_host ""} {as_user ""} {raise_error 1}} {
   set result [start_sge_bin "qconf" "-dstree" $on_host $as_user]

   set messages(index) "0 1"
   set messages(0) [translate_macro MSG_SGETEXT_REMOVEDLIST_SSS "*" "*" [translate_macro MSG_OBJ_SHARETREE]]
   set messages(1) [translate_macro MSG_SGETEXT_DOESNOTEXIST_S [translate_macro MSG_OBJ_SHARETREE]]

   set ret [handle_sge_errors "del_sharetree" "qconf -dstree" $result messages $raise_error]

   return $ret
}

#****** sge_sharetree/stree_buffer_init() **************************************
#  NAME
#     stree_buffer_init() -- initialize a sharetree buffer
#
#  SYNOPSIS
#     stree_buffer_init { stree_var } 
#
#  FUNCTION
#     Initializes a sharetree buffer.
#
#     A sharetree buffer is used to build, modify, query a sharetree
#     in memory.
#     
#     Sharetree nodes can be added, modified, deleted.
#     The shares assigned to a certain node can be queried.
#
#     An in memory sharetree buffer can be made permanent: 
#     It is sent to sge_qmaster via qconf -Mstree.
#
#     A sharetree buffer can be read into memory by querying 
#     it from qmaster (qconf -sstree).
#
#     Dumping a sharetree buffer shows it in a user readable form.
#
#  INPUTS
#     stree_var - name of the sharetree buffer variable
#
#  NOTES
#     Internal representation of a sharetree buffer:
#     stree(index) lists the nodes (node names)
#     stree(<node>) stores the shares of a node
#     stree(<node>,children) stores the names of a nodes child nodes
#
#  SEE ALSO
#     sge_sharetree/del_sharetree()
#     sge_sharetree/stree_buffer_add_node()
#     sge_sharetree/stree_buffer_mod_node()
#     sge_sharetree/stree_buffer_del_node()
#     sge_sharetree/stree_buffer_get_node()
#     sge_sharetree/stree_buffer_commit()
#     sge_sharetree/stree_buffer_read()
#     sge_sharetree/stree_buffer_dump()
#     sge_sharetree/test_stree_buffer()
#*******************************************************************************
proc stree_buffer_init {stree_var} {
   upvar $stree_var stree

   # if the buffer already exists - delete it
   if {[info exists stree]} {
      unset stree
   }

   # initialize Root node
   set stree(index) {/}
   set stree(/) 1
   set stree(/,children) {}
}

#****** sge_sharetree/stree_buffer_add_node() **********************************
#  NAME
#     stree_buffer_add_node() -- add a node to a sharetree buffer
#
#  SYNOPSIS
#     stree_buffer_add_node { stree_var node shares } 
#
#  FUNCTION
#     Adds a node to the given sharetree buffer.
#     Only operates on the sharetree buffer.
#     To modify the sharetree in qmaster, call stree_buffer_commit.
#
#  INPUTS
#     stree_var - sharetree buffer
#     node      - node name
#     shares    - node's shares
#
#  RESULT
#     0   - on success
#     < 0 - on error
#
#  SEE ALSO
#     sge_sharetree/stree_buffer_commit()
#*******************************************************************************
proc stree_buffer_add_node {stree_var node shares} {
   upvar $stree_var stree

   set ret 0

   if {[lsearch -exact $stree(index) $node] >= 0 || [info exists stree($node)]} {
      add_proc_error "stree_buffer_add_node" -1 "sharetree node $node already exists"
      set ret -1
   } else {
      set parent [file dirname $node]
      if {[lsearch -exact $stree(index) $parent] < 0 || ![info exists stree($parent)]} {
         add_proc_error "stree_buffer_add_node" -1 "parent node for sharetree node $node does not exist"
         set ret -2
      } else {
         lappend stree(index) $node
         set stree(index) [lsort -dictionary $stree(index)]

         set stree($node) $shares
         set stree($node,children) {}

         lappend stree($parent,children) $node
         set stree($parent,children) [lsort -dictionary $stree($parent,children)]
      }
   }

   return $ret
}

#****** sge_sharetree/stree_buffer_mod_node() **********************************
#  NAME
#     stree_buffer_mod_node() -- modify a node in sharetree buffer
#
#  SYNOPSIS
#     stree_buffer_mod_node { stree_var node shares } 
#
#  FUNCTION
#     Modify a node in the sharetree buffer (sets new value for shares)
#     Only operates on the sharetree buffer.
#     To modify the sharetree in qmaster, call stree_buffer_commit.
#
#  INPUTS
#     stree_var - sharetree buffer
#     node      - node name
#     shares    - new value for shares
#
#  RESULT
#     0   - on success
#     < 0 - on error
#
#  SEE ALSO
#     sge_sharetree/stree_buffer_commit()
#*******************************************************************************
proc stree_buffer_mod_node {stree_var node shares} {
   upvar $stree_var stree

   set ret 0

   if {[lsearch -exact $stree(index) $node] < 0 || ![info exists stree($node)]} {
      add_proc_error "stree_buffer_mod_node" -1 "sharetree node $node does not exist"
      set ret -1
   } else {
      set stree($node) $shares
   }

   return $ret
}

#****** sge_sharetree/stree_buffer_del_node() **********************************
#  NAME
#     stree_buffer_del_node() -- delete a node in a sharetree buffer
#
#  SYNOPSIS
#     stree_buffer_del_node { stree_var node } 
#
#  FUNCTION
#     Deletes the given node in the sharetree buffer.
#     Only operates on the sharetree buffer.
#     To modify the sharetree in qmaster, call stree_buffer_commit.
#
#  INPUTS
#     stree_var - sharetree buffer
#     node      - node name
#
#  RESULT
#     0   - on success
#     < 0 - on error
#
#  BUGS
#     Does not remove itself from parent node and does not 
#     respect child nodes!
#
#  SEE ALSO
#     sge_sharetree/stree_buffer_commit()
#*******************************************************************************
proc stree_buffer_del_node {stree_var node} {
   upvar $stree_var stree

   set ret 0

   if {$node == "/"} {
      add_proc_error "stree_buffer_del_node" -1 "cannot delete root node $node"
      set ret -1
   } else {
      set pos [lsearch -exact $stree(index) $node]
      if {$pos < 0 || ![info exists stree($node)]} {
         add_proc_error "stree_buffer_del_node" -1 "sharetree node $node does not exist"
         set ret -2
      } else {
         # delete all children of this node
         foreach child $stree($node,children) {
            stree_buffer_del_node stree $child
         }

         # remove node from parent children list
         set parent [file dirname $node]
         if {[lsearch -exact $stree(index) $parent] < 0 || ![info exists stree($parent)]} {
            add_proc_error "stree_buffer_del_node" -1 "parent node for sharetree node $node does not exist"
            set ret -3
         } else {
            set parent_pos [lsearch -exact $stree($parent,children) $node]
            if {$parent_pos < 0} {
               add_proc_error "stree_buffer_del_node" -1 "parent node $parent does not reference $node as child"
               set ret -4
            } else {
               set stree($parent,children) [lreplace $stree($parent,children) $parent_pos $parent_pos]
            }
         }

         # now unset data of node
         unset stree($node)
         unset stree($node,children)
         set stree(index) [lreplace $stree(index) $pos $pos]
      }
   }

   return $ret
}

#****** sge_sharetree/stree_buffer_get_node() **********************************
#  NAME
#     stree_buffer_get_node() -- get the shares of a certain node
#
#  SYNOPSIS
#     stree_buffer_get_node { stree_var node } 
#
#  FUNCTION
#     Returns the shares of the given node.
#
#  INPUTS
#     stree_var - sharetree buffer
#     node      - node name
#
#  RESULT
#     number of shares or -1 in case of errors
#*******************************************************************************
proc stree_buffer_get_node {stree_var node} {
   upvar $stree_var stree

   set ret -1

   if {[lsearch -exact $stree(index) $node] < 0 || ![info exists stree($node)]} {
      add_proc_error "stree_buffer_mod_node" -1 "sharetree node $node does not exist"
   } else {
      set ret $stree($node)
   }

   return $ret
}

#****** sge_sharetree/stree_buffer_commit() ************************************
#  NAME
#     stree_buffer_commit() -- modify sharetree according to sharetree buffer
#
#  SYNOPSIS
#     stree_buffer_commit { stree_var } 
#
#  FUNCTION
#     Modifies the sharetree in sge_qmaster (qconf -Mstree) according to
#     the contents of the given (in memory) sharetree buffer.
#
#  INPUTS
#     stree_var - the sharetree to commit
#     {on_host ""}    - execute qconf on this host (default: qmaster host)
#     {as_user ""}    - execute qconf as this user (default: CHECK_USER)
#     {raise_error 1} - raise error condition in case of errors?
#
#  RESULT
#     0   - on success
#     < 0 - on error, see sge_procedures/get_sge_error()
#
#  SEE ALSO
#     sge_procedures/get_sge_error()
#*******************************************************************************
proc stree_buffer_commit {stree_var {on_host ""} {as_user ""} {raise_error 1}} {
   global CHECK_OUTPUT

   upvar $stree_var stree

   set ret 0

   # open a temporary file - will be deleted automatically by testsuite
   set tmp_filename [get_tmp_file_name]
   set f [open $tmp_filename "w"]
   
   # recursivly output sharetree
   set id 0
   foreach node $stree(index) {
      set name [file tail $node]
      if {$name == ""} {
         set name "Root"
      }
      puts $f "id=$id"
      puts $f "name=$name"
      puts $f "shares=$stree($node)"
      puts $f "type=1"
      puts -nonewline $f "childnodes="
      if {[llength $stree($node,children)] == 0} {
         puts $f "NONE"
      } else {
         # childnodes are expected as references to node ids
         set first 1
         foreach child $stree($node,children) {
            set pos [lsearch -exact $stree(index) $child]
            if {$pos < 0} {
               add_proc_error "stree_buffer_commit" -1 "cannot find child node $child in sharetree index"
               set ret -1
               break
            } else {
               if {$first} {
                  puts -nonewline $f "$pos"
                  set first 0
               } else {
                  puts -nonewline $f ",$pos"
               }
            }
         }
         puts $f ""
      }
      incr id
   }

   # close tmp file
   close $f

   # commit sharetree change
   if {$ret == 0} {
      set result [start_sge_bin "qconf" "-Mstree $tmp_filename" $on_host $as_user]
      set messages(index) {0}
      set messages(0) [translate_macro MSG_TREE_CHANGEDSHARETREE]

      set ret [handle_sge_errors "stree_buffer_commit" "qconf -Mstree $tmp_filename" $result messages $raise_error]
   }

   return $ret
}

#****** sge_sharetree/stree_parse_line() ***************************************
#  NAME
#     stree_parse_line() -- parse a line of qconf -sstree output
#
#  SYNOPSIS
#     stree_parse_line { name line } 
#
#  FUNCTION
#     Utility function called by stree_buffer_read.
#     Takes a line of qconf -sstree output and the name
#     of the expected config parameter,
#     and returns the config value.
#
#  INPUTS
#     name - name of the expected parameter
#     line - line to parse
#
#  RESULT
#     the parameter value, or "" on error
#
#  SEE ALSO
#     sge_sharetree/stree_buffer_read()
#*******************************************************************************
proc stree_parse_line {name line} {
   set ret ""
   set split_line [split [string trim $line] "="]
   if {[lindex $split_line 0] != $name} {
      add_proc_error "stree_parse_line" -1 "invalid sharetree line, should be \"$name=...\", but is \"$line\""
   } else {
      set ret [lindex $split_line 1]
   }

   return $ret
}

#****** sge_sharetree/stree_buffer_read() **************************************
#  NAME
#     stree_buffer_read() -- query sharetree and store it in sharetree buffer
#
#  SYNOPSIS
#     stree_buffer_read { stree_var } 
#
#  FUNCTION
#     Fetches the sharetree definition from sge_qmaster (qconf -sstree),
#     and stores it into a sharetree buffer.
#
#  INPUTS
#     stree_var - the target sharetree structure
#
#  RESULT
#     0   - on success
#     < 0 - on error
#*******************************************************************************
proc stree_buffer_read {stree_var} {
   global CHECK_OUTPUT

   upvar $stree_var stree

   set ret 0

   # if the buffer already exists - delete it
   if {[info exists stree]} {
      unset stree
   }

   set result [start_sge_bin "qconf" "-sstree"]
   if {$prg_exit_state != 0} {
      add_proc_error "stree_buffer_read" -1 "error reading sharetree:\n$result"
      set ret -1
   } else {
      set lines [split [string trim $result] "\n"]
      
      # we get 5 lines per sharetree node - do some verification
      set num_lines [llength $lines]
      if {[expr $num_lines % 5] != 0} {
         add_proc_error "stree_buffer_read" -1 "incorrect sharetree number of lines:\n$result"
         set ret -2
      } else {
         set line_idx 0
         while {$line_idx < $num_lines} {
            set id         [stree_parse_line "id"           [lindex $lines $line_idx]] ; incr line_idx
            set name       [stree_parse_line "name"         [lindex $lines $line_idx]] ; incr line_idx
            set type       [stree_parse_line "type"         [lindex $lines $line_idx]] ; incr line_idx
            set shares     [stree_parse_line "shares"       [lindex $lines $line_idx]] ; incr line_idx
            set childnodes [stree_parse_line "childnodes"   [lindex $lines $line_idx]] ; incr line_idx

            if {$id == 0} {
               # this is the root node
               set name "/"
            } else {
               # we have to create the full path name
               # look for a node that has the current nodes (by id) as child
               foreach node $stree(index) {
                  if {[lsearch -exact $stree($node,children) $id] >= 0} {
                     if {$node == "/"} {
                        set name "/$name"
                     } else {
                        set name "$node/$name"
                     }
                     break
                  }
               }
            }

            lappend stree(index) $name
            set stree($name) $shares
            if {$childnodes == "NONE"} {
               set stree($name,children) {}
            } else {
               set stree($name,children) [split $childnodes ","]
            }
         }

         # now we have read all nodes, but the children are still referenced by id
         # loop over all nodes
         foreach node $stree(index) {
            # and replace the ids by node names
            set num_children [llength $stree($node,children)]
            for {set i 0} {$i < $num_children} {incr i} {
               set child_id [lindex $stree($node,children) $i]
               set child_name [lindex $stree(index) $child_id]
               set stree($node,children) [lreplace $stree($node,children) $i $i $child_name]
            }
         }
      }
   }

   return $ret
}

#****** sge_sharetree/stree_buffer_dump() **************************************
#  NAME
#     stree_buffer_dump() -- dump the contents of a sharetree buffer
#
#  SYNOPSIS
#     stree_buffer_dump { stree_var } 
#
#  FUNCTION
#     Dumps the contents of a sharetree buffer in the form:
#     <node_name> <shares>
#
#  INPUTS
#     stree_var - the sharetree to dump
#
#  EXAMPLE
#
#*******************************************************************************
proc stree_buffer_dump {stree_var} {
   global CHECK_OUTPUT

   upvar $stree_var stree

   # determine longest node name
   set len 0
   foreach node $stree(index) {
      set node_len [string length $node]
      if { $node_len > $len} {
         set len $node_len
      }
   }

   # dump nodes
   foreach node $stree(index) {
      puts $CHECK_OUTPUT [format "%-${len}s %6d" $node $stree($node)]
   }
}

#****** sge_sharetree/test_stree_buffer() **************************************
#  NAME
#     test_stree_buffer() -- test for sharetree_buffer functions
#
#  SYNOPSIS
#     test_stree_buffer { } 
#
#  FUNCTION
#     Tests the sharetree_buffer functions.
#     Call through testsuite commandline option "execute_func".
#
#  EXAMPLE
#     expect check.exp execute_func test_stree_buffer
#
#  SEE ALSO
#     sge_sharetree/del_sharetree()
#     sge_sharetree/stree_buffer_init()
#     sge_sharetree/stree_buffer_add_node()
#     sge_sharetree/stree_buffer_mod_node()
#     sge_sharetree/stree_buffer_del_node()
#     sge_sharetree/stree_buffer_get_node()
#     sge_sharetree/stree_buffer_commit()
#     sge_sharetree/stree_buffer_read()
#     sge_sharetree/stree_buffer_dump()
#*******************************************************************************
proc test_stree_buffer {} {
   global ts_config CHECK_OUTPUT
   global CHECK_USER

   # make sure, CHECK_USER has been created
   submit_job "$ts_config(product_root)/examples/jobs/sleeper.sh"

   # test initialization
   set sharetree(foo) "bar"
   stree_buffer_init sharetree

   # add nodes, try duplicate action
   stree_buffer_add_node sharetree "/default" 10
   stree_buffer_add_node sharetree "/default" 20
   stree_buffer_add_node sharetree "/$CHECK_USER" 20

   stree_buffer_dump sharetree
   stree_buffer_commit sharetree

   puts $CHECK_OUTPUT "created sharetree"
   wait_for_enter

   stree_buffer_read new_sharetree
   stree_buffer_dump new_sharetree

   puts $CHECK_OUTPUT "read sharetree from qmaster"
   wait_for_enter
  
   # modify the read sharetree
   stree_buffer_mod_node new_sharetree "/$CHECK_USER" 50
   stree_buffer_add_node new_sharetree "/mytestproject" 100
   stree_buffer_add_node new_sharetree "/mytestproject/default" 10
   stree_buffer_add_node new_sharetree "/mytestproject/$CHECK_USER" 30
   stree_buffer_del_node new_sharetree "/default"
   stree_buffer_get_node new_sharetree "/mytestproject"

   # do some invalid additions / modifications
   stree_buffer_add_node new_sharetree "/blah/default" 20
   stree_buffer_mod_node new_sharetree "/blah" 123
   stree_buffer_mod_node new_sharetree "/blah/default" 20
   stree_buffer_del_node new_sharetree "/blah"
   stree_buffer_get_node new_sharetree "/blah"

   stree_buffer_dump new_sharetree
   stree_buffer_commit new_sharetree

   puts $CHECK_OUTPUT "modified sharetree"
   wait_for_enter

   stree_buffer_read new_modified_sharetree
   stree_buffer_dump new_modified_sharetree

   puts $CHECK_OUTPUT [stree_buffer_get_node new_modified_sharetree "/"]
   puts $CHECK_OUTPUT [stree_buffer_get_node new_modified_sharetree "/default"]
   puts $CHECK_OUTPUT [stree_buffer_get_node new_modified_sharetree "/mytestproject/$CHECK_USER"]
   puts $CHECK_OUTPUT [stree_buffer_get_node new_modified_sharetree "/mytestproject/blah"] ;# supposed to fail

   stree_buffer_del_node new_modified_sharetree "/mytestproject"
   stree_buffer_del_node new_modified_sharetree "/" ;# supposed to fail

   stree_buffer_dump new_modified_sharetree
   stree_buffer_commit new_modified_sharetree

   puts $CHECK_OUTPUT "reread and modified sharetree from qmaster"
   wait_for_enter
  
   del_sharetree

   puts $CHECK_OUTPUT "deleted sharetree"
}

#****** sge_sharetree/sge_share_mon() ******************************************
#  NAME
#     sge_share_mon() -- call sge_share_mon and return data
#
#  SYNOPSIS
#     sge_share_mon { output_var {on_host ""} {as_user ""} {raise_error 1} } 
#
#  FUNCTION
#     Calls sge_share_mon to retrieve a snapshot of the sharetree usage.
#
#     If sharetree data is delivered by sge_share_mon, it is parsed and
#     returned in a TCL array in the form:
#        out(index)  {node_1 node_2 ... node_n}
#        out(node_1,curr_time)
#        out(node_1,user_name)
#        out(node_1,shares)
#        ...
#        out(node_n,curr_time)
#        ...
#
#     All fields delivered by sge_share_mon (see sge_share_mon -h) are reported,
#     except the "node_name" field - it is the node name.
#
#  INPUTS
#     output_var      - TCL array to return data
#     {on_host ""}    - execute sge_share_mon on this host
#     {as_user ""}    - execute sge_share_mon as this user
#     {raise_error 1} - raise an error condition on error (default), or just
#                       output the error message to stdout
#
#  RESULT
#        0 - on success
#       -1 - no sharetree configured
#       -2 - permission denied
#     -999 - other errors
#
#  SEE ALSO
#     parser/parse_csv()
#*******************************************************************************
proc sge_share_mon {output_var {on_host ""} {as_user ""} {raise_error 1}} {
   global ts_config CHECK_OUTPUT

   upvar $output_var out

   # clear output variable
   if {[info exists out]} {
      unset out
   }

   set ret 0
   set result [start_sge_utilbin "sge_share_mon" "-h -d , -c 1" $on_host $as_user]

   if {$prg_exit_state == 0} {
      set ret [parse_csv out result "," "node_name"]
   } else {
      set messages(index) {-1}
      set messages(-1) [translate_macro MSG_SGESHAREMON_NOSHARETREE]
      set ret [handle_sge_errors "sge_share_mon" "sge_share_mon -h -d , -c 1" $result messages $raise_error]
   }
}
