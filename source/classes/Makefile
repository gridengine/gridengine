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

DRMAAJ_JAVA   = $(CLASSES_DIR)/com/sun/grid/drmaa/JobInfoImpl.java \
                $(CLASSES_DIR)/com/sun/grid/drmaa/JobTemplateImpl.java \
                $(CLASSES_DIR)/com/sun/grid/drmaa/SessionFactoryImpl.java \
                $(CLASSES_DIR)/com/sun/grid/drmaa/SessionImpl.java \
                $(CLASSES_DIR)/org/ggf/drmaa/AlreadyActiveSessionException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/AuthorizationException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/DefaultContactStringException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/ConflictingAttributeValuesException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/DrmaaException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/DeniedByDrmException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/DrmCommunicationException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/DrmaaRuntimeException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/DrmsExitException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/DrmsInitException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/ExitTimeoutException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/FileTransferMode.java \
                $(CLASSES_DIR)/org/ggf/drmaa/HoldInconsistentStateException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/InconsistentStateException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/InternalException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/InvalidArgumentException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/InvalidAttributeException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/InvalidAttributeFormatException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/InvalidAttributeValueException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/InvalidContactStringException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/InvalidJobException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/InvalidJobTemplateException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/JobInfo.java \
                $(CLASSES_DIR)/org/ggf/drmaa/JobTemplate.java \
                $(CLASSES_DIR)/org/ggf/drmaa/NoActiveSessionException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/NoDefaultContactStringException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/NoResourceUsageException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/PartialTimestamp.java \
                $(CLASSES_DIR)/org/ggf/drmaa/PartialTimestampFormat.java \
                $(CLASSES_DIR)/org/ggf/drmaa/ReleaseInconsistentStateException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/ResumeInconsistentStateException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/Session.java \
                $(CLASSES_DIR)/org/ggf/drmaa/SessionException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/SessionFactory.java \
                $(CLASSES_DIR)/org/ggf/drmaa/SuspendInconsistentStateException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/TryLaterException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/UnsupportedAttributeException.java \
                $(CLASSES_DIR)/org/ggf/drmaa/Version.java

DRMAAJ_CLASS  = $(DRMAAJ_JAVA:.java=.class)

DRMAAJ_TEST_JAVA = $(CLASSES_DIR)/com/sun/grid/drmaa/DrmaaSuite.java \
                   $(CLASSES_DIR)/com/sun/grid/drmaa/JobInfoImplTest.java \
                   $(CLASSES_DIR)/com/sun/grid/drmaa/JobTemplateImplTest.java \
                   $(CLASSES_DIR)/com/sun/grid/drmaa/SessionFactoryImplTest.java \
                   $(CLASSES_DIR)/com/sun/grid/drmaa/SessionImplJobTest.java \
                   $(CLASSES_DIR)/com/sun/grid/drmaa/SessionImplTest.java \
                   $(CLASSES_DIR)/org/ggf/drmaa/DrmaaSuite.java \
                   $(CLASSES_DIR)/org/ggf/drmaa/FileTransferModeTest.java \
                   $(CLASSES_DIR)/org/ggf/drmaa/JobInfoTest.java \
                   $(CLASSES_DIR)/org/ggf/drmaa/JobTemplateTest.java \
                   $(CLASSES_DIR)/org/ggf/drmaa/PartialTimestampFormatTest.java \
                   $(CLASSES_DIR)/org/ggf/drmaa/PartialTimestampTest.java \
                   $(CLASSES_DIR)/org/ggf/drmaa/SessionFactoryTest.java \
                   $(CLASSES_DIR)/org/ggf/drmaa/VersionTest.java \
                   $(CLASSES_DIR)/DrmaaSuite.java \
                   $(CLASSES_DIR)/TestDrmaa.java

DRMAAJ_TEST_CLASS  = $(DRMAAJ_TEST_JAVA:.java=.class)

DRMAAJ_EXAMPLES_JAVA = $(CLASSES_DIR)/com/sun/grid/drmaa/howto/Howto1.java \
                       $(CLASSES_DIR)/com/sun/grid/drmaa/howto/Howto2.java \
                       $(CLASSES_DIR)/com/sun/grid/drmaa/howto/Howto2_1.java \
                       $(CLASSES_DIR)/com/sun/grid/drmaa/howto/Howto3.java \
                       $(CLASSES_DIR)/com/sun/grid/drmaa/howto/Howto3_1.java \
                       $(CLASSES_DIR)/com/sun/grid/drmaa/howto/Howto3_2.java \
                       $(CLASSES_DIR)/com/sun/grid/drmaa/howto/Howto4.java \
                       $(CLASSES_DIR)/com/sun/grid/drmaa/howto/Howto5.java \
                       $(CLASSES_DIR)/com/sun/grid/drmaa/howto/Howto6.java \
                       $(CLASSES_DIR)/DrmaaExample.java

DRMAAJ_EXAMPLES_CLASS  = $(DRMAAJ_EXAMPLES_JAVA:.java=.class)

DRMAAJLIB      = libjdrmaa$(SHAREDEXT)
DRMAAJLIB_SRC  = com_sun_grid_drmaa_SessionImpl.c
DRMAAJLIB_OBJS = $(DRMAAJLIB_SRC:.c=.o)

###############################################################################
# global rules: all, clean
###############################################################################
classes_all: drmaaj

classes_clean: drmaaj_clean

drmaaj_clean: drmaaj_meta_clean
	$(RM) $(DRMAAJLIB_OBJS) $(DRMAAJLIB)
	$(RM) -Rf org/ggf/drmaa com/sun/grid/drmaa

drmaaj_jar_clean:
	@$(RM) drmaa.jar

drmaaj_meta_clean:
	@$(RM) -Rf META-INF

drmaaj_doc_clean:
	@$(RM) ../../doc/javadocs/org/ggf/drmaa/*.html
	@$(RM) ../../doc/javadocs/com/sun/ggf/drmaa/*.html
	@$(RM) ../../doc/javadocs/resources/*.gif
	@$(RM) ../../doc/javadocs/*.html

###############################################################################
# rules to build DRMAA Java binding
###############################################################################
drmaaj: drmaaj_jar drmaaj_lib drmaaj_test

drmaaj_jar: drmaaj_jar_clean drmaaj_classes drmaaj_meta
	$(JAVA_HOME)/$(JAVA_BINDIR)/jar cvf drmaa.jar $(DRMAAJ_CLASS) META-INF

drmaaj_test: drmaaj_jar_clean drmaaj_classes drmaaj_test_classes drmaaj_meta
	$(JAVA_HOME)/$(JAVA_BINDIR)/jar cvf drmaa.jar $(DRMAAJ_CLASS) $(DRMAAJ_TEST_CLASS) META-INF

drmaaj_all_classes: drmaaj_classes drmaaj_test_classes drmaaj_example_classes

drmaaj_classes:
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath .:../classes/junit.jar -d . $(DRMAAJ_JAVA)

drmaaj_test_classes:
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath .:../classes/junit.jar -d . $(DRMAAJ_TEST_JAVA)

drmaaj_example_classes:
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath .:../classes/junit.jar -d . $(DRMAAJ_EXAMPLES_JAVA)

drmaaj_meta: drmaaj_meta_clean
	@mkdir META-INF
	@mkdir META-INF/services
	@echo com.sun.grid.drmaa.SessionFactoryImpl > META-INF/services/org.ggf.drmaa.SessionFactory

drmaaj_doc: drmaaj_classes drmaaj_doc_clean
	$(JAVA_HOME)/$(JAVA_BINDIR)/javadoc -sourcepath $(CLASSES_DIR) -classpath . -d ../../doc/javadocs -version -overview $(CLASSES_DIR)/overview.html $(DRMAAJ_JAVA)


###############################################################################
# rules to build library
###############################################################################
drmaaj_lib: $(DRMAAJLIB_OBJS) drmaalib
	$(SHAREDLD) $(SHARED_LFLAGS) -o libjdrmaa$(JAVA_SHAREDEXT) $(DRMAAJLIB_OBJS) -ldrmaa -lc $(DLLIB)

###############################################################################
# rules to build object codes
###############################################################################
com_sun_grid_drmaa_SessionImpl.o: $(SRC_DIR)/com_sun_grid_drmaa_SessionImpl.c
	$(CC) $(CFLAGS) $(SHARED_CFLAGS) -c $(SRC_DIR)/com_sun_grid_drmaa_SessionImpl.c

###############################################################################
# rules to build classes
###############################################################################
AuthorizationException.class: $(CLASSES_DIR)/org/ggf/drmaa/AuthorizationException.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/AuthorizationException.java

ConflictingAttributeValuesException.class: $(CLASSES_DIR)/org/ggf/drmaa/ConflictingAttributeValuesException.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/ConflictingAttributeValuesException.java

DRMAAException.class: $(CLASSES_DIR)/org/ggf/drmaa/DRMAAException.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/DRMAAException.java

DRMAASession.class: $(CLASSES_DIR)/org/ggf/drmaa/DRMAASession.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/DRMAASession.java

DRMAASessionFactory.class: $(CLASSES_DIR)/org/ggf/drmaa/DRMAASessionFactory.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/DRMAASessionFactory.java

DRMCommunicationException.class: $(CLASSES_DIR)/org/ggf/drmaa/DRMCommunicationException.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/DRMCommunicationException.java

DRMSExitException.class: $(CLASSES_DIR)/org/ggf/drmaa/DRMSExitException.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/DRMSExitException.java

DRMSInitException.class: $(CLASSES_DIR)/org/ggf/drmaa/DRMSInitException.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/DRMSInitException.java

DefaultContactStringException.class: $(CLASSES_DIR)/org/ggf/drmaa/DefaultContactStringException.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/DefaultContactStringException.java

DeniedByDRMException.class: $(CLASSES_DIR)/org/ggf/drmaa/DeniedByDRMException.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/DeniedByDRMException.java

ExitTimeoutException.class: $(CLASSES_DIR)/org/ggf/drmaa/ExitTimeoutException.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/ExitTimeoutException.java

InconsistentStateException.class: $(CLASSES_DIR)/org/ggf/drmaa/InconsistentStateException.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/InconsistentStateException.java

InternalException.class: $(CLASSES_DIR)/org/ggf/drmaa/InternalException.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/InternalException.java

InvalidArgumentException.class: $(CLASSES_DIR)/org/ggf/drmaa/InvalidArgumentException.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/InvalidArgumentException.java

InvalidAttributeException.class: $(CLASSES_DIR)/org/ggf/drmaa/InvalidAttributeException.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/InvalidAttributeException.java

InvalidAttributeFormatException.class: $(CLASSES_DIR)/org/ggf/drmaa/InvalidAttributeFormatException.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/InvalidAttributeFormatException.java

InvalidAttributeValueException.class: $(CLASSES_DIR)/org/ggf/drmaa/InvalidAttributeValueException.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/InvalidAttributeValueException.java

InvalidContactStringException.class: $(CLASSES_DIR)/org/ggf/drmaa/InvalidContactStringException.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/InvalidContactStringException.java

InvalidJobException.class: $(CLASSES_DIR)/org/ggf/drmaa/InvalidJobException.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/InvalidJobException.java

JobInfo.class: $(CLASSES_DIR)/org/ggf/drmaa/JobInfo.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/JobInfo.java

JobTemplate.class: $(CLASSES_DIR)/org/ggf/drmaa/JobTemplate.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/JobTemplate.java

NoActiveSessionException.class: $(CLASSES_DIR)/org/ggf/drmaa/NoActiveSessionException.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/NoActiveSessionException.java

NoResourceUsageDataException.class: $(CLASSES_DIR)/org/ggf/drmaa/NoResourceUsageDataException.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/NoResourceUsageDataException.java

SessionAlreadyActiveException.class: $(CLASSES_DIR)/org/ggf/drmaa/SessionAlreadyActiveException.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/SessionAlreadyActiveException.java

SessionException.class: $(CLASSES_DIR)/org/ggf/drmaa/SessionException.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/SessionException.java

TryLaterException.class: $(CLASSES_DIR)/org/ggf/drmaa/TryLaterException.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/TryLaterException.java

UnsupportedAttributeException.class: $(CLASSES_DIR)/org/ggf/drmaa/UnsupportedAttributeException.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/org/ggf/drmaa/UnsupportedAttributeException.java

SGEJobInfo.class: $(CLASSES_DIR)/com/sun/grid/drmaa/SGEJobInfo.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/com/sun/grid/drmaa/SGEJobInfo.java

SGEJobTemplate.class: $(CLASSES_DIR)/com/sun/grid/drmaa/SGEJobTemplate.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/com/sun/grid/drmaa/SGEJobTemplate.java

SGESession.class: $(CLASSES_DIR)/com/sun/grid/drmaa/SGESession.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/com/sun/grid/drmaa/SGESession.java

SGESessionFactory.class: $(CLASSES_DIR)/com/sun/grid/drmaa/SGESessionFactory.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/com/sun/grid/drmaa/SGESessionFactory.java

#------------------------------------------------------------------------------

test_drmaaj: $(CLASSES_DIR)/TestDRMAA.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/TestDrmaa.java

drmaaj_example: $(CLASSES_DIR)/DRMAAExample.java
	$(JAVA_HOME)/$(JAVA_BINDIR)/javac -classpath . -d . $(CLASSES_DIR)/DrmaaExample.java
