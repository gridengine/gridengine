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
package com.sun.grid.jgdi.event;

import com.sun.grid.jgdi.JGDI;
import com.sun.grid.jgdi.JGDIException;
import com.sun.grid.jgdi.JGDIFactory;
import com.sun.grid.jgdi.EventClient;
import java.text.DateFormat;

/**
 *
 * @author  richard.hierlmeier@sun.com
 */
public class QEvent implements EventListener {
   
   private JGDI jgdi;
   private EventClient evc;
   
   public QEvent(String url) throws JGDIException {
      
      jgdi = JGDIFactory.newInstance(url);
      
      evc = JGDIFactory.createEventClient(jgdi, 0);
      evc.addEventListener(this);
      evc.subscribeAll();
      
   }
   
   public void start() throws InterruptedException {
      evc.start();
   }
   
   public static void main(String[] args) {
      
      try {
         QEvent qevt = new QEvent(args[0]);
         
         qevt.start();
         
         Runtime.getRuntime().addShutdownHook( qevt.new ShutdownHandler() );
         
         Thread.currentThread().sleep(Integer.MAX_VALUE);
      } catch( Exception e) {
         e.printStackTrace();
      }
      
   }
   
   
   class ShutdownHandler extends Thread {
      
      public void run() {
         try {
            System.out.println("close event client");
            evc.close();
            System.out.println("close jgdi");
            jgdi.close();
         } catch ( Exception e ) {
            
            e.printStackTrace();
         }
      }
   }
   
   
   
   
   
   private static void usage() {
      
      
   }
   
   private static DateFormat df = DateFormat.getTimeInstance(DateFormat.SHORT);
   
   public void eventOccured(Event evt) {
      
      System.out.println(evt);
      System.out.flush();
      
   }
   
}
