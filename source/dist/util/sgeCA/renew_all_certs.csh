#!/bin/csh

# extend the validity of the CA certificate by
set CADAYS = 165
# extend the validity of the daemon certificate by
set DAEMONDAYS = 165
# extend the validity of the user certificate by
set USERDAYS = 165



if ( ! ($?SGE_ROOT && $?SGE_CELL) ) then
   echo "SGE_ROOT environment not set"
   exit 1
endif

if ( $?COMMD_PORT ) then
   set CA_PORT = port$COMMD_PORT
else
   set CA_PORT = sge_commd
endif

set CERT = "/var/sgeCA/$CA_PORT/$SGE_CELL/userkeys"

echo $CERT

# renew the ca certificate
$SGE_ROOT/util/sgeCA/sge_ca -renew_ca -days $CADAYS

# renew the daemon certificate
$SGE_ROOT/util/sgeCA/sge_ca -renew_sys -days $DAEMONDAYS 

# renew all user certificates
foreach i ($CERT/*)
   set user = `basename $i`
   $SGE_ROOT/util/sgeCA/sge_ca -renew $user -days $USERDAYS
end
