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

#
# adapt the following vars according to your setup
#
CODEBASE=classes
OREILLY_SERVLETS=../lib/cos.jar
S1PS_ROOT=/opt

SRCS=SunTCP.java AdminTCP.java
CLASSPATH=${S1PS_ROOT}/SUNWam/lib/servlet.jar:${S1PS_ROOT}/SUNWam/lib/am_services.jar:${S1PS_ROOT}/SUNWam/lib/am_sdk.jar:${OREILLY_SERVLETS}:${CODEBASE}

all:
	@if [ ! -r ${OREILLY_SERVLETS} ]; then \
		echo "You need the O'Reilly Servlet library to build GridPortal."; \
		echo "The latest version is available at http://www.servlets.com"; \
		exit 1; \
	fi
	@mkdir -p ${CODEBASE}
	@javac -deprecation -d ${CODEBASE} -classpath ${CLASSPATH} $(SRCS)
	@(cd ${CODEBASE}; jar cf gep.jar com)
   
dist: all
	mkdir -p dist/WEB-INF/lib
	mkdir -p dist/workspace
	cp -r channels dist 
	cp -r apps dist
	cp -r bin  dist
	cp -r vnc_patches dist
	cp classes/gep.jar dist/WEB-INF/lib
	cp web.xml.template dist/WEB-INF
	cp install      dist
	cp install.defaults    dist
	(cd dist; find . -name CVS -exec rm -rf {} \; ;tar cf ../geportal.tar .)
	gzip geportal.tar

clean:
	rm -rf ${CODEBASE}
   
distclean: clean
	rm -rf dist
	rm -f geportal.tar.gz

