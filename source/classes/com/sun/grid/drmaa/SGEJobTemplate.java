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
 * SGEJobTemplate.java
 *
 * Created on March 4, 2004, 10:09 AM
 */

package com.sun.grid.drmaa;

import java.util.*;

import org.ggf.drmaa.*;

/**
 *
 * @author  dan.templeton@sun.com
 */
public class SGEJobTemplate extends JobTemplate {
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
	private static final String TRANSFER_FILES = "drmaa_tranfer_files";
	private static final String HOLD = "drmaa_hold";
	private static final String ACTIVE = "drmaa_active";
   private SGESession session = null;
   private int id = -1;
   
   /** Creates a new instance of SGEJobTemplate */
   SGEJobTemplate (SGESession session, int id) {
      this.session = session;
      this.id = id;
   }
   
   public void setRemoteCommand (String remoteCommand) throws DRMAAException {
      this.setAttribute (REMOTE_COMMAND, remoteCommand);
   }
   
   public String getRemoteCommand () {
      return (String)this.getAttribute (REMOTE_COMMAND).get (0);
   }
   
   public void setInputParameters (String[] args) throws DRMAAException {
      this.setAttribute (INPUT_PARAMETERS, Arrays.asList (args));
   }
   
   public String[] getInputParameters () {
      List result = this.getAttribute (INPUT_PARAMETERS);
      
      return (String[])result.toArray (new String[result.size ()]);
   }
   
   public void setJobSubmissionState (int state) throws DRMAAException {
      String stateString = null;
      
      if (state == super.HOLD) {
         stateString = this.HOLD;
      }
      else {
         stateString = this.ACTIVE;
      }
      
      this.setAttribute (JOB_SUBMISSION_STATE, stateString);
   }
   
   public int getJobSubmissionState () {
      String stateString = (String)this.getAttribute (JOB_SUBMISSION_STATE).get (0);
      
      if (stateString.equals (this.HOLD)) {
         return super.HOLD;
      }
      else {
         return super.ACTIVE;
      }
   }
   
   public void setJobEnvironment (Properties env) throws DRMAAException {
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
      }
      
      this.setAttribute (JOB_ENVIRONMENT, Arrays.asList (envStrings));
   }
   
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

   public void setWorkingDirectory (String wd) throws DRMAAException {
      this.setAttribute (WORKING_DIRECTORY, wd);
   }
   
   public String getWorkingDirectory () {
      return (String)this.getAttribute (WORKING_DIRECTORY).get (0);
   }
   
   public void setJobCategory (String category) throws DRMAAException {
     this.setAttribute (JOB_CATEGORY, category);
   }
   
   public String getJobCategory () {
      return (String)this.getAttribute (JOB_CATEGORY).get (0);
   }
   
   public void setNativeSpecification (String spec) throws DRMAAException {
      this.setAttribute (NATIVE_SPECIFICATION, spec);
   }
   
   public String getNativeSpecification () {
     return (String)this.getAttribute (NATIVE_SPECIFICATION).get (0);
   }
   
   public void setEmailAddresses (String[] email) throws DRMAAException {
      this.setAttribute (EMAIL_ADDRESS, Arrays.asList (email));
   }
   
   public String[] getEmailAddresses () {
      List emails = this.getAttribute (EMAIL_ADDRESS);
      return (String[])emails.toArray (new String[emails.size ()]);
   }
   
   public void setBlockEmail (boolean blockEmail) throws DRMAAException {
      if (blockEmail) {
         this.setAttribute (BLOCK_EMAIL, "1");
      }
      else {
         this.setAttribute (BLOCK_EMAIL, "0");
      }
   }
   
   public boolean getBlockEmail () {
      String block = (String)this.getAttribute (BLOCK_EMAIL).get (0);
      
      if (block.equals ("1")) {
         return true;
      }
      else {
         return false;
      }
   }
   
   public void setStartTime (Date startTime) throws DRMAAException {
      this.setAttribute (START_TIME, this.getStringFromDate (startTime));
   }
   
   public Date getStartTime () {
      return this.getDateFromString ((String)this.getAttribute (START_TIME).get (0));
   }
   
   public void setJobName (String name) throws DRMAAException {
      this.setAttribute (JOB_NAME, name);
   }
   
   public String getJobName () {
      return (String)this.getAttribute (JOB_NAME).get (0);
   }
   
   public void setInputPath (String inputPath) throws DRMAAException {
      this.setAttribute (INPUT_PATH, inputPath);
   }
   
   public String getInputPath () {
      return (String)this.getAttribute (INPUT_PATH).get (0);
   }
   
   public void setOutputPath (String outputPath) throws DRMAAException {
      this.setAttribute (INPUT_PATH, outputPath);
   }
   
   public String getOututPath () {
      return (String)this.getAttribute (OUTPUT_PATH).get (0);
   }
   
   public void setErrorPath (String errorPath) throws DRMAAException {
      this.setAttribute (ERROR_PATH, inputPath);
   }
   
   public String getErrorPath () {
      return (String)this.getAttribute (ERROR_PATH).get (0);
   }
   
   public void setJoinFiles (boolean join) throws DRMAAException {
      if (join) {
         this.setAttribute (JOIN_FILES, "1");
      }
      else {
         this.setAttribute (JOIN_FILES, "0");
      }
   }
   
   public boolean getJoinFiles () {
      String block = (String)this.getAttribute (JOIN_FILES).get (0);
      
      if (block.equals ("1")) {
         return true;
      }
      else {
         return false;
      }
   }
   
   private List getAttribute (String name) {
      String[] values = session.nativeGetAttribute (name);
      
      return Arrays.asList (values);
   }
   
   private void setAttribute (String name, List value) throws DRMAAException {
      session.nativeSetAttributeValues (name, (String[])value.toArray (new String[value.size ()]));
   }
   
   private void setAttribute (String name, String value) throws DRMAAException {
      session.nativeSetAttributeValue (name, value);
   }   
   
   public void delete () throws DRMAAException {
      session.nativeDeleteJobTemplate (this);
   }
   
   int getId () {
      return id;
   }
   
   private String getStringFromDate (Date date) {
      return null;
   }
   
   private Date getDateFromString (String date) {
      return null;
   }
}
