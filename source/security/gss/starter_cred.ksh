#!/bin/sh

set +u

if [ -z "$SGE_ROOT" ]
then
   echo "\$SGE_ROOT environment variable not set" >> error
   echo 100 > exit_status
   exit 100
fi
SGE_CELL=${SGE_CELL:-default}

arch=`$SGE_ROOT/util/arch`
eval `grep "^USER=" environment`
eval `grep "^JOB_ID=" environment`
eval `grep "^KRB5CCNAME=" environment`
export USER
export JOB_ID
export KRB5CCNAME

# Remove KRB5CCNAME from the environment file and
# change ownership of environment file, so we can set
# the new KRB5CCNAME returned by k5dcelogin below.

grep -v "^KRB5CCNAME=" <environment >environment.new
mv environment.new environment
chown $USER environment

exec $SGE_ROOT/utilbin/$arch/k5dcelogin /bin/sh -c "echo KRB5CCNAME=\$KRB5CCNAME>>environment; exec $SGE_ROOT/bin/$arch/sge_shepherd -bg; echo could not exec shepherd >> error; exit 100" 2>> error

echo "could not exec k5dcelogin" >> error
echo 100 > exit_status
exit 100
