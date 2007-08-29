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
import com.sun.grid.jgdi.configuration.AdvanceReservationImpl;
import com.sun.grid.jgdi.configuration.JGDIAnswer;
import com.sun.grid.jgdi.configuration.User;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import static com.sun.grid.jgdi.util.JGDIShell.getResourceString;

public class QrDelCommand extends AbstractCommand {

   /** Creates a new instance of QModCommand */
   public QrDelCommand(Shell shell, String name) {
      super(shell, name);
   }

   public String getUsage() {
      return getResourceString("sge.version.string") + "\n" + getResourceString("usage.qrdel");
   }

   public void run(String[] args) throws Exception {

      JGDI jgdi = getShell().getConnection();

      if (jgdi == null) {
         throw new IllegalStateException("Not connected");
      }
      if (args.length == 0) {
         throw new IllegalArgumentException("Invalid number of arguments");
      }

      PrintWriter pw = new PrintWriter(System.out);


      boolean force = false;
      boolean isArListSupplied = false;
      List<String> userList = new ArrayList<String>();
      List<String> arList = new ArrayList<String>();
      List<AdvanceReservation> delList = new ArrayList<AdvanceReservation>();

      for (int i = 0; i < args.length; i++) {
         if (args[i].equals("-help")) {
            pw.println(getUsage());
            return;
         } else if (args[i].equals("-f")) {
            force = true;
         } else if (args[i].equals("-u")) {
            i++;
            userList = Arrays.asList(args[i].split(","));
         } else if (!isArListSupplied) {
            isArListSupplied = true;

            arList = Arrays.asList(args[i].split(","));
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
      if (isArListSupplied) {
        boolean found;
        int arId;
        for (AdvanceReservation ar : new ArrayList<AdvanceReservation>(ars)) {
           found = false;
           for (String arStr : arList) {
              try {
                 arId = Integer.parseInt(arStr);
                 if (ar.getId() == arId) {
                    found = true;
                    break;
                 }
              } catch (NumberFormatException ex) {
                 //Not an id, perhaps an AR name
                 if (ar.getName().equals(arStr)) {
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
         for (AdvanceReservation ar : new ArrayList<AdvanceReservation>(ars)) {
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
      //Finally delete all matched ARs
      List<JGDIAnswer> answers = new ArrayList<JGDIAnswer>();
      for (AdvanceReservation ar : ars) {
         jgdi.deleteAdvanceReservationWithAnswer(ar, answers);
      }
      pw.println("_exit_code="+printAnswers(answers, pw)+"_");
   }
}