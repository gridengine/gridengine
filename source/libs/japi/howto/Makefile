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

JAPIHOWTO_DIR   = $(JAPILIB_DIR)/howto

###############################################################################
# rules to build object codes
###############################################################################
howto1: howto1.o $(DRMAALIB)
	$(LD_WRAPPER) $(CC) $(CFLAGS) -o howto1 howto1.o $(LFLAGS) -ldrmaa $(LIBS) $(SLIBS) $(DLLIB)

howto1.o: $(JAPIHOWTO_DIR)/howto1.c
	$(CC) $(CFLAGS) -c $(JAPIHOWTO_DIR)/howto1.c

howto2: howto2.o $(DRMAALIB)
	$(LD_WRAPPER) $(CC) $(CFLAGS) -o howto2 howto2.o $(LFLAGS) -ldrmaa $(LIBS) $(SLIBS) $(DLLIB)

howto2.o: $(JAPIHOWTO_DIR)/howto2.c
	$(CC) $(CFLAGS) -c $(JAPIHOWTO_DIR)/howto2.c

howto2_1: howto2_1.o $(DRMAALIB)
	$(LD_WRAPPER) $(CC) $(CFLAGS) -o howto2_1 howto2_1.o $(LFLAGS) -ldrmaa $(LIBS) $(SLIBS) $(DLLIB)

howto2_1.o: $(JAPIHOWTO_DIR)/howto2_1.c
	$(CC) $(CFLAGS) -c $(JAPIHOWTO_DIR)/howto2_1.c

howto3: howto3.o $(DRMAALIB)
	$(LD_WRAPPER) $(CC) $(CFLAGS) -o howto3 howto3.o $(LFLAGS) -ldrmaa $(LIBS) $(SLIBS) $(DLLIB)

howto3.o: $(JAPIHOWTO_DIR)/howto3.c
	$(CC) $(CFLAGS) -c $(JAPIHOWTO_DIR)/howto3.c
