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

import java.beans.BeanInfo;
import java.beans.IntrospectionException;
import java.beans.Introspector;
import java.beans.PropertyDescriptor;
import java.io.PrintWriter;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.text.DateFormat;
import java.text.DecimalFormat;
import java.text.Format;
import java.text.MessageFormat;
import java.text.SimpleDateFormat;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;

/**
 *
 * @author rh150277
 */
public class QueueInstanceSummaryPrinter {
   
   private final static String HASHES = "##############################################################################################################";
   
   /* regular output */
   private final static String JHUL1 = "---------------------------------------------------------------------------------------------";
   /* -g t */
   private final static String JHUL2 = "-";
   /* -ext */
   private final static String JHUL3 = "-------------------------------------------------------------------------------";
   /* -t */
   private final static String JHUL4 = "-----------------------------------------------------";
   /* -urg */
   private final static String JHUL5 = "----------------------------------------------------------------";
   /* -pri */
   private final static String JHUL6 = "-----------------------------------";
   
   private boolean job_header_printed;
   private int last_job_id;
   private String lastQueue;
   
   public static class Table {
      private List columns = new LinkedList();
      
      private Class clazz;
      
      int rowWidth = -1;
      
      
      public Table(Class clazz) {
         this.clazz = clazz;
      }
      
      public Column addCol(String name, String header) throws IntrospectionException {
         return addCol(name, header, 0);
      }
      
      
      
      public Column addCol(String name, String header, int width) throws IntrospectionException {
         Column col = new PropertyColumn(name, header, width);
         addCol(col);
         return col;
      }
      
      public Column addCol(String name, String header, int width, int alignment) throws IntrospectionException {
         Column col = new PropertyColumn(name, header, width, alignment);
         addCol(col);
         return col;
      }
      
      public Column addCol(String name, String header, int width, int alignment, Calc calc) throws IntrospectionException {
         Column col = new CalcColumn(name, header, width, alignment, calc);
         addCol(col);
         return col;
      }
      
      
      public Column addCol(String name, String header, int width, Format format) throws IntrospectionException {
         Column col = new PropertyColumn(name, header, width, format);
         addCol(col);
         return col;
      }
      
      public Column addCol(String name, String header, int width, Format format, Calc calc) throws IntrospectionException {
         Column col = new CalcColumn(name, header, width, format, calc);
         addCol(col);
         return col;
      }
      
      
      public Column addCol(String name, String header, int width, Calc calc) throws IntrospectionException {
         Column col = new CalcColumn(name, header, width, calc);
         addCol(col);
         return col;
      }
      
      public void addCol(Column col) {
         columns.add(col);
         rowWidth = -1;
      }
      
      public void printHeader(PrintWriter pw) {
         Iterator iter = columns.iterator();
         while(iter.hasNext()) {
            Column col = (Column)iter.next();
            col.printStr(pw, col.getHeader());
            pw.print(' ');
         }
         pw.println();
      }
      
      
      
      public int getRowWidth() {
         if(rowWidth < 0) {
            int ret = 0;
            Iterator iter = columns.iterator();
            while(iter.hasNext()) {
               Column col = (Column)iter.next();
               ret += col.getWidth() + 1;
            }
            rowWidth = ret;
         }
         return rowWidth;
      }
      
      public void printRow(PrintWriter pw, Object obj) {
         Iterator iter = columns.iterator();
         while(iter.hasNext()) {
            ((Column)iter.next()).print(pw, obj);
            pw.print(' ');
         }
         pw.println();
      }
      
      public void printDelimiter(PrintWriter pw, char del) {
         for(int i = 0; i < getRowWidth(); i++) {
            pw.print(del);
         }
         pw.println();
      }
      
      public abstract class Column {
         
         public static final int DEFAULT_COL_WIDTH = 10;
         
         String name;
         private String header;
         private Format format;
         private int width;
         
         public static final int RIGHT  = 0;
         public static final int CENTER = 1;
         public static final int LEFT   = 2;
         
         
         private int alignment = LEFT;
         private Method getter = null;
         
         public Column(String name, String header) {
            this(name, header, DEFAULT_COL_WIDTH);
         }
         
         public Column(String name, String header, int width) {
            this(name, header, width, LEFT);
         }
         
         public Column(String name, String header, int width, int alignment) {
            this(name, header, width, alignment, null);
         }
         
         public Column(String name, String header, Format format) {
            this(name, header);
            this.setFormat(format);
         }
         
         public Column(String name, String header, int width, int alignment,  Format format) {
            this.name = name;
            this.setHeader(header);
            this.setWidth(width);
            this.alignment = alignment;
            this.setFormat(format);
         }
         
         
         
         public String getName() {
            return name;
         }
         
         
         public abstract Object getValue(Object obj);
         
         
         public void printStr(PrintWriter pw, String str) {
            
            int spaceCount = getWidth() - str.length();
            
            if(spaceCount == 0) {
               pw.print(str);
            } else {
               switch(getAlignment()) {
                  case RIGHT:
                     if (spaceCount > 0) {
                        for(int i = 0; i < spaceCount; i++) {
                           pw.print(' ');
                        }
                        pw.print(str);
                     } else { /* spaceCount < 0 */
                        str = str.substring(Math.abs(spaceCount));
                        pw.print(str);
                     }
                     break;
                  case CENTER:
                     if (spaceCount > 0) {
                        int prefix = spaceCount / 2;
                        for(int i = 0; i < prefix; i++) {
                           pw.print(' ');
                        }
                        pw.print(str);
                        spaceCount -= prefix;
                        for(int i = 0; i < spaceCount; i++) {
                           pw.print(' ');
                        }
                     } else {
                        str = str.substring(Math.abs(spaceCount));
                        pw.print(str);
                     }
                  case LEFT:
                     if(spaceCount > 0) {
                        pw.print(str);
                        for(int i = 0; i < spaceCount; i++) {
                           pw.print(' ');
                        }
                     } else {
                        str = str.substring(0, str.length() - Math.abs(spaceCount));
                        pw.print(str);
                     }
               }
            }
         }
         
         
         public void print(PrintWriter pw, Object obj) {
            Object value = getValue(obj);
            String str = null;
            if(value == null) {
               str = "";
            } else {
               if(getFormat() == null) {
                  str = value.toString();
               } else {
                   try {
                  str = getFormat().format(value);
                   } catch(RuntimeException e) {
                       System.err.println("format error in colum " + getName() + " formatter can not format value " + value);
                       throw e;
                   }
               }
            }
            printStr(pw, str);
         }
         
         public int getAlignment() {
            return alignment;
         }
         
         public void setAlignment(int alignment) {
            this.alignment = alignment;
         }
         
         public Format getFormat() {
            return format;
         }
         
         public void setFormat(Format format) {
            this.format = format;
         }
         
         public String getHeader() {
            return header;
         }
         
         public void setHeader(String header) {
            this.header = header;
         }
         
         public int getWidth() {
            return width;
         }
         
         public void setWidth(int width) {
            this.width = width;
         }
      }
      
      class PropertyColumn extends Column {
         
         private Method getter = null;
         
         public PropertyColumn(String name, String header) {
            this(name, header, DEFAULT_COL_WIDTH);
         }
         
         public PropertyColumn(String name, String header, int width) {
            this(name, header, width, LEFT);
         }
         
         public PropertyColumn(String name, String header, int width, int alignment) {
            this(name, header, width, alignment, null);
         }
         
         public PropertyColumn(String name, String header, Format format) {
            this(name, header);
            this.setFormat(format);
         }
         
         public PropertyColumn(String name, String header, int width, Format format) {
            this(name, header, width);
            this.setFormat(format);
         }
         
         public PropertyColumn(String name, String header, int width, int alignment,  Format format) {
            super(name, header, width, alignment, format);
            BeanInfo beanInfo = null;
            
            Class aClass = clazz;
            
            outer:
               while(getter == null && aClass != null) {
                  getter = getGetter(name, aClass);
                  if(getter == null) {
                     Class [] interfaces = aClass.getInterfaces();
                     for(int i = 0; i < interfaces.length; i++) {
                        getter = getGetter(name, interfaces[i]);
                        if(getter != null) {
                           break outer;
                        }
                     }
                     aClass = aClass.getSuperclass();
                  }
               }
               if(getter == null) {
                  throw new IllegalStateException("getter for " + name + " not found in class " + clazz.getName());
               }
         }
         
         private Method getGetter(String name, Class aClass) {
            BeanInfo beanInfo = null;
            try {
               beanInfo = Introspector.getBeanInfo(aClass);
            } catch (IntrospectionException ex) {
               IllegalStateException ex1 = new IllegalStateException("Can not introspec class " + clazz.getName());
               ex1.initCause(ex);
               throw ex1;
            }
            PropertyDescriptor [] props = beanInfo.getPropertyDescriptors();
            for(int i = 0; i < props.length; i++) {
               if(props[i].getName().equalsIgnoreCase(name)) {
                  getter = props[i].getReadMethod();
                  if(getter == null) {
                     throw new IllegalStateException("property " + name + " has not read method");
                  }
                  return getter;
               }
            }
            return null;
         }
         
         
         public Object getValue(Object obj) {
            try {
               return getter.invoke(obj, null);
            } catch (IllegalAccessException ex) {
               IllegalStateException ex1 = new IllegalStateException("No access in property " + clazz.getName() + "." + getName());
               ex1.initCause(ex);
               throw ex1;
            } catch (InvocationTargetException ex) {
               IllegalStateException ex1 = new IllegalStateException("Error in getter " + clazz.getName() + "." + getName());
               ex1.initCause(ex.getTargetException());
               throw ex1;
            }
         }
         
      }
      
      
      class CalcColumn extends Column {
         
         private Calc calc;
         
         public CalcColumn(String name, String header, Calc calc) {
            this(name, header, DEFAULT_COL_WIDTH, calc);
         }
         
         public CalcColumn(String name, String header, int width, Calc calc) {
            this(name, header, width, LEFT, calc);
         }
         
         public CalcColumn(String name, String header, int width, int alignment, Calc calc) {
            this(name, header, width, alignment, null, calc);
         }
         
         public CalcColumn(String name, String header, Format format, Calc calc) {
            this(name, header, calc);
            this.setFormat(format);
         }
         
         public CalcColumn(String name, String header, int width,  Format format, Calc calc) {
            super(name, header, width);
            setFormat(format);
            this.calc = calc;
         }
         
         public CalcColumn(String name, String header, int width, int alignment,  Format format, Calc calc) {
            super(name, header, width, alignment, format);
            this.calc = calc;
         }
         
         public Object getValue(Object obj) {
            return calc.getValue(obj);
         }
         
      }
      
      interface Calc {
         public Object getValue(Object obj);
      }
   }
   
   public static final DecimalFormat DEFAULT_NUMBER_FORMAT = new DecimalFormat("#####0");
   public static final DecimalFormat POINT_SIX_FORMAT = new DecimalFormat("####0.000000");
   
   public static Table createJobSummaryTable(QueueInstanceSummaryOptions options) throws IntrospectionException {
      boolean sge_urg, sge_pri, sge_ext, sge_time, tsk_ext;
      boolean print_job_id;
      
      
      sge_ext = options.showAdditionalAttributes();
      tsk_ext = options.showExtendedSubTaskInfo();
      sge_urg = options.showJobUrgency();
      sge_pri = options.showJobPriorities();
      sge_time = !sge_ext;
      sge_time = sge_time | tsk_ext | sge_urg | sge_pri;
      
      Table table = new Table(JobSummary.class);
      
      table.addCol("id", "job-ID ", 7, Table.Column.RIGHT);
      table.addCol("priority", "prior", 7);
      if (sge_pri||sge_urg) {
         table.addCol("normalizedUrgency","nurg", 9);
      }
      if (sge_pri) {
         table.addCol("normalizedRequestedPriority", "npprior", 10);
      }
      if (sge_pri||sge_ext) {
         table.addCol("normalizedTickets","ntckts", 6);
      }
      if (sge_urg) {
         table.addCol("urgency","urg", 8);
         table.addCol("rrcontr","rrcontr", 8);
         table.addCol("wtcontr","wtcontr", 8);
         table.addCol("wtcontr","dlcontr", 8);
      }
      if (sge_pri) {
         table.addCol("priority","ppri", 7);
      }
      table.addCol("name", "name", 10);
      table.addCol("user", "user", 12);
      if (sge_ext) {
         table.addCol("project", "project", 15);
         table.addCol("department", "department", 15);
      }
      table.addCol("state", "state", 5);
      if (sge_time) {
         
         Table.Calc timeCalc = new Table.Calc() {
            public Object getValue(Object obj) {
               JobSummary js = (JobSummary)obj;
               if(js.isRunning()) {
                  return js.getStartTime();
               } else {
                  return js.getSubmitTime();
               }
            }
         };
         table.addCol("submitTime", "submit/start at     ", 19, DEFAULT_DATE_FORMAT, timeCalc);
      }
      if(sge_urg) {
         table.addCol("deadline", "deadline", 15);
      }
      if(sge_ext) {
         table.addCol("cpuUsage", "cpu", 8, new CpuUsageCalc());
         table.addCol("memUsage", "mem", 7, Table.Column.RIGHT, new MemUsageCalc());
         table.addCol("ioUsage", "io", 7, Table.Column.RIGHT, new IOUsageCalc());
         table.addCol("tickets", "tckts", 8, new TicketCalc(sge_ext) {
            public long getValue(JobSummary js) {
               return js.getTickets();
            }
         });
         table.addCol("overrideTickets",  "ovrts", 8, new TicketCalc(sge_ext) {
            public long getValue(JobSummary js) {
               return js.getOverrideTickets();
            }
         });
         table.addCol("otickets", "otckt", 8, new TicketCalc(sge_ext) {
            public long getValue(JobSummary js) {
               return js.getOtickets();
            }
         });
         table.addCol("ftickets", "ftckt", 8,new TicketCalc(sge_ext) {
            public long getValue(JobSummary js) {
               return js.getFtickets();
            }
         });
         table.addCol("stickets", "stckt", 8, new TicketCalc(sge_ext) {
            public long getValue(JobSummary js) {
               return js.getStickets();
            }
         });
         
         table.addCol("share", "share", 8, new ShareCalc(sge_ext));
      }
      //table.addCol("queueAssigned", "qs", 5);
      if(!options.showFullOutput()) {
         table.addCol("queue", "queue", 30);
      }
      // if(options.showPEJobs()) {
      if (options.showExtendedSubTaskInfo()) {
         table.addCol("masterQueue", "master", 8, Table.Column.LEFT);
      } else {
         table.addCol("slots", "slots", 8, Table.Column.RIGHT);
      }
      table.addCol("ja-taskId", "ja-task-ID", 10, new JaTaskIdCalc());
      
      
      if (tsk_ext) {
         Table.Calc fixedValue = new Table.Calc() {
            public Object getValue(Object obj) {
               return "   NA ";
            }
         };
         table.addCol("taskId", "task-ID ", 7, fixedValue);
         table.addCol("state", "state" , 6, new StatCalc() );
         // TODO retrieve values from first task
         table.addCol("cpuUsage", "cpu", 8, fixedValue);
         table.addCol("memUsage", "mem", 7, Table.Column.RIGHT, fixedValue);
         table.addCol("ioUsage", "io", 7, Table.Column.RIGHT, fixedValue);
         table.addCol("stat", "stat", 5, fixedValue);
         table.addCol("failed", "failed", 7, fixedValue);
      }
      return table;
   }
   
   private static class StatCalc implements Table.Calc {
      
      public Object getValue(Object obj) {
         JobSummary js = (JobSummary)obj;
         
         List tl = js.getTaskList();
         
         if (tl.isEmpty()) {
            return "";
         } else {
            TaskSummary ts = (TaskSummary)tl.get(0);
            return ts.getState();
         }
      }
   }
   
   private static class CpuUsageCalc implements Table.Calc {
      public Object getValue(Object obj) {
         JobSummary js = (JobSummary)obj;
         if(!js.hasCpuUsage()) {
            if(js.isRunning()) {
               return "NA";
            } else {
               return "";
            }
         } else {
            int secs, minutes, hours, days;
            
            secs = js.getCpuUsage();
            
            days    = secs/(60*60*24);
            secs   -= days*(60*60*24);
            
            hours   = secs/(60*60);
            secs   -= hours*(60*60);
            
            minutes = secs/60;
            secs   -= minutes*60;
            
            return MessageFormat.format("{0,number}:{1,number,00}:{1,number,00}:{1,number,00}",
                    new Object [] {
               new Integer(days),
               new Integer(hours),
               new Integer(minutes),
               new Integer(secs)
            });
         }
      }
   }
   
   
   private abstract static class UsageCalc implements Table.Calc {
      
      private DecimalFormat format = new DecimalFormat("#.00000");
      
      protected abstract boolean hasValue(JobSummary js);
      protected abstract double getValue(JobSummary js);
      
      public Object getValue(Object obj) {
         JobSummary js = (JobSummary)obj;
         if(!hasValue(js)) {
            if(js.isRunning()) {
               return "NA";
            } else {
               return "";
            }
         } else {
            return format.format(getValue(js));
         }
      }
   }
   
   private static class MemUsageCalc extends UsageCalc {
      protected boolean hasValue(JobSummary js) {
         return js.hasMemUsage();
      }
      
      protected double getValue(JobSummary js) {
         return js.getMemUsage();
      }
   }
   
   private static class IOUsageCalc extends UsageCalc {
      protected boolean hasValue(JobSummary js) {
         return js.hasIoUsage();
      }
      
      protected double getValue(JobSummary js) {
         return js.getIoUsage();
      }
   }
   
   private static abstract class TicketCalc implements Table.Calc {
      
      private DecimalFormat format = new DecimalFormat("###00");
      
      private boolean sge_ext;
      
      public TicketCalc(boolean sge_ext) {
         this.sge_ext = sge_ext;
      }
      
      protected abstract long getValue(JobSummary js);
      
      public Object getValue(Object obj) {
         JobSummary js = (JobSummary)obj;
         if(js.isZombie()) {
            return "NA ";
         } else {
            if(sge_ext || js.isQueueAssigned()) {
               return format.format(getValue(js));
            } else {
               return "";
            }
         }
      }
   }
   
   private static class ShareCalc implements Table.Calc {
      
      private DecimalFormat format = new DecimalFormat("###.00");
      
      private boolean sge_ext;
      
      public ShareCalc(boolean sge_ext) {
         this.sge_ext = sge_ext;
      }
      
      public Object getValue(Object obj) {
         JobSummary js = (JobSummary)obj;
         if(js.isZombie()) {
            return "NA ";
         } else {
            if(sge_ext || js.isQueueAssigned()) {
               return format.format(js.getShare());
            } else {
               return "";
            }
         }
      }
   }
   
   private static class JaTaskIdCalc implements Table.Calc {
      
      private DecimalFormat format = new DecimalFormat("###.00");
      
      public Object getValue(Object obj) {
         JobSummary js = (JobSummary)obj;
         if(js.isArray() && js.getTaskId() != null) {
            return js.getTaskId();
         } else {
            return "";
         }
      }
   }
   
   
   private static final  DateFormat DEFAULT_DATE_FORMAT = new SimpleDateFormat("MM/dd/yyyy HH:mm:ss");
   
   private static Table createQueueInstanceSummaryTable() throws IntrospectionException {
      
      Table ret = new Table(QueueInstanceSummary.class);
      
      ret.addCol("name", "queuename", 30);
      ret.addCol("queueType", "qtype", 5);
      ret.addCol("usedSlots", "used/tot.", 4);
      
      Table.Calc slotCalc = new Table.Calc() {
         
         public Object getValue(Object obj) {
            
            QueueInstanceSummary qi = (QueueInstanceSummary)obj;
            
            StringBuffer ret = new StringBuffer();
            ret.append(qi.getUsedSlots());
            ret.append('/');
            ret.append(qi.getFreeSlots() + qi.getUsedSlots());
            return ret.toString();
         }
      };
      
      ret.addCol("totalSlots", "used/tot.", 4, slotCalc);
      
      Table.Calc loadAvgCalc = new Table.Calc() {
         
         public Object getValue(Object obj) {
            
            QueueInstanceSummary qi = (QueueInstanceSummary)obj;
            
            if(qi.hasLoadValue()) {
               return new Double(qi.getLoadAvg());
            } else {
               return "-NA-";
            }
         }
      };
      
      ret.addCol("load_avg", "load_avg", 8, loadAvgCalc);
      ret.addCol("arch", "arch", 22);
      ret.addCol("state", "states", 10);
      
      return ret;
      
   }
   
   public static void print(PrintWriter pw, QueueInstanceSummaryResult result, QueueInstanceSummaryOptions options) {
      Table jobSummaryTable = null;
      Table qiTable = null;
      try {
         jobSummaryTable = createJobSummaryTable(options);
         qiTable = createQueueInstanceSummaryTable();
      } catch (IntrospectionException ex) {
         IllegalStateException ex1 = new IllegalStateException("intospection error");
         ex1.initCause(ex);
         throw ex1;
      }
      
      Iterator iter = result.getQueueInstanceSummary().iterator();
      
      boolean hadJobs = false;
      if(iter.hasNext()) {
         if(options.showFullOutput()) {
            qiTable.printHeader(pw);
         }
         while(iter.hasNext()) {
            if(options.showFullOutput()) {
               qiTable.printDelimiter(pw, '-');
            }
            QueueInstanceSummary qi = (QueueInstanceSummary)iter.next();
            if(options.showFullOutput()) {
               qiTable.printRow(pw, qi);
               
               Iterator domIter = qi.getResourceDominanceSet().iterator();
               while(domIter.hasNext()) {
                  String dom = (String)domIter.next();
                  Iterator resIter = qi.getResourceNames(dom).iterator();
                  while(resIter.hasNext()) {
                     String name = (String)resIter.next();
                     pw.print("    dom:name=");
                     pw.println(qi.getResourceValue(dom, name));
                  }
               }
            }
            Iterator jobIter = qi.getJobList().iterator();
            if(jobIter.hasNext()) {
               while(jobIter.hasNext()) {
                  if(!hadJobs && !options.showFullOutput()) {
                     jobSummaryTable.printHeader(pw);
                     jobSummaryTable.printDelimiter(pw, '-');
                  }
                  JobSummary js = (JobSummary)jobIter.next();
                  jobSummaryTable.printRow(pw, js);
                  if(options.showRequestedResourcesForJobs()) {
                     printRequestedResources(pw, js);
                  }
                  hadJobs = true;
               }
            }
         }
      }
      
      List jobList = result.getPendingJobs();
      if (!jobList.isEmpty()) {
         if(options.showFullOutput()) {
            pw.println();
            qiTable.printDelimiter(pw, '#');
            pw.println(" - PENDING JOBS - PENDING JOBS - PENDING JOBS - PENDING JOBS - PENDING JOBS");
            qiTable.printDelimiter(pw, '#');
         }
         
         iter = jobList.iterator();
         if(!hadJobs && !options.showFullOutput()) {
            jobSummaryTable.printHeader(pw);
            jobSummaryTable.printDelimiter(pw, '-');
         }
         while(iter.hasNext()) {
            JobSummary js = (JobSummary)iter.next();
            jobSummaryTable.printRow(pw, js);
            if(options.showRequestedResourcesForJobs()) {
               printRequestedResources(pw, js);
            }
            hadJobs = true;            
         }
      }
      
      jobList = result.getErrorJobs();
      if (!jobList.isEmpty()) {
         if(options.showFullOutput()) {
            pw.println();
            qiTable.printDelimiter(pw, '#');
            pw.println("     - ERROR JOBS - ERROR JOBS - ERROR JOBS - ERROR JOBS - ERROR JOBS");
            qiTable.printDelimiter(pw, '#');
         }
         if(!hadJobs && !options.showFullOutput()) {
            jobSummaryTable.printHeader(pw);
            jobSummaryTable.printDelimiter(pw, '-');
         }
         iter = jobList.iterator();
         while(iter.hasNext()) {
            JobSummary js = (JobSummary)iter.next();
            jobSummaryTable.printRow(pw, js);
            if(options.showRequestedResourcesForJobs()) {
               printRequestedResources(pw, js);
            }
            hadJobs = true;
         }
      }
      
      jobList = result.getFinishedJobs();
      if (!jobList.isEmpty()) {
         if(options.showFullOutput()) {
            pw.println();
            qiTable.printDelimiter(pw, '#');
            pw.println("- FINISHED JOBS - FINISHED JOBS - FINISHED JOBS - FINISHED JOBS - FINISHED JOBS");
            qiTable.printDelimiter(pw, '#');
         }
         iter = jobList.iterator();
         if(!hadJobs && !options.showFullOutput()) {
            jobSummaryTable.printHeader(pw);
            jobSummaryTable.printDelimiter(pw, '-');
         }
         while(iter.hasNext()) {
            JobSummary js = (JobSummary)iter.next();
            jobSummaryTable.printRow(pw, js);
            if(options.showRequestedResourcesForJobs()) {
               printRequestedResources(pw, js);
            }
            hadJobs = true;
         }
      }
      
      jobList = result.getZombieJobs();
      if (!jobList.isEmpty()) {
         if(options.showFullOutput()) {
            pw.println();
            qiTable.printDelimiter(pw, '#');
            pw.println("   - ZOMBIE JOBS - ZOMBIE JOBS - ZOMBIE JOBS - ZOMBIE JOBS - ZOMBIE JOBS");
            qiTable.printDelimiter(pw, '#');
         }
         iter = jobList.iterator();
         if(!hadJobs && !options.showFullOutput()) {
            jobSummaryTable.printHeader(pw);
            jobSummaryTable.printDelimiter(pw, '-');
         }
         while(iter.hasNext()) {
            JobSummary js = (JobSummary)iter.next();
            jobSummaryTable.printRow(pw, js);
            if(options.showRequestedResourcesForJobs()) {
               printRequestedResources(pw, js);
            }
            hadJobs = true;
         }
      }
      
   }
   
   private static void printRequestedResources(PrintWriter pw, JobSummary js) {
      pw.print("      Full jobname: ");
      pw.println(js.getName());
      if(js.getMasterQueue() != null) {
         pw.print("      Master Queue: ");
         pw.println(js.getMasterQueue());
      }
      
      if(js.getGrantedPEName() != null) {
         pw.print("      Requested PE: ");
         pw.println(js.getParallelEnvironmentName() + " " + js.getParallelEnvironmentRange());
         pw.print("      Granted PE: ");
         pw.print( js.getGrantedPEName() );
         pw.print(" ");
         pw.println( js.getGrantedPESlots());
      }
      
      pw.print("      Hard Resources: ");
      Iterator iter = js.getHardRequestNames().iterator();
      boolean firstRes = true;
      while(iter.hasNext()) {
         if(firstRes) {
            firstRes = false;
         } else {
            pw.print(", ");
         }
         String resName = (String)iter.next();
         pw.print(resName);
         pw.print("=");
         HardRequestValue value = js.getHardRequestValue(resName);
         pw.print(value.getValue());
         pw.print(" (");
         pw.print(POINT_SIX_FORMAT.format(value.getContribution()));
         pw.print(")");
      }
      pw.println();
      pw.print("      Soft Resources: ");
      iter = js.getSoftRequestNames().iterator();
      firstRes = true;
      while(iter.hasNext()) {
         if(firstRes) {
            firstRes = false;
         } else {
            pw.print(", ");
         }
         String resName = (String)iter.next();
         pw.print(resName);
         pw.print("=");
         pw.print(js.getSoftRequestValue(resName));
      }
      pw.println();
   }
}
