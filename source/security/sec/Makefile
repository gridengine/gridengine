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
SEC_SDIR = ../security/sec
SECREQOBJS = sec_req.o sec_genrsa.o sec_certtext.o sec_makereq.o \
	     sec_setup_path.o
SECCAOBJS = sec_ca.o sec_genrsa.o sec_certtext.o sec_makereq.o sec_certify.o \
	    sec_setup_path.o
SECSIGNOBJS = sec_sign.o sec_certify.o sec_setup_path.o
SECVERIFYOBJS = sec_verify.o sec_setup_path.o
SECLIBOBJS = sec_lib.o sec_other.o sec_pack.o sec_setup_path.o
SECLIB = libsec.a
SECALL = sec_req sec_ca sec_sign sec_verify
#SECALLLIBS = $(SECLIB) libcrypto.a

sec_depend:
	$(DEPEND) -f$(SEC_SDIR)/sec_dependencies $(SEC_SDIR)/*.c

sec_messages:
	@echo Creating $@
	@for i in `ls $(SEC_SDIR)/*.c`; do \
	   $(CC) $(CFLAGS) -DEXTRACT_MESSAGES -E $$i | $(XGETTEXT) - ;\
	done

#---------------------------------------------------------------------
include $(SEC_SDIR)/sec_dependencies

#----------------------------------------------------------------------------
# rules 
#----------------------------------------------------------------------------

sec_all:	$(SECLIB) sec_req sec_ca sec_sign sec_verify

sec_clean:
	rm -f $(SECLIB) $(SECREQOBJS) $(SECCAOBJS) $(SECLIBOBJS) $(SECALL) \
              $(SECSIGNOBJ) $(SECVERIFYOBJS)

sec_req:	$(SECREQOBJS) $(SECLIBS)
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $(SECREQOBJS) $(LIBS) $(SECLIBS)

sec_ca:		$(SECCAOBJS) $(SECLIBS)
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $(SECCAOBJS) $(LIBS) $(SECLIBS) 

sec_sign:	$(SECSIGNOBJS) $(SECLIBS)
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $(SECSIGNOBJS) $(LIBS) $(SECLIBS)

sec_verify:	$(SECVERIFYOBJS) $(SECLIBS)
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $(SECVERIFYOBJS) $(LIBS) $(SECLIBS)

$(SECLIB):	$(SECLIBOBJS)
	$(AR) $(ARFLAGS) $@ $?
	$(RANLIB) $@

#---------------------------------------------------------------------------

sec_lib.o:	$(SEC_SDIR)/sec_lib.c
	$(CC) $(CFLAGS) -c $(SEC_SDIR)/sec_lib.c

sec_other.o:	$(SEC_SDIR)/sec_other.c
	$(CC) $(CFLAGS) -c $(SEC_SDIR)/sec_other.c

sec_pack.o:	$(SEC_SDIR)/sec_pack.c
	$(CC) $(CFLAGS) -c $(SEC_SDIR)/sec_pack.c

sec_req.o:	$(SEC_SDIR)/sec_req.c
	$(CC) $(CFLAGS) -c $(SEC_SDIR)/sec_req.c

sec_ca.o:	$(SEC_SDIR)/sec_ca.c
	$(CC) $(CFLAGS) -c $(SEC_SDIR)/sec_ca.c

sec_sign.o:	$(SEC_SDIR)/sec_sign.c
	$(CC) $(CFLAGS) -c $(SEC_SDIR)/sec_sign.c

sec_verify.o:	$(SEC_SDIR)/sec_verify.c
	$(CC) $(CFLAGS) -c $(SEC_SDIR)/sec_verify.c

sec_genrsa.o:	$(SEC_SDIR)/sec_genrsa.c
	$(CC) $(CFLAGS) -c $(SEC_SDIR)/sec_genrsa.c

sec_certtext.o:	$(SEC_SDIR)/sec_certtext.c
	$(CC) $(CFLAGS) -c $(SEC_SDIR)/sec_certtext.c

sec_makereq.o:	$(SEC_SDIR)/sec_makereq.c
	$(CC) $(CFLAGS) -c $(SEC_SDIR)/sec_makereq.c

sec_certify.o:	$(SEC_SDIR)/sec_certify.c
	$(CC) $(CFLAGS) -c $(SEC_SDIR)/sec_certify.c

sec_setup_path.o:	$(SEC_SDIR)/sec_setup_path.c
	$(CC) $(CFLAGS) -c $(SEC_SDIR)/sec_setup_path.c
