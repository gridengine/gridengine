#!/bin/ksh

# starter method to transfer jobs to another SGE cluster
# NOTE: make sure the host which this script runs on is a submit host
# for the remote cluster
# below should contain path to SGE_ROOT of cluster where jobs are being sent


### SITE SPECIFIC PARAMETERS

# how often to poll remote cluster
REMOTE_CHECK_INT=5

# project for jobs in remote cluster
REMOTE_PROJECT=sgeproject1

# remote cluster settings
REMOTE_COMMD=536
REMOTE_SGE_ROOT=/net/tcf27-b030/share/gridware
REMOTE_SGE_CELL=default
#### END SITE SPECIFIC ####

LOCAL_ARCH=`$SGE_ROOT/util/arch`
LOCAL_SGE_ROOT=$SGE_ROOT
LOCAL_SGE_CELL=$SGE_CELL
LOCAL_JOB_ID=$JOB_ID

# this command can be a bottleneck for many simultaneous submissions
$SGE_ROOT/bin/$LOCAL_ARCH/qstat -j $JOB_ID > $TMPDIR/qstat 2>/dev/null

export COMMD_PORT=$REMOTE_COMMD
export SGE_ROOT=$REMOTE_SGE_ROOT
export SGE_CELL=$REMOTE_SGE_CELL
ARCH=`$SGE_ROOT/util/arch`
export PATH=$SGE_ROOT/bin/$ARCH:$PATH
# avoid PARENT being set due to JOB_ID env var of current job
unset JOB_ID

# set up and submit jobs

hard_requests=`awk '/^hard resource/ {print $3}' $TMPDIR/qstat`
soft_requests=`awk '/^soft resource/ {print $3}' $TMPDIR/qstat`
stdout_path_list=`awk '/^stdout_path_list/ {print $2}' $TMPDIR/qstat`
stderr_path_list=`awk '/^stderr_path_list/ {print $2}' $TMPDIR/qstat`
env_list=`awk '/^env_list/ {print $2}' $TMPDIR/qstat`
project=`awk '/^project/ {print $2}' $TMPDIR/qstat`
merge=`awk '/^merge/ {print $2}' $TMPDIR/qstat`
grep "job-array" $TMPDIR/qstat > /dev/null 2>&1
arrayjob=$?

if [ "X$stdout_path_list" == "X" ]; then
	stdout_string=""
else
	stdout_string="-o $stdout_path_list"
fi

if [ "X$stderr_path_list" == "X" ]; then
	stderr_string=""
else
	stderr_string="-e $stderr_path_list"
fi

if [ "X$merge" == "X" ]; then
	merge_string=""
else
	merge_string="-j $merge"
fi

if [ "X$hard_requests" == "X" ]; then
	hard_requests_string=""
else
	hard_requests_string="-hard -l $hard_requests"
fi

if [ "X$soft_requests" == "X" ]; then
	soft_requests_string=""
else
	soft_requests_string="-soft -l $soft_requests"
fi

if [ "X$env_list" == "X" ]; then
	env_list_string=""
else
	env_list_string="-v $env_list"
fi

if [ "X$REMOTE_PROJECT" == "X" ]; then
     if [ "X$project" == "X" ]; then
         project_string=""
     else
        project_string="-P $project"
     fi
else
        project_string="-P $REMOTE_PROJECT"
fi

STAMP=`/usr/bin/date +%d%H%M`

rm $TMPDIR/qstat
cd $SGE_O_WORKDIR

if [ $arrayjob -eq 0 ]; then
# process as array job

output=`qsub -N transfer.$STAMP -cwd \
	-A $JOB_NAME.$JOB_ID \
	-t $SGE_TASK_ID:$SGE_TASK_ID:2 \
	$stdout_string \
	$stderr_string \
	$hard_requests_string \
	$soft_requests_string \
	$env_list_string \
	$merge_string \
	$project_string \
	$*`
jid=`echo $output | awk '/^your/ { print substr($3, 1, index ($3, ".") - 1) }`

else
# process as regular job
output=`qsub -N transfer.$STAMP -cwd \
	-A $JOB_NAME.$JOB_ID \
	$stdout_string \
	$stderr_string \
	$hard_requests_string \
	$soft_requests_string \
	$env_list_string \
	$merge_string \
	$project_string \
	$*`
jid=`echo $output | awk '/^your/ {print \$3}'`
# end of submission
fi

if [ "X$jid" == "X" ]; then
# job was never started, so exit
	exit 1
fi

# need to store this JID somewhere for starter, suspend, resume to use
# use context variables on "this" job
# qalter needs to work on LOCAL job, not remote job!
# also, need to store the remote JID in accounting field,
# so that resource utilization can be retrieved later from the
# remote cluster

# execute in subshell to avoid changing vars
(	SGE_ROOT=$LOCAL_SGE_ROOT
	SGE_CELL=$LOCAL_SGE_CELL
	unset -v COMMD_PORT
	ARCH=`$SGE_ROOT/util/arch`
	PATH=$SGE_ROOT/bin/$ARCH:$PATH
	echo $jid > $TMPDIR/REMOTE_JID
# store remote JID in accounting record.
# NOTE: this gives incorrect information in the
# case of array jobs.
#	qalter -A REMOTE_JID=$jid $LOCAL_JOB_ID > /dev/null 2>&1
# NOTE2: commented out to reduce load on qmaster.  This becomes
# an issue with large numbers of tasks, eg, job arrays
)

#qstat -j $jid  > /dev/null 2>&1
# NOTE: sometimes the above is not always reliable indicator if the job is done
# or not.  The below provides an alternative
qstat -s prs | grep $jid > /dev/null 2>&1
val=$?
while [ $val -eq 0 ] ; do
	sleep $REMOTE_CHECK_INT
#	qstat -j $jid > /dev/null 2>&1
	qstat -s prs | grep $jid > /dev/null 2>&1
	val=$?
done

