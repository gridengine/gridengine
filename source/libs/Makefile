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
include $(SDIR)/libs/comm/Makefile
include $(SDIR)/libs/gdi/Makefile
include $(SDIR)/libs/spool/Makefile
include $(SDIR)/libs/sched/Makefile

###############################################################################
# Variable definitions for targets to build
###############################################################################
SGELIB      = libsge$(LIBEXT)

###############################################################################
# global rules: all, clean, depend, messages
###############################################################################
libs: gdilib spoollib commlib culllib rmonlib schedlib utilib $(SGELIB)

libs_clean: gdilib_clean spoollib_clean commlib_clean culllib_clean rmonlib_clean schedlib_clean utilib_clean $(SGELIB)_clean

$(SGELIB)_clean: 
	$(RM) libsge.a libsge$(SHAREDEXT)

libs_depend: gdilib_depend spoollib_depend commlib_depend culllib_depend rmonlib_depend schedlib_depend utilib_depend

libs_messages: gdilib_messages spoollib_messages commlib_messages cull_messages schedlib_messages utilib_messages rmonlib_messages


###############################################################################
# rules to build library libsge.a
###############################################################################
libsge.a:	$(SCHEDLIB_OBJS) $(GDILIB_OBJS) $(SECLIB_OBJS) $(KRBLIBS) $(COMMLIB_OBJS) $(CULLLIB_OBJS) $(RMONLIB_OBJS) $(UTILIB_OBJS) 
	$(AR) $(ARFLAGS) libsge.a $(SCHEDLIB_OBJS) $(GDILIB_OBJS) $(SECLIB_OBJS) $(KRBLIBS) $(COMMLIB_OBJS) $(CULLLIB_OBJS) $(RMONLIB_OBJS) $(UTILIB_OBJS) 
	$(RANLIB) $(SGELIB)

libsge$(SHAREDEXT): $(SCHEDLIB_OBJS) $(GDILIB_OBJS) $(SECLIB_OBJS) $(KRBLIBS) $(COMMLIB_OBJS) $(CULLLIB_OBJS) $(RMONLIB_OBJS) $(UTILIB_OBJS) $(ZLIB)
	$(SHAREDLD) $(SHARED_LFLAGS) -o libsge$(SHAREDEXT) $(SCHEDLIB_OBJS) $(GDILIB_OBJS) $(SECLIB_OBJS) $(KRBLIBS) $(COMMLIB_OBJS) $(CULLLIB_OBJS) $(RMONLIB_OBJS) $(UTILIB_OBJS) $(LOADAVGLIBS) $(LIBS) -lc
