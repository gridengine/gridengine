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
RMON_DIR = $(SDIR)/libs/rmon

RMONLIB = librmon$(QIDLEXT)$(LIBEXT)

STD_INC = $(RMON_DIR)/rmon_h.h \
	$(RMON_DIR)/rmon_def.h \
	$(RMON_DIR)/rmon.h \
	$(RMON_DIR)/rmon_rmon.h

PROTOCOL_INC = $(RMON_DIR)/rmon_h.h \
	$(RMON_DIR)/rmon_io.h \
	$(RMON_DIR)/rmon_err.h \
	$(RMON_DIR)/rmon_rmon.h \
	$(RMON_DIR)/rmon.h

SPY_INC = $(RMON_DIR)/rmon_h.h \
	$(RMON_DIR)/rmon_def.h \
	$(RMON_DIR)/rmon_err.h \
	$(RMON_DIR)/rmon.h \
	$(RMON_DIR)/rmon_rmon.h \
	$(RMON_DIR)/rmon_spy.h 

RMON_INC = $(RMON_DIR)/rmon_h.h \
	$(RMON_DIR)/rmon_def.h \
	$(RMON_DIR)/rmon_err.h \
	$(RMON_DIR)/rmon.h \
	$(RMON_DIR)/rmon_rmon.h \
	$(RMON_DIR)/rmon_request.h \
	$(RMON_DIR)/rmon_connect.h 

RMOND_INC = $(RMON_DIR)/rmon_h.h \
	$(RMON_DIR)/rmon_err.h \
	$(RMON_DIR)/rmon.h \
	$(RMON_DIR)/rmon_rmon.h \
	$(RMON_DIR)/rmon_request.h 

LIST_INC = $(RMON_DIR)/rmon_h.h \
	$(RMON_DIR)/rmon_def.h \
	$(RMON_DIR)/rmon_err.h \
	$(RMON_DIR)/rmon_rmon.h \
	$(RMON_DIR)/rmon.h

MTOOLS_INC = $(RMON_DIR)/rmon_h.h \
	$(RMON_DIR)/rmon_def.h \
	$(RMON_DIR)/rmon.h \
	$(RMON_DIR)/rmon_rmon.h \
	$(RMON_DIR)/rmon_request.h \
	$(RMON_DIR)/rmon_connect.h \
	$(RMON_DIR)/rmon_siginit.h

RMON_ALL = rmond rmon spy knecht knecht_light knecht_long mdel \
	mlevel mquit mstat msysstat mjob mconf rmon_strip

# -------------------------------------------
# source definitions
#

RMONLIB_SRC = rmon_macros$(QIDLEXT).c rmon_semaph.c \
				rmon_monitoring_level.c

RMONDC_SRC = rmon_m_c_client_register.c rmon_m_c_spy_register.c \
	rmon_m_c_monitoring_level.c rmon_m_c_mstat.c \
	rmon_m_c_mdel.c rmon_m_c_mquit.c rmon_m_c_mjob.c \
	rmon_m_c_flush.c rmon_m_c_spy_exit.c rmon_m_c_mconf.c

RMOND_SRC = $(RMONDC_SRC) \
	rmon_io.c rmon_err.c rmon_siginit.c rmon_connect.c \
	rmon_client_number.c rmon_request.c rmon_spy_list.c \
	rmon_client_list.c rmon_transition_list.c rmon_convert.c \
	rmon_job_list.c rmon_wait_list.c rmon_message_list.c \
	rmon_spy_protocol.c rmon_client_protocol.c \
	rmon_transition_protocol.c rmon_job_protocol.c \
	rmon_wait_protocol.c rmon_message_protocol.c \
	rmon_server.c rmon_conf.c rmon_restart.c rmon_daemon.c \
	rmon_lock.c 

RMON_SRC = rmon_io.c rmon_err.c rmon_semaph.c rmon_siginit.c \
	rmon_connect.c rmon_request.c rmon_c_c_flush.c \
	rmon_c_c_monitoring_level.c rmon_c_c_client_register.c \
	rmon_message_list.c rmon_message_protocol.c rmon_convert.c \
	rmon_conf.c rmon_monitoring_level.c

SPY_SRC = rmon_err.c rmon_io.c rmon_s_c_flush.c rmon_request.c \
	rmon_job_list.c rmon_job_protocol.c rmon_message_list.c \
	rmon_message_protocol.c rmon_connect.c rmon_s_c_sleep.c \
	rmon_s_c_wake_up.c rmon_s_c_spy_register.c \
	rmon_s_c_overflow.c rmon_s_c_monitoring_level.c \
	rmon_semaph.c rmon_siginit.c rmon_convert.c rmon_server.c \
	rmon_s_c_exit.c rmon_conf.c rmon_daemon.c rmon_s_c_mconf.c \
	rmon_lock.c rmon_restart.c rmon_spy_list.c

MSTAT_SRC = rmon_err.c rmon_io.c rmon_siginit.c rmon_connect.c \
	rmon_request.c rmon_get_stat.c rmon_spy_list.c \
	rmon_wait_list.c rmon_client_list.c rmon_job_list.c \
	rmon_transition_list.c rmon_spy_protocol.c \
	rmon_wait_protocol.c rmon_client_protocol.c \
	rmon_job_protocol.c rmon_transition_protocol.c \
	rmon_message_list.c  rmon_convert.c rmon_conf.c

MSYSSTAT_SRC = rmon_err.c rmon_io.c rmon_siginit.c rmon_connect.c \
	rmon_request.c rmon_get_stat.c rmon_spy_list.c \
	rmon_wait_list.c rmon_client_list.c rmon_job_list.c \
	rmon_transition_list.c rmon_spy_protocol.c \
	rmon_wait_protocol.c rmon_client_protocol.c \
	rmon_job_protocol.c rmon_transition_protocol.c \
	rmon_message_list.c rmon_convert.c rmon_conf.c

MDEL_SRC = rmon_err.c rmon_io.c rmon_siginit.c rmon_connect.c \
	rmon_request.c rmon_convert.c rmon_conf.c

MLEVEL_SRC = rmon_err.c rmon_io.c rmon_siginit.c rmon_request.c \
	rmon_connect.c rmon_c_c_monitoring_level.c rmon_convert.c \
	rmon_conf.c

MJOB_SRC = rmon_err.c rmon_io.c rmon_siginit.c rmon_job_protocol.c \
	rmon_connect.c rmon_request.c rmon_convert.c rmon_conf.c 

MQUIT_SRC = rmon_err.c rmon_io.c rmon_siginit.c rmon_connect.c \
	rmon_request.c rmon_wait_list.c rmon_wait_protocol.c \
	rmon_spy_list.c rmon_spy_protocol.c rmon_message_list.c \
	rmon_convert.c rmon_conf.c

MCONF_SRC = rmon_err.c rmon_io.c rmon_siginit.c rmon_request.c \
	rmon_spy_list.c rmon_convert.c rmon_message_list.c \
	rmon_connect.c rmon_conf.c

KNECHT_SRC = rmon_daemon.c rmon_err.c

# ------------------------------------------
# object definitions
#

RMONLIB_OBJS 	= $(RMONLIB_SRC:.c=.o)
RMOND_OBJS 	= $(RMOND_SRC:.c=.o)	$(RMONLIB)
RMON_OBJS	= $(RMON_SRC:.c=.o)	$(RMONLIB)
SPY_OBJS 	= $(SPY_SRC:.c=.o)  	$(RMONLIB)
MSTAT_OBJS 	= $(MSTAT_SRC:.c=.o) 	$(RMONLIB)
MSYSSTAT_OBJS	= $(MSYSSTAT_SRC:.c=.o) 	$(RMONLIB)
MDEL_OBJS 	= $(MDEL_SRC:.c=.o) 	$(RMONLIB)
MLEVEL_OBJS 	= $(MLEVEL_SRC:.c=.o) 	$(RMONLIB)
MJOB_OBJS 	= $(MJOB_SRC:.c=.o) 	$(RMONLIB)
MQUIT_OBJS 	= $(MQUIT_SRC:.c=.o)	$(RMONLIB)
MCONF_OBJS 	= $(MCONF_SRC:.c=.o)	$(RMONLIB)

KNECHT_OBJS	= $(KNECHT_SRC:.c=.o)	$(RMONLIB)


RMON_ALLOBJS = $(RMONLIB_OBJS) $(RMON_OBJS) $(SPY_OBJS) \
	$(MSTAT_OBJS) $(MSYSSTAT_OBJS) $(MDEL_OBJS) $(MLEVEL_OBJS) \
	$(MJOB_OBJS) $(MQUIT_OBJS) $(MCONF_OBJS)

rmon_all: $(RMON_ALL)

rmonlib: $(RMONLIB)

#
# remove objects
#

# RMON_ALLSRC = `ls -1 $(RMON_DIR)/*.c`
# RMON_ALLOBJ = `../util/allobj.sh $(RMON_ALLSRC)`

rmon_tidy:
	rm -f $(RMON_ALLOBJS)
#
# remove objects and executables
#
rmonlib_clean:  rmon_tidy
	rm -f $(RMON_ALL) librmon.a librmon$(SHAREDEXT)

rmonlib_depend:
	$(DEPEND) -f$(RMON_DIR)/rmon_dependencies $(RMON_DIR)/*.c

rmonlib_messages:
	@echo Creating $@
	@for i in `ls $(RMON_DIR)/*.c`; do \
	   $(CC) $(CFLAGS) -DEXTRACT_MESSAGES -E $$i | $(XGETTEXT) - ;\
	done

#
# remove symblols from executables
#
rmon_strip:
	strip rmond
	strip rmon 
	strip spy 
	strip knecht
	strip knecht_light
	strip knecht_long
	strip mdel
	strip mlevel
	strip mquit
	strip mstat
	strip msysstat
	strip mjob
	strip mconf

#
#  rmond
#

rmond: rmon_rmond.o $(RMOND_OBJS) 
	$(CC) $(CFLAGS) -o rmond rmon_rmond.o $(RMOND_OBJS) $(LFLAGS) $(LIBS)

###############################################################################
# include dependencies for object files generated by sge_depend
###############################################################################
include $(RMON_DIR)/rmon_dependencies

rmon_rmond.o: $(RMON_DIR)/rmon_rmond.c \
	$(RMOND_INC) \
	$(RMON_DIR)/rmon_def.h \
	$(RMON_DIR)/rmon_siginit.h \
	$(RMON_DIR)/rmon_daemon.h \
	$(RMON_DIR)/rmon_spy_protocol.h \
	$(RMON_DIR)/rmon_client_number.h \
	$(RMON_DIR)/rmon_job_list.h \
	$(RMON_DIR)/rmon_spy_list.h \
	$(RMON_DIR)/rmon_wait_list.h \
	$(RMON_DIR)/rmon_client_list.h \
	$(RMON_DIR)/rmon_transition_list.h \
	$(RMON_DIR)/rmon_monitoring_level.h \
	$(RMON_DIR)/rmon_m_c_client_register.h \
	$(RMON_DIR)/rmon_m_c_flush.h \
	$(RMON_DIR)/rmon_m_c_mdel.h \
	$(RMON_DIR)/rmon_m_c_mjob.h \
	$(RMON_DIR)/rmon_m_c_monitoring_level.h \
	$(RMON_DIR)/rmon_m_c_mquit.h \
	$(RMON_DIR)/rmon_m_c_mstat.h \
	$(RMON_DIR)/rmon_m_c_spy_register.h \
	$(RMON_DIR)/rmon_m_c_mconf.h \
	$(RMON_DIR)/rmon_m_c_spy_exit.h 
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_rmond.c

#
# rmon 
#

rmon: rmon_rmon.o $(RMON_OBJS) 
	$(CC) $(CFLAGS) -o rmon rmon_rmon.o $(RMON_OBJS) $(LFLAGS) $(LIBS)

rmon_rmon.o:   $(RMON_DIR)/rmon_rmon.c $(RMON_INC) \
	$(RMON_DIR)/rmon_siginit.h \
	$(RMON_DIR)/rmon_message_list.h \
	$(RMON_DIR)/rmon_message_protocol.h \
	$(RMON_DIR)/rmon_c_c_client_register.h \
	$(RMON_DIR)/rmon_c_c_flush.h \
	$(RMON_DIR)/rmon_c_c_monitoring_level.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_rmon.c

#
# spy
#


spy:	rmon_spy.o $(SPY_OBJS)
	$(CC) $(CFLAGS) -o spy rmon_spy.o $(SPY_OBJS) $(LFLAGS) $(LIBS)

rmon_spy.o: 	$(RMON_DIR)/rmon_spy.c $(SPY_INC) \
	$(RMON_DIR)/rmon_semaph.h \
	$(RMON_DIR)/rmon_siginit.h \
	$(RMON_DIR)/rmon_daemon.h \
	$(RMON_DIR)/rmon_request.h \
	$(RMON_DIR)/rmon_connect.h \
	$(RMON_DIR)/rmon_job_list.h \
	$(RMON_DIR)/rmon_job_protocol.h \
	$(RMON_DIR)/rmon_message_list.h \
	$(RMON_DIR)/rmon_message_protocol.h \
	$(RMON_DIR)/rmon_s_c_sleep.h \
	$(RMON_DIR)/rmon_s_c_wake_up.h \
	$(RMON_DIR)/rmon_s_c_exit.h \
	$(RMON_DIR)/rmon_s_c_flush.h \
	$(RMON_DIR)/rmon_s_c_overflow.h \
	$(RMON_DIR)/rmon_s_c_spy_register.h \
	$(RMON_DIR)/rmon_s_c_mconf.h \
	$(RMON_DIR)/rmon_s_c_monitoring_level.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_spy.c

#
# knecht
# 


knecht: rmon_knecht.o $(KNECHT_OBJS) 
	$(CC) $(CFLAGS) -o knecht rmon_knecht.o $(KNECHT_OBJS) $(LFLAGS) $(LIBS)

rmon_knecht.o: 	$(RMON_DIR)/rmon_knecht.c \
	$(RMON_DIR)/rmon_sge_rmon.h 
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_knecht.c

#
# knecht_light
# 

knecht_light:	rmon_knecht_light.o $(KNECHT_OBJS) 
	$(CC) $(CFLAGS) -o knecht_light rmon_knecht_light.o $(KNECHT_OBJS) $(LFLAGS) $(LIBS)

rmon_knecht_light.o: 	$(RMON_DIR)/rmon_knecht_light.c \
	$(RMON_DIR)/rmon_daemon.h\
	$(RMON_DIR)/rmon_rmon.h \
	$(RMON_DIR)/rmon.h 
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_knecht_light.c

#
# knecht_long
# 

knecht_long:	rmon_knecht_long.o $(KNECHT_OBJS) 
	$(CC) $(CFLAGS) -o knecht_long rmon_knecht_long.o $(KNECHT_OBJS) $(LFLAGS) $(LIBS)

rmon_knecht_long.o: 	$(RMON_DIR)/rmon_knecht_long.c \
	$(RMON_DIR)/rmon_daemon.h \
	$(RMON_DIR)/rmon_rmon.h \
	$(RMON_DIR)/rmon.h 
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_knecht_long.c

#
# mstat 
#


mstat:	rmon_mstat.o $(MSTAT_OBJS) 
	$(CC) $(CFLAGS) -o mstat rmon_mstat.o $(MSTAT_OBJS) $(LFLAGS) $(LIBS)

rmon_mstat.o: 	$(RMON_DIR)/rmon_mstat.c $(MTOOLS_INC) \
	$(RMON_DIR)/rmon_spy_list.h \
	$(RMON_DIR)/rmon_wait_list.h \
	$(RMON_DIR)/rmon_client_list.h \
	$(RMON_DIR)/rmon_transition_list.h \
	$(RMON_DIR)/rmon_monitoring_level.h \
	$(RMON_DIR)/rmon_transition_protocol.h \
	$(RMON_DIR)/rmon_get_stat.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_mstat.c

rmon_get_stat.o: $(RMON_DIR)/rmon_get_stat.c $(MTOOLS_INC) \
	$(RMON_DIR)/rmon_spy_protocol.h \
	$(RMON_DIR)/rmon_wait_protocol.h \
	$(RMON_DIR)/rmon_client_protocol.h \
	$(RMON_DIR)/rmon_transition_protocol.h \
	$(RMON_DIR)/rmon_get_stat.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_get_stat.c

#
# msysstat 
#


msysstat:	rmon_msysstat.o $(MSYSSTAT_OBJS) 
	$(CC) $(CFLAGS) -o msysstat rmon_msysstat.o $(MSYSSTAT_OBJS) $(LFLAGS) $(LIBS)

rmon_msysstat.o:	$(RMON_DIR)/rmon_msysstat.c $(MTOOLS_INC) \
	$(RMON_DIR)/rmon_spy_list.h \
	$(RMON_DIR)/rmon_spy_protocol.h \
	$(RMON_DIR)/rmon_wait_list.h \
	$(RMON_DIR)/rmon_wait_protocol.h \
	$(RMON_DIR)/rmon_monitoring_level.h \
	$(RMON_DIR)/rmon_client_list.h \
	$(RMON_DIR)/rmon_client_protocol.h \
	$(RMON_DIR)/rmon_transition_list.h \
	$(RMON_DIR)/rmon_transition_protocol.h \
	$(RMON_DIR)/rmon_get_stat.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_msysstat.c

#
# mdel  
#


mdel:	rmon_mdel.o $(MDEL_OBJS) 
	$(CC) $(CFLAGS) -o mdel rmon_mdel.o $(MDEL_OBJS) $(LFLAGS) $(LIBS)

rmon_mdel.o: 	$(RMON_DIR)/rmon_mdel.c $(MTOOLS_INC)
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_mdel.c


#
# mlevel  
#


mlevel:	rmon_mlevel.o $(MLEVEL_OBJS) 
	$(CC) $(CFLAGS) -o mlevel rmon_mlevel.o $(MLEVEL_OBJS) $(LFLAGS) $(LIBS)

rmon_mlevel.o: 	$(RMON_DIR)/rmon_mlevel.c $(MTOOLS_INC) \
	$(RMON_DIR)/rmon_c_c_monitoring_level.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_mlevel.c

#
# mjob  
#


mjob:	rmon_mjob.o $(MJOB_OBJS) 
	$(CC) $(CFLAGS) -o mjob rmon_mjob.o $(MJOB_OBJS) $(LFLAGS) $(LIBS)

rmon_mjob.o: 	$(RMON_DIR)/rmon_mjob.c $(MTOOLS_INC) \
	$(RMON_DIR)/rmon_job_list.h \
	$(RMON_DIR)/rmon_job_protocol.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_mjob.c

#
# mquit
#


mquit:	rmon_mquit.o $(MQUIT_OBJS) 
	$(CC) $(CFLAGS) -o mquit rmon_mquit.o $(MQUIT_OBJS) $(LFLAGS) $(LIBS)

rmon_mquit.o: 	$(RMON_DIR)/rmon_mquit.c $(MTOOLS_INC)
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_mquit.c


#
# mconf
#


mconf:	rmon_mconf.o $(MCONF_OBJS) 
	$(CC) $(CFLAGS) -o mconf rmon_mconf.o $(MCONF_OBJS) $(LFLAGS) $(LIBS)

rmon_mconf.o:    $(RMON_DIR)/rmon_mconf.c $(MTOOLS_INC)
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_mconf.c

rmon_lock.o:		$(RMON_DIR)/rmon_lock.c  
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_lock.c

#---------------------------------------------------------------------
# build the remote monitoring library


RMON_LIBINC = $(RMON_DIR)/rmon_def.h $(RMON_DIR)/rmon_rmon.h \
	$(RMON_DIR)/rmon.h $(RMON_DIR)/rmon_monitoring_level.h $(RMON_DIR)/rmon_semaph.h \
	$(RMON_DIR)/rmon_piped_message.h $(RMON_DIR)/rmon_io.h

librmon.a: $(RMONLIB_OBJS)
	$(AR) $(ARFLAGS) librmon.a $(RMONLIB_OBJS)
	$(RANLIB) librmon.a

librmon$(SHAREDEXT): $(RMONLIB_OBJS)
	$(SHAREDLD) $(SHARED_LFLAGS) -o librmon$(SHAREDEXT) $(RMONLIB_OBJS) $(LIBS) -lc

rmon_macros$(QIDLEXT).o:$(RMON_DIR)/rmon_macros.c $(RMON_LIBINC)
	$(CC) -o $@ $(CFLAGS) $(NOERR_CFLAG) $(SHARED_CFLAGS) -c $(RMON_DIR)/rmon_macros.c

rmon_semaph.o:$(RMON_DIR)/rmon_semaph.c $(RMON_LIBINC)
	$(CC) $(CFLAGS) $(SHARED_CFLAGS) -c $(RMON_DIR)/rmon_semaph.c

rmon_monitoring_level.o:  $(RMON_DIR)/rmon_monitoring_level.c $(RMON_LIBINC)
	$(CC) $(CFLAGS) $(SHARED_CFLAGS) -c $(RMON_DIR)/rmon_monitoring_level.c

rmon_piped_message.o:  $(RMON_DIR)/rmon_piped_message.c $(RMON_LIBINC)
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_piped_message.c

#---------------------------------------------------------------------
#
#
# build the objects
#
#

rmon_err.o:		$(RMON_DIR)/rmon_err.c $(STD_INC) 
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_err.c

rmon_request.o: 	$(RMON_DIR)/rmon_request.c $(STD_INC) \
	$(RMON_DIR)/rmon_request.h \
	$(RMON_DIR)/rmon_io.h \
	$(RMON_DIR)/rmon_convert.h \
	$(RMON_DIR)/rmon_err.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_request.c

rmon_convert.o:	$(RMON_DIR)/rmon_convert.c $(STD_INC) \
	$(RMON_DIR)/rmon_err.h \
	$(RMON_DIR)/rmon_convert.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_convert.c

rmon_io.o:	$(RMON_DIR)/rmon_io.c $(STD_INC) \
	$(RMON_DIR)/rmon_err.c
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_io.c

rmon_server.o:   $(RMON_DIR)/rmon_server.c $(STD_INC) \
	$(RMON_DIR)/rmon_server.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_server.c

rmon_siginit.o:	$(RMON_DIR)/rmon_siginit.c $(STD_INC) \
	$(RMON_DIR)/rmon_siginit.h \
	$(RMON_DIR)/rmon_err.h 
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_siginit.c

rmon_connect.o:	$(RMON_DIR)/rmon_connect.c $(STD_INC) \
	$(RMON_DIR)/rmon_connect.h \
	$(RMON_DIR)/rmon_request.h \
	$(RMON_DIR)/rmon_conf.h \
	$(RMON_DIR)/rmon_def.h \
	$(RMON_DIR)/rmon_io.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_connect.c

rmon_daemon.o:	$(RMON_DIR)/rmon_daemon.c $(STD_INC) \
	$(RMON_DIR)/rmon_daemon.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_daemon.c

rmon_conf.o:		$(RMON_DIR)/rmon_conf.c $(STD_INC) $(RMON_DIR)/rmon_conf.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_conf.c

rmon_restart.o:	$(RMON_DIR)/rmon_restart.c $(STD_INC) \
	$(RMON_DIR)/rmon_restart.h \
	$(RMON_DIR)/rmon_connect.h \
	$(RMON_DIR)/rmon_request.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_restart.c

# list modules

rmon_job_list.o:	$(RMON_DIR)/rmon_job_list.c $(LIST_INC) \
	$(RMON_DIR)/rmon_job_list.h  
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_job_list.c

rmon_spy_list.o:	$(RMON_DIR)/rmon_spy_list.c $(LIST_INC) \
	$(RMON_DIR)/rmon_spy_list.h \
	$(RMON_DIR)/rmon_monitoring_level.h \
	$(RMON_DIR)/rmon_message_list.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_spy_list.c

rmon_wait_list.o:	$(RMON_DIR)/rmon_wait_list.c $(LIST_INC) $(RMON_DIR)/rmon_wait_list.h \
	$(RMON_DIR)/rmon_monitoring_level.h 
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_wait_list.c

rmon_client_list.o:	$(RMON_DIR)/rmon_client_list.c $(LIST_INC) $(RMON_DIR)/rmon_client_list.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_client_list.c

rmon_message_list.o:	$(RMON_DIR)/rmon_message_list.c $(LIST_INC) $(RMON_DIR)/rmon_message_list.h \
	$(RMON_DIR)/rmon_monitoring_level.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_message_list.c

rmon_client_number.o:	$(RMON_DIR)/rmon_client_number.c $(LIST_INC) $(RMON_DIR)/rmon_client_number.h  
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_client_number.c

rmon_transition_list.o:	$(RMON_DIR)/rmon_transition_list.c $(LIST_INC) \
	$(RMON_DIR)/rmon_transition_list.h \
	$(RMON_DIR)/rmon_monitoring_level.h \
	$(RMON_DIR)/rmon_client_list.h \
	$(RMON_DIR)/rmon_spy_list.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_transition_list.c

# protocol modules

rmon_job_protocol.o:	$(RMON_DIR)/rmon_job_protocol.c $(PROTOCOL_INC) \
	$(RMON_DIR)/rmon_job_protocol.h \
	$(RMON_DIR)/rmon_job_list.h \
	$(RMON_DIR)/rmon_convert.h 
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_job_protocol.c

rmon_spy_protocol.o:	$(RMON_DIR)/rmon_spy_protocol.c $(PROTOCOL_INC) \
	$(RMON_DIR)/rmon_spy_protocol.h \
	$(RMON_DIR)/rmon_monitoring_level.h \
	$(RMON_DIR)/rmon_spy_list.h \
	$(RMON_DIR)/rmon_convert.h \
	$(RMON_DIR)/rmon_def.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_spy_protocol.c

rmon_wait_protocol.o:	$(RMON_DIR)/rmon_wait_protocol.c $(PROTOCOL_INC) \
	$(RMON_DIR)/rmon_wait_protocol.h \
	$(RMON_DIR)/rmon_wait_list.h \
	$(RMON_DIR)/rmon_monitoring_level.h \
	$(RMON_DIR)/rmon_convert.h \
	$(RMON_DIR)/rmon_def.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_wait_protocol.c

rmon_client_protocol.o:	$(RMON_DIR)/rmon_client_protocol.c $(PROTOCOL_INC) \
	$(RMON_DIR)/rmon_client_protocol.h \
	$(RMON_DIR)/rmon_client_list.h \
	$(RMON_DIR)/rmon_job_protocol.h \
	$(RMON_DIR)/rmon_job_list.h \
	$(RMON_DIR)/rmon_convert.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_client_protocol.c

rmon_message_protocol.o:	$(RMON_DIR)/rmon_message_protocol.c $(PROTOCOL_INC) \
	$(RMON_DIR)/rmon_message_protocol.h \
	$(RMON_DIR)/rmon_monitoring_level.h \
	$(RMON_DIR)/rmon_message_list.h \
	$(RMON_DIR)/rmon_convert.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_message_protocol.c

rmon_transition_protocol.o:	$(RMON_DIR)/rmon_transition_protocol.c $(PROTOCOL_INC) \
	$(RMON_DIR)/rmon_transition_protocol.h \
	$(RMON_DIR)/rmon_monitoring_level.h \
	$(RMON_DIR)/rmon_transition_list.h \
	$(RMON_DIR)/rmon_convert.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_transition_protocol.c

#
# monitor - cases
#
rmon_m_c_client_register.o: $(RMON_DIR)/rmon_m_c_client_register.c $(RMOND_INC) \
	$(RMON_DIR)/rmon_def.h \
	$(RMON_DIR)/rmon_client_list.h \
	$(RMON_DIR)/rmon_client_number.h \
	$(RMON_DIR)/rmon_m_c_client_register.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_m_c_client_register.c

rmon_m_c_spy_register.o: $(RMON_DIR)/rmon_m_c_spy_register.c $(RMOND_INC) \
	$(RMON_DIR)/rmon_def.h \
	$(RMON_DIR)/rmon_spy_list.h \
	$(RMON_DIR)/rmon_wait_list.h \
	$(RMON_DIR)/rmon_client_list.h \
	$(RMON_DIR)/rmon_transition_list.h \
	$(RMON_DIR)/rmon_s_c_sleep.h \
	$(RMON_DIR)/rmon_m_c_spy_register.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_m_c_spy_register.c

rmon_m_c_spy_exit.o: $(RMON_DIR)/rmon_m_c_spy_exit.c $(RMOND_INC) \
	$(RMON_DIR)/rmon_def.h \
	$(RMON_DIR)/rmon_spy_list.h \
	$(RMON_DIR)/rmon_message_list.h \
	$(RMON_DIR)/rmon_message_protocol.h \
	$(RMON_DIR)/rmon_m_c_spy_exit.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_m_c_spy_exit.c

rmon_m_c_flush.o: $(RMON_DIR)/rmon_m_c_flush.c $(RMOND_INC) \
	$(RMON_DIR)/rmon_connect.h \
	$(RMON_DIR)/rmon_m_c_flush.h \
	$(RMON_DIR)/rmon_spy_list.h \
	$(RMON_DIR)/rmon_client_list.h \
	$(RMON_DIR)/rmon_monitoring_level.h \
	$(RMON_DIR)/rmon_message_list.h \
	$(RMON_DIR)/rmon_transition_list.h \
	$(RMON_DIR)/rmon_message_protocol.h 
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_m_c_flush.c

rmon_m_c_monitoring_level.o: $(RMON_DIR)/rmon_m_c_monitoring_level.c $(RMOND_INC) \
	$(RMON_DIR)/rmon_def.h \
	$(RMON_DIR)/rmon_connect.h \
	$(RMON_DIR)/rmon_spy_list.h \
	$(RMON_DIR)/rmon_wait_list.h \
	$(RMON_DIR)/rmon_client_list.h \
	$(RMON_DIR)/rmon_transition_list.h \
	$(RMON_DIR)/rmon_monitoring_level.h \
	$(RMON_DIR)/rmon_m_c_monitoring_level.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_m_c_monitoring_level.c

rmon_m_c_mstat.o: $(RMON_DIR)/rmon_m_c_mstat.c $(RMOND_INC) \
	$(RMON_DIR)/rmon_spy_list.h \
	$(RMON_DIR)/rmon_wait_list.h \
	$(RMON_DIR)/rmon_client_list.h \
	$(RMON_DIR)/rmon_transition_list.h \
	$(RMON_DIR)/rmon_job_list.h \
	$(RMON_DIR)/rmon_spy_protocol.h \
	$(RMON_DIR)/rmon_wait_protocol.h \
	$(RMON_DIR)/rmon_client_protocol.h \
	$(RMON_DIR)/rmon_transition_protocol.h \
	$(RMON_DIR)/rmon_job_protocol.h \
	$(RMON_DIR)/rmon_m_c_mstat.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_m_c_mstat.c

rmon_m_c_mquit.o: $(RMON_DIR)/rmon_m_c_mquit.c $(RMOND_INC) \
	$(RMON_DIR)/rmon_m_c_mquit.h \
	$(RMON_DIR)/rmon_def.h \
	$(RMON_DIR)/rmon_connect.h \
	$(RMON_DIR)/rmon_spy_list.h \
	$(RMON_DIR)/rmon_wait_list.h \
	$(RMON_DIR)/rmon_client_list.h \
	$(RMON_DIR)/rmon_transition_list.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_m_c_mquit.c

rmon_m_c_mdel.o:	$(RMON_DIR)/rmon_m_c_mdel.c $(RMOND_INC) \
	$(RMON_DIR)/rmon_m_c_mdel.h \
	$(RMON_DIR)/rmon_def.h \
	$(RMON_DIR)/rmon_request.h \
	$(RMON_DIR)/rmon_connect.h \
	$(RMON_DIR)/rmon_spy_list.h \
	$(RMON_DIR)/rmon_wait_list.h \
	$(RMON_DIR)/rmon_client_list.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_m_c_mdel.c

rmon_m_c_mjob.o:	$(RMON_DIR)/rmon_m_c_mjob.c $(RMOND_INC) \
	$(RMON_DIR)/rmon_m_c_mjob.h \
	$(RMON_DIR)/rmon_m_c_monitoring_level.h \
	$(RMON_DIR)/rmon_def.h \
	$(RMON_DIR)/rmon_monitoring_level.h \
	$(RMON_DIR)/rmon_client_number.h \
	$(RMON_DIR)/rmon_spy_list.h \
	$(RMON_DIR)/rmon_transition_list.h \
	$(RMON_DIR)/rmon_job_list.h \
	$(RMON_DIR)/rmon_job_protocol.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_m_c_mjob.c

rmon_m_c_mconf.o:$(RMON_DIR)/rmon_m_c_mconf.c $(RMOND_INC) \
	$(RMON_DIR)/rmon_m_c_mconf.h \
	$(RMON_DIR)/rmon_spy_list.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_m_c_mconf.c

#
# client - cases
#

rmon_c_c_client_register.o:	$(RMON_DIR)/rmon_c_c_client_register.c $(RMON_INC) \
	$(RMON_DIR)/rmon_c_c_client_register.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_c_c_client_register.c

rmon_c_c_monitoring_level.o: $(RMON_DIR)/rmon_c_c_monitoring_level.c $(RMON_INC) \
	$(RMON_DIR)/rmon_c_c_monitoring_level.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_c_c_monitoring_level.c

rmon_c_c_flush.o: 		$(RMON_DIR)/rmon_c_c_flush.c $(RMON_INC) \
	$(RMON_DIR)/rmon_c_c_flush.h \
	$(RMON_DIR)/rmon_message_list.h \
	$(RMON_DIR)/rmon_message_protocol.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_c_c_flush.c

#
# spy - cases
#

rmon_s_c_spy_register.o: $(RMON_DIR)/rmon_s_c_spy_register.c $(SPY_INC) \
	$(RMON_DIR)/rmon_s_c_spy_register.h \
	$(RMON_DIR)/rmon_restart.h \
	$(RMON_DIR)/rmon_monitoring_level.h \
	$(RMON_DIR)/rmon_request.h \
	$(RMON_DIR)/rmon_connect.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_s_c_spy_register.c

rmon_s_c_sleep.o:	$(RMON_DIR)/rmon_s_c_sleep.c $(SPY_INC) \
	$(RMON_DIR)/rmon_s_c_sleep.h \
	$(RMON_DIR)/rmon_monitoring_level.h \
	$(RMON_DIR)/rmon_request.h \
	$(RMON_DIR)/rmon_semaph.h \
	$(RMON_DIR)/rmon_job_list.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_s_c_sleep.c

rmon_s_c_wake_up.o:	$(RMON_DIR)/rmon_s_c_wake_up.c $(SPY_INC) \
	$(RMON_DIR)/rmon_s_c_wake_up.h \
	$(RMON_DIR)/rmon_s_c_sleep.h \
	$(RMON_DIR)/rmon_s_c_spy_register.h \
	$(RMON_DIR)/rmon_request.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_s_c_wake_up.c

rmon_s_c_exit.o:		$(RMON_DIR)/rmon_s_c_exit.c $(SPY_INC) \
	$(RMON_DIR)/rmon_s_c_exit.h \
	$(RMON_DIR)/rmon_request.h \
	$(RMON_DIR)/rmon_connect.h \
	$(RMON_DIR)/rmon_semaph.h \
	$(RMON_DIR)/rmon_message_list.h \
	$(RMON_DIR)/rmon_message_protocol.h 
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_s_c_exit.c

rmon_s_c_flush.o:		$(RMON_DIR)/rmon_s_c_flush.c $(SPY_INC) \
	$(RMON_DIR)/rmon_s_c_flush.h \
	$(RMON_DIR)/rmon_request.h \
	$(RMON_DIR)/rmon_message_list.h \
	$(RMON_DIR)/rmon_message_protocol.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_s_c_flush.c

rmon_s_c_overflow.o:		$(RMON_DIR)/rmon_s_c_overflow.c $(SPY_INC) \
	$(RMON_DIR)/rmon_s_c_overflow.h \
	$(RMON_DIR)/rmon_monitoring_level.h \
	$(RMON_DIR)/rmon_piped_message.h \
	$(RMON_DIR)/rmon_message_list.h \
	$(RMON_DIR)/rmon_job_list.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_s_c_overflow.c

rmon_s_c_monitoring_level.o: $(RMON_DIR)/rmon_s_c_monitoring_level.c $(SPY_INC) \
	$(RMON_DIR)/rmon_s_c_monitoring_level.h \
	$(RMON_DIR)/rmon_monitoring_level.h \
	$(RMON_DIR)/rmon_request.h \
	$(RMON_DIR)/rmon_job_list.h \
	$(RMON_DIR)/rmon_job_protocol.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_s_c_monitoring_level.c

rmon_s_c_mconf.o:		$(RMON_DIR)/rmon_s_c_mconf.c $(SPY_INC) \
	$(RMON_DIR)/rmon_s_c_mconf.h
	$(CC) $(CFLAGS) -c $(RMON_DIR)/rmon_s_c_mconf.c
