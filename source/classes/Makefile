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

# Variable definitions for targets to build

CLASSES_DIR   = $(SDIR)/classes
SRC_DIR       = $(CLASSES_DIR)/src

DRMAAJLIB      = libjdrmaa$(SHAREDEXT)
DRMAAJLIB_SRC  = com_sun_grid_drmaa_SessionImpl.c
DRMAAJLIB_OBJS = $(DRMAAJLIB_SRC:.c=.o)

###############################################################################
# global rules: all, clean
###############################################################################
classes_all: drmaaj

classes_clean: drmaaj_clean

drmaaj_clean: drmaaj_ant_clean
	$(RM) $(DRMAAJLIB_OBJS) $(DRMAAJLIB)

drmaaj_ant_clean:
	ant -f $(CLASSES_DIR)/build.xml clean

###############################################################################
# rules to build DRMAA Java binding
###############################################################################
drmaaj: drmaaj_ant drmaaj_lib

drmaaj_ant:
	ant -f $(CLASSES_DIR)/build.xml all

drmaaj_ant_doc:
	ant -f $(CLASSES_DIR)/build.xml javadoc

###############################################################################
# rules to build library
###############################################################################
drmaaj_lib: $(DRMAAJLIB_OBJS)
	$(SHAREDLD) $(SHARED_LFLAGS) -o libjdrmaa$(JAVA_SHAREDEXT) $(DRMAAJLIB_OBJS) -ldrmaa -lc $(DLLIB)

###############################################################################
# rules to build object codes
###############################################################################
com_sun_grid_drmaa_SessionImpl.o: $(SRC_DIR)/com_sun_grid_drmaa_SessionImpl.c
	$(CC) $(CFLAGS) $(SHARED_CFLAGS) -c $(SRC_DIR)/com_sun_grid_drmaa_SessionImpl.c
