qsub -o /dev/null -j y -N STARTED -q all.q $SGE_ROOT/examples/jobs/sleeper.sh 1000
qsub -o /dev/null -j y -N SEQUENTIAL -q all.q $SGE_ROOT/examples/jobs/sleeper.sh 10000
#qsub -o /dev/null -j y -N ARRAY -q all.q -t 1-4 $SGE_ROOT/examples/jobs/sleeper.sh 10000
qsub -o /dev/null -j y -N HOLD -q all.q -h $SGE_ROOT/examples/jobs/sleeper.sh 10000
qsub -o /dev/null -j y -N PENDING -q all.q -t 1-4 -p -100 $SGE_ROOT/examples/jobs/sleeper.sh 10000
#qsub -o /dev/null -j y -N AMD64  -q all.q  -t 1-5 $SGE_ROOT/examples/jobs/sleeper.sh 10000
