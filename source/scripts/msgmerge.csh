#!/bin/csh -f
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

set cmd = $1 
setenv ARCH `./aimk -no-mk`
set MSGMERGE = ./3rdparty/gettext/${ARCH}/bin/msgmerge
set MSGFMT = ./3rdparty/gettext/${ARCH}/bin/msgfmt
set MSGUNIQ = ./3rdparty/gettext/${ARCH}/bin/msguniq
#set MSGFMT = /usr/bin/msgfmt
#set MSGFMT = /home/codine/gettext/${ARCH}/bin/msgfmt
#set MSGFMT = /usr/bin/msgfmt
set LOCALEDIR = "./dist/locale"
#set LANGUAGES = "de en en_FW.MBE en_FW.ASCII"
#set LANGUAGES = "de en fr ja zh"
set LANGUAGES = ""
set MSGPO = "gridengine.po"
set MSGMO = "gridengine.mo"
set MSGPOT = "gridengine.pot"
set MSGPOTNOTUNIQ = "gridenginenotuniq.pot"

# uniq the pot file
sed 's/charset=CHARSET/charset=ascii/' ${LOCALEDIR}/${MSGPOTNOTUNIQ} > /tmp/${MSGPOTNOTUNIQ}
$MSGUNIQ -o "${LOCALEDIR}/${MSGPOT}" "/tmp/${MSGPOTNOTUNIQ}"
rm "/tmp/${MSGPOTNOTUNIQ}"


if ( $cmd == "merge") then
   foreach I ($LANGUAGES)
      echo "####################### Language: ${I} ###############################"
      echo "perform merge in ${LOCALEDIR}/${I}/LC_MESSAGES/${MSGPO}"
      if ( ! -d ${LOCALEDIR}/${I}/LC_MESSAGES ) then
         mkdir -p ${LOCALEDIR}/${I}/LC_MESSAGES
      endif
      if ( -f ${LOCALEDIR}/${I}/LC_MESSAGES/${MSGPO} ) then
         echo ${LOCALEDIR}/${I}/LC_MESSAGES/${MSGPO} saved as ${LOCALEDIR}/${I}/LC_MESSAGES/${MSGPO}.bak
         mv ${LOCALEDIR}/${I}/LC_MESSAGES/${MSGPO} ${LOCALEDIR}/${I}/LC_MESSAGES/${MSGPO}.bak
         $MSGMERGE -o "${LOCALEDIR}/${I}/LC_MESSAGES/${MSGPO}" "${LOCALEDIR}/${I}/LC_MESSAGES/${MSGPO}.bak" "${LOCALEDIR}/${MSGPOT}"
      else
         cp ${LOCALEDIR}/${MSGPOT} ${LOCALEDIR}/${I}/LC_MESSAGES/${MSGPO} 
      endif
   end
   echo ""
   echo "######################################################################"
   echo "#       W A R N I N G       W A R N I N G       W A R N I N G        #"
   echo "######################################################################"
   echo "# please look forward for some (fuzzy) messages in the *.po files!   #"
   echo "# Messages with the fuzzy comment should be viewed. This are new     #"
   echo "# localized messages. The generated translation may be wrong!        #"
   echo "# If you think a fuzzy translation is correct then remove the fuzzy  #"
   echo "# comment. Thank you.                                                #"
   echo "######################################################################"
else
   if ( $ARCH == "SOLARIS" || $ARCH == "SOLARIS64" ) then
      foreach I ($LANGUAGES)
         if ( -f ${LOCALEDIR}/${I}/LC_MESSAGES/${MSGMO} ) then
            echo ${LOCALEDIR}/${I}/LC_MESSAGES/${MSGMO} saved as ${LOCALEDIR}/${I}/LC_MESSAGES/${MSGMO}.bak
            mv ${LOCALEDIR}/${I}/LC_MESSAGES/${MSGMO} ${LOCALEDIR}/${I}/LC_MESSAGES/${MSGMO}.bak
         endif
         echo "producing binary MO File from ${LOCALEDIR}/${I}/LC_MESSAGES/${MSGPO}"
         $MSGFMT -o "${LOCALEDIR}/${I}/LC_MESSAGES/${MSGMO}" "${LOCALEDIR}/${I}/LC_MESSAGES/${MSGPO}" 
      end
   else
      echo "Message formatting must be done on Solaris"
      exit 1
   endif   
endif
