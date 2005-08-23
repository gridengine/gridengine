#
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

dl() {
   case $1 in
      0) unset SGE_DEBUG_LEVEL; unset SGE_ND ;;
      1) SGE_DEBUG_LEVEL="2 0 0 0 0 0 0 0"; export SGE_DEBUG_LEVEL; SGE_ND="true"; export SGE_ND ;;
      2) SGE_DEBUG_LEVEL="3 0 0 0 0 0 0 0"; export SGE_DEBUG_LEVEL; SGE_ND="true"; export SGE_ND ;;
      3) SGE_DEBUG_LEVEL="2 2 0 0 0 0 2 0"; export SGE_DEBUG_LEVEL; SGE_ND="true"; export SGE_ND ;;
      4) SGE_DEBUG_LEVEL="3 3 0 0 0 0 3 0"; export SGE_DEBUG_LEVEL; SGE_ND="true"; export SGE_ND ;;
      5) SGE_DEBUG_LEVEL="3 0 0 3 0 0 3 0"; export SGE_DEBUG_LEVEL; SGE_ND="true"; export SGE_ND ;;
      6) SGE_DEBUG_LEVEL="32 32 32 0 0 32 32 0"; export SGE_DEBUG_LEVEL; SGE_ND="true"; export SGE_ND ;;
      8|7) echo dl: $1 is a still unused debugging level ;;
      9) SGE_DEBUG_LEVEL="2 2 2 0 0 0 0 0"; export SGE_DEBUG_LEVEL; SGE_ND="true"; export SGE_ND ;;
      10) SGE_DEBUG_LEVEL="3 3 3 0 0 0 0 3"; export SGE_DEBUG_LEVEL; SGE_ND="true"; export SGE_ND ;;
      *) echo "usage: dl <debugging_level>"
         echo "       debugging_level 0 - 10"
   esac
}
