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
# change the following variables according to your installation
#
IPS=/opt/SUNWips
CODEBASE=classes
OREILLY_SERVLETS=../../lib/cos.jar

SRCS=SunTCP.java AdminTCP.java
CLASSPATH=${IPS}/lib/servlet.jar:${IPS}/lib/ips_sdk.jar:${OREILLY_SERVLETS}:${CODEBASE}

all:
	@if [ ! -r ${OREILLY_SERVLETS} ]; then \
		echo "You need the O'Reilly Servlet library to build GridPortal."; \
		echo "The latest version is available at http://www.servlets.com"; \
		exit 1; \
	fi
	@mkdir -p ${CODEBASE}
	@javac -deprecation -d ${CODEBASE} -classpath ${CLASSPATH} $(SRCS)
	@(cd ${CODEBASE}; jar cf suntcp.jar com)
   
dist: all
	mkdir -p dist/lib
	mkdir -p dist/bin
	mkdir -p dist/apps
	mkdir -p dist/workspace
	cp -r templates dist 
	cp -r vnc_patches dist
	cp classes/suntcp.jar dist/lib
	cp tools/gethomedir dist/bin
	cp install      dist
	find dist -name CVS -exec rm -rf {} \;
	(cd dist; tar cf ../gridportal.tar .)
	gzip gridportal.tar

clean:
	rm -rf ${CODEBASE}

distclean: clean
	rm -rf dist
	rm -f gridportal.tar.gz

