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

JAPIHOWTO_SRC   = howto1.c howto2.c howto2_1.c howto3.c howto3_1.c howto3_2.c howto4.c howto5.c howto6.c

JAPIHOWTO_OBJS  = $(JAPIHOWTO_SRC:.c=.o)

JAPIHOWTO_ALL   =  howto1 howto2 howto2_1 howto3 howto3_1 howto3_2 howto4 howto5 howto6

japihowto_clean: 
	$(RM) $(JAPIHOWTO_ALL) $(JAPIHOWTO_OBJS)

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

howto3_1: howto3_1.o $(DRMAALIB)
	$(LD_WRAPPER) $(CC) $(CFLAGS) -o howto3_1 howto3_1.o $(LFLAGS) -ldrmaa $(LIBS) $(SLIBS) $(DLLIB)

howto3_1.o: $(JAPIHOWTO_DIR)/howto3_1.c
	$(CC) $(CFLAGS) -c $(JAPIHOWTO_DIR)/howto3_1.c

howto3_2: howto3_2.o $(DRMAALIB)
	$(LD_WRAPPER) $(CC) $(CFLAGS) -o howto3_2 howto3_2.o $(LFLAGS) -ldrmaa $(LIBS) $(SLIBS) $(DLLIB)

howto3_2.o: $(JAPIHOWTO_DIR)/howto3_2.c
	$(CC) $(CFLAGS) -c $(JAPIHOWTO_DIR)/howto3_2.c

howto4: howto4.o $(DRMAALIB)
	$(LD_WRAPPER) $(CC) $(CFLAGS) -o howto4 howto4.o $(LFLAGS) -ldrmaa $(LIBS) $(SLIBS) $(DLLIB)

howto4.o: $(JAPIHOWTO_DIR)/howto4.c
	$(CC) $(CFLAGS) -c $(JAPIHOWTO_DIR)/howto4.c

howto5: howto5.o $(DRMAALIB)
	$(LD_WRAPPER) $(CC) $(CFLAGS) -o howto5 howto5.o $(LFLAGS) -ldrmaa $(LIBS) $(SLIBS) $(DLLIB)

howto5.o: $(JAPIHOWTO_DIR)/howto5.c
	$(CC) $(CFLAGS) -c $(JAPIHOWTO_DIR)/howto5.c

howto6: howto6.o $(DRMAALIB)
	$(LD_WRAPPER) $(CC) $(CFLAGS) -o howto6 howto6.o $(LFLAGS) -ldrmaa $(LIBS) $(SLIBS) $(DLLIB)

howto6.o: $(JAPIHOWTO_DIR)/howto6.c
	$(CC) $(CFLAGS) -c $(JAPIHOWTO_DIR)/howto6.c
