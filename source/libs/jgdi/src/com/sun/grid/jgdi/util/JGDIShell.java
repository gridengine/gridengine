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
package com.sun.grid.jgdi.util;

import com.sun.grid.jgdi.JGDI;
import com.sun.grid.jgdi.JGDIException;
import com.sun.grid.jgdi.JGDIFactory;
import com.sun.grid.jgdi.configuration.GEObject;
import com.sun.grid.jgdi.configuration.xml.XMLUtil;
import com.sun.grid.jgdi.util.shell.Command;
import com.sun.grid.jgdi.util.shell.CommandAnnotation;
import com.sun.grid.jgdi.util.shell.HistoryCommand;
import com.sun.grid.jgdi.util.shell.Shell;
import java.io.BufferedReader;
import java.io.EOFException;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.security.PrivilegedAction;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.ResourceBundle;
import java.util.TreeSet;
import java.util.logging.Level;
import java.util.logging.LogManager;
import java.util.logging.Logger;
import javax.security.auth.Subject;
import javax.security.auth.callback.Callback;
import javax.security.auth.callback.CallbackHandler;
import javax.security.auth.callback.ConfirmationCallback;
import javax.security.auth.callback.NameCallback;
import javax.security.auth.callback.PasswordCallback;
import javax.security.auth.callback.TextOutputCallback;
import javax.security.auth.callback.UnsupportedCallbackException;
import javax.security.auth.login.LoginContext;
import javax.security.auth.login.LoginException;
import java.io.File;
import java.net.URI;
import java.net.URL;
import java.util.concurrent.CancellationException;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;

/**
 *
 */
public class JGDIShell implements Runnable, Shell {
   
   private static Logger logger = Logger.getLogger(JGDIShell.class.getName());
   private static String PROMPT = "jgdi> ";
   
   private LinkedList<HistoryElement> historyList = new LinkedList<HistoryElement>();
   private int historyIndex = 0;
   private int maxHistory = 30;
   
   private JGDI jgdi;
  
   private Map<String, Command> cmdMap = new HashMap<String, Command>();
   private TreeSet<String> cmdSet = null;
   private ReadlineHandler readlineHandler;
   private static final ResourceBundle usageResources = ResourceBundle.getBundle("com.sun.grid.jgdi.util.shell.UsageResources");
   private final PrintWriter pw;
   
   //TODO LP: We should consider having single PrintWriter for all commands
   public JGDIShell() {
      pw = new PrintWriter(System.out);
      cmdMap.put("connect", new ConnectCommand());
      cmdMap.put("exit", new ExitCommand());
      cmdMap.put("help", new HelpCommand());
      cmdMap.put("debug", new DebugCommand());
      cmdMap.put("history", new PrintHistoryCommand());
      cmdMap.put("xmldump", new XMLDumpCommand());

      // Register all command classes
      try {
         List<Class> classes = getAllAnnotatedClasses(this.getClass().getPackage(), CommandAnnotation.class);
         for (Class cls : classes) {
            addAnnotatedCommand( cls);
         }
      } catch (Exception ex) {
         pw.println(ex.getMessage());
      }

      cmdSet = new TreeSet<String>(cmdMap.keySet());
      readlineHandler = createReadlineHandler();

      //Read resource bundles
   }

   /**
    * Registe the annotated  class
    * @param cls Annotated class child of AbstractCommand 
    * @throws java.lang.Exception 
    */
   private void addAnnotatedCommand(Class cls) throws Exception {
      @SuppressWarnings("unchecked")
      CommandAnnotation ca = (CommandAnnotation) cls.getAnnotation(CommandAnnotation.class);
      final Object newInstance = cls.newInstance();
      if( newInstance instanceof Command) {
        cmdMap.put(ca.value(), (Command) newInstance);
      } else {
          throw new IllegalStateException("Can not cast "+newInstance.getClass().getName()+" to Command");
      }
   }

   private Command getCommand(String name) throws Exception {
      return cmdMap.get(name);
   }

   /**
    * Getter method
    * @return a <b>JGDI</b> object
    */
   public JGDI getConnection() {
      if (jgdi == null) {
         throw new IllegalStateException("Not connected");
      }
      return jgdi;
   }

   /**
    * Getter method
    * @return a logger
    */
   public Logger getLogger() {
      return logger;
   }

   /**
    * Getter method
    * @return a writer
    */
   public PrintWriter getPrintWriter() {
      if (pw == null) {
         throw new IllegalStateException("pw is not initialized");
      } 
      return pw;
   }

   public void run() {

      try {
         while (true) {
            String line = readlineHandler.readline(PROMPT);
            //Print empty line
            if (line == null) {
               continue;
            }
            runCommand(line);
         }  
      } catch(EOFException eofe) {
         // Ignore
      } catch(IOException ioe) {
         logger.severe(ioe.getMessage());
      }
      readlineHandler.cleanup();
      System.exit(0);
   }
   
   private void runCommand(String line) {
      line = line.trim(); 
      if (line.length() == 0) {
         return;
      }
      try {
         if (line.charAt(0) == '!') {
            // get command from history
            String name = line.substring(1);
            try {
               int id = Integer.parseInt(name);
               line = null;
               Iterator iter = historyList.iterator();
               while (iter.hasNext()) {
                  HistoryElement elem = (HistoryElement) iter.next();
                  if (elem.getId() == id) {
                     line = elem.getLine();
                     break;
                  }
               }
               if (line == null) {
                  throw new IllegalArgumentException("command with id " + id + " not found in history");
               }
            } catch (NumberFormatException nfe) {
               throw new IllegalArgumentException("Expected !<num>, but got: " + line);
            }
         }
         ParsedLine parsedLine = new ParsedLine(line);
         Command cmd = getCommand(parsedLine.cmd);
         if (cmd == null) {
            runShellCommand(line);
            return;
         }

         if (cmd instanceof HistoryCommand) {
            if (historyList.size() > maxHistory) {
               historyList.removeFirst();
            }
            historyList.add(new HistoryElement(++historyIndex, line));
         }

         cmd.init(this);
         cmd.run(parsedLine.args);
      } catch (CancellationException expected) {
      } catch (Exception ex) {
         pw.println(ex.getMessage());
         logger.info("Command failed: " + ex.getMessage());
      } finally {
         pw.flush();
      }
   }
   
   public void runShellCommand(String line) throws InterruptedException, IOException, InterruptedException {
      pw.println("Executing /bin/sh -c " + line);
      pw.flush();
      String[] cmds = {"/bin/sh", "-c", line};
      Process p = Runtime.getRuntime().exec(cmds);
      GetResponse to = new GetResponse(p.getInputStream());
      GetResponse te = new GetResponse(p.getErrorStream());
      to.start();
      te.start();
      p.waitFor();
      to.join();
      te.join();
   }
   
   @SuppressWarnings("unchecked")
   public static void main(String[] args) {
      String url = null;
      boolean useCsp = false;
      String cmdParams = "";
      
      for (int i = 0; i < args.length; i++) {
         if (args[i].equals("-c")) {
            i++;
            if (i>=args.length) {
               logger.severe("Missing connection url");
               System.exit(1);
            }
            url = args[i];
         } else if (args[i].equals("-csp")) {
            useCsp = true;
         } else {
            cmdParams += args[i] + " ";
         }
      }
      
      final JGDIShell shell = new JGDIShell();
      
      if (useCsp) {
         LoginContext lc = null;
         String jaasContextName = "jgdi";
         logger.config("setup jaas login context for jgdi");
         final String finalUrl = url;
         
         try {
            lc = new LoginContext(jaasContextName, new MyCallbackHandler());
            
            lc.login();
            try {
               Subject.doAs(lc.getSubject(), new PrivilegedAction() {
                  public Object run() {
                     shell.exec(finalUrl);
                     return null;
                  }
               });
            } finally {
               lc.logout();
            }
            
         } catch (LoginException ex) {
            ex.printStackTrace();
         }
      } else {
         if (cmdParams.length()>0) {
            shell.exec(url, cmdParams);
         } else {
            shell.exec(url);
         }
      }
   }
   
   public static String getResourceString(String key) {
      return usageResources.getString(key);
   }
   
   private void exec(final String url) {
      if (url != null) {
         runCommand("connect " + url);
      }
      run(); //We didn't get any other params, so we actually run the JGDIShell
   }
   
   private void exec(final String url, final String cmdParams) {
      if (url != null) {
         runCommand("connect " + url);
         //We just want to execute a single command passed as param and exit.
         runCommand(cmdParams);
         System.exit(1);
      }
   }
   
   class ParsedLine {
      String cmd;
      String [] args;
      
      public ParsedLine(String line) {
         
         if (logger.isLoggable(Level.FINE)) {
            logger.fine("parse line '" + line + "'");
         }
         
         
         int i = 0;
         for (i = 0; i < line.length(); i++) {
            if (Character.isWhitespace(line.charAt(i))) {
               cmd = line.substring(0,i).toLowerCase();
               if (logger.isLoggable(Level.FINE)) {
                  logger.fine("cmd is '" + cmd + "'");
               }
               break;
            }
         }
         
         if (cmd == null) {
            cmd = line.toLowerCase();
            args = new String[0];
            if (logger.isLoggable(Level.FINE)) {
               logger.fine("cmd is '" + cmd + "' has no args");
            }
            return;
         }
         
         ArrayList<String> argList = new ArrayList<String>();
         
         while (i<line.length()) {
            
            char c = line.charAt(i);
            
            if (Character.isWhitespace(c)) {
               i++;
               continue;
            }
            
            if (c == '"') {
               // Quoted String
               i++;
               int startIndex = i;
               
               while (i<line.length() && line.charAt(i) != '"') {
                  i++;
               }
               
               if (i>=line.length()) {
                  throw new IllegalArgumentException("Unclosed \" at column " + startIndex);
               }
               String arg = line.substring(startIndex, i);
               if (logger.isLoggable(Level.FINE)) {
                  logger.fine("arg[" + argList.size() + "] = " + arg);
               }
               argList.add(arg);
               i++;
            } else {
               int startIndex = i;
               while (i<line.length() && !Character.isWhitespace(line.charAt(i))) {
                  i++;
               }
               if (startIndex < i) {
                  String arg = line.substring(startIndex, i);
                  if (logger.isLoggable(Level.FINE)) {
                     logger.fine("arg[" + argList.size() + "] = " + arg);
                  }
                  argList.add(arg);
               } else {
                  logger.fine("empty arg");
               }
            }
         }
         args = new String[argList.size()];
         argList.toArray(args);
      }
      
   }
   
   class ExitCommand implements HistoryCommand {
      public String getUsage() {
         return "exit";
      }
      
      public void run(String[] args) throws Exception {
         if (jgdi != null) {
            try {
               jgdi.close();
            } catch(JGDIException ex1) {
               final String msg = "close failed: " + ex1.getMessage();
               pw.println(msg);
               logger.warning(msg);
            }
         }
         readlineHandler.cleanup();
         System.exit(0);
      }

      public void init(Shell shell) throws Exception {
      }
   }
   
   class HelpCommand implements Command {
      public String getUsage() {
         return "help [command]";
      }
      
      public void run(String[] args) throws Exception {
         
         switch(args.length) {
         case 0: {
            Iterator iter = cmdMap.keySet().iterator();
            pw.println("Available commands: ");
            while (iter.hasNext()) {
               pw.println((String)iter.next());
            }
         }
         break;
         case 1: {
            Command cmd = getCommand(args[0]);
            if (cmd == null) {
               pw.println("command " + args[0] + " not found");
            }
            pw.println(cmd.getUsage());
         }
         break;
         default:
            throw new IllegalArgumentException("Invalid number of arguments");
         }
      }

      public void init(Shell shell) throws Exception {
      }
   }
   
   class ConnectCommand implements HistoryCommand {
      
      public String getUsage() {
         return "connect bootstrap:///<sge_root>@<sge_cell>:<qmaster_post>";
      }
      
      public void run(String[] args) throws Exception {
         
         if (args.length != 1) {
            throw new IllegalArgumentException("Invalid argument count");
         }
         
         if (jgdi != null) {
            try {
               jgdi.close();
            } catch(JGDIException ex1) {
               final String msg = "close failed: " + ex1.getMessage();
               pw.println(msg);
               logger.warning(msg);
            }
         }
         logger.info("connect to " + args[0]);
         jgdi = JGDIFactory.newInstance(args[0]);
      }

      public void init(Shell shell) throws Exception {
      }
   }
   
   class DebugCommand implements HistoryCommand {
      public String getUsage() {
         return "debug [-l <logger>] [<level>]";
      }
      
      // TODO Logger.global is deprecated, clean up this
      @SuppressWarnings("deprecation")
      public void run(String[] args) throws Exception {
         
         String loggerName = logger.getName();
         Level level = null;
         
         switch(args.length) {
         case 0:
            loggerName = null;
            level = null;
            break;
         case 1:
            loggerName = args[0];
            break;
         case 2:
            loggerName = args[0];
            level = Level.parse(args[1]);
            break;
         default:
            throw new IllegalArgumentException("Invalid number of arguments");
         }
         
         Enumeration en = LogManager.getLogManager().getLoggerNames();
         while (en.hasMoreElements()) {
            Logger aLogger = LogManager.getLogManager().getLogger((String)en.nextElement());
            if (aLogger.equals(Logger.global) || aLogger.getName() == null ||
               aLogger.getName().length() == 0) {
               continue;
            }
            if (loggerName == null || aLogger.getName().startsWith(loggerName)) {
               if (level == null) {
                  logger.info(aLogger.getName() + ": " + aLogger.getLevel());
               } else {
                  logger.info("set level of logger " + aLogger.getName() + " to " + level);
                  aLogger.setLevel(level);
               }
            }
         }
      }

      public void init(Shell shell) throws Exception {
      }
   }
   
   class PrintHistoryCommand implements Command {
      
      public String getUsage() {
         return "history";
      }
      
      public void run(String[] args) throws Exception {
         Iterator iter = historyList.iterator();         
         while (iter.hasNext()) {
            HistoryElement elem = (HistoryElement)iter.next();
            pw.printf("%5d %s%n", elem.getId(), elem.getLine());
         }
      }

      public void init(Shell shell) throws Exception {
      }
   }
   
   class XMLDumpCommand implements HistoryCommand {
      
      public String getUsage() {
         return "xmldump <object type> (all|<object name>)";
      }
      
      public void run(String[] args) throws Exception {
         
         if (args.length != 2) {
            throw new IllegalAccessException("Invalid number of arguments");
         }
         if (jgdi == null) {
            throw new IllegalStateException("Not connected");
         }
         if (args[1].equals("all")) {
            
            Method method = JGDI.class.getMethod("get" + args[0] + "List", (java.lang.Class[])null);
            
            List list = (List)method.invoke(jgdi, (java.lang.Object[])null);
            
            Iterator iter = list.iterator();
            while (iter.hasNext()) {
               Object obj = iter.next();
               XMLUtil.write((GEObject)obj,System.out);
               System.out.flush();
            }
         } else {
            Method method = JGDI.class.getMethod("get" + args[0] , new Class[] { String.class });
            
            Object obj = method.invoke(jgdi, new Object[] { args[1] });
            XMLUtil.write((GEObject)obj,System.out);
            System.out.flush();
         }
      }

      public void init(Shell shell) throws Exception {
      }
   }
   
   static class HistoryElement {
      private int id;
      private String line;
      
      public HistoryElement(int id, String line) {
         this.id = id;
         this.line = line;
      }
      
      public int getId() {
         return id;
      }
      
      public String getLine() {
         return line;
      }
      
   }
   
   private ReadlineHandler createReadlineHandler() {
      try {
         Class readlineClass = Class.forName("org.gnu.readline.Readline");
         Class readlineLibClass = Class.forName("org.gnu.readline.ReadlineLibrary");
         
         @SuppressWarnings("unchecked")
         Method byNameMethod = readlineLibClass.getMethod("byName", new Class[] { String.class });
         @SuppressWarnings("unchecked")
         Method loadMethod = readlineClass.getMethod("load", new Class[] { readlineLibClass });
         
         String [] libs = { "GnuReadline", "Editline", "Getline", "PureJava" };
         
         for (int i = 0; i < libs.length; i++) {
            try {
               Object readlineLib = byNameMethod.invoke(readlineLibClass, new Object [] { libs[i] });
               
               if (readlineLib == null) {
                  logger.fine("lib ReadLine." + libs[i] + " is unknown");
                  continue;
               }
               
               loadMethod.invoke(readlineClass, new Object[] { readlineLib });
               
               @SuppressWarnings("unchecked")
               Method initReadlineMethod = readlineClass.getMethod("initReadline", new Class [] { String.class });
               initReadlineMethod.invoke(readlineClass, new Object [] { "JGDIShell" });

               NativeReadlineHandler ret = new NativeReadlineHandler(readlineClass);
               logger.info("use Readline." + libs[i]);
               return ret;
            } catch(Exception e) {
               logger.log(Level.FINE, "Readline." + libs[i] + " failed", e);
            }
         }
      } catch (NoSuchMethodException ex) {
         // Ignore
      } catch(ClassNotFoundException cnfe) {
         // Ignore
      }
      return new DefaultReadlineHandler();
   }
   
   /**
    * Attempts to list annotated classes in the given package and its ancestors
    * as determined by the context class loader
    *
    * @param pkg a <b>Package</b> as a starting point
    * @param annotated <b>Class</b> the annotation class to search
    * @return a list of all annotated classes that exist within that package
    * @throws ClassNotFoundException if something went wrong
    */
   @SuppressWarnings("unchecked")
   public static List<Class> getAllAnnotatedClasses(Package pkg, Class annotated) throws ClassNotFoundException {
      ArrayList<Class> classes = new ArrayList<Class>();
       try {
         ClassLoader cld = Thread.currentThread().getContextClassLoader();
         if (cld == null) {
            throw new ClassNotFoundException("Can't get class loader.");
         }
         String path = pkg.getName().replace('.', '/')+"/";
        // This will hold a list of directories matching the pckgname. 
        //There may be more than one if a package is split over multiple jars/paths
        Enumeration<URL> resources = cld.getResources(path);
         while (resources.hasMoreElements()) {
            final URL nextElement = resources.nextElement();
            // Slipt out just the jar file name
            final JarFile jarFile = new JarFile(new File(new URI(nextElement.getPath().split("!")[0])));
            // Go throught all elements in jar and ask for annotation for those in pkg
            for (Enumeration e = jarFile.entries(); e.hasMoreElements();) {
               JarEntry current = (JarEntry) e.nextElement();
               final String name = current.getName();
               // Is at least from the same package as pkg and it is a class
               if (name.length() > path.length() && name.substring(0, path.length()).equals(path) && name.endsWith(".class")) {
                  try {
                     final Class forName = Class.forName(current.getName().replaceAll("/", ".").replace(".class", ""));
                     if (forName.isAnnotationPresent(annotated)) {
                        classes.add(forName);
                     }
                  } catch (NoClassDefFoundError expected) {
                  }
               }
            }
         }
      } catch (Exception ex) {
         throw new ClassNotFoundException(ex.getMessage());
      }

      return classes;
   }
 
  


   interface ReadlineHandler {
      
      public String readline(String prompt) throws IOException;
      public void cleanup();
   }
   
   
   class NativeReadlineHandler implements ReadlineHandler {
      
      private Class  readlineClass;
      private Method readlineMethod;
      private Method cleanupMethod;
      
      @SuppressWarnings("unchecked")
      public NativeReadlineHandler(Class readlineClass) throws Exception {
         
         this.readlineClass = readlineClass;
         this.readlineMethod = readlineClass.getMethod("readline", new Class[] { String.class });
         this.cleanupMethod = readlineClass.getMethod("cleanup", (java.lang.Class[])null);
         
         Class completerClass = Class.forName("org.gnu.readline.ReadlineCompleter");
         
         Method setCompleterMethod = readlineClass.getMethod("setCompleter", new Class[] { completerClass });
         
         InvocationHandler handler = new NativeCompletionHandler();
         Object completer = Proxy.newProxyInstance(Thread.currentThread().getContextClassLoader(), new Class[] { completerClass }, handler);
         
         // Setup a completer proxy
         setCompleterMethod.invoke(readlineClass, new Object[] { completer });
      }
      
      public String readline(String prompt)  throws IOException {
         try {
            return (String)readlineMethod.invoke(readlineClass, new Object[] { prompt });
         } catch (IllegalAccessException ex) {
            IllegalStateException ilse = new IllegalStateException("Unknown error");
            ilse.initCause(ex);
            throw ilse;
         } catch (InvocationTargetException ex) {
            if (ex.getTargetException() instanceof IOException) {
               throw (IOException)ex.getTargetException();
            } else if (ex.getTargetException() instanceof RuntimeException) {
               throw (RuntimeException)ex.getTargetException();
            } else {
               IllegalStateException ilse = new IllegalStateException("Unknown error");
               ilse.initCause(ex.getTargetException());
               throw ilse;
            }
         }
      }
      
      public void cleanup() {
         try {
            this.cleanupMethod.invoke(readlineClass, (java.lang.Object[])null);
         } catch(Exception e) {
            logger.log(Level.WARNING, "cleanup failed", e);
         }
      }
   }
   
   class NativeCompletionHandler implements InvocationHandler {
      Iterator possibleValues;
      
      public java.lang.String complete(java.lang.String text, int state) {
         //System.out.println("complete " + text + ", " + state);
         if (state == 0) {
            possibleValues = cmdSet.tailSet(text).iterator();
         }
         if (possibleValues.hasNext()) {
            String nextKey = (String) possibleValues.next();
            if (nextKey.startsWith(text)) {
               return nextKey;
            }
         }
         return null;
      }
      
      public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
         return complete((String)args[0], ((Integer)args[1]).intValue());
      }
   }
   
   class DefaultReadlineHandler implements ReadlineHandler {
      private BufferedReader in;
      
      public DefaultReadlineHandler() {
         in = new BufferedReader(new InputStreamReader(System.in));
      }
      
      public String readline(String prompt) throws IOException {
         System.out.print(prompt);
         return in.readLine();
      }
      
      public void cleanup() {
         try {
            in.close();
         } catch(IOException ioe) {
            logger.log(Level.WARNING, "cleanup failed", ioe);
         }
      }
   }
   
   private static class MyCallbackHandler implements CallbackHandler {
      
      public MyCallbackHandler() {
      }
      
      public void handle(Callback[] callbacks) throws IOException, UnsupportedCallbackException {
         
         for (int i = 0; i < callbacks.length; i++) {
            
            if (callbacks[i] instanceof TextOutputCallback) {
               logger.fine("skip text output callback " + callbacks[i]);
               continue;
            } else if (callbacks[i] instanceof NameCallback) {
               
               NameCallback cb = (NameCallback)callbacks[i];
               
               if (cb.getPrompt().indexOf("alias") >= 0) {
                  logger.fine("user.name: " + System.getProperty("user.name"));
                  cb.setName(System.getProperty("user.name"));
               } else {
                  throw new UnsupportedCallbackException(callbacks[i]);
               }
               
            } else if (callbacks[i] instanceof PasswordCallback) {
               
               PasswordCallback cb = (PasswordCallback)callbacks[i];
               cb.setPassword(new char[0]);
            } else if (callbacks[i] instanceof ConfirmationCallback) {
               ConfirmationCallback cb = (ConfirmationCallback)callbacks[i];
               cb.setSelectedIndex(cb.getDefaultOption());
            } else {
               throw new UnsupportedCallbackException(callbacks[i]);
            }
            
         }
      }
   }
   
   /**
    * Helper for handlinng shell streams in runShellCommand 
    */
   class GetResponse extends Thread {
      private InputStream stream;
      
      public GetResponse(InputStream is) {
         stream = is;
      }
      
      @Override
      public void run() {
         while (true) {
            try {
               int c = stream.read();
               if (c == -1) {
                  break;
               }
               pw.print((char)c);
            } catch (IOException ex) {
               ex.printStackTrace();
            }
         }
      }
   }
}
