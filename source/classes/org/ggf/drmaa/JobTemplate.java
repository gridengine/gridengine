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

import java.util.*;

/** This class represents a remote job and its attributes.  It is used to
 * set up the environment for a job to be submitted.
 * @author dan.templeton@sun.com
 * @see DRMAASession
 */
public abstract class JobTemplate {
   /** Placeholder which represents the home directory in workingDirectory, inputPath,
    * outputPath, and errorPath attributes
    */   
   private static final String HOME_DIRECTORY = "$drmaa_hd_ph$";
   /** Placeholder which represents the working directory in workingDirectory,
    * inputPath, outputPath, and errorPath attributes
    */   
   private static final String WORKING_DIRECTORY = "$drmaa_wd_ph$";
   /** The names of all the required attributes */   
   private static final String[] attributeNames = new String[] {
      "blockEmail",
      "emailAddresses",
      "errorPath",
      "inputParameters",
      "inputPath",
      "jobCategory",
      "jobEnvironment",
      "jobName",
      "jobSubmissionState",
      "joinFiles",
      "nativeSpecification",
      "outputPath",
      "remoteCommand",
      "startTime",
      "workingDirectory"
   };

	/** jobSubmissionState which means job has been queued, but it is NOT
    * eligible to run
    */	
	public static final int HOLD = 0;
	/** jobSumissionState which means job has been queued, and is eligible to run */	
	public static final int ACTIVE = 1;
   /** transferFiles value which means not to transfer any files */   
   public static final byte TRANSFER_NONE = 0x00;
   /** transferFiles values which means to export error files */   
   public static final byte TRANSFER_ERROR_FILES = 0x01;
   /** transferFiles value which means to import input files */   
   public static final byte TRANSFER_INPUT_FILES = 0x02;
   /** transferFiles value which means to export output files */   
   public static final byte TRANSFER_OUTPUT_FILES = 0x04;

   /** Remote command to execute */   
   protected String remoteCommand = null;
   /** Input parameters passed as arguments to the job */   
   protected String[] args = null;
   /** Job state at submission */   
   protected int state = ACTIVE;
   /* I used a Properties here instead of a Map because Properties are always
    * strings, and Properties can have a set of parent Properties to whose value
    * unset attributes default. */
   /** The environment values that define the remote environment */   
   protected Properties env = null;
   /** The directory where the job is executed. */   
   protected String wd = null;
   /** An implementation-defined string specifying how to resolve site-specific resources
    * and/or policies
    */   
   protected String category = null;
   /** An implementation-defined string that is passed by the end user to DRMAA to specify
    * site-specific resources and/or policies
    */   
   protected String spec = null;
   /** E-mail addresses used to report the job completion and status */   
   protected String[] email = null;
   /** Blocks sending e-mail by default, regardless of the DRMS setting */   
   protected boolean blockEmail = false;
   /** The earliest time when the job MAY be eligible to be run */   
   protected Date startTime = null;
   /** Job name */   
   protected String name = null;
   /** The job's standard input stream */   
   protected String inputPath = null;
   /** The job's standard output stream */   
   protected String outputPath = null;
   /** The job's standard error stream */   
   protected String errorPath = null;
   /** Whether the error stream should be intermixed with the output stream */   
   protected boolean join = false;
      
   /** Set the command to execute as the job.  The command
    * is relative to the execution host and is evaluated on the
    * execution host.  No binary file management is done.
    * @param remoteCommand The command to execute as the job
    * @throws DRMAAException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeFormatException</LI>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    *
    */
   public void setRemoteCommand (String remoteCommand) throws DRMAAException {
      this.remoteCommand = remoteCommand;
   }
   
   /** Get the command to execute as the job.  The command
	 * is relative to the execution host and is evaluated on the
    * execution host.  No binary file management is done.
	 * @return The command to execute as the job or null if it has not been set
	 */
   public String getRemoteCommand () {
      return remoteCommand;
   }
   
	/** Set the parameters passed as arguments to the job.
    * @param args The parameters passed as arguments to the job
    * @throws DRMAAException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeFormatException</LI>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    *
    */
   public void setInputParameters (String[] args) throws DRMAAException {
      this.args = new String[args.length];
      System.arraycopy (args, 0, this.args, 0, args.length);
   }
   
	/** Get The parameters passed as arguments to the job.
	 * @return The parameters passed as arguments to the job or null if they have
    * not been set
	 */	
   public String[] getInputParameters () {
      if (args != null) {
         String[] returnValue = new String[args.length];

         System.arraycopy (args, 0, returnValue, 0, args.length);

         return returnValue;
      }
      else {
         return null;
      }
   }
   
	/** Set the job state at submission.  This might be useful for a rather
    * rudimentary, but very general, job dependent execution.  The states are
    * HOLD and ACTIVE:<BR>
    * ACTIVE means job has been queued, and is eligible to run
    * HOLD means job has been queued, but it is NOT eligible to run
    * @param state The job state at submission
    * @throws DRMAAException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    *
    */	
   public void setJobSubmissionState (int state) throws DRMAAException {
      if ((state != ACTIVE) && (state != HOLD)) {
         throw new IllegalArgumentException ("Invalid state");
      }
      
      this.state = state;
   }
   
	/** Get the job state at submission.  The states are HOLD and ACTIVE:<BR>
	 * ACTIVE means job has been queued, and is eligible to run
	 * HOLD means job has been queued, but it is NOT eligible to run
	 * @return The job state at submission
	 */	
   public int getJobSubmissionState () {
      return state;
   }
   
	/** Set the environment values that define the remote environment.
    * The values override the remote environment values if there is a collision.
    * If above is not possible, it is implementation dependent.
    * @param env The environment values that define the remote environment
    * @throws DRMAAException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeFormatException</LI>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    *
    */
   public void setJobEnvironment (Properties env) throws DRMAAException {
      /* We keep the original reference instead of making a copy because we want
       * to enable changing environment entries without having to make two more
       * copies of all of the enviroment variables (one for the get and one for
       * the set) */
      this.env = env;
   }
   
	/** Get the environment values that define the remote environment.
	 * The values override the remote environment values if there is a collision.
	 * If above is not possible, it is implementation dependent.
	 * @return The environment values that define the remote environment or null
    * if it has not been set
	 */
   public Properties getJobEnvironment () {
      /* We return the original reference instead of returning a copy because we
       * want to enable changing environment entries without having to make two
       * more copies of all of the enviroment variables (one for the get and one
       * for the set) */
      return env;
   }

   /** Set the directory where the job is executed.  If the working directory is
    * not set, behavior is implementation dependent.  The working directory is
    * evaluated relative to the execution host.<BR>
    * A <CODE>$drmaa_hd_ph$</CODE> placeholder at the beginning denotes that the
    * remaining portion of the directory name is resolved
    * relative to the job submiter's home directory on the execution host.<BR>
    * The <CODE>$drmaa_incr_ph$</CODE> placeholder can be used at any position
    * within the directory name of parametric jobs and will be replaced
    * by the underlying DRM system with the parametric jobs' index.<BR>
    * The directory name must be specified in a syntax that is common at the
    * host where the job will be executed.<BR>
    * If no placeholder is used, an absolute directory specification
    * is expected.<BR>
    * If the directory does not exist when the job is run, the job enters the
    * state FAILED.
    * @param wd The directory where the job is executed
    * @throws DRMAAException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeFormatException</LI>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    *
    */
   public void setWorkingDirectory (String wd) throws DRMAAException {
      if (wd.indexOf (HOME_DIRECTORY) > 0) {
         throw new InvalidAttributeFormatException ("$drmaa_hd_ph$ may only appear at the beginning of the path.");
      }
      
      this.wd = wd;
   }
   
   /** Get the directory where the job is executed.  The working directory is
    * evaluated relative to the execution host.<BR>
	 * A <CODE>$drmaa_hd_ph$</CODE> placeholder at the beginning denotes that the
    * remaining portion of the directory name is resolved
	 * relative to the job submiter's home directory on the execution host.<BR>
	 * The <CODE>$drmaa_incr_ph$</CODE> placeholder can be used at any position
    * within the directory name of parametric jobs and will be replaced
	 * by the underlying DRM system with the parametric jobs' index.<BR>
	 * The directory name is specified in a syntax that is common at the
	 * host where the job will be executed.<BR>
	 * If no placeholder is used, an absolute directory specification
	 * is represented.<BR>
    * @return The directory where the job is executed or null if it has not been
    * set
	 */
   public String getWorkingDirectory () {
      return wd;
   }

   /** Set an opaque string specifying how to resolve site-specific resources
    * and/or policies.
    * @param category An opaque string specifying how to resolve site-specific
    * resources and/or policies.
    * @throws DRMAAException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeFormatException</LI>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    *
    */
   public void setJobCategory (String category) throws DRMAAException {
      this.category = category;
   }
   
   /** Get the opaque string specifying how to resolve site-specific resources
	 * and/or policies.
    * @return The opaque string specifying how to resolve site-specific
	 * resources and/or policies or null if it has not been set
	 */
   public String getJobCategory () {
      return category;
   }
   
	/** Set an opaque string that is passed by the end user to DRMAA to specify
    * site-specific resources and/or policies.
    * @param spec An opaque string that is passed by the end user to DRMAA to
    * specify site-specific resources and/or policies
    * @throws DRMAAException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeFormatException</LI>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setNativeSpecification (String spec) throws DRMAAException {
      this.spec = spec;
   }
   
	/** Get the opaque string that is passed by the end user to DRMAA to specify
    * site-specific resources and/or policies.
    * @return The opaque string that is passed by the end user to DRMAA to
    * specify site-specific resources and/or policies or null if it has not been
    * set
    */
   public String getNativeSpecification () {
      return spec;
   }
   
	/** Set the list of email addresses used to report the job completion and
    * status.
    * @param email The list of email addresses used to report the job completion
    * and status.
    * @throws DRMAAException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeFormatException</LI>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */	
   public void setEmailAddresses (String[] email) throws DRMAAException {
      this.email = new String[email.length];
      System.arraycopy (email, 0, this.email, 0, email.length);
   }
   
	/** Get the list of email addresses used to report the job completion and
    * status.
    * @return The list of email addresses used to report the job completion
    * and status or null if they have not been set
    */	
   public String[] getEmailAddresses () {
      if (email != null) {
         String[] returnValue = new String[email.length];

         System.arraycopy (email, 0, this.email, 0, email.length);

         return returnValue;
      }
      else {
         return null;
      }
   }
   
	/** Set whether to block sending e-mail by default, regardless of the DRMS
    * settings.
    * @param blockEmail Whether to block sending e-mail by default
    * @throws DRMAAException Maybe be one of the following:
    * <UL>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setBlockEmail (boolean blockEmail) throws DRMAAException {
      this.blockEmail = blockEmail;
   }
   
	/** Get whether to block sending e-mail by default, regardless of the DRMS
    * settings.
    * @return Whether to block sending e-mail by default
	 */
   public boolean getBlockEmail () {
      return blockEmail;
   }
   
	/** Set the earliest time when the job may be eligible to be run.
    * @param startTime The earliest time when the job may be eligible to be run
    * @throws DRMAAException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setStartTime (Date startTime) throws DRMAAException {
      if (startTime.getTime () < System.currentTimeMillis ()) {
         throw new IllegalArgumentException ("Start time is in the past.");
      }
      
      this.startTime = startTime;
   }
   
	/** Get the earliest time when the job may be eligible to be run.
    * @return The earliest time when the job may be eligible to be run or null
    * if it has not been set
	 */
   public Date getStartTime () {
      if (startTime != null) {
         return (Date)startTime.clone ();
      }
      else {
         return null;
      }
   }
   
	/** Set the name of the job.  A job name will be comprised of alpha-numeric
    * and _ characters.  The DRMAA implementation may truncate client
    * provided job names to an implementation defined length that is at least 31
    * characters.
    * @param name The name of the job
    * @throws DRMAAException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeFormatException</LI>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setJobName (String name) throws DRMAAException {
      this.name = name;
   }
   
	/** Get the name of the job.  A job name will be comprised of alpha-numeric
    * and _ characters.
    * @return The name of the job or null if it has not been set
	 */
   public String getJobName () {
      return name;
   }
   
	/** Set the job's standard input path.
    * Unless set elsewhere, if not explicitly set in the job template, the job
    * is started with an empty input stream.<BR>
    * If set, specifies the network path of the job's input stream in
    * the form of [hostname]:file_path<BR>
    * When the TRANSFER_FILES job template attribute is supported and contains
    * the character 'i', the input file will be fetched by the underlying DRM
    * system from the specified host or from the submit host if no hostname
    * is specified.<BR>
    * When the TRANSFER_FILES job template attribute is not supported or does
    * not contain the character 'i', the input file is always expected at the
    * host where the job is executed irrespectively of a possibly hostname
    * specified.<BR>
    * The <CODE>$drmaa_incr_ph$</CODE> placeholder can be used at any position
    * within the file path of parametric job templates and will be replaced
    * by the underlying DRM system with the parametric job's index.<BR>
    * A <CODE>$drmaa_hd_ph$</CODE> placeholder at the beginning of the file path
    * denotes that the remaining portion of the file path is relative to the job
    * submiter's home directory on the host where the file is located.<BR>
    * A <CODE>$drmaa_wd_ph$</CODE> placeholder at the beginning of the file path
    * denotes that the remaining portion of the file path is relative to the
    * job's working directory on the host where the file is located.<BR>
    * The file path must be specified in a syntax that is common at the host
    * where the file is located.<BR>
    * When the job is run, if this attribute is set, and the file can't be read,
    * the job will enter the state FAILED.
    * @param inputPath The job's standard input path
    * @throws DRMAAException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeFormatException</LI>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setInputPath (String inputPath) throws DRMAAException {
      this.checkPath (inputPath);
      this.inputPath = inputPath;
   }
   
	/** Get the job's standard input path.<BR>
	 * Specifies the network path of the job's input stream in
	 * the form of [hostname]:file_path<BR>
	 * When the TRANSFER_FILES job template attribute is supported and contains
	 * the character 'i', the input file will be fetched by the underlying DRM
	 * system from the specified host or from the submit host if no hostname
	 * is specified.<BR>
	 * When the TRANSFER_FILES job template attribute is not supported or does
	 * not contain the character 'i', the input file is always expected at the
	 * host where the job is executed irrespectively of a possibly hostname
	 * specified.<BR>
	 * The <CODE>$drmaa_incr_ph$</CODE> placeholder can be used at any position
	 * within the file path of parametric job templates and will be replaced
	 * by the underlying DRM system with the parametric job's index.<BR>
	 * A <CODE>$drmaa_hd_ph$</CODE> placeholder at the beginning of the file path
	 * denotes that the remaining portion of the file path is relative to the job
    * submiter's home directory on the host where the file is located.<BR>
	 * A <CODE>$drmaa_wd_ph$</CODE> placeholder at the beginning of the file path
	 * denotes that the remaining portion of the file path is relative to the
    * job's working directory on the host where the file is located.<BR>
	 * The file path is specified in a syntax that is common at the host
	 * where the file is located.<BR>
    * @return The job's standard input path or null if it has not been set
	 */
   public String getInputPath () {
      return inputPath;
   }
   
	/** Sets how to direct the job's standard output.
    * If not explicitly set in the job template, the whereabouts of the jobs
    * output stream is not defined.
    * If set, specifies the network path of the job's output stream file in the
    * form of [hostname]:file_path<BR>
    * When the TRANSFER_FILES job template attribute is supported and contains
    * the character 'o', the output file will be transferred by the underlying
    * DRM system to the specified host or to the submit host if no hostname is
    * specified.<BR>
    * When the TRANSFER_FILES job template attribute is not supported or does not
    * contain the character 'o', the output file is always kept at the host where
    * the job is executed irrespectively of a possibly hostname specified.<BR>
    * The <CODE>$drmaa_incr_ph$</CODE> placeholder can be used at any position
    * within the file path of parametric job templates and will be replaced
    * by the underlying DRM system with the parametric job's index.<BR>
    * A <CODE>$drmaa_hd_ph$</CODE> placeholder at the beginning of the file path
    * denotes that the remaining portion of the file path is relative to the job
    * submiter's home directory on the host where the file is located.<BR>
    * A <CODE>$drmaa_wd_ph$</CODE> placeholder at the beginning of the file path
    * denotes that the remaining portion of the file path is relative to the
    * job's working directory on the host where the file is located.<BR>
    * The file path must be specified in a syntax that is common at the host
    * where the file is located.<BR>
    * When the job is run, if this attribute is set, and the file can't be
    * written before execution the job will enter the state FAILED.
    * @param outputPath How to direct the job's standard output
    * @throws DRMAAException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeFormatException</LI>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setOutputPath (String outputPath) throws DRMAAException {
      this.checkPath (outputPath);
      this.outputPath = outputPath;
   }
   
	/** Gets how to direct the job's standard output.
	 * If set, specifies the network path of the job's output stream file in the
	 * form of [hostname]:file_path<BR>
	 * When the TRANSFER_FILES job template attribute is supported and contains
	 * the character 'o', the output file will be transferred by the underlying
	 * DRM system to the specified host or to the submit host if no hostname is
	 * specified.<BR>
	 * When the TRANSFER_FILES job template attribute is not supported or does not
	 * contain the character 'o', the output file is always kept at the host where
	 * the job is executed irrespectively of a possibly hostname specified.<BR>
	 * The <CODE>$drmaa_incr_ph$</CODE> placeholder can be used at any position
	 * within the file path of parametric job templates and will be replaced
	 * by the underlying DRM system with the parametric job's index.<BR>
	 * A <CODE>$drmaa_hd_ph$</CODE> placeholder at the beginning of the file path
	 * denotes that the remaining portion of the file path is relative to the job
    * submiter's home directory on the host where the file is located.<BR>
	 * A <CODE>$drmaa_wd_ph$</CODE> placeholder at the beginning of the file path
	 * denotes that the remaining portion of the file path is relative to the
    * job's working directory on the host where the file is located.<BR>
	 * The file path is specified in a syntax that is common at the host
	 * where the file is located.<BR>
    * @return How to direct the job's standard output or null if it has not been
    * set
	 */
   public String getOutputPath () {
      return outputPath;
   }
   
	/** Sets how to direct the job's standard error.
    * If not explicitly set in the job template, the whereabouts of the job's
    * error stream is not defined. If set, specifies the network path of the
    * job's error stream file in the form [hostname]:file_path<BR>
    * When the TRANSFER_FILES job template attribute is supported and contains
    * the character 'e', the output file will be transferred by the underlying
    * DRM system to the specified host or to the submit host if no hostname is
    * specified.<BR>
    * When the TRANSFER_FILES job template attribute is not supported or does not
    * contain the character 'e', the error file is always kept at the host where
    * the job is executed irrespectively of a possibly hostname specified.<BR>
    * The <CODE>$drmaa_incr_ph$</CODE> placeholder can be used at any position
    * within the file path of parametric job templates and will be replaced
    * by the underlying DRM system with the parametric job's index.<BR>
    * A <CODE>$drmaa_hd_ph$</CODE> placeholder at the beginning of the file path
    * denotes that the remaining portion of the file path is relative to the job
    * submiter's home directory on the host where the file is located.<BR>
    * A <CODE>$drmaa_wd_ph$</CODE> placeholder at the beginning of the file path
    * denotes that the remaining portion of the file path is relative to the
    * job's working directory on the host where the file is located.<BR>
    * The file path must be specified in a syntax that is common at the host
    * where the file is located.<BR>
    * When the job is run, if this attribute is set, and the file can't be
    * written before execution the job will enter the state FAILED.
    * @param errorPath How to direct the job's standard error
    * @throws DRMAAException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeFormatException</LI>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setErrorPath (String errorPath) throws DRMAAException {
      this.checkPath (errorPath);
      this.errorPath = errorPath;
   }
   
	/** Gets how to direct the job's standard error.
	 * If not explicitly set in the job template, the whereabouts of the job's
	 * error stream is not defined. If set, specifies the network path of the
	 * job's error stream file in the form [hostname]:file_path<BR>
	 * When the TRANSFER_FILES job template attribute is supported and contains
	 * the character 'e', the output file will be transferred by the underlying
	 * DRM system to the specified host or to the submit host if no hostname is
	 * specified.<BR>
	 * When the TRANSFER_FILES job template attribute is not supported or does not
	 * contain the character 'e', the error file is always kept at the host where
	 * the job is executed irrespectively of a possibly hostname specified.<BR>
	 * The <CODE>$drmaa_incr_ph$</CODE> placeholder can be used at any position
	 * within the file path of parametric job templates and will be replaced
	 * by the underlying DRM system with the parametric job's index.<BR>
	 * A <CODE>$drmaa_hd_ph$</CODE> placeholder at the beginning of the file path
	 * denotes that the remaining portion of the file path is relative to the job
    * submiter's home directory on the host where the file is located.<BR>
	 * A <CODE>$drmaa_wd_ph$</CODE> placeholder at the beginning of the file path
	 * denotes that the remaining portion of the file path is relative to the
    * job's working directory on the host where the file is located.<BR>
	 * The file path must be specified in a syntax that is common at the host
	 * where the file is located.<BR>
    * @return How to direct the job's standard error
	 */
   public String getErrorPath () {
      return errorPath;
   }
   
	/** Sets whether the error stream should be intermixed with the output
    * stream. If not explicitly set in the job template the attribute defaults
    * to false.<BR>
    * If true is specified the underlying DRM system will ignore the value of
    * the errorPath attribute and intermix the standard error stream with the
    * standard output stream as specified with outputPath.
    * @param join Whether the error stream should be intermixed with the output
    * stream
    * @throws DRMAAException Maybe be one of the following:
    * <UL>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setJoinFiles (boolean join) throws DRMAAException {
      this.join = join;
   }
   
	/** Gets whether the error stream should be intermixed with the output
	 * stream.
	 * If true is specified the underlying DRM system will ignore the value of
	 * the errorPath attribute and intermix the standard error stream with the
	 * standard output stream as specified with outputPath.
    * @return Whether the error stream should be intermixed with the output
	 * stream
	 */
   public boolean getJoinFiles () {
      return join;
   }
   
	/** Sets how to transfer files between hosts.
    * If not explicitly set in the job template the attribute defaults to
    * TRANSFER_NONE.  Any combination of TRANSFER_ERROR_FILES,
    * TRANSFER_INPUT_FILES and TRANSFER_OUTPUT_FILES can be specified by oring
    * them together.<BR>
    * TRANSFER_ERROR_FILES causes the errorPath attribute to specify the
    * location to which error files should be transfered after the job
    * finishes.<BR>
    * TRANSFER_INPUT_FILES causes the inputPath attribute to specify the
    * location from which input files should be transfered before the job
    * starts.<BR>
    * TRANSFER_OUTPUT_FILES causes the outputPath attribute to specify the
    * location to which output files should be transfered after the job
    * finishes.<BR>
    * @param transfer How to transfer files between hosts.  May be
    * TRANSFER_NONE, or a combination of TRANSFER_ERROR_FILES,
    * TRANSFER_INPUT_FILES and/or TRANSFER_OUTPUT_FILES ored together
    * @throws DRMAAException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setTransferFiles (byte transfer) throws DRMAAException {
      throw new UnsupportedAttributeException ("The transferFiles attribute is not supported.");
   }
   
	/** Gets how to transfer files between hosts.  May be
	 * any combination of TRANSFER_ERROR_FILES,
    * TRANSFER_INPUT_FILES and TRANSFER_OUTPUT_FILES ored together.<BR>
    * TRANSFER_ERROR_FILES causes the errorPath attribute to specify the
    * location to which error files should be transfered after the job
    * finishes.<BR>
	 * TRANSFER_INPUT_FILES causes the inputPath attribute to specify the
    * location from which input files should be transfered before the job
    * starts.<BR>
	 * TRANSFER_OUTPUT_FILES causes the outputPath attribute to specify the
    * location to which output files should be transfered after the job
    * finishes.<BR>
    * @return How to transfer files between hosts.  May be
    * TRANSFER_NONE, or a combination of TRANSFER_ERROR_FILES,
    * TRANSFER_INPUT_FILES and/or TRANSFER_OUTPUT_FILES ored together
	 */
   public byte getTransferFiles () {
      throw new UnsupportedAttributeException ("The transferFiles attribute is not supported.");
   }
   
	/** Sets a deadline after which the DRMS will terminate the job.
    * @param deadline The deadline after which the DRMS will terminate the job
    * @throws DRMAAException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setDeadlineTime (Date deadline) throws DRMAAException {
      throw new UnsupportedAttributeException ("The deadlineTime attribute is not supported.");
   }
   
	/** Sets a deadline after which the DRMS will terminate the job.
    * @return The deadline after which the DRMS will terminate the job
	 */
   public Date getDeadlineTime () {
      throw new UnsupportedAttributeException ("The deadlineTime attribute is not supported.");
   }
   
	/** Sets when the job's wall clock time limit has
    * been exceeded.  The DRMS will terminate a job that has exceeded its wall
    * clock time limit.  Note that time spent suspended is also accounted for
    * here.<BR>
    * @param hardWallclockLimit When the job's wall clock time limit has been
    * exceeded.  Specified in seconds
    * @throws DRMAAException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setHardWallclockTimeLimit (long hardWallclockLimit) throws DRMAAException {
      throw new UnsupportedAttributeException ("The hardWallclockTimeLimit attribute is not supported.");
   }
   
	/** Gets the duration of the job's wall clock time limit.  The DRMS will
    * terminate a job that has exceeded its wall
	 * clock time limit.  Note that time spent suspended is also accounted for
    * here.<BR>
    * @return When the job's wall clock time limit has been exceeded.
    * Specified in seconds
	 */
   public long getHardWallclockTimeLimit () {
      throw new UnsupportedAttributeException ("The hardWallclockTimeLimit attribute is not supported.");
   }
   
	/** Sets an estimate as to how much wall clock time job will need to
    * complete. Note that time spent suspended is also accounted for here.<BR>
    * This attribute is intended to assist the scheduler.
    * If the time specified in insufficient, the
    * drmaa-implementation may impose a scheduling penalty.
    * @param softWallclockLimit An estimate as to how much wall clock time job
    * will need to complete.  Specified in seconds
    * @throws DRMAAException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setSoftWallClockTimeLimit (long softWallclockLimit) throws DRMAAException {
      throw new UnsupportedAttributeException ("The softWallclockTimeLimit attribute is not supported.");
   }
   
	/** Gets an estimate as to how much wall clock time job will need to
    * complete. Note that time spent suspended is also accounted for here.<BR>
	 * This attribute is intended to assist the scheduler.
	 * If the time specified in insufficient, the
	 * drmaa-implementation may impose a scheduling penalty.
    * @return An estimate as to how much wall clock time job will need
    * to complete.  Specified in seconds
	 */
   public long getSoftWallClockTimeLimit () {
      throw new UnsupportedAttributeException ("The softWallclockTimeLimit attribute is not supported.");
   }
   
	/** Sets how long the job may be in a running state before its limit has been
    * exceeded, and therefore is terminated by the DRMS.
    * @param hardRunLimit How long the job may be in a running state before its
    * limit has been exceeded.  Specified in seconds
    * @throws DRMAAException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setHardRunDurationLimit (long hardRunLimit) throws DRMAAException {
      throw new UnsupportedAttributeException ("The hardRunDurationLimit attribute is not supported.");
   }
   
	/** Gets how long the job may be in a running state before its limit has been
	 * exceeded, and therefore is terminated by the DRMS.
    * @return How long the job may be in a running state before its limit has
    * been exceeded.  Specified in seconds
	 */
   public long getHardRunDurationLimit () {
      throw new UnsupportedAttributeException ("The hardRunDurationLimit attribute is not supported.");
   }
   
	/** Sets an estimate as to how long the job will need to remain in a running
    * state to complete.  This attribute is intended to assist the scheduler. If
    * the time specified in insufficient, the DRMAA implementation may impose a
    * scheduling penalty.<BR>
    * @param softRunLimit An estimate as to how long the job will need to remain
    * in a running state to complete.  Specified in seconds
    * @throws DRMAAException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setSoftRunDurationLimit (long softRunLimit) throws DRMAAException {
      throw new UnsupportedAttributeException ("The softRunDurationLimit attribute is not supported.");
   }

	/** Gets an estimate as to how long the job will need to remain in a running
    * state to complete.  This attribute is intended to assist the scheduler. If
    * the time specified in insufficient, the DRMAA implementation may impose a
    * scheduling penalty.<BR>
    * @return An estimate as to how long the job will need to remain
    * in a running state to complete.  Specified in seconds
	 */
   public long getSoftRunDurationLimit () {
      throw new UnsupportedAttributeException ("The softRunDurationLimit attribute is not supported.");
   }
	
	/** Creates a new instance of JobTemplate */
	protected JobTemplate () {
	}
	
   /** SHALL return the list of supported attribute names.  This list SHALL
    * include supported DRMAA reserved attribute names and native attribute
    * names.
	 * @return the list of supported attribute names
	 */	
	public List getAttributeNames () {
      ArrayList allNames = null;
      List requiredNames = Arrays.asList (attributeNames);
      List optionalNames = this.getOptionalAttributeNames ();
      
      allNames = new ArrayList (requiredNames.size () + optionalNames.size ());
      allNames.addAll (requiredNames);
      allNames.addAll (optionalNames);
      
      return allNames;
   }
   
   /** This method returns the names of all optional and implementation-specific
    * attributes supported by this DRMAA implementation.  Unless overridden by the
    * DRMAA implementation, this method returns an empty list.
    * @return The names of all optional and implementation-specific
    * attributes supported by this DRMAA implementation
    */   
   protected List getOptionalAttributeNames () {
      return Collections.EMPTY_LIST;
   }
   
   /** Deallocate a job template. This routine has no effect on jobs.
    * @throws DRMAAException May be one of the following:
    * <UL>
    * <LI>DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE</LI>
    * </UL>
    */
   public abstract void delete () throws DRMAAException;

   /** Checks for a valid path.  Throws an IllegalArgumentException is the path
    * is not valid.
    * @param path The path to validate
    */
   private void checkPath (String path) {
      if (path.indexOf (HOME_DIRECTORY) > 0) {
         throw new IllegalArgumentException ("$drmaa_hd_ph$ may only appear at the beginning of the path.");
      }
      
      if (path.indexOf (WORKING_DIRECTORY) > 0) {
         throw new IllegalArgumentException ("$drmaa_wd_ph$ may only appear at the beginning of the path.");
      }
   }
}
