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
package com.sun.grid.drmaa;

import java.text.NumberFormat;
import java.util.*;
import java.util.regex.*;

import org.ggf.drmaa.*;

/** <p>This class represents a remote job and its attributes.  It is used to
 * set up the environment for a job to be submitted.</p>
 * <h3>DRMMA Attributes</h3>
 * <p>DRMAA job template attributes can be set from six different sources.  In
 * order of precedence, from lowest to highest, there are: options
 * set by DRMAA automatically by default, options set in the sge_request file,
 * options set in the script file, options set by the jobCategory
 * property, options set by the nativeSpecification property, and
 * options set through other DRMAA attributes.</p>
 * <p>By default DRMAA sets four options for all jobs.  They are &quot;-w e&quot;,
 * &quot;-p 0&quot;, &quot;-b yes&quot;, and &quot;-shell no&quot;.  This means
 * that by default, unschedulable jobs cannot be submitted, all jobs will have
 * priority 0, all jobs will be treated as
 * binary, i.e. no scripts args will be parsed, and all jobs will be executed
 * without a wrapper shell.</p>
 * The sge_request file, found in the $SGE_ROOT/$SGE_CELL/common directory, may
 * contain options to be applied to all jobs.  See the sge_request(5) man page
 * for more information.</p>
 * <p>If the sge_request file contains &quot;-b no&quot; or if the
 * nativeSpecification attribute is set and contains &quot;-b no&quot;, the
 * script file will be parsed for in-line arguments. Otherwise, no scripts args
 * will be interpreted.  See the qsub(1) man page for more information.</p>
 * <p>If the jobCategory attribute is set, and the category it points to
 * exists in one of the qtask files, the options associated with that category
 * will be applied to the job template.  See the qtask(5) man page and the
 * jobCategory attribute accessors below for more information.</p>
 * <p>If the nativeSpecification attribute is set, all options contained therein
 * will be applied to the job template.  See the nativeSpecification accessors
 * below for more information.</p>
 * <p>Other DRMAA attributes will override any previous settings.  For example,
 * if the sge_request file contains &quot;-j y&quot;, but the joinFiles
 * attribute is set to <i>false</i>, the ultimate result is that the input and
 * output files will remain separate.</p>
 * <h3>Attribute Correlations</h3>
 * <p>The following DRMAA attributes correspond to the following qsub
 * options:</p>
 * <table>
 *  <tr><th>DRMAA Attribute</th><th>qsub Option</th></tr>
 *  <tr><td>remoteCommand</td><td>script file</td>
 *  <tr><td>args</td><td>script file arguments</td>
 *  <tr><td>jobSubmissionState = HOLD</td><td>-h</td>
 *  <tr><td>jobEnvironment</td><td>-v</td>
 *  <tr><td>workingDirectory</td><td>NONE</td>
 *  <tr><td>jobCategory</td><td>NONE</td>
 *  <tr><td>nativeSpecification</td><td>NONE</td>
 *  <tr><td>emailAddresses</td><td>-M</td>
 *  <tr><td>blockEmail = true</td><td>-m n</td>
 *  <tr><td>startTime</td><td>-a</td>
 *  <tr><td>jobName</td><td>-N</td>
 *  <tr><td>inputPath</td><td>-i</td>
 *  <tr><td>outputPath</td><td>-o</td>
 *  <tr><td>errorPath</td><td>-e</td>
 *  <tr><td>joinFiles</td><td>-j</td>
 *  <tr><td>transferFiles</td><td>NONE</td>
 * </table>
 * <p>The following attributes are unsupported by Grid Engine:</p>
 * <ul>
 * <li>deadlineTime</li>
 * <li>hardWallclockTimeLimit</li>
 * <li>softWallclockTimeLimit</li>
 * <li>hardRunDurationTimeLimit</li>
 * <li>softRunDurationTimeLimit</li>
 * </ul>
 * <p>Using the accessors for any of these attributes will result in an
 * UnsupportedAttributeException being thrown.</p>
 * @author dan.templeton@sun.com
 * @see org.ggf.drmaa.JobTemplate
 * @see org.ggf.drmaa.Session
 * @see com.sun.grid.drmaa.SessionImpl
 * @since 0.5
 */
public class JobTemplateImpl extends JobTemplate {
	private static final String REMOTE_COMMAND = "drmaa_remote_command";
	private static final String INPUT_PARAMETERS = "drmaa_v_argv";
	private static final String JOB_SUBMISSION_STATE = "drmaa_js_state";
	private static final String JOB_ENVIRONMENT = "drmaa_v_env";
	private static final String WORKING_DIRECTORY = "drmaa_wd";
	private static final String JOB_CATEGORY = "drmaa_job_category";
	private static final String NATIVE_SPECIFICATION = "drmaa_native_specification";
	private static final String EMAIL_ADDRESS = "drmaa_v_email";
	private static final String BLOCK_EMAIL = "drmaa_block_email";
	private static final String START_TIME = "drmaa_start_time";
	private static final String JOB_NAME = "drmaa_job_name";
	private static final String INPUT_PATH = "drmaa_input_path";
	private static final String OUTPUT_PATH = "drmaa_output_path";
	private static final String ERROR_PATH = "drmaa_error_path";
	private static final String JOIN_FILES = "drmaa_join_files";
	private static final String TRANSFER_FILES = "drmaa_transfer_files";
/* Not supported   
	private static final String DEADLINE_TIME = "drmaa_deadline_time"
	private static final String HARD_WALLCLOCK_TIME_LIMIT = "drmaa_wct_hlimit"
	private static final String SOFT_WALLCLOCK_TIME_LIMIT = "drmaa_wct_slimit"
	private static final String HARD_RUN_DURATION_LIMIT = "drmaa_run_duration_hlimit"
	private static final String SOFT_RUN_DURATION_LIMIT = "drmaa_run_duration_slimit"
 */
	private static final String HOLD = "drmaa_hold";
	private static final String ACTIVE = "drmaa_active";
   private static PartialTimestampFormat ptf = new PartialTimestampFormat ();
   private SessionImpl session = null;
   private int id = -1;
   
   /** Creates a new instance of JobTemplateImpl
    * @param session the associated SessionImpl object
    * @param id the id of this job template
    */
   JobTemplateImpl (SessionImpl session, int id) {
      this.session = session;
      this.id = id;
   }
   
   /** Returns this template's native id.
    * @return the template's native id
    */
   int getId () {
      return id;
   }
   
   /** <p>Specifies the remote command to execute.  The remoteCommand must be the
    * path of an executable that is available at the job's execution host.  If
    * the path is relative, it is assumed to be relative to the working
    * directory set through the workingDirectory attibute.  If workingDirectory
    * is not set, the path is assumed to be relative to the user's home
    * directory.</p>
    * <p>The file pointed to by remoteCommand may either be an executable binary
    * or an executable script.  If a script, the script must include the path to
    * the shell in a #! line at the beginning of the script.  By default, the
    * remote command will be executed directly, as by exec. (See the exec(2) man
    * page.)  To have the remote command executed in a shell, such as to
    * preserve environment settings, use the nativeSpecification attribute to
    * include the &quot;-shell yes&quot; option.  Jobs which are executed by a
    * wrapper shell fail differently from jobs which are executed directly.  A
    * job executed by a wrapper shell which contains a user error, such as an
    * invalid path to the executable, will execute successfully and exit with a
    * return code of 1.  A job executed directly which contains such an error
    * will enter the <code>FAILED</code> state upon execution.</p>
    * <p>No binary file management is done.</p>
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
      this.setAttribute (REMOTE_COMMAND, remoteCommand);
   }
   
   /** Get the command to execute as the job.  The command
	 * is relative to the execution host and is evaluated on the
    * execution host.  No binary file management is done.
	 * @return The command to execute as the job or null if it has not been set
	 */
   public String getRemoteCommand () {
      return (String)this.getAttribute (REMOTE_COMMAND).get (0);
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
      this.setAttribute (INPUT_PARAMETERS, Arrays.asList (args));
   }
   
	/** Get The parameters passed as arguments to the job.
	 * @return The parameters passed as arguments to the job or null if they have
    * not been set
	 */	
   public String[] getArgs () {
      List result = this.getAttribute (INPUT_PARAMETERS);
      
      return (String[])result.toArray (new String[result.size ()]);
   }
   
	/** <p>Specifies the job state at submission.  The possible values are <code>HOLD</code> and
    * <code>ACTIVE</code>:</p>
    * <p><code>ACTIVE</code> means the job is runnable.</p>
    * <p><code>HOLD</code> means the job is submitted in user hold state (either
    * <code>Session.USER_ON_HOLD</code> or <code>Session.USER_SYSTEM_ON_HOLD</code>). This is
    * equivalent to the qsub submit option `-h'.
    * @param state The job state at submission
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */	
   public void setJobSubmissionState (int state) throws DrmaaException {
      String stateString = null;
      
      if (state == super.HOLD) {
         stateString = this.HOLD;
      }
      else {
         stateString = this.ACTIVE;
      }
      
      this.setAttribute (JOB_SUBMISSION_STATE, stateString);
   }
   
	/** <p>Get the job state at submission.  The states are <code>HOLD</code> and <code>ACTIVE</code>:</p>
    * <p><code>ACTIVE</code> means the job is runnable.</p>
    * <p><code>HOLD</code> means the job is submitted in user hold state.</p>
    * @return The job state at submission
    */	
   public int getJobSubmissionState () {
      String stateString = (String)this.getAttribute (JOB_SUBMISSION_STATE).get (0);
      
      if (stateString.equals (this.HOLD)) {
         return super.HOLD;
      }
      else {
         return super.ACTIVE;
      }
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
    *
    */
   public void setJobEnvironment (Properties env) throws DrmaaException {
      String[] envStrings = new String[env.size ()];
      Iterator i = env.keySet ().iterator ();
      int count = 0;
      String key = null;
      StringBuffer nameValue = null;
      
      while (i.hasNext ()) {
         key = (String)i.next ();         
         nameValue = new StringBuffer (key);
         nameValue.append ('=');
         nameValue.append (env.getProperty (key));
         envStrings[count] = nameValue.toString ();
         count++;
      }
      
      this.setAttribute (JOB_ENVIRONMENT, Arrays.asList (envStrings));
   }
   
	/** Get the environment values that define the remote environment.
	 * The values override the remote environment values if there is a collision.
	 * @return The environment values that define the remote environment or null
    * if it has not been set
	 */
   public Properties getJobEnvironment () {
      List props = this.getAttribute (JOB_ENVIRONMENT);
      Properties env = new Properties ();
      Iterator i = props.iterator ();
      
      while (i.hasNext ()) {
         String entry = (String)i.next ();
         env.setProperty (entry.substring (0, entry.indexOf ('=')), entry.substring (entry.indexOf ('=') + 1));
      }
      
      return env;
   }

   /** <p>Specifies the directory name where the job will be executed. The
    * working directory is evaluated relative to the execution host.</p>
    * <p>A <CODE>HOME_DIRECTORY</CODE> placeholder at the beginning denotes that
    * the remaining portion of the directory name is resolved relative to the
    * job submiter's home directory on the execution host.</p>
    * <p>The <CODE>PARAMETRIC_INDEX</CODE> placeholder can be used at any
    * position within the directory name of parametric jobs and will be replaced
    * by the underlying DRM system with the parametric jobs' indices.</p>
    * <p>The directory name must be specified in a syntax that is common at the
    * host where the job will be executed.  If no placeholder is used, an
    * absolute directory specification is recommended. If set to a relative path
    * and no placeholder is used, a path relative to the user's home directory
    * is assumed.  If not set, the working directory will default to the user's
    * home directory.  If the directory does not exist when the job is run, the
    * job enters the state <code>FAILED</code>.</p>
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
      this.setAttribute (WORKING_DIRECTORY, wd);
   }
   
   /** <p>Get the directory name where the job will be executed. The
    * working directory is evaluated relative to the execution host.</p>
    * <p>A <CODE>HOME_DIRECTORY</CODE> placeholder at the beginning denotes that
    * the remaining portion of the directory name is resolved relative to the
    * job submiter's home directory on the execution host.</p>
    * <p>The <CODE>PARAMETRIC_INDEX</CODE> placeholder can be used at any
    * position within the directory name of parametric jobs and will be replaced
    * by the underlying DRM system with the parametric jobs' index.</p>
    * <p>The directory name must be specified in a syntax that is common at the
    * host where the job will be executed.  If set to a relative path
    * and no placeholder is used, a path relative to the user's home directory
    * is assumed.</p>
    * @return The directory where the job is executed or null if it has not been
    * set
    */
   public String getWorkingDirectory () {
      return (String)this.getAttribute (WORKING_DIRECTORY).get (0);
   }
   
   /** <p>Specifies the DRMAA job category. The category string is used by the
    * underlying DRM as reference into the qtask file. Certain qsub options used
    * in the referenced qtask file line are applied to the job template
    * before submission to allow site-specific resolving of resources and/or
    * policies.  The cluster qtask file, the local qtask file, and the user
    * qtask file are searched. See the qtask(5) man page.  Job settings resulting
    * from job template category
    * are overridden by settings resulting from the job template
    * nativeSpecification attribute as well as by explict DRMAA job template
    * settings.</p>
    * <p>The options -help, -t, -verify, and -w w|v are ignored.  The -cwd
    * option is ignored unless the $SGE_DRMAA_ALLOW_CWD environment variable is
    * set.</p>
    * @param category An opaque string specifying how to resolve site-specific
    * resources and/or policies.
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeFormatException</LI>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>InvalidArgumentException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setJobCategory (String category) throws DrmaaException {
     this.setAttribute (JOB_CATEGORY, category);
   }
   
   /** Get the opaque string specifying how to resolve site-specific resources
	 * and/or policies.
    * @return The opaque string specifying how to resolve site-specific
	 * resources and/or policies or null if it has not been set
	 */
   public String getJobCategory () {
      return (String)this.getAttribute (JOB_CATEGORY).get (0);
   }
   
	/** Specifies native qsub options which will be interpreted as part of the
    * DRMAA job template.  All options available to the qsub command may be used
    * in the nativeSpecification, except for -help, -t, -verify, and -w w|v.
    * -cwd may only be used if the $SGE_DRMAA_ALLOW_CWD enviroment variable is
    * set.  Options set in the nativeSpecification will be overridden by the
    * corresponding DRMAA attributes.  See the qsub(1) man page for more information
    * on qsub command line options.
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
      this.setAttribute (NATIVE_SPECIFICATION, spec);
   }
   
	/** Get the opaque string that is passed by the end user to DRMAA to specify
    * site-specific resources and/or policies.
    * @return The opaque string that is passed by the end user to DRMAA to
    * specify site-specific resources and/or policies or null if it has not been
    * set
    */
   public String getNativeSpecification () {
     return (String)this.getAttribute (NATIVE_SPECIFICATION).get (0);
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
   public void setEmailAddresses (String[] email) throws DrmaaException {
      this.setAttribute (EMAIL_ADDRESS, Arrays.asList (email));
   }
   
	/** Get the list of email addresses used to report the job completion and
    * status.
    * @return The list of email addresses used to report the job completion
    * and status or null if they have not been set
    */	
   public String[] getEmailAddresses () {
      List emails = this.getAttribute (EMAIL_ADDRESS);
      return (String[])emails.toArray (new String[emails.size ()]);
   }
   
	/** Specifies whether e-mail sending shall blocked or not.  By default email
    * is not sent.  If, however, a setting in a cluster or user settings file or
    * the nativeSpecification or jobCategory attribute enables sending email in
    * association with job events, the blockEmail attribute will override that
    * setting, causing no email to be sent.
    * @param blockEmail Whether to block sending e-mail by default
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setBlockEmail (boolean blockEmail) throws DrmaaException {
      if (blockEmail) {
         this.setAttribute (BLOCK_EMAIL, "1");
      }
      else {
         this.setAttribute (BLOCK_EMAIL, "0");
      }
   }
   
	/** Get whether to block sending e-mail by default, regardless of the DRMS
    * settings.
    * @return Whether to block sending e-mail by default
	 */
   public boolean getBlockEmail () {
      String block = (String)this.getAttribute (BLOCK_EMAIL).get (0);
      
      return block.equals ("1");
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
      this.setAttribute (START_TIME, ptf.format (startTime));
   }
   
	/** Get the earliest time when the job may be eligible to be run.
    * @return The earliest time when the job may be eligible to be run or null
    * if it has not been set
	 */
   public PartialTimestamp getStartTime () {
      try {
         return ptf.parse ((String)this.getAttribute (START_TIME).get (0));
      }
      catch (java.text.ParseException e) {
         /* This should never happen! */
         throw new InternalException ("drmaa_start_time attribute is unparsable");
      }
   }
   
	/** Set the name of the job.  A job name will be comprised of alpha-numeric
    * and _ characters.  The DRMAA implementation may truncate client
    * provided job names to an implementation defined length that is at least 31
    * characters.  Setting the job name is equivalent to use of the qsub
    * submit option `-N' with <i>name</i> as option argument and has the same
    * restrictions.  See the qsub(1) man page.
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
      this.setAttribute (JOB_NAME, name);
   }
   
	/** Get the name of the job.  A job name will be comprised of alpha-numeric
    * and _ characters.
    * @return The name of the job or null if it has not been set
	 */
   public String getJobName () {
      return (String)this.getAttribute (JOB_NAME).get (0);
   }
   
	/** Set the job's standard input path.
    * Unless set elsewhere, if not explicitly set in the job template, the job
    * is started with an empty input stream.<BR>
    * If set, specifies the network path of the job's input stream in
    * the form of [hostname]:file_path<BR>
    * When the TranferFileMode object's inputStream property is set to
    * <code>true</code>, the input file will be fetched by the underlying DRM
    * system from the specified host or from the submit host if no hostname
    * is specified.<BR>
    * When the TranferFileMode object's inputStream property is not set or is set to
    * <code>false</code>, the input file is always expected at the
    * host where the job is executed irrespectively of any hostname
    * specified.<BR>
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
    * When the job is run, if this attribute is set, and the file can't be read,
    * the job will enter the state <code>FAILED</code>.
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
      this.setAttribute (INPUT_PATH, inputPath);
   }
   
	/** Get the job's standard input path.
    * Specifies the network path of the job's input stream in
    * the form of [hostname]:file_path<BR>
    * When the TranferFileMode object's inputStream property is set to
    * <code>true</code>, the input file will be fetched by the underlying DRM
    * system from the specified host or from the submit host if no hostname
    * is specified.<BR>
    * When the TranferFileMode object's inputStream property is not set or is set to
    * <code>false</code>, the input file is always expected at the
    * host where the job is executed irrespectively of any hostname
    * specified.<BR>
    * The <CODE>PARAMETRIC_INDEX</CODE> placeholder can be used at any position
    * within the file path of parametric job templates and will be replaced
    * by the underlying DRM system with the parametric job's index.<BR>
    * A <CODE>HOME_DIRECTORY</CODE> placeholder at the beginning of the file path
    * denotes that the remaining portion of the file path is relative to the job
    * submiter's home directory on the host where the file is located.<BR>
    * A <CODE>WORKING_DIRECTORY</CODE> placeholder at the beginning of the file path
    * denotes that the remaining portion of the file path is relative to the
    * job's working directory on the host where the file is located.<BR>
    * The file path is specified in a syntax that is common at the host
    * where the file is located.<BR>
    * @return The job's standard input path or null if it has not been set
    */
   public String getInputPath () {
      return (String)this.getAttribute (INPUT_PATH).get (0);
   }
   
	/** Sets how to direct the job's standard output.
    * If not explicitly set in the job template, the whereabouts of the jobs
    * output stream is not defined.
    * If set, specifies the network path of the job's output stream file in the
    * form of [hostname]:file_path<BR>
    * When the TranferFileMode object's outputStream property is set to
    * <code>true</code>, the output file will be fetched by the underlying
    * DRM system from the specified host or from the submit host if no hostname
    * is specified.<BR>
    * When the transferFiles job template attribute is not set or does
    * not contain TRANSFER_OUTPUT_FILES, the output file is always expected at
    * the host where the job is executed irrespectively of any hostname
    * specified.<BR>
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
    * When the job is run, if this attribute is set, and the file can't be
    * written before execution the job will enter the state <code>FAILED</code>.
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
      this.setAttribute (OUTPUT_PATH, outputPath);
   }
   
	/** Gets how to direct the job's standard output.
    * If set, specifies the network path of the job's output stream file in the
    * form of [hostname]:file_path<BR>
    * When the tranferFiles job template attribute is set to
    * TRANSFER_OUTPUT_FILES, the output file will be fetched by the underlying
    * DRM system from the specified host or from the submit host if no hostname
    * is specified.<BR>
    * When the TranferFileMode object's outputStream property is not set or is set to
    * <code>false</code>, the output file is always expected at
    * the host where the job is executed irrespectively of any hostname
    * specified.<BR>
    * The <CODE>PARAMETRIC_INDEX</CODE> placeholder can be used at any position
    * within the file path of parametric job templates and will be replaced
    * by the underlying DRM system with the parametric job's index.<BR>
    * A <CODE>HOME_DIRECTORY</CODE> placeholder at the beginning of the file path
    * denotes that the remaining portion of the file path is relative to the job
    * submiter's home directory on the host where the file is located.<BR>
    * A <CODE>WORKING_DIRECTORY</CODE> placeholder at the beginning of the file path
    * denotes that the remaining portion of the file path is relative to the
    * job's working directory on the host where the file is located.<BR>
    * The file path is specified in a syntax that is common at the host
    * where the file is located.<BR>
    * @return How to direct the job's standard output or null if it has not been
    * set
    */
   public String getOutputPath () {
      return (String)this.getAttribute (OUTPUT_PATH).get (0);
   }
   
	/** Sets how to direct the job's standard error.
    * If not explicitly set in the job template, the whereabouts of the job's
    * error stream is not defined. If set, specifies the network path of the
    * job's error stream file in the form [hostname]:file_path<BR>
    * When the TranferFileMode object's errorStream property is set to
    * <code>true</code>, the error file will be fetched by the underlying
    * DRM system from the specified host or from the submit host if no hostname
    * is specified.<BR>
    * When the transferFiles job template attribute is not set or does
    * not contain TRANSFER_ERROR_FILES, the error file is always expected at
    * the host where the job is executed irrespectively of any hostname
    * specified.<BR>
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
    * When the job is run, if this attribute is set, and the file can't be
    * written before execution the job will enter the state <code>FAILED</code>.
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
      this.setAttribute (ERROR_PATH, errorPath);
   }
   
	/** Gets how to direct the job's standard error.
    * If not explicitly set in the job template, the whereabouts of the job's
    * error stream is not defined. If set, specifies the network path of the
    * job's error stream file in the form [hostname]:file_path<BR>
    * When the TRANSFER_FILES job template attribute is supported and contains
    * the character 'e', the output file will be transferred by the underlying
    * DRM system to the specified host or to the submit host if no hostname is
    * specified.<BR>
    * When the TranferFileMode object's inputStream property is not set or is set to
    * <code>false</code>, the error file is always kept at the host where
    * the job is executed irrespectively of a possibly hostname specified.<BR>
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
      return (String)this.getAttribute (ERROR_PATH).get (0);
   }
   
	/** Sets whether the error stream should be intermixed with the output
    * stream. If not explicitly set in the job template the attribute defaults
    * to <i>false</i>.<BR>
    * If <i>true</i>, the underlying DRM system will ignore the value of
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
      if (join) {
         this.setAttribute (JOIN_FILES, "y");
      }
      else {
         this.setAttribute (JOIN_FILES, "n");
      }
   }
   
	/** Gets whether the error stream should be intermixed with the output
	 * stream.  If <i>true</i>, the underlying DRM system will ignore the value
	 * of the errorPath attribute and intermix the standard error stream with the
	 * standard output stream as specified with outputPath.
    * @return Whether the error stream should be intermixed with the output
	 * stream
	 */
   public boolean getJoinFiles () {
      String block = (String)this.getAttribute (JOIN_FILES).get (0);
      
      return block.equalsIgnoreCase ("y");
   }
   
	/** <p>Sets how to transfer files between hosts.
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
    * <p>See setInputPath(), setOutputPath() and setErrorPath() for information
    * about how to specify the standard input file, standard output file and
    * standard error file.</p>
    * <p>The file transfer mechanism itself must be configured by the
    * administrator. (see the sge_conf(5) man page.)  When it is configured, the
    * administrator has to enable transferFiles by settings the execd param,
    * delegated_file_staging, to true. If it is not configured,
    * transferFiles is not enabled and accessing this property will result in an
    * UnsupportedAttributeException being thrown.</p>
    * @param mode How to transfer files between hosts.
    * @throws DrmaaException Maybe be one of the following:
    * <UL>
    * <LI>InvalidAttributeValueException</LI>
    * <LI>ConflictingAttributeValuesException</LI>
    * </UL>
    */
   public void setTransferFiles (FileTransferMode mode) throws DrmaaException {
      StringBuffer buf = new StringBuffer ();
      
      if (mode.getInputStream ()) {
         buf.append ('i');
      }
      
      if (mode.getOutputStream ()) {
         buf.append ('o');
      }
      
      if (mode.getErrorStream ()) {
         buf.append ('e');
      }
      
      this.setAttribute (TRANSFER_FILES, buf.toString ());
   }
   
	/** <p>Gets how to transfer files between hosts.
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
    * @return How to transfer files between hosts.  May be
    * TRANSFER_NONE, or a combination of TRANSFER_ERROR_FILES,
    * TRANSFER_INPUT_FILES and/or TRANSFER_OUTPUT_FILES ored together
    */
   public FileTransferMode getTransferFiles () {
      String buf = (String)this.getAttribute (TRANSFER_FILES).get (0);
      
      return new FileTransferMode ((buf.indexOf ('i') != -1),
                                   (buf.indexOf ('o') != -1),
                                   (buf.indexOf ('e') != -1));
   }
   
   private List getAttribute (String name) {
      String[] values = session.nativeGetAttribute (id, name);
      
      return Arrays.asList (values);
   }
   
   private void setAttribute (String name, List value) throws DrmaaException {
      session.nativeSetAttributeValues (id, name, (String[])value.toArray (new String[value.size ()]));
   }
   
   private void setAttribute (String name, String value) throws DrmaaException {
      session.nativeSetAttributeValue (id, name, value);
   }   
   
   /** <p>Returns the list of supported properties names.  With the execd param,
    * delegated_file_staging set to false, this list includes only the list of DRMAA
    * required properties.  With delegated_file_staging set to true, the list also
    * includes the transferFiles property.</p>
    * @return the list of supported attribute names
    */	
   public List getAttributeNames () {
      return Arrays.asList (session.nativeGetAttributeNames (id));
   }
   
   /** Tests whether this JobTemplateImpl represents the same job template as
    *  the given object.  This means that even if two JobTemplateImpl's have all
    *  the same settings, they are not equal.  This is because the
    *  JobTemplateImpl has a native peer.
    * @param obj the object against which to compare
    * @return whether the the given object is the same as this object
    */
   public boolean equals (Object obj) {
      if (obj instanceof JobTemplateImpl) {
         return (this.getId () == ((JobTemplateImpl)obj).getId ());
      }
      else {
         return false;
      }
   }
   
   /** Returns a hash code based on the id used to associate this
    *  JobTemplateImpl with its native peer.
    * @return the has code
    */
   public int hashCode () {
      return this.getId ();
   }
}
