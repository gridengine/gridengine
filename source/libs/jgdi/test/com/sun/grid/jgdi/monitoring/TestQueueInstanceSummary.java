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
package com.sun.grid.jgdi.monitoring;

import com.sun.grid.jgdi.JGDI;
import com.sun.grid.jgdi.monitoring.filter.QueueFilter;
import com.sun.grid.jgdi.monitoring.filter.QueueStateFilter;
import com.sun.grid.jgdi.monitoring.filter.ResourceAttributeFilter;
import java.io.PrintWriter;
import java.util.Iterator;
import java.util.List;
import java.util.logging.Level;
import junit.framework.Test;
import junit.framework.TestSuite;
/*
 * TestQueueInstanceSummary.java
 *
 * Created on February 3, 2006, 1:02 PM
 *
 * To change this template, choose Tools | Options and locate the template under
 * the Source Creation and Management node. Right-click the template and choose
 * Open. You can then make changes to the template in the Source Editor.
 */

/**
 *
 * @author rh150277
 */
public class TestQueueInstanceSummary extends com.sun.grid.jgdi.BaseTestCase  {
    
    public TestQueueInstanceSummary(String testName) {
        super(testName);
    }
    
    protected void setUp() throws Exception {
        System.loadLibrary( "jgdi" );
        super.setUp();
    }
    
    public static Test suite() {
        TestSuite suite = new TestSuite( TestQueueInstanceSummary.class);
        return suite;
    }
    
    public void testFullOutput() throws Exception {
        
        JGDI jgdi = createJGDI();
        try {
            QueueInstanceSummaryOptions options = new QueueInstanceSummaryOptions();
            
            options.setShowFullOutput(true);
            
            QueueInstanceSummaryResult result = jgdi.getQueueInstanceSummary(options);
            if(logger.isLoggable(Level.FINE)) {
                PrintWriter pw = new PrintWriter(System.out);
                QueueInstanceSummaryPrinter.print(pw, result, options);
                pw.flush();
            }
        } finally {
            jgdi.close();
        }
    }
    
    
    public void testAllResourceAttributes() throws Exception {
        
        JGDI jgdi = createJGDI();
        try {
            QueueInstanceSummaryOptions options = new QueueInstanceSummaryOptions();
            
            ResourceAttributeFilter raf = new ResourceAttributeFilter();
            options.setResourceAttributeFilter(raf);
            
            QueueInstanceSummaryResult result = jgdi.getQueueInstanceSummary(options);
            if(logger.isLoggable(Level.FINE)) {
                PrintWriter pw = new PrintWriter(System.out);
                QueueInstanceSummaryPrinter.print(pw, result, options);
                pw.flush();
            }
        } finally {
            jgdi.close();
        }
    }
    
    
    
    public void testQueueStateFilterSuspended() throws Exception {
        
        QueueInstanceSummaryOptions options = new QueueInstanceSummaryOptions();
        options.setShowFullOutput(true);
        
        JGDI jgdi = createJGDI();
        try {
            
            QueueInstanceSummaryResult result = jgdi.getQueueInstanceSummary(options);
            
            List qiList = result.getQueueInstanceSummary();
            
            
            QueueStateFilter qsf = new QueueStateFilter();
            qsf.setSuspended(true);
            options.setQueueStateFilter(qsf);
            
            Iterator qiIter = qiList.iterator();
            while(qiIter.hasNext()) {
                
                QueueInstanceSummary qis = (QueueInstanceSummary)qiIter.next();
                
                logger.fine("Suspend queue " + qis.getName());
                
                jgdi.suspendQueues( new String[] { qis.getName() }, false );
                
                
                result = jgdi.getQueueInstanceSummary(options);
                
                if(logger.isLoggable(Level.FINE)) {
                    PrintWriter pw = new PrintWriter(System.out);
                    QueueInstanceSummaryPrinter.print(pw, result, options);
                    pw.flush();
                }
                
                Iterator iter = result.getQueueInstanceSummary().iterator();
                
                
                boolean found = false;
                while(iter.hasNext()) {
                    
                    QueueInstanceSummary susQis = (QueueInstanceSummary)iter.next();
                    assertTrue("exepect state 's' for queue instance " + susQis , susQis.getState().indexOf('s') >= 0);
                    
                    if(qis.getName().equals(susQis.getName())) {
                        found = true;
                    }
                }
                assertTrue("suspended queue instance " + qis.getName() + " is not in result", found);
                
                logger.fine("Unsuspend queue " + qis.getName());
                jgdi.unsuspendQueues( new String[] { qis.getName() }, false );
                
            }
            
            
            
        } finally {
            jgdi.close();
        }
    }
    
    public void testQueueStateFilterDisabled() throws Exception {
        
        QueueInstanceSummaryOptions options = new QueueInstanceSummaryOptions();
        options.setShowFullOutput(true);
        QueueStateFilter qsf = new QueueStateFilter();
        qsf.setDisabled(true);
        options.setQueueStateFilter(qsf);
        
        JGDI jgdi = createJGDI();
        try {
            
            QueueInstanceSummaryResult result = jgdi.getQueueInstanceSummary(options);
            
            List qiList = result.getQueueInstanceSummary();
            
            // Disable all queues
            jgdi.disableQueues( new String[] { "*" }, false);
            
            result = jgdi.getQueueInstanceSummary(options);
            
            List newQiList = result.getQueueInstanceSummary();
            
            Iterator iter = qiList.iterator();
            while(iter.hasNext()) {
                QueueInstanceSummary qis = (QueueInstanceSummary)iter.next();
                
                boolean found = false;
                Iterator newIter = newQiList.iterator();
                while(newIter.hasNext()) {
                    QueueInstanceSummary qis1 = (QueueInstanceSummary)newIter.next();
                    
                    if(qis.getName().equals(qis1.getName())) {
                        found = true;
                        break;
                    }
                }
                assertTrue("queue instance " + qis.getName() + " has not been found in disabled queue list", found);
            }
            
            jgdi.enableQueues( new String[] { "*" }, true);
            
            result = jgdi.getQueueInstanceSummary(options);
            
            newQiList = result.getQueueInstanceSummary();
            
            if(logger.isLoggable(Level.FINE)) {
                PrintWriter pw = new PrintWriter(System.out);
                QueueInstanceSummaryPrinter.print(pw, result, options);
                pw.flush();
            }
            
            assertEquals("got queue instances, but all queues are disabled", 0, newQiList.size());
            
            
        } finally {
            jgdi.close();
        }
        
    }
    
    public void testQueueFilter() throws Exception {
        
        QueueInstanceSummaryOptions options = new QueueInstanceSummaryOptions();
        options.setShowFullOutput(true);
        
        JGDI jgdi = createJGDI();
        
        try {
            QueueInstanceSummaryResult result = jgdi.getQueueInstanceSummary(options);
            
            List qiList = result.getQueueInstanceSummary();
            
            Iterator iter = qiList.iterator();
            while(iter.hasNext()) {
                QueueInstanceSummary qis = (QueueInstanceSummary)iter.next();
                
                logger.fine("Filter for queue instance " + qis.getName());
                QueueFilter qf = new QueueFilter();
                qf.addQueue(qis.getName());
                options.setQueueFilter(qf);
                
                result = jgdi.getQueueInstanceSummary(options);
                
                if(logger.isLoggable(Level.FINE)) {
                    PrintWriter pw = new PrintWriter(System.out);
                    QueueInstanceSummaryPrinter.print(pw, result, options);
                    pw.flush();
                }
                
                Iterator newIter = result.getQueueInstanceSummary().iterator();
                assertTrue("Missing queue instance " + qis.getName(), newIter.hasNext());
                
                while(newIter.hasNext()) {
                    QueueInstanceSummary newQis = (QueueInstanceSummary)newIter.next();
                    assertEquals("Queue instance " + newQis.getName() + " is not in queue filter",
                            qis.getName(), newQis.getName() );
                }
            }
            
        } finally {
            jgdi.close();
        }
        
        
    }
    
    
}
