###############################################################################
# Variable definitions for targets to build
###############################################################################
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
QMASTER_DIR = $(SDIR)/daemons/qmaster

QMASTER_TARGETS = sge_qmaster

QMASTER_SRC =  qmaster$(QIDLEXT).c  \
              ck_to_do_qmaster.c \
              sge_c_ack.c \
              sge_c_gdi$(QIDLEXT).c \
              sge_calendar_qmaster$(QIDLEXT).c \
              sge_follow.c \
              sge_give_jobs$(QIDLEXT).c \
              reschedule.c \
              job_exit.c \
              sge_host_qmaster.c \
              sge_job_qmaster.c \
              sge_manop_qmaster.c \
              sge_pe_qmaster.c \
              sge_ckpt_qmaster.c \
              sge_qmod_qmaster.c \
              sge_queue_qmaster$(QIDLEXT).c \
              sge_rusage.c \
              sge_sharetree_qmaster.c \
              sge_userprj_qmaster.c \
              sge_usermap_qmaster.c \
              sge_hostgroup_qmaster.c \
              sge_userset_qmaster.c \
              complex_qmaster.c \
              configuration_qmaster.c \
              job_report_qmaster.c \
              qmaster_to_execd.c \
              sched_conf_qmaster.c \
              setup_qmaster.c \
              subordinate_qmaster.c \
              time_event.c \
              sge_user_mapping.c
             
QMASTER_ADD_SRC = usage.c sge_options.c job_log.c \
              startprog.c shutdown.c \
              admin_mail.c \
              category.c \
              read_write_complex.c \
              complex_history.c \
              config_file.c \
              execution_states.c \
              lock.c \
              mail.c \
              opt_history.c \
              qmaster_heartbeat.c \
              qmaster_running.c \
              read_list.c \
              read_object.c \
              read_write_cal.c  \
              read_write_ckpt.c  \
              read_write_host.c \
              read_write_job.c \
              read_write_manop.c \
              read_write_pe.c  \
              read_write_ume.c  \
              read_write_host_group.c \
              read_write_queue.c \
              read_write_userprj.c \
              read_write_userset.c \
              rw_configuration.c \
              sched_conf.c \
              read_write_sharetree.c \
              sig_handlers.c \
              unparse_job_cull.c 

QMASTER_OBJS  = $(QMASTER_SRC:.c=.o)

QMASTER_ADD_OBJS = $(QMASTER_ADD_SRC:.c=.o) 

QMASTER_ADD_LIBS = -lsched -lgdi $(SECLIB) $(KRBLIB) -lcull -lcom -luti -lrmon

QMASTER_LIB_DEPENDS = $(SCHEDLIB) $(GDILIB) $(SECLIBS) $(KRBLIBS) \
                      $(CULLLIB) $(COMMLIB) $(UTILIB) $(RMONLIB) \
                      $(ZLIB) $(TPLIB)

###############################################################################
# global rules: all, clean, depend, messages
###############################################################################
qmaster: $(QMASTER_TARGETS)
	
qmaster_clean:
	$(RM) $(QMASTER_TARGETS) $(QMASTER_OBJS)

qmaster_depend:
	$(DEPEND) -f$(QMASTER_DIR)/qmaster_dependencies $(QMASTER_DIR)/*.c

qmaster_messages:
	@echo Creating $@
	@for i in `ls $(QMASTER_DIR)/*.c`; do \
	   $(CC) $(CFLAGS) -DEXTRACT_MESSAGES -E $$i | $(XGETTEXT) - ; \
	done

###############################################################################
# rules to build binaries
###############################################################################
sge_qmaster:	$(QMASTER_OBJS) $(QMASTER_ADD_OBJS) $(QMASTER_LIB_DEPENDS)
		$(CC) -o $@ $(LFLAGS) $(QMASTER_OBJS) $(QMASTER_ADD_OBJS) $(QMASTER_ADD_LIBS) $(SLIBS) $(LIBS) 

###############################################################################
# include dependencies for object files generated by sge_depend
###############################################################################
include $(QMASTER_DIR)/qmaster_dependencies

###############################################################################
# rules to build object codes
###############################################################################
sge_qmod_qmaster.o:	$(QMASTER_DIR)/sge_qmod_qmaster.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/sge_qmod_qmaster.c

ck_to_do_qmaster.o:	$(QMASTER_DIR)/ck_to_do_qmaster.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/ck_to_do_qmaster.c

sge_userset_qmaster.o:	$(QMASTER_DIR)/sge_userset_qmaster.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/sge_userset_qmaster.c

sge_c_ack.o:    $(QMASTER_DIR)/sge_c_ack.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/sge_c_ack.c

sge_c_gdi$(QIDLEXT).o:	$(QMASTER_DIR)/sge_c_gdi.c
		$(CC) -o $@ $(CFLAGS) -c $(QMASTER_DIR)/sge_c_gdi.c

sge_calendar_qmaster$(QIDLEXT).o: $(QMASTER_DIR)/sge_calendar_qmaster.c
		$(CC) -o $@ $(CFLAGS) -c $(QMASTER_DIR)/sge_calendar_qmaster.c

sge_job_qmaster.o:	$(QMASTER_DIR)/sge_job_qmaster.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/sge_job_qmaster.c

sge_manop_qmaster.o:	$(QMASTER_DIR)/sge_manop_qmaster.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/sge_manop_qmaster.c

sge_follow.o:   $(QMASTER_DIR)/sge_follow.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/sge_follow.c

sge_give_jobs$(QIDLEXT).o: $(QMASTER_DIR)/sge_give_jobs.c
		$(CC) -o $@ $(CFLAGS) -c $(QMASTER_DIR)/sge_give_jobs.c   

reschedule.o: $(QMASTER_DIR)/reschedule.c
		$(CC) -o $@ $(CFLAGS) -c $(QMASTER_DIR)/reschedule.c   

sge_host_qmaster.o: $(QMASTER_DIR)/sge_host_qmaster.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/sge_host_qmaster.c

sge_pe_qmaster.o: $(QMASTER_DIR)/sge_pe_qmaster.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/sge_pe_qmaster.c

sge_ckpt_qmaster.o: $(QMASTER_DIR)/sge_ckpt_qmaster.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/sge_ckpt_qmaster.c

sge_queue_qmaster$(QIDLEXT).o: $(QMASTER_DIR)/sge_queue_qmaster.c
		$(CC) -o $@ $(CFLAGS) -c $(QMASTER_DIR)/sge_queue_qmaster.c

sge_rusage.o:	$(QMASTER_DIR)/sge_rusage.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/sge_rusage.c

sge_userprj_qmaster.o:	$(QMASTER_DIR)/sge_userprj_qmaster.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/sge_userprj_qmaster.c

sge_usermap_qmaster.o:	$(QMASTER_DIR)/sge_usermap_qmaster.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/sge_usermap_qmaster.c

sge_hostgroup_qmaster.o:	$(QMASTER_DIR)/sge_hostgroup_qmaster.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/sge_hostgroup_qmaster.c

sge_sharetree_qmaster.o: $(QMASTER_DIR)/sge_sharetree_qmaster.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/sge_sharetree_qmaster.c

complex_qmaster.o: $(QMASTER_DIR)/complex_qmaster.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/complex_qmaster.c

configuration_qmaster.o:  $(QMASTER_DIR)/configuration_qmaster.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/configuration_qmaster.c

job_exit.o   : $(QMASTER_DIR)/job_exit.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/job_exit.c

job_report_qmaster.o   : $(QMASTER_DIR)/job_report_qmaster.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/job_report_qmaster.c

sched_conf_qmaster.o   : $(QMASTER_DIR)/sched_conf_qmaster.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/sched_conf_qmaster.c

setup_qmaster.o   : $(QMASTER_DIR)/setup_qmaster.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/setup_qmaster.c

subordinate_qmaster.o   : $(QMASTER_DIR)/subordinate_qmaster.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/subordinate_qmaster.c

time_event.o   : $(QMASTER_DIR)/time_event.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/time_event.c

sge_user_mapping.o   : $(QMASTER_DIR)/sge_user_mapping.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/sge_user_mapping.c

qmaster.o: 	$(QMASTER_DIR)/qmaster.c
		$(CC) -o $@ $(CFLAGS) -c $(QMASTER_DIR)/qmaster.c

qmaster_to_execd.o   : $(QMASTER_DIR)/qmaster_to_execd.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/qmaster_to_execd.c

qmaster_license.o : $(QMASTER_DIR)/qmaster_license.c
		$(CC) $(CFLAGS) -c $(QMASTER_DIR)/qmaster_license.c

