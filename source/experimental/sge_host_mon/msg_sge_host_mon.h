#ifndef __MSG_SGE_HOST_MON_H
#define __MSG_SGE_HOST_MON_H
/*___INFO__MARK_BEGIN__*/
/*************************************************************************
 * 
 *  The Contents of this file are made available subject to the terms of
 *  the Sun Industry Standards Source License Version 1.2
 * 
 *  Sun Microsystems Inc., March, 2001
 * 
 * 
 *  Sun Industry Standards Source License Version 1.2
 *  =================================================
 *  The contents of this file are subject to the Sun Industry Standards
 *  Source License Version 1.2 (the "License"); You may not use this file
 *  except in compliance with the License. You may obtain a copy of the
 *  License at http://www.gridengine.sunsource.net/license.html
 * 
 *  Software provided under this License is provided on an "AS IS" basis,
 *  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
 *  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
 *  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
 *  See the License for the specific provisions governing your rights and
 *  obligations concerning the Software.
 * 
 *   The Initial Developer of the Original Code is: Sun Microsystems, Inc.
 * 
 *   Copyright: 2001 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/
/* 
** schedd/sge_host_mon.c
*/ 
#define MSG_JOB_NOJOBLIST                          _("No job list")
#define MSG_JOB_SPLITJOBLISTFINISHSTATEFAILED   _("couldn't split job list concerning finished state\n")
#define MSG_JOB_SPLITJOBLISTRUNNINGSTATEFAILED  _("couldn't split job list concerning running state\n")
#define MSG_SGEHOSTMON_USAGE                    _("host_usage:")
#define MSG_SGEHOSTMON_c_OPT_USAGE              _("number of collections (default is infinite)\n")
#define MSG_SGEHOSTMON_d_OPT_USAGE              _("delimiter between columns (default is <TAB>)\n")
#define MSG_SGEHOSTMON_f_OPT_USAGE              _("list of fields to print\n")
#define MSG_SGEHOSTMON_h_OPT_USAGE              _("print a header containing the field names\n")
#define MSG_SGEHOSTMON_i_OPT_USAGE              _("collection interval in seconds (default is 15)\n")
#define MSG_SGEHOSTMON_l_OPT_USAGE              _("delimiter between nodes (default is <CR>)\n")
#define MSG_SGEHOSTMON_m_OPT_USAGE              _("output file fopen mode (default is \"w\")\n")
#define MSG_SGEHOSTMON_n_OPT_USAGE              _("use name=value format\n")
#define MSG_SGEHOSTMON_o_OPT_USAGE              _("output file\n")
#define MSG_SGEHOSTMON_r_OPT_USAGE              _("delimiter between collection records (default is <CR>)\n")
#define MSG_SGEHOSTMON_s_OPT_USAGE              _("format of displayed strings (default is %%s)\n")
#define MSG_SGEHOSTMON_t_OPT_USAGE              _("show formatted times\n")
#define MSG_SGEHOSTMON_u_OPT_USAGE              _("show decayed usage (since timestamp) in nodes\n")
#define MSG_SGEHOSTMON_x_OPT_USAGE              _("exclude non-leaf nodes\n")
#define MSG_ERROR_XISNOTAVALIDINTERVAL_S        _("%s is not a valid interval\n")
#define MSG_ERROR_XISNOTAVALIDCOUNT_S           _("%s is not a valid count\n")
#define MSG_FILE_COULDNOTOPENXFORY_SS           _("could not open %s for %s\n")
#define MSG_ERROR_UNABLETODUMPLIST              _("Unable to dump list\n")



#endif /* __MSG_SGE_HOST_MON_H */
