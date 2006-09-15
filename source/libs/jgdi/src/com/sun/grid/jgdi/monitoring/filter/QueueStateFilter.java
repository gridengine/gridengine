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
package com.sun.grid.jgdi.monitoring.filter;

/**
 *
 * @author  richard.hierlmeier@sun.com
 */
public class QueueStateFilter {
   
   private int options;
   
//  [-qs {a|c|d|o|s|u|A|C|D|E|S}]     selects queues, which are in the given state(s)
   
   public static final int ALARM                   = 0x0001; // a
   public static final int CONFIGURATION_AMBIGUOUS = 0x0002; // c
   public static final int DISABLED                = 0x0004; // d
   public static final int ORPHANED                = 0x0008; // o 
   public static final int SUSPENDED               = 0x000F; // s
   public static final int UNKNOWN                 = 0x0010; // u
   public static final int SUSPEND_ALARM           = 0x0012; // A
   public static final int CALENDAR_SUSPENDED      = 0x0014; // C
   public static final int CALENDAR_DISABLED       = 0x0018; // D
   public static final int ERROR                   = 0x001F; // E
   public static final int SUBORDINATE             = 0x0100; // S
   
   static class StateToString {
      int state;
      char letter;
      
      public StateToString(int state, char letter) {
         this.state = state;
         this.letter = letter;
      }
      
   }
   
   private static final StateToString [] STATE_TO_STR = {
      new StateToString(ALARM,'a'),
      new StateToString(CONFIGURATION_AMBIGUOUS, 'c'),
      new StateToString(DISABLED, 'd'),
      new StateToString(ORPHANED, 'o'),
      new StateToString(SUSPENDED, 's'),
      new StateToString(UNKNOWN, 'u'),
      new StateToString(SUSPEND_ALARM, 'A'),
      new StateToString(CALENDAR_SUSPENDED, 'C'),
      new StateToString(CALENDAR_DISABLED, 'D'),
      new StateToString(ERROR, 'E'),
      new StateToString(SUBORDINATE, 'S')
   };
   
   public QueueStateFilter() {
      this(0);
   }
   /** Creates a new instance of QueueStateFilter */
   public QueueStateFilter(int mask) {
      this.options = mask;
   }
   
   private void set(int state, boolean flag) {
       if(flag) {
           options |= state;
       } else {
           options &= ~state;
       }
   }
   
   private boolean isSet(int state) {
       return (options & state) == state;
   }
   
   public boolean hasAlarm() {
      return isSet(ALARM);
   }
   
   public void setAlarm(boolean flag) {
       set(ALARM, flag);
   }
   
   public boolean hasConfigurationAmbiguous() {
       return isSet(CONFIGURATION_AMBIGUOUS);
   }
   
   public void setConfigurationAmbiguous(boolean flag) {
       set(CONFIGURATION_AMBIGUOUS, flag);
   }

   public boolean hasDisabled() {
       return isSet(DISABLED);
   }
   
   public void setDisabled(boolean flag) {
       set(DISABLED, flag);
   }
   
   public boolean hasOrphaned() {
       return isSet(ORPHANED);
   }
   
   public void setOrphaned(boolean flag) {
       set(ORPHANED, flag);
   }
   
   
   public boolean hasSuspended() {
       return isSet(SUSPENDED);
   }
   
   public void setSuspended(boolean flag) {
       set(SUSPENDED, flag);
   }
   
   
   public boolean hasUnknown() {
       return isSet(UNKNOWN);
   }
   
   public void setUnknown(boolean flag) {
       set(UNKNOWN, flag);
   }
   
   
   public boolean hasSuspendAlarm() {
       return isSet(SUSPEND_ALARM);
   }
   
   public void setSuspendAlarm(boolean flag) {
       set(SUSPEND_ALARM, flag);
   }
   
   public boolean hasCalendarSuspend() {
       return isSet(CALENDAR_SUSPENDED);
   }
   
   public void setCalendarSuspend(boolean flag) {
       set(CALENDAR_SUSPENDED, flag);
   }
   
   public boolean hasCalendarDisabled() {
       return isSet(CALENDAR_DISABLED);
   }
   
   public void setCalendarDisabled(boolean flag) {
       set(CALENDAR_DISABLED, flag);
   }
   
   public boolean hasError() {
       return isSet(ERROR);
   }
   
   public void setError(boolean flag) {
       set(ERROR, flag);
   }

   public boolean hasSubordinate() {
       return isSet(SUBORDINATE);
   }
   
   public void setSubordinate(boolean flag) {
       set(SUBORDINATE, flag);
   }

   public static QueueStateFilter parse(String options) {
      QueueStateFilter ret = new QueueStateFilter();
      char c;
      outer:
      for(int i = 0; i < options.length(); i++) {
         c = options.charAt(i);
         for(int stateIndex = 0; stateIndex < STATE_TO_STR.length; stateIndex++) {
            if(STATE_TO_STR[stateIndex].letter == c) {
               ret.set(STATE_TO_STR[stateIndex].state, true);
               continue outer;
            }
         }
         throw new IllegalArgumentException("Unknown queue state " + c);
      }
      return ret;
   }
   
   public String getOptions() {
      char [] buf = new char[STATE_TO_STR.length];
      int bufIndex = 0;
      for(int i = 0; i < STATE_TO_STR.length; i++) {
         if( (STATE_TO_STR[i].state & options) == STATE_TO_STR[i].state) {
            buf[bufIndex++] = STATE_TO_STR[i].letter;
         }         
      }
      return new String(buf, 0, bufIndex);
   }
   
   public String toString() {
      StringBuffer ret = new StringBuffer();
      ret.append("QueueStateFilter[");
      ret.append(getOptions());
      ret.append( " (");
      ret.append(Integer.toHexString(options));
      ret.append( " )");
      ret.append(']');
      return ret.toString();
   }
   
}
