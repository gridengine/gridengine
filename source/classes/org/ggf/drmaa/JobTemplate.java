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
 *  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
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
/*
 * JobTemplate.java
 *
 * Created on June 17, 2003, 10:40 AM
 */

package org.ggf.drmaa;

import java.util.List;
import java.util.Set;

/** This class represents a remote job and its attributes.  It is used to
 * set up the environment for a job to be submitted.
 * @author dan.templeton@sun.com
 * @see DRMAASession
 */
public abstract class JobTemplate {
	/** The command to execute as the job.
	 * It is relative to the execution host.<BR>
	 * It is evaluated on the execution host.<BR>
	 * No binary file management is done.<BR>
	 */	
	public static final String REMOTE_COMMAND = "drmaa_remote_command";
	/** These parameters are passed as arguments to the job. */	
	public static final String INPUT_PARAMETERS = "drmaa_v_argv";
	/** Job state at submission.  This might be useful for a rather rudimentary,
	 * but very general job-dependent execution.  The states are HOLD and ACTIVE:<BR>
	 * ACTIVE means job has been queued, and is eligible to run
	 * HOLD means job has been queued, but it is NOT eligible to run
	 */	
	public static final String JOB_SUBMISSION_STATE = "drmaa_js_state";
	/** The environment values that define the remote environment.
	 * Each string will comply with the format &lt;name&gt;=&lt;value&gt;.
	 * The values override the remote environment values if there is a collision.
	 * If above is not possible, it is implementation dependent.
	 */	
	public static final String JOB_ENVIRONMENT = "drmaa_v_env";
	/** This attribute specifies the directory where the job is executed.
	 * If not set, it is implementation dependent.
	 * Evaluated relative to the execution host.<BR>
	 * A <CODE>$drmaa_hd_ph$</CODE> placeholder at the beginning denotes the remaining
	 * portion of the directory name as a relative directory name resolved
	 * relative to the job users home directory at the execution host.<BR>
	 * The <CODE>$drmaa_incr_ph$</CODE> placeholder can be used at any position within
	 * the directory name of parametric job templates and will be substituted
	 * by the underlying DRM system with the parametric jobs' index.<BR>
	 * The directory name must be specified in a syntax that is common at the
	 * host where the job is executed.<BR>
	 * If set and no placeholder is used, an absolute directory specification
	 * is expected.<BR>
	 * If set and the directory does not exist, the job enters the state FAILED.
	 */	
	public static final String WORKING_DIRECTORY = "drmaa_wd";
	/** An opaque string specifying how to resolve site-specific resources
	 * and/or policies.
	 */	
	public static final String JOB_CATEGORY = "drmaa_job_category";
	/** An opaque string that is passed by the end user to DRMAA to specify
	 * site-specific resources and/or policies.
	 */	
	public static final String NATIVE_SPECIFICATION = "drmaa_native_specification";
	/** A list of email addresses.  It is used to report the job completion and status. */	
	public static final String EMAIL_ADDRESS = "drmaa_v_email";
	/** Used to block sending e-mail by default, regardless of the DRMS settings.
	 * <UL>
	 * <LI>1 - block</LI>
	 * <LI>0 - do not block.</LI>
	 * </UL>
	 */	
	public static final String BLOCK_EMAIL = "drmaa_block_email";
	/** This attribute specifies the earliest time when the job may be eligible
	 * to be run.  This is a required attribute.<BR>
	 * The value of the attribute will be of the form
	 * [[[[CC]YY/]MM/]DD] hh:mm[:ss] [{-|+}UU:uu]
	 * where<BR>
	 * <P>CC is the first two digits of the year (century-1)<BR>
	 * YY is the last two digits of the year<BR>
	 * MM is the two digits of the month [01,12]<BR>
	 * DD is the two-digit day of the month [01,31]<BR>
	 * hh is the two-digit hour of the day [00,23]<BR>
	 * mm is the two-digit minute of the day [00,59]<BR>
	 * ss is the two-digit second of the minute [00,61]<BR>
	 * UU is the two-digit hours since (before) UTC<BR>
	 * uu is the two-digit minutes since (before) UTC</P>
	 * If the optional UTC-offset is not specified, the offset
	 * associated with the local timezone will be used.
	 * If the day (DD) is not specified, the current day will
	 * be used unless the specified hour:mm:ss has already
	 * elapsed, in which case the next day will be used.<BR>
	 * Similarly for month (MM), year (YY), and century-1 (CC).
	 * Example:<BR>
	 * The time: Sep 3 4:47:27 PM PDT 2002,
	 * could be represented as: 2002/09/03 16:47:27 -07:00
	 */	
	public static final String START_TIME = "drmaa_start_time";
	/** The name of the job.  A job name will comprise alpha-numeric and _ characters.
	 * The drmaa-implementation will not provide the client with a job name longer
	 * than JOBNAME_BUFFER -1 (1023) characters.
	 * The drmaa-implementation may truncate any client-provided job name to an
	 * implementationdefined length that is at least 31 characters.
	 */	
	public static final String JOB_NAME = "drmaa_job_name";
	/** Specifies the jobs? standard input.
	 * Unless set elsewhere, if not explicitly set in the job template, the job
	 * is started with an empty input stream.<BR>
	 * If set, specifies the network path of the jobs input stream file of
	 * the form [hostname]:file_path<BR>
	 * When the TRANSFER_FILES job template attribute is supported and contains
	 * the character 'i', the input file will be fetched by the underlying DRM
	 * system from the specified host or from the submit host if no hostname
	 * is specified.<BR>
	 * When the TRANSFER_FILES job template attribute is not supported or does
	 * not contain the character 'i', the input file is always expected at the
	 * host where the job is executed irrespectively of a possibly hostname
	 * specified.<BR>
	 * The <CODE>$drmaa_incr_ph$</CODE> placeholder can be used at any position
	 * within the file path of parametric job templates and will be substituted
	 * by the underlying DRM system with the parametric jobs' index.<BR>
	 * A <CODE>$drmaa_hd_ph$</CODE> placeholder at the begin of the file path
	 * denotes the remaining portion of the file path as a relative file
	 * specification resolved relative to the job users home directory at the
	 * host where the file is located.<BR>
	 * A <CODE>$drmaa_wd_ph$</CODE> placeholder at the begin of the file path
	 * denotes the remaining portion of the file path as a relative file
	 * specification resolved relative to the jobs working directory at
	 * the host where the file is located.<BR>
	 * The file path must be specified in a syntax that is common at the host
	 * where the file is located.<BR>
	 * If set, and the file can't be read, the job enters the state FAILED.
	 */	
	public static final String INPUT_PATH = "drmaa_input_path";
	/** Specifies how to direct the jobs? standard output.
	 * If not explicitly set in the job template, the whereabouts of the jobs
	 * output stream is not defined.
	 * If set, specifies the network path of the jobs output stream file of the form
	 * [hostname]:file_path<BR>
	 * When the TRANSFER_FILES job template attribute is supported and contains
	 * the character 'o', the output file will be transferred by the underlying
	 * DRM system to the specified host or to the submit host if no hostname is
	 * specified.<BR>
	 * When the TRANSFER_FILES job template attribute is not supported or does not
	 * contain the character 'o', the output file is always kept at the host where
	 * the job is executed irrespectively of a possibly hostname specified.<BR>
	 * The <CODE>$drmaa_incr_ph$</CODE> placeholder can be used at any position
	 * within the file path of parametric job templates and will be substituted
	 * by the underlying DRM system with the parametric jobs' index.<BR>
	 * A <CODE>$drmaa_hd_ph$</CODE> placeholder at the begin of the file path
	 * denotes the remaining portion of the file path as a relative file
	 * specification resolved relative to the job users home directory at the
	 * host where the file is located.<BR>
	 * A <CODE>$drmaa_wd_ph$</CODE> placeholder at the begin of the file path
	 * denotes the remaining portion of the file path as a relative file
	 * specification resolved relative to the jobs working directory at the host
	 * where the file is located.<BR>
	 * The file path must be specified in a syntax that is common at the host
	 * where the file is located.<BR>
	 * If set and the file can't be written before execution the job enters the state
	 * FAILED.
	 */	
	public static final String OUTPUT_PATH = "drmaa_output_path";
	/** Specifies how to direct the jobs? standard error.
	 * If not explicitly set in the job template, the whereabouts of the jobs'
	 * error stream is not defined. If set, specifies the network path of the
	 * jobs' error stream file of the form [hostname]:file_path<BR>
	 * When the TRANSFER_FILES job template attribute is supported and contains
	 * the character 'e', the output file will be transferred by the underlying
	 * DRM system to the specified host or to the submit host if no hostname is
	 * specified.<BR>
	 * When the TRANSFER_FILES job template attribute is not supported or does not
	 * contain the character 'e', the error file is always kept at the host where
	 * the job is executed irrespectively of a possibly hostname specified.<BR>
	 * The <CODE>$drmaa_incr_ph$</CODE> placeholder can be used at any position
	 * within the file path of parametric job templates and will be substituted
	 * by the underlying DRM system with the parametric jobs' index.<BR>
	 * A <CODE>$drmaa_hd_ph$</CODE> placeholder at the begin of the file path
	 * denotes the remaining portion of the file path as a relative file
	 * specification resolved relative to the job users home directory at the
	 * host where the file is located.<BR>
	 * A <CODE>$drmaa_wd_ph$</CODE> placeholder at the begin of the file path
	 * denotes the remaining portion of the file path as a relative file
	 * specification resolved relative to the jobs working directory at the
	 * host where the file is located.<BR>
	 * The file path must be specified in a syntax that is common at the host
	 * where the file is located.<BR>
	 * If set and the file can't be written before execution the job enters the state
	 * FAILED.
	 */	
	public static final String ERROR_PATH = "drmaa_error_path";
	/** Specifies if the error stream should be intermixed with the output stream.
	 * If not explicitly set in the job template the attribute defaults to 'n'.
	 * Either 'y' or 'n' can be specified.<BR>
	 * If 'y' is specified the underlying DRM system will ignore the value of the
	 * ERROR_PATH attribute and intermix the standard error stream with the standard
	 * output stream as specified with OUTPUT_PATH.
	 */	
	public static final String JOIN_FILES = "drmaa_join_files";
	/** Specifies how to transfer files between hosts.
	 * If not explicitly set in the job template the attribute defaults to '.'
	 * Any combination of 'e', 'i' and 'o' can be specified.<BR>
	 * Whether the character 'e' is specified impacts the behavior of the
	 * ERROR_PATH attribute.<BR>
	 * Whether the character 'i' is specified impacts the behavior of the
	 * INPUT_PATH attribute.<BR>
	 * Whether the character 'o' is specified impacts the behavior of the
	 * OUTPUT_PATH attribute.
	 */	
	public static final String TRANSFER_FILES = "drmaa_tranfer_files";
	/** This attribute specifies a deadline after which the DRMS will terminate a job.
	 * The value of the attribute will be of the form
	 * [[[[CC]YY/]MM/]DD] hh:mm[:ss] [{-|+}UU:uu]
	 * where<BR>
	 * CC is the first two digits of the year (century-1)<BR>
	 * YY is the last two digits of the year<BR>
	 * MM is the two digits of the month [01,12]<BR>
	 * DD is the two digit day of the month [01,31]<BR>
	 * hh is the two digit hour of the day [00,23]<BR>
	 * mm is the two digit minute of the day [00,59]<BR>
	 * ss is the two digit second of the minute [00,61]<BR>
	 * UU is the two digit hours since (before) UTC<BR>
	 * uu is the two digit minutes since (before) UTC<BR>
	 * If an optional portion of the time specification is omitted,
	 * then the termination time will be determined based upon the
	 * the job's earliest start time.<BR>
	 * If the day (DD) is not specified, the earliest start day
	 * for the job will be used unless the specified hour:mm:ss
	 * precedes the corresponding portion of the job start time,
	 * in which case the next day will be used.<BR>
	 * Similarly for month (MM), year (YY), and century-1 (CC).
	 * Example:<BR>
	 * The time: Sep 3 4:47:27 PM PDT 2002,
	 * could be represented as: 2002/09/03 16:47:27 -07:00
	 */	
	public static final String DEADLINE_TIME = "drmaa_deadline_time";
	/** This attribute specifies when the job's wall clock time limit has
	 * been exceeded. The DRMS will terminate a job that has exceeded its wall
	 * clock time limit. Note that the suspended time is also accumulated here.<BR>
	 * The value of the attribute will be of the form
	 * [[h:]m:]s
	 * where<BR>
	 * h is one or more digits representing hours<BR>
	 * m is one or more digits representing minutes<BR>
	 * s is one or more digits representing seconds<BR>
	 * Example:<BR>
	 * To terminate a job after 2 hours and 30 minutes,
	 * any of the following can be passed:
	 * 2:30:0, 1:90:0, 150:0
	 */	
	public static final String HARD_WALLCLOCK_TIME_LIMIT = "drmaa_wct_hlimit";
	/** This attribute specifies an estimate as to how long the
	 * job will need wall clock time to complete. Note that
	 * the suspended time is also accumulated here.<BR>
	 * This attribute is intended to assist the scheduler.
	 * If the time specified in insufficient, the
	 * drmaa-implementation may impose a scheduling penalty.
	 * The value of the attribute will be of the form
	 * [[h:]m:]s
	 * where<BR>
	 * h is one or more digits representing hours<BR>
	 * m is one or more digits representing minutes<BR>
	 * s is one or more digits representing seconds
	 */	
	public static final String SOFT_WALLCLOCK_TIME_LIMIT = "drmaa_wct_slimit";
	/** This attribute specifies how long the job may be in a running state
	 * before its limit has been exceeded, and therefore is terminated by
	 * the DRMS.
	 * This is a reserved attribute named drmaa_run_duration_hlimit
	 * The value of the attribute will be of the form
	 * [[h:]m:]s
	 * where<BR>
	 * h is one or more digits representing hours<BR>
	 * m is one or more digits representing minutes<BR>
	 * s is one or more digits representing seconds
	 */	
	public static final String HARD_RUN_DURATION_LIMIT = "drmaa_run_duration_hlimit";
	/** This attribute specifies an estimate as to how long the job will need
	 * to remain in a running state to complete.
	 * This attribute is intended to assist the scheduler. If the time specified
	 * in insufficient, the drmaaimplementation may impose a scheduling penalty.<BR>
	 * The value of the attribute will be of the form
	 * [[h:]m:]s
	 * where<BR>
	 * h is one or more digits representing hours<BR>
	 * m is one or more digits representing minutes<BR>
	 * s is one or more digits representing seconds
	 */	
	public static final String SOFT_RUN_DURATION_LIMIT = "drmaa_run_duration_slimit";
	/** JOB_SUBMISSION_STATE which means job has been queued, but it is NOT
	 * eligible to run
	 */	
	public static final int HOLD = 0;
	/** JOB_SUBMISSION_STATE which means job has been queued, and is eligible to run */	
	public static final int ACTIVE = 1;
	
	/** Creates a new instance of JobTemplate */
	protected JobTemplate () {
	}
	
	/** Adds ('name', 'value') pair to list of attributes in the job template.
    * @param name the name of the attribute to set
    * @param value the value to which to set the attribute
    * @throws DRMAAException May be one of the following:
    * <UL>
    * <LI>InvalidAttributeFormatException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    * @see java.util.Collections#singletonList()
    */	
	public abstract void setAttribute (String name, String value) throws DRMAAException;
	
	/** Adds ('name', 'values') pair to list of attributes in the job template.
	 * @param name The name of the attribute to set
	 * @param value The list of values to which to set the attribute
	 * @throws DRMAAException May be one of the following:
	 * <UL>
	 * <LI>InvalidAttributeFormatException</LI>
	 * <LI>InvalidAttributeValueException</LI>
	 * <LI>ConflictingAttributeValuesException</LI>
	 * </UL>
	 */	
	public abstract void setAttribute (String name, List value) throws DRMAAException;
	
   /** If 'name' is an existing attribute name in the job template, then
	 * the value of 'name' SHALL be returned; otherwise, NULL is returned.  If the
	 * value of the attribute is not a vector, the returned List will be a
    * singleton List.
	 * @param name The name of the attribute whose value to return
	 * @return The value of the attribute
	 */	
	public abstract List getAttribute (String name);

   /** SHALL return the set of supported attribute names.  This set SHALL
    * include supported DRMAA reserved attribute names and native attribute
    * names.
	 * @return the set of supported attribute names
	 */	
	public abstract Set getAttributeNames ();
   
   /** Deallocate a job template. This routine has no effect on jobs.
    * @throws DRMAAException May be one of the following:
    * <UL>
    * <LI>DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE</LI>
    * </UL>
    */
   public abstract void delete () throws DRMAAException;
}
