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
/*___INFO__MARK_END__*/
package org.ggf.drmaa;

import java.util.*;

/** <p>This class represents a template to be used for the creation of a job.  The
 * properties set on a JobTemplate object as used to create a new job and set up
 * the new job's environment.</p>
 * <p>There is a 1:n relationship between JobTemplates and jobs.  A single JobTemplate
 * can be used to submit any number of jobs.  Once a job has been submitted, e.g.
 * via Session.runJob(), the JobTemplate no longer has any affect on the job.
 * Changes made to the JobTemplate will have no affect on already running jobs.
 * Deleting the JobTemplate (via Session.deleteJobTemplate()) also has no effect on
 * running jobs.</p
 * <p>Once a JobTemplate has been created (via Session.createJobTemplate()), it
 * is the responsibility of the developer to delete it when no longer needed
 * (via Session.deleteJobTemplate ()).  Failure to do so may result in a memory
 * leak.</p>
 * <p>Example:</p>
 * <pre>public static void main (String[] args) {
 *   SessionFactory factory = SessionFactory.getFactory ();
 *   Session session = factory.getSession ();
 *
 *   try {
 *      session.init (null);
 *      JobTemplate jt = session.createJobTemplate ();
 *      jt.setRemoteCommand ("sleeper.sh");
 *      jt.setWorkingDirectory (HOME_DIRECTORY + "/jobs");
 *      jt.setArgs (new String[] {"5"});
 *
 *      String id = session.runJob (jt);
 *
 *      session.deleteJobTemplate (jt);
 *      session.exit ();
 *   }
 *   catch (DrmaaException e) {
 *      System.out.println ("Error: " + e.getMessage ());
 *   }
 * }
 * </pre>
 * @author dan.templeton@sun.com
 * @see Session
 * @since 0.4.2
 */
public class JobTemplate {
   /** The names of all the required properties */   
   private static final String[] attributeNames = new String[] {
      "args",
      "blockEmail",
      "email",
      "errorPath",
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

	/** jobSubmissionState which means the job has been queued but it is not
    * eligible to run
    */	
	public static final int HOLD = 0;
	/** jobSumissionState which means the job has been queued and is eligible to run */	
	public static final int ACTIVE = 1;
   /** Placeholder which represents the home directory in the workingDirectory, inputPath,
    * outputPath, and errorPath properties
    */   
   public static final String HOME_DIRECTORY = "$drmaa_hd_ph$";
   /** Placeholder which represents the working directory in the workingDirectory,
    * inputPath, outputPath, and errorPath properties
    */   
   public static final String WORKING_DIRECTORY = "$drmaa_wd_ph$";
   /** Placeholder which represents the task id for a task in a parametric job in the
    * workingDirectory, inputPath, outputPath, and errorPath properties
    */   
   public static final String PARAMETRIC_INDEX = "$drmaa_incr_ph$";

   /** Remote command to execute */   
   protected String remoteCommand = null;
   /** Input parameters passed as arguments to the job */   
   protected String[] args = null;
   /** Job state at submission, either HOLD or ACTIVE */   
   protected int state = ACTIVE;
   /* I used a Properties here instead of a Map because Properties are always
    * strings, and Properties can have a set of parent Properties to whose value
    * unset attributes default. */
   /** The environment values that define the job's remote environment */   
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
   /** The earliest time when the job may be eligible to be run */   
   protected PartialTimestamp startTime = null;
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

   /* Create a new instance of a JobTemplate. */
   public JobTemplate () {
   }
   
   /** Set the command string to execute as the job.  The command
    * is relative to the execution host and is evaluated on the
    * execution host.  No binary file management is done.
    * @param remoteCommand The command to execute as the job
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeFormatException</LI>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setRemoteCommand (String remoteCommand) throws DrmaaException {
      this.remoteCommand = remoteCommand;
   }
   
   /** Get the command string to execute as the job.  The command
    * is relative to the execution host and is evaluated on the
    * execution host.  No binary file management is done.
    * @return The command to execute as the job or null if it has not been set
    */
   public String getRemoteCommand () {
      return remoteCommand;
   }
   
	/** Set the parameters passed as arguments to the job.
    * @param args The parameters passed as arguments to the job
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeFormatException</LI>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    *
    */
   public void setArgs (String[] args) throws DrmaaException {
      if (args != null) {
         this.args = new String[args.length];
         System.arraycopy (args, 0, this.args, 0, args.length);
      }
      else {
         this.args = null;
      }
   }
   
	/** Get The parameters passed as arguments to the job.
	 * @return The parameters passed as arguments to the job or null if they have
    * not been set
	 */	
   public String[] getArgs () {
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
    * HOLD means job has been queued, but it is not eligible to run
    * @param state The job state at submission
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */	
   public void setJobSubmissionState (int state) throws DrmaaException {
      if ((state != ACTIVE) && (state != HOLD)) {
         throw new IllegalArgumentException ("Invalid state");
      }
      
      this.state = state;
   }
   
	/** Get the job state at submission.  The states are HOLD and ACTIVE:<BR>
    * ACTIVE means job has been queued, and is eligible to run
    * HOLD means job has been queued, but it is not eligible to run
    * @return The job state at submission
    */	
   public int getJobSubmissionState () {
      return state;
   }
   
	/** Set the environment values that define the remote environment.
    * The values override the remote environment values if there is a collision.
    * @param env The environment values that define the remote environment
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeFormatException</LI>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setJobEnvironment (Properties env) throws DrmaaException {
      if (env != null) {
         this.env = (Properties)env.clone ();
      }
      else {
         this.env = null;
      }
   }
   
	/** Get the environment values that define the remote environment.
    * The values override the remote environment values if there is a collision.
    * @return The environment values that define the remote environment or null
    * if it has not been set
    */
   public Properties getJobEnvironment () {
      if (env != null) {
         return (Properties)env.clone ();
      }
      else {
         return null;
      }
   }

   /** Set the directory where the job is executed.  If the working directory is
    * not set, behavior is implementation dependent.  The working directory is
    * evaluated relative to the execution host.<BR>
    * A <CODE>HOME_DIRECTORY</CODE> placeholder at the beginning denotes that the
    * remaining portion of the directory name is resolved
    * relative to the job submiter's home directory on the execution host.<BR>
    * The <CODE>PARAMETRIC_INDEX</CODE> placeholder can be used at any position
    * within the directory name of parametric jobs and will be replaced
    * by the underlying DRM system with the parametric jobs' index.<BR>
    * The directory name must be specified in a syntax that is common at the
    * host where the job will be executed.<BR>
    * If no placeholder is used, an absolute directory specification
    * is expected.<BR>
    * If the directory does not exist when the job is run, the job enters the
    * state FAILED.
    * @param wd The directory where the job is executed
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeFormatException</LI>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setWorkingDirectory (String wd) throws DrmaaException {
      if (wd.indexOf (HOME_DIRECTORY) > 0) {
         throw new InvalidAttributeFormatException ("$drmaa_hd_ph$ may only appear at the beginning of the path.");
      }
      
      this.wd = wd;
   }
   
   /** Get the directory where the job is executed.  The working directory is
    * evaluated relative to the execution host.<BR>
    * A <CODE>HOME_DIRECTORY</CODE> placeholder at the beginning denotes that the
    * remaining portion of the directory name is resolved
    * relative to the job submiter's home directory on the execution host.<BR>
    * The <CODE>PARAMETRIC_INDEX</CODE> placeholder can be used at any position
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
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeFormatException</LI>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    *
    */
   public void setJobCategory (String category) throws DrmaaException {
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
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeFormatException</LI>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setNativeSpecification (String spec) throws DrmaaException {
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
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeFormatException</LI>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */	
   public void setEmail (String[] email) throws DrmaaException {
      if (email != null) {
         this.email = new String[email.length];
         System.arraycopy (email, 0, this.email, 0, email.length);
      }
      else {
         this.email = null;
      }
   }
   
	/** Get the list of email addresses used to report the job completion and
    * status.
    * @return The list of email addresses used to report the job completion
    * and status or null if they have not been set
    */	
   public String[] getEmail () {
      if (email != null) {
         String[] returnValue = new String[email.length];

         System.arraycopy (email, 0, returnValue, 0, email.length);

         return returnValue;
      }
      else {
         return null;
      }
   }
   
	/** Set whether to block sending e-mail by default, regardless of the DRMS
    * settings.  This property can only be used to prevent email from being sent.
    * It cannot force the DRM to send email.
    * @param blockEmail Whether to block sending e-mail by default
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setBlockEmail (boolean blockEmail) throws DrmaaException {
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
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setStartTime (PartialTimestamp startTime) throws DrmaaException {
      if (startTime != null) {
         if (startTime.getTimeInMillis () < System.currentTimeMillis ()) {
            throw new IllegalArgumentException ("Start time is in the past.");
         }

         this.startTime = startTime;
      }
      else {
         startTime = null;
      }
   }
   
	/** Get the earliest time when the job may be eligible to be run.
    * @return The earliest time when the job may be eligible to be run or null
    * if it has not been set
	 */
   public PartialTimestamp getStartTime () {
      if (startTime != null) {
         return (PartialTimestamp)startTime.clone ();
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
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeFormatException</LI>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setJobName (String name) throws DrmaaException {
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
    * When the transferFiles property is supported and has it's inputStream property
    * set, the input file will be fetched by the underlying DRM
    * system from the specified host or from the submit host if no hostname
    * is specified.<BR>
    * When the transferFiles property is not supported or does
    * not have it's inputStream property set, the input file is always expected to be
    * at the host where the job is executed irrespective of a whether a hostname
    * is specified in the path.<BR>
    * The <CODE>PARAMETRIC_INDEX</CODE> placeholder can be used at any position
    * within the file path of parametric job templates and will be replaced
    * by the underlying DRM system with the parametric job's index.<BR>
    * A <CODE>HOME_DIRECTORY</CODE> placeholder at the beginning of the file path
    * denotes that the remaining portion of the file path is relative to the job
    * submiter's home directory on the host where the file is located.<BR>
    * A <CODE>WORKING_DIRECTORY</CODE> placeholder at the beginning of the file path
    * denotes that the remaining portion of the file path is relative to the
    * job's working directory on the host where the file is located.<BR>
    * The file path must be specified in a syntax that is common at the host
    * where the file is located.<BR>
    * When the job is run, if this property is set, and the file can't be read,
    * the job will enter the state FAILED.
    * @param inputPath The job's standard input path
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeFormatException</LI>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setInputPath (String inputPath) throws DrmaaException {
      this.checkPath (inputPath);
      this.inputPath = inputPath;
   }
   
	/** Get the job's standard input path.
    * Specifies the network path of the job's input stream in
    * the form of [hostname]:file_path<BR>
    *  When the transferFiles property is supported and has it's inputStream property
    * set, the input file will be fetched by the underlying DRM
    * system from the specified host or from the submit host if no hostname
    * is specified.<BR>
    * When the transferFiles property is not supported or does
    * not have it's inputStream property set, the input file is always expected to be
    * at the host where the job is executed irrespective of a whether a hostname
    * is specified in the path.<BR>
    * The <CODE>PARAMETRIC_INDEX</CODE> placeholder can be used at any position
    * within the file path of parametric job templates and will be replaced
    * by the underlying DRM system with the parametric job's index.<BR>
    * A <CODE>HOME_DIRECTORY</CODE> placeholder at the beginning of the file path
    * denotes that the remaining portion of the file path is relative to the job
    * submiter's home directory on the host where the file is located.<BR>
    * A <CODE>WORKING_DIRECTORY</CODE> placeholder at the beginning of the file path
    * denotes that the remaining portion of the file path is relative to the
    * job's working directory on the host where the file is located.<BR>
    * The file path must be specified in a syntax that is common at the host
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
    *  When the transferFiles property is supported and has it's outputStream property
    * set, the output file will be transferred by the underlying
    * DRM system to the specified host or to the submit host if no hostname is
    * specified.<BR>
    * When the transferFiles property is not supported or does
    * not have it's outputStream property set, the output file is always kept
    * at the host where the job is executed irrespective of a whether a hostname
    * is specified in the path.<BR>
    * The <CODE>PARAMETRIC_INDEX</CODE> placeholder can be used at any position
    * within the file path of parametric job templates and will be replaced
    * by the underlying DRM system with the parametric job's index.<BR>
    * A <CODE>HOME_DIRECTORY</CODE> placeholder at the beginning of the file path
    * denotes that the remaining portion of the file path is relative to the job
    * submiter's home directory on the host where the file is located.<BR>
    * A <CODE>WORKING_DIRECTORY</CODE> placeholder at the beginning of the file path
    * denotes that the remaining portion of the file path is relative to the
    * job's working directory on the host where the file is located.<BR>
    * The file path must be specified in a syntax that is common at the host
    * where the file is located.<BR>
    * When the job is run, if this property is set, and the file can't be
    * written before execution the job will enter the state FAILED.
    * @param outputPath How to direct the job's standard output
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeFormatException</LI>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setOutputPath (String outputPath) throws DrmaaException {
      this.checkPath (outputPath);
      this.outputPath = outputPath;
   }
   
	/** Gets how to direct the job's standard output.
    * If set, specifies the network path of the job's output stream file in the
    * form of [hostname]:file_path<BR>
    *  When the transferFiles property is supported and has it's outputStream property
    * set, the output file will be transferred by the underlying
    * DRM system to the specified host or to the submit host if no hostname is
    * specified.<BR>
    * When the transferFiles property is not supported or does
    * not have it's outputStream property set, the output file is always kept
    * at the host where the job is executed irrespective of a whether a hostname
    * is specified in the path.<BR>
    * The <CODE>PARAMETRIC_INDEX</CODE> placeholder can be used at any position
    * within the file path of parametric job templates and will be replaced
    * by the underlying DRM system with the parametric job's index.<BR>
    * A <CODE>HOME_DIRECTORY</CODE> placeholder at the beginning of the file path
    * denotes that the remaining portion of the file path is relative to the job
    * submiter's home directory on the host where the file is located.<BR>
    * A <CODE>WORKING_DIRECTORY</CODE> placeholder at the beginning of the file path
    * denotes that the remaining portion of the file path is relative to the
    * job's working directory on the host where the file is located.<BR>
    * The file path must be specified in a syntax that is common at the host
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
    * When the transferFiles property is supported and has it's errorStream property
    * set, the error file will be transferred by the underlying
    * DRM system to the specified host or to the submit host if no hostname is
    * specified.<BR>
    * When the transferFiles property is not supported or does
    * not have it's errorStream property set, the error file is always kept
    * at the host where the job is executed irrespective of a whether a hostname
    * is specified in the path.<BR>
    * The <CODE>PARAMETRIC_INDEX</CODE> placeholder can be used at any position
    * within the file path of parametric job templates and will be replaced
    * by the underlying DRM system with the parametric job's index.<BR>
    * A <CODE>HOME_DIRECTORY</CODE> placeholder at the beginning of the file path
    * denotes that the remaining portion of the file path is relative to the job
    * submiter's home directory on the host where the file is located.<BR>
    * A <CODE>WORKING_DIRECTORY</CODE> placeholder at the beginning of the file path
    * denotes that the remaining portion of the file path is relative to the
    * job's working directory on the host where the file is located.<BR>
    * The file path must be specified in a syntax that is common at the host
    * where the file is located.<BR>
    * When the job is run, if this property is set, and the file can't be
    * written before execution the job will enter the state FAILED.
    * @param errorPath How to direct the job's standard error
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeFormatException</LI>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setErrorPath (String errorPath) throws DrmaaException {
      this.checkPath (errorPath);
      this.errorPath = errorPath;
   }
   
	/** Gets how to direct the job's standard error.
    * If not explicitly set in the job template, the whereabouts of the job's
    * error stream is not defined. If set, specifies the network path of the
    * job's error stream file in the form [hostname]:file_path<BR>
    * When the transferFiles property is supported and has it's errorStream property
    * set, the error file will be transferred by the underlying
    * DRM system to the specified host or to the submit host if no hostname is
    * specified.<BR>
    * When the transferFiles property is not supported or does
    * not have it's errorStream property set, the error file is always kept
    * at the host where the job is executed irrespective of a whether a hostname
    * is specified in the path.<BR>
    * The <CODE>PARAMETRIC_INDEX</CODE> placeholder can be used at any position
    * within the file path of parametric job templates and will be replaced
    * by the underlying DRM system with the parametric job's index.<BR>
    * A <CODE>HOME_DIRECTORY</CODE> placeholder at the beginning of the file path
    * denotes that the remaining portion of the file path is relative to the job
    * submiter's home directory on the host where the file is located.<BR>
    * A <CODE>WORKING_DIRECTORY</CODE> placeholder at the beginning of the file path
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
    * stream. If not explicitly set in the job template this property defaults
    * to <CODE>false</CODE>.<BR>
    * If <CODE>true</CODE> is specified the underlying DRM system will ignore the value of
    * the errorPath attribute and intermix the standard error stream with the
    * standard output stream as specified with outputPath.
    * @param join Whether the error stream should be intermixed with the output
    * stream
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setJoinFiles (boolean join) throws DrmaaException {
      this.join = join;
   }
   
	/** Gets whether the error stream should be intermixed with the output
    * stream.
    * If <CODE>true</CODE> is specified the underlying DRM system will ignore the value of
    * the errorPath attribute and intermix the standard error stream with the
    * standard output stream as specified with outputPath.
    * @return Whether the error stream should be intermixed with the output
    * stream
    */
   public boolean getJoinFiles () {
      return join;
   }
   
	/** Sets how to transfer files between hosts.  The FileTransferMode's properties
    * will be used to determine which files to transfer.<BR>
    * If the FileTransferMode object's errorStream property is set to
    * <CODE>true</CODE>, the errorPath attribute is taken to specify the
    * location to which error files should be transfered after the job
    * finishes.<BR>
    * If the FileTransferMode object's inputStream property is set to
    * <CODE>true</CODE>, the inputPath attribute is taken to specify the
    * location from which input files should be transfered before the job
    * starts.<BR>
    * If the FileTransferMode object's outputStream property is set to
    * <CODE>true</CODE>, the outputPath attribute is taken to specify the
    * location to which output files should be transfered after the job
    * finishes.<BR>
    * @param mode How to transfer files between hosts.
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setTransferFiles (FileTransferMode mode) throws DrmaaException {
      throw new UnsupportedAttributeException ("The transferFiles attribute is not supported.");
   }
   
	/** Gets how to transfer files between hosts.
    * If the FileTransferMode object's errorStream property is set to
    * <CODE>true</CODE>, the errorPath attribute is taken to specify the
    * location to which error files should be transfered after the job
    * finishes.<BR>
    * If the FileTransferMode object's inputStream property is set to
    * <CODE>true</CODE>, the inputPath attribute is taken to specify the
    * location from which input files should be transfered before the job
    * starts.<BR>
    * If the FileTransferMode object's outputStream property is set to
    * <CODE>true</CODE>, the outputPath attribute is taken to specify the
    * location to which output files should be transfered after the job
    * finishes.<BR>
    * @return How to transfer files between hosts.
    */
   public FileTransferMode getTransferFiles () {
      throw new UnsupportedAttributeException ("The transferFiles attribute is not supported.");
   }
   
	/** Sets a deadline after which the DRMS will terminate the job.
    * @param deadline The deadline after which the DRMS will terminate the job
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setDeadlineTime (PartialTimestamp deadline) throws DrmaaException {
      throw new UnsupportedAttributeException ("The deadlineTime attribute is not supported.");
   }
   
	/** Sets a deadline after which the DRMS will terminate the job.
    * @return The deadline after which the DRMS will terminate the job
	 */
   public PartialTimestamp getDeadlineTime () {
      throw new UnsupportedAttributeException ("The deadlineTime attribute is not supported.");
   }
   
	/** Sets when the job's wall clock time limit has
    * been exceeded.  The DRMS will terminate a job that has exceeded its wall
    * clock time limit.  Note that time spent suspended is also accounted for
    * here.<BR>
    * @param hardWallclockLimit When the job's wall clock time limit has been
    * exceeded.  Specified in seconds
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setHardWallclockTimeLimit (long hardWallclockLimit) throws DrmaaException {
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
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setSoftWallclockTimeLimit (long softWallclockLimit) throws DrmaaException {
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
   public long getSoftWallclockTimeLimit () {
      throw new UnsupportedAttributeException ("The softWallclockTimeLimit attribute is not supported.");
   }
   
	/** Sets how long the job may be in a running state before its limit has been
    * exceeded, and therefore is terminated by the DRMS.
    * @param hardRunLimit How long the job may be in a running state before its
    * limit has been exceeded.  Specified in seconds
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setHardRunDurationLimit (long hardRunLimit) throws DrmaaException {
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
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setSoftRunDurationLimit (long softRunLimit) throws DrmaaException {
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
	
   /** Returns the list of supported property names.  This list
    * includes supported DRMAA reserved property names (both required and optional)
    * and DRM-specific property names.
    * @return the list of supported property names
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
    * properties supported by this DRMAA implementation.  Unless overridden by the
    * DRMAA implementation, this method returns an empty list.
    * This method is used by the getAttributeNames() method to construct the full list
    * of implementation-supported property names.
    * @return The names of all optional and implementation-specific
    * properties supported by this DRMAA implementation
    * @see #getAttributeNames
    */   
   protected List getOptionalAttributeNames () {
      return Collections.EMPTY_LIST;
   }
   
   /** Checks for a valid path.  Throws an IllegalArgumentException is the path
    * is not valid.
    * @param path The path to validate
    */
   private void checkPath (String path) {
      /* On a null path, we just return because null paths are OK. */
      if (path == null) {
         return;
      }
      
      if (path.indexOf (HOME_DIRECTORY) > 0) {
         throw new IllegalArgumentException ("$drmaa_hd_ph$ may only appear at the beginning of the path.");
      }
      
      if (path.indexOf (WORKING_DIRECTORY) > 0) {
         throw new IllegalArgumentException ("$drmaa_wd_ph$ may only appear at the beginning of the path.");
      }
   }

   /** Converts this JobTemplate into a String which contains all property
    *  settings.
    * @return a string containing all property settings
    */
   public String toString () {
      StringBuffer out = new StringBuffer ();
      boolean firstProperty = true;
      
      if (getArgs () != null) {
         String[] args = getArgs ();
         boolean firstArg = true;
         
         firstProperty = false;
         out.append ("{args = ");
         
         for (int count = 0; count < args.length; count++) {
            if (firstArg) {
               firstArg = false;
            }
            else {
               out.append (", ");
            }
            
            out.append ("\"");
            out.append (args[count]);
            out.append ("\"");
         }
         
         out.append ("}");
      }
      
      if (firstProperty) {
         firstProperty = false;
      }
      else {
         out.append (" ");
      }

      out.append ("{blockEmail = ");
      out.append (Boolean.toString (getBlockEmail ()));
      out.append ("}");
      
      try {
         if (getDeadlineTime () != null) {
            out.append (" {deadlineTime = \"");
            out.append (new PartialTimestampFormat ().format (getDeadlineTime ()));
            out.append ("\"}");
         }
      }
      catch (UnsupportedAttributeException e) {
         /* Skip it. */
      }
      
      if (getEmail() != null) {
         String[] email = getEmail ();
         boolean firstEmail = true;
         
         out.append (" {email = ");
         
         for (int count = 0; count < email.length; count++) {
            if (firstEmail) {
               firstEmail = false;
            }
            else {
               out.append (", ");
            }
            
            out.append ("\"");
            out.append (email[count]);
            out.append ("\"");
         }
         
         out.append ("}");
      }
      
      if (getErrorPath () != null) {
         out.append (" {errorPath = ");
         out.append (getErrorPath ());
         out.append ("}");
      }
      
      try {
         if (getHardRunDurationLimit () != 0L) {
            out.append (" {hardRunDurationLimit = ");
            out.append (getHardRunDurationLimit ());
            out.append ("ms}");
         }
      }
      catch (UnsupportedAttributeException e) {
         /* Skip it. */
      }
      
      try {
         if (getHardWallclockTimeLimit () != 0L) {
            out.append (" {hardWallclockTimeLimit = ");
            out.append (getHardWallclockTimeLimit ());
            out.append ("ms}");
         }
      }
      catch (UnsupportedAttributeException e) {
         /* Skip it. */
      }
      
      if (getInputPath () != null) {
         out.append (" {inputPath = ");
         out.append (getInputPath ());
         out.append ("}");
      }
      
      if (getJobCategory () != null) {
         out.append (" {jobCategory = ");
         out.append (getJobCategory ());
         out.append ("}");
      }
      
      if (getJobEnvironment () != null) {
         Properties env = getJobEnvironment ();
         Iterator i = env.keySet ().iterator ();
         boolean firstEnv = true;
         
         out.append (" {jobEnvironment = ");
         
         while (i.hasNext ()) {
            String entry = (String)i.next ();
            if (firstEnv) {
               firstEnv = false;
            }
            else {
               out.append (", ");
            }
            
            out.append ("[\"");
            out.append (entry);
            out.append ("\" = \"");
            out.append (env.getProperty (entry));
            out.append ("\"]");
         }
         
         out.append ("}");
      }
      
      if (getJobName () != null) {
         out.append (" {jobName = ");
         out.append (getJobName ());
         out.append ("}");
      }
      
      out.append (" {jobSubmissionState = ");
      if (getJobSubmissionState () == HOLD) {
         out.append ("HOLD}");
      }
      else {
         out.append ("ACTIVE}");
      }
      
      out.append (" {joinFiles = ");
      out.append (Boolean.toString (getJoinFiles ()));
      out.append ("}");
      
      if (getNativeSpecification () != null) {
         out.append (" {nativeSpecification = \"");
         out.append (getNativeSpecification ());
         out.append ("\"}");
      }
      
      if (getOutputPath () != null) {
         out.append (" {outputPath = ");
         out.append (getOutputPath ());
         out.append ("}");
      }
      
      if (getRemoteCommand () != null) {
         out.append (" {remoteCommand = ");
         out.append (getRemoteCommand ());
         out.append ("}");
      }
      
      try {
         if (getSoftRunDurationLimit () != 0L) {
            out.append (" {softRunDurationLimit = ");
            out.append (getSoftRunDurationLimit ());
            out.append ("ms}");
         }
      }
      catch (UnsupportedAttributeException e) {
         /* Skip it. */
      }
      
      try {
         if (getSoftWallclockTimeLimit () != 0L) {
            out.append (" {softWallclockTimeLimit = ");
            out.append (getSoftWallclockTimeLimit ());
            out.append ("ms}");
         }
      }
      catch (UnsupportedAttributeException e) {
         /* Skip it. */
      }
      
      if (getStartTime () != null) {
         out.append (" {startTime = \"");
         out.append (new PartialTimestampFormat ().format (getStartTime ()));
         out.append ("\"}");
      }
      
      try {
         if (getTransferFiles () != null) {
            out.append (" {transferFiles = \"");
            out.append (getTransferFiles ().toString ());
            out.append ("\"}");
         }
      }
      catch (UnsupportedAttributeException e) {
         /* Skip it. */
      }
      
      if (getWorkingDirectory () != null) {
         out.append (" {workingDirectory = ");
         out.append (getWorkingDirectory ());
         out.append ("}");
      }
      
      return out.toString ();
   }
   
   /** Tests whether this JobTemplate has the same property values as the
    *  given object.
    * @param obj the object against which to compare
    * @return whether the the given object has the same properties as this
    * object
    */
   public boolean equals (Object obj) {
      if (obj instanceof JobTemplate) {
         return this.toString ().equals (obj.toString ());
      }
      else {
         return false;
      }
   }
   
   /** Returns a hash code based on the properties set in this
    *  JobTemplateImpl.
    * @return the has code
    */
   public int hashCode () {
      return this.toString ().hashCode ();
   }
}
