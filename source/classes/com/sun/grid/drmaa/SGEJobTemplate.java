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

import java.util.*;
import java.util.regex.*;

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
	private static final String DEADLINE_TIME = "drmaa_deadline_time";
	private static final String HARD_WALLCLOCK_TIME_LIMIT = "drmaa_wct_hlimit";
	private static final String SOFT_WALLCLOCK_TIME_LIMIT = "drmaa_wct_slimit";
	private static final String HARD_RUN_DURATION_LIMIT = "drmaa_run_duration_hlimit";
	private static final String SOFT_RUN_DURATION_LIMIT = "drmaa_run_duration_slimit";
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
      this.setAttribute (OUTPUT_PATH, outputPath);
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
         this.setAttribute (JOIN_FILES, "y");
      }
      else {
         this.setAttribute (JOIN_FILES, "n");
      }
   }
   
   public boolean getJoinFiles () {
      String block = (String)this.getAttribute (JOIN_FILES).get (0);
      
      if (block.equalsIgnoreCase ("y")) {
         return true;
      }
      else {
         return false;
      }
   }
   
   public void setTransferFiles (byte transfer) throws DRMAAException {
      StringBuffer buf = new StringBuffer ();
      
      if ((transfer & TRANSFER_INPUT_FILES) == 1) {
         buf.append ('i');
      }
      
      if ((transfer & TRANSFER_OUTPUT_FILES) == 1) {
         buf.append ('o');
      }
      
      if ((transfer & TRANSFER_ERROR_FILES) == 1) {
         buf.append ('e');
      }
      
      this.setAttribute (TRANSFER_FILES, buf.toString ());
   }
   
   public byte getTransferFiles () {
      String buf = (String)this.getAttribute (TRANSFER_FILES).get (0);
      byte ret = TRANSFER_NONE;
      
      if (buf.indexOf ('i') != -1) {
         ret = (byte)(ret | TRANSFER_INPUT_FILES);
      }
      
      if (buf.indexOf ('o') != -1) {
         ret = (byte)(ret | TRANSFER_OUTPUT_FILES);
      }
      
      if (buf.indexOf ('e') != -1) {
         ret = (byte)(ret | TRANSFER_ERROR_FILES);
      }
      
      return ret;
   }
   
   public void setDeadlineTime (Date deadline) throws DRMAAException {
      this.setAttribute (DEADLINE_TIME, this.getStringFromDate (startTime));
   }
   
   public Date getDeadlineTime () {
      return this.getDateFromString ((String)this.getAttribute (DEADLINE_TIME).get (0));
   }
   
   public void setHardWallclockTimeLimit (long hardWallclockLimit) throws DRMAAException {
      this.setAttribute (HARD_WALLCLOCK_TIME_LIMIT, Long.toString (hardWallclockLimit));
   }
   
   public long getHardWallclockTimeLimit () {
      return Long.parseLong ((String)this.getAttribute (HARD_WALLCLOCK_TIME_LIMIT).get (0));
   }
   
   public void setSoftWallClockTimeLimit (long softWallclockLimit) throws DRMAAException {
      this.setAttribute (SOFT_WALLCLOCK_TIME_LIMIT, Long.toString (softWallclockLimit));
   }
   
   public long getSoftWallClockTimeLimit () {
      return Long.parseLong ((String)this.getAttribute (SOFT_WALLCLOCK_TIME_LIMIT).get (0));
   }
   
   public void setHardRunDurationLimit (long hardRunLimit) throws DRMAAException {
      this.setAttribute (HARD_RUN_DURATION_LIMIT, Long.toString (hardRunLimit));
   }
   
   public long getHardRunDurationLimit () {
      return Long.parseLong ((String)this.getAttribute (HARD_RUN_DURATION_LIMIT).get (0));
   }
   
   public void setSoftRunDurationLimit (long softRunLimit) throws DRMAAException {
      this.setAttribute (SOFT_RUN_DURATION_LIMIT, Long.toString (softRunLimit));
   }

   public long getSoftRunDurationLimit () {
      return Long.parseLong ((String)this.getAttribute (SOFT_RUN_DURATION_LIMIT).get (0));
   }
   
   private List getAttribute (String name) {
      String[] values = session.nativeGetAttribute (id, name);
      
      return Arrays.asList (values);
   }
   
   private void setAttribute (String name, List value) throws DRMAAException {
      session.nativeSetAttributeValues (id, name, (String[])value.toArray (new String[value.size ()]));
   }
   
   private void setAttribute (String name, String value) throws DRMAAException {
      session.nativeSetAttributeValue (id, name, value);
   }   
   
   public List getAttributeNames () {
      return Arrays.asList (session.nativeGetAttributeNames (id));
   }
   
   public void delete () throws DRMAAException {
      session.nativeDeleteJobTemplate (id);
   }
   
   int getId () {
      return id;
   }
   
   private String getStringFromDate (Date date) {
      StringBuffer dateString = new StringBuffer ();
      Calendar cal = Calendar.getInstance ();
      
      cal.setTime (date);
      
      dateString.append (Integer.toString (cal.get (Calendar.YEAR)));
      dateString.append ('/');
      dateString.append (Integer.toString (cal.get (Calendar.MONTH) + 1));
      dateString.append ('/');
      dateString.append (Integer.toString (cal.get (Calendar.DAY_OF_MONTH)));
      dateString.append (' ');
      dateString.append (Integer.toString (cal.get (Calendar.HOUR_OF_DAY)));
      dateString.append (':');
      dateString.append (Integer.toString (cal.get (Calendar.MINUTE)));
      dateString.append (':');
      dateString.append (Integer.toString (cal.get (Calendar.SECOND)));
      
      return dateString.toString ();
   }
   
   private Date getDateFromString (String date) {
      Pattern p = Pattern.compile("((((\\d\\d)?(\\d\\d)/)?(\\d\\d)/)?(\\d\\d))? (\\d\\d):(\\d\\d)(:(\\d\\d))? ((+|-) (\\d\\d):(\\d\\d))?");
      Matcher m = p.matcher(date);
      
      if (m.matches()) {
         Calendar cal = null;
         String year12 = m.group (1);
         String year34 = m.group (2);
         String month = m.group (4);
         String day = m.group (6);
         String hour = m.group (8);
         String minute = m.group (9);
         String second = m.group (10);
         String tzSign = m.group (12);
         String tzHour = m.group (13);
         String tzMinute = m.group (14);
         
         if (!tzSign.equals ("") && !tzHour.equals ("") && !tzMinute.equals ("")) {
            StringBuffer tz = new StringBuffer ("GMT");
            
            tz.append (tzSign);
            tz.append (tzHour);
            tz.append (':');
            tz.append (tzMinute);
            
            cal = Calendar.getInstance (TimeZone.getTimeZone (tz.toString ()));
         }
         else {
            cal = Calendar.getInstance ();
         }
         
         if (!second.equals ("")) {
            cal.set (Calendar.SECOND, Integer.parseInt (second));
         }
         
         cal.set (Calendar.MINUTE, Integer.parseInt (minute));
         cal.set (Calendar.HOUR_OF_DAY, Integer.parseInt (hour));
         
         if (!day.equals ("")) {
            cal.set (Calendar.DAY_OF_MONTH, Integer.parseInt (day));
         
            if (!month.equals ("")) {
               cal.set (Calendar.MONTH, Integer.parseInt (month) - 1);

               if (!year12.equals ("") && !year34.equals ("")) {
                  cal.set (Calendar.YEAR, Integer.parseInt (year12 + year34));
               }
               else if (!year34.equals ("")) {
                  cal.set (Calendar.YEAR, 2000 + Integer.parseInt (year34));
               }
               else if (cal.getTime ().getTime () < System.currentTimeMillis ()) {
                  cal.add (Calendar.YEAR, 1);
               }
            }
            else if (cal.getTime ().getTime () < System.currentTimeMillis ()) {
               cal.add (Calendar.MONTH, 1);
            }
         }
         else if (cal.getTime ().getTime () < System.currentTimeMillis ()) {
            cal.add (Calendar.DAY_OF_MONTH, 1);
         }
         
         return cal.getTime ();
      }
      else {
         return null;
      }
   }
}
