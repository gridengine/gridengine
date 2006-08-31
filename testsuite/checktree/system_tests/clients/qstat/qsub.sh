qsub -o /dev/null -j y -N STARTED -q all.q $SGE_ROOT/examples/jobs/sleeper.sh 10000
qsub -o /dev/null -j y -N SEQUENTIAL -q all.q $SGE_ROOT/examples/jobs/sleeper.sh 10000
qsub -o /dev/null -j y -N ARRAY -q all.q -t 1-4 $SGE_ROOT/examples/jobs/sleeper.sh 10000
qsub -o /dev/null -j y -N PARALLEL -q all.q -pe mytestpe 2 $SGE_ROOT/examples/jobs/sleeper.sh 10000
qsub -o /dev/null -j y -N ERROR -q all.q -S /foo/bar/test $SGE_ROOT/examples/jobs/sleeper.sh 10000
qsub -o /dev/null -j y -N HOLD -q all.q -h $SGE_ROOT/examples/jobs/sleeper.sh 10000
qsub -o /dev/null -j y -N PENDING -q all.q -t 1-4 -p -100 $SGE_ROOT/examples/jobs/sleeper.sh 10000
qsub -o /dev/null -j y -N AMD64 -l arch=lx24-amd64 -t 1-5 $SGE_ROOT/examples/jobs/sleeper.sh 10000
#qsub -o /dev/null -j y -N QERROR -q wrong.q $SGE_ROOT/examples/jobs/sleeper.sh 10000
