QMAKE_MAKEFILE_VARIABLE="abc123"
export QMAKE_MAKEFILE_VARIABLE

variables:
	./echo_variable.sh MAKELEVEL ;\
	./echo_variable.sh MAKEFLAGS ;\
	./echo_variable.sh MFLAGS ;\
	./echo_variable.sh MAKEOVERRIDES ;\
	./echo_variable.sh QMAKE_MAKEFILE_VARIABLE ;\
	./echo_variable.sh QMAKE_TEST

rreq:
	SGE_RREQ="-v QMAKE_TEST=rreq" ./echo_variable.sh QMAKE_TEST

compile_1:
	echo "compiling 1" ; sleep 5 ; echo "done compiling 1"

compile_2:
	echo "compiling 2" ; sleep 10 ; echo "done compiling 2"

link: variables compile_1 compile_2
	echo "linking" ; sleep 2 ; echo "done linking"

recursive:
	$(MAKE) $(MFLAGS) link
