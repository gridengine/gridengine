###############################################################################
# include sub Makefiles
###############################################################################
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

include $(SDIR)/libs/rmon/Makefile
include $(SDIR)/libs/uti/Makefile
include $(SDIR)/libs/cull/Makefile
include $(SDIR)/libs/lck/Makefile
include $(SDIR)/libs/comm/Makefile
include $(SDIR)/libs/sgeobj/Makefile
include $(SDIR)/libs/gdi/Makefile
include $(SDIR)/libs/evc/Makefile
include $(SDIR)/libs/evm/Makefile
include $(SDIR)/libs/mir/Makefile
include $(SDIR)/libs/sched/Makefile
include $(SDIR)/libs/spool/Makefile
include $(SDIR)/libs/japi/Makefile

###############################################################################
# Variable definitions for targets to build
###############################################################################
SGELIB      = libsge$(LIBEXT)

###############################################################################
# global rules: all, clean, depend, messages
###############################################################################
libs: sgeobjlib gdilib evclib evmlib mirlib spoollib culllib rmonlib schedlib utilib lcklib japi_all comm_all $(SGELIB)

libs_clean: gdilib_clean sgeobjlib_clean evclib_clean evmlib_clean mirlib_clean spoollib_clean commlib_clean commlistslib_clean culllib_clean rmonlib_clean schedlib_clean utilib_clean lcklib_clean japilib_clean $(SGELIB)_clean

$(SGELIB)_clean: 
	$(RM) libsge.a libsge$(SHAREDEXT)

libs_depend: gdilib_depend sgeobjlib_depend evclib_depend evmlib_depend mirlib_depend spoollib_depend culllib_depend rmonlib_depend schedlib_depend utilib_depend lcklib_depend japilib_depend commlib_depend commlistslib_depend

libs_messages: gdilib_messages sgeobjlib_messages evclib_messages evmlib_messages mirlib_messages spoollib_messages commlib_messages cull_messages schedlib_messages utilib_messages rmonlib_messages lcklib_messages japilib_messages


###############################################################################
# rules to build library libsge.a
###############################################################################
libsge.a:	$(SCHEDLIB_OBJS) $(GDILIB_OBJS) $(SGEOBJLIB_OBJS) $(SGEOBJDLIB_OBJS) $(SECLIB_OBJS) $(KRBLIBS) $(COMMLIB_OBJS) $(CULLLIB_OBJS) $(RMONLIB_OBJS) $(UTILIB_OBJS) $(LCKLIB_OBJS) 
	$(AR) $(ARFLAGS) libsge.a $(SCHEDLIB_OBJS) $(GDILIB_OBJS) $(SGEOBJLIB_OBJS) $(SGEOBJDLIB_OBJS) $(SECLIB_OBJS) $(KRBLIBS) $(COMMLIB_OBJS) $(CULLLIB_OBJS) $(RMONLIB_OBJS) $(UTILIB_OBJS) $(LCKLIB_OBJS) 
	$(RANLIB) $(SGELIB)

libsge$(SHAREDEXT): $(SCHEDLIB_OBJS) $(MIRLIB) $(EVCLIB) $(GDILIB_OBJS) $(SGEOBJLIB_OBJS) $(SGEOBJDLIB) $(SECLIB_OBJS) $(KRBLIBS) $(COMMLIB_OBJS) $(CULLLIB_OBJS) $(RMONLIB_OBJS) $(UTILIB_OBJS) $(LCKLIB_OBJS)
	$(SHAREDLD) $(SHARED_LFLAGS) -o libsge$(SHAREDEXT) $(SCHEDLIB_OBJS) $(MIRLIB) $(EVCLIB) $(GDILIB_OBJS) $(SGEOBJLIB_OBJS) $(SGEOBJDLIB) $(SECLIB_OBJS) $(KRBLIBS) $(COMMLIB_OBJS) $(CULLLIB_OBJS) $(RMONLIB_OBJS) $(UTILIB_OBJS) $(LCKLIB_OBJS) $(LOADAVGLIBS) $(LIBS) -lc
