/*___INFO__MARK_BEGIN__*/ /*************************************************************************
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
package com.sun.grid.jgdi.util.shell;

import com.sun.grid.jgdi.JGDI;
import com.sun.grid.jgdi.configuration.AdvanceReservation;
import com.sun.grid.jgdi.configuration.JGDIAnswer;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Date;
import java.util.List;
import java.util.TimeZone;
import static com.sun.grid.jgdi.util.JGDIShell.getResourceString;

public class QrStatCommand extends AbstractCommand {

   /** Creates a new instance of QModCommand */
   public QrStatCommand(Shell shell, String name) {
      super(shell, name);
   }

   public String getUsage() {
      return getResourceString("sge.version.string") + "\n" + getResourceString("usage.qrstat");
   }

   public void run(String[] args) throws Exception {

      JGDI jgdi = getShell().getConnection();

      if (jgdi == null) {
         throw new IllegalStateException("Not connected");
      }

      PrintWriter pw = new PrintWriter(System.out);

      List<String> userList = new ArrayList<String>();
      List<String> arList = new ArrayList<String>();

      for (int i = 0; i < args.length; i++) {
         if (args[i].equals("-help")) {
            pw.println(getUsage());
            return;
         } else if (args[i].equals("-ar")) {
            i++;
            arList = Arrays.asList(args[i].split(","));
         } else if (args[i].equals("-u")) {
            i++;
            userList = Arrays.asList(args[i].split(",")); 
         } else {
            pw.println("error: ERROR! invalid option argument \"" + args[i] + "\"");
            pw.println("Usage: qrdel -help");
            return;
         }
      }
      //Let's take ar_list and look for candidates to delete
      @SuppressWarnings("unchecked")
      List<AdvanceReservation> ars = (List<AdvanceReservation>) jgdi.getAdvanceReservationList();
      //Filter out just the ars in the arList
      if (arList.size() > 0) {
        boolean found;
        int arId;
        for (AdvanceReservation ar : ars) {
           found = false;
           for (String arStr : arList) {
              try {
                 arId = Integer.parseInt(arStr);
                 if (ar.getId() == arId) {
                    arList.remove(arStr);
                    found = true;
                    break;
                 }
              } catch (NumberFormatException ex) {
                 //Not an id, perhaps an AR name
                 if (ar.getName().equals(arStr)) {
                    arList.remove(arStr);
                    found = true;
                    break;
                 }
              }
           }
           //If not specified in the list we won't delete this ar later
           if (!found) {
              ars.remove(ar);
           }
        }
      }
      //Now we have a list of ARs to delete
      //Let's filter out AR that belong to not specifed users
      if (userList.size() > 0) {
         boolean isValid = false;
         for (AdvanceReservation ar : ars) {
            isValid = false;
            for (String user : userList) {
               if (ar.getOwner().equals(user)) {
                  isValid = true;
                  break;
               }
            }
            //Exclude ARs for not specified users
            if (!isValid) {
               ars.remove(ar);
            }
         }
      }
      //Exit if we have nothing to report
      if (ars.size() == 0) {
          return;
      }
      //Finally delete all matched ARs
      if (arList.size() > 0) {
          //TODO: finish
          pw.println("NOT IMPLEMENTED");
          return;
      }
      List<JGDIAnswer> answers = new ArrayList<JGDIAnswer>();
      pw.println("ar-id   name       owner        state start at             end at               duration");
      pw.println("----------------------------------------------------------------------------------------");
      for (AdvanceReservation ar : ars) {
         pw.printf("%1$7d %2$-10.10s %3$-12.12s %4$-5.5s %5$-20.20s %6$-20.20s %7$s\n",
               ar.getId(), ar.getName()==null ? "" : ar.getName() , ar.getOwner(), ar.getStateAsString(), 
               getDateAndTimeAsString(ar.getStartTime()), getDateAndTimeAsString(ar.getEndTime()) , getDurationAsString(ar.getDuration()));
      }
      //TimeZone.setDefault(currentTz);
      pw.println("_exit_code="+printAnswers(answers, pw)+"_");
      pw.flush();
   }
   
   public static String getDateAndTimeAsString(int time) {
       if (time == -1) {
           return "N/A";
       }
       return String.format("%1$tm/%1$td/%1$tY %1$tT", new Date(time*1000L));
   }
   
   public static String getDurationAsString(int duration) {
      if (duration == -1) {
         return "N/A";
      }
      Date d = new Date(duration * 1000L);
      return String.format("%1$d:%2$tM:%2$tS", duration / 3600L, d);
   }
}