#!/bin/sh

date=`date '+%y/%m/%d %H:%M%S'`

echo "$date: start job $*" 
echo "    SGE_STARTER_SHELL_PATH=$SGE_STARTER_SHELL_PATH"
echo "    SGE_STARTER_SHELL_START_MODE=$SGE_STARTER_SHELL_START_MODE"
echo "    SGE_STARTER_USE_LOGIN_SHELL=$SGE_STARTER_USE_LOGIN_SHELL"

$SGE_STARTER_SHELL_PATH $*
res=$?

date=`date '+%y/%m/%d %H:%M%S'`

echo "$date: finished ($res): $*"
exit $res
