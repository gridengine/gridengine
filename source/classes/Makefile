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

JDRMAA_JAVA   = AuthorizationException.java \
                ConflictingAttributeValuesException.java \
                DRMAAException.java DRMAASession.java \
                DRMAASessionFactory.java DRMCommunicationException.java \
                DRMSExitException.java DRMSInitException.java \
                DefaultContactStringException.java \
                DeniedByDRMException.java ExitTimeoutException.java \
                InconsistentStateException.java InternalException.java \
                InvalidArgumentException.java InvalidAttributeException.java \
                InvalidAttributeFormatException.java \
                InvalidAttributeValueException.java \
                InvalidContactStringException.java InvalidJobException.java \
                JobInfo.java JobTemplate.java NoActiveSessionException.java \
                NoResourceUsageDataException.java \
                SessionAlreadyActiveException.java SessionException.java \
                TryLaterException.java UnsupportedAttributeException.java \
                SGEJobInfo.java SGEJobTemplate.java SGESession.java \
                SGESessionFactory.java

JDRMAA_CLASS  = $(JDRMAA_JAVA:.java=.class)

JDRMAALIB      = libjdrmaa$(SHAREDEXT)
JDRMAALIB_SRC  = com_sun_grid_drmaa_SGESession.c
JDRMAALIB_OBJS = $(JDRMAALIB_SRC:.c=.o)

###############################################################################
# global rules: all, clean
###############################################################################
classes_all: jdrmaa

classes_clean: jdrmaa_clean

jdrmaa_clean:
	$(RM) $(JDRMAALIB_OBJS) $(JDRMAALIB)
	$(RM) -Rf org/ggf/drmaa com/sun/grid/drmaa

###############################################################################
# rules to build DRMAA Java binding
###############################################################################
jdrmaa: jdrmaa_jar jdrmaa_lib

jdrmaa_jar: jdrmaa_classes
	@cd $(CLASSES_DIR); $(JAVA_HOME)/bin/jar cvf $(SOURCE)/$(COMPILE_ARCH)/drmaa.jar org/ggf/drmaa/*.class com/sun/grid/drmaa/*.class; cd $(SOURCE)/$(COMPILE_ARCH)

jdrmaa_classes:
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/*.java $(CLASSES_DIR)/com/sun/grid/drmaa/*.java

###############################################################################
# rules to build library
###############################################################################
jdrmaa_lib: $(JDRMAALIB_OBJS) drmaalib
	$(SHAREDLD) $(SHARED_LFLAGS) -o libjdrmaa$(SHAREDEXT) $(JDRMAALIB_OBJS) -ldrmaa -lc

###############################################################################
# rules to build object codes
###############################################################################
com_sun_grid_drmaa_SGESession.o: $(SRC_DIR)/com_sun_grid_drmaa_SGESession.c
	$(CC) $(CFLAGS) $(SHARED_CFLAGS) -c $(SRC_DIR)/com_sun_grid_drmaa_SGESession.c

###############################################################################
# rules to build classes
###############################################################################
AuthorizationException.class: $(CLASSES_DIR)/org/ggf/drmaa/AuthorizationException.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/AuthorizationException.java

ConflictingAttributeValuesException.class: $(CLASSES_DIR)/org/ggf/drmaa/ConflictingAttributeValuesException.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/ConflictingAttributeValuesException.java

DRMAAException.class: $(CLASSES_DIR)/org/ggf/drmaa/DRMAAException.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/DRMAAException.java

DRMAASession.class: $(CLASSES_DIR)/org/ggf/drmaa/DRMAASession.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/DRMAASession.java

DRMAASessionFactory.class: $(CLASSES_DIR)/org/ggf/drmaa/DRMAASessionFactory.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/DRMAASessionFactory.java

DRMCommunicationException.class: $(CLASSES_DIR)/org/ggf/drmaa/DRMCommunicationException.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/DRMCommunicationException.java

DRMSExitException.class: $(CLASSES_DIR)/org/ggf/drmaa/DRMSExitException.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/DRMSExitException.java

DRMSInitException.class: $(CLASSES_DIR)/org/ggf/drmaa/DRMSInitException.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/DRMSInitException.java

DefaultContactStringException.class: $(CLASSES_DIR)/org/ggf/drmaa/DefaultContactStringException.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/DefaultContactStringException.java

DeniedByDRMException.class: $(CLASSES_DIR)/org/ggf/drmaa/DeniedByDRMException.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/DeniedByDRMException.java

ExitTimeoutException.class: $(CLASSES_DIR)/org/ggf/drmaa/ExitTimeoutException.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/ExitTimeoutException.java

InconsistentStateException.class: $(CLASSES_DIR)/org/ggf/drmaa/InconsistentStateException.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/InconsistentStateException.java

InternalException.class: $(CLASSES_DIR)/org/ggf/drmaa/InternalException.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/InternalException.java

InvalidArgumentException.class: $(CLASSES_DIR)/org/ggf/drmaa/InvalidArgumentException.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/InvalidArgumentException.java

InvalidAttributeException.class: $(CLASSES_DIR)/org/ggf/drmaa/InvalidAttributeException.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/InvalidAttributeException.java

InvalidAttributeFormatException.class: $(CLASSES_DIR)/org/ggf/drmaa/InvalidAttributeFormatException.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/InvalidAttributeFormatException.java

InvalidAttributeValueException.class: $(CLASSES_DIR)/org/ggf/drmaa/InvalidAttributeValueException.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/InvalidAttributeValueException.java

InvalidContactStringException.class: $(CLASSES_DIR)/org/ggf/drmaa/InvalidContactStringException.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/InvalidContactStringException.java

InvalidJobException.class: $(CLASSES_DIR)/org/ggf/drmaa/InvalidJobException.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/InvalidJobException.java

JobInfo.class: $(CLASSES_DIR)/org/ggf/drmaa/JobInfo.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/JobInfo.java

JobTemplate.class: $(CLASSES_DIR)/org/ggf/drmaa/JobTemplate.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/JobTemplate.java

NoActiveSessionException.class: $(CLASSES_DIR)/org/ggf/drmaa/NoActiveSessionException.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/NoActiveSessionException.java

NoResourceUsageDataException.class: $(CLASSES_DIR)/org/ggf/drmaa/NoResourceUsageDataException.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/NoResourceUsageDataException.java

SessionAlreadyActiveException.class: $(CLASSES_DIR)/org/ggf/drmaa/SessionAlreadyActiveException.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/SessionAlreadyActiveException.java

SessionException.class: $(CLASSES_DIR)/org/ggf/drmaa/SessionException.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/SessionException.java

TryLaterException.class: $(CLASSES_DIR)/org/ggf/drmaa/TryLaterException.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/TryLaterException.java

UnsupportedAttributeException.class: $(CLASSES_DIR)/org/ggf/drmaa/UnsupportedAttributeException.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/UnsupportedAttributeException.java

SGEJobInfo.class: $(CLASSES_DIR)/com/sun/grid/drmaa/SGEJobInfo.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/com/sun/grid/drmaa/SGEJobInfo.java

SGEJobTemplate.class: $(CLASSES_DIR)/com/sun/grid/drmaa/SGEJobTemplate.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/com/sun/grid/drmaa/SGEJobTemplate.java

SGESession.class: $(CLASSES_DIR)/com/sun/grid/drmaa/SGESession.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/com/sun/grid/drmaa/SGESession.java

SGESessionFactory.class: $(CLASSES_DIR)/com/sun/grid/drmaa/SGESessionFactory.java
	$(JAVA_HOME)/bin/javac -classpath . -d . $(CLASSES_DIR)/com/sun/grid/drmaa/SGESessionFactory.java

#------------------------------------------------------------------------------

test_jdrmaa: $(CLASSES_DIR)/TestDRMAA.java
	$(JAVA_HOME)/bin/javac -classpath $(CLASSES_DIR) -d . $(CLASSES_DIR)/TestDRMAA.java

jdrmaa_example: $(CLASSES_DIR)/DRMAAExample.java
	$(JAVA_HOME)/bin/javac -classpath $(CLASSES_DIR) -d . $(CLASSES_DIR)/DRMAAExample.java
