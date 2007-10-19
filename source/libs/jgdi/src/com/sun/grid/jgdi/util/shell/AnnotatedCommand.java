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
package com.sun.grid.jgdi.util.shell;

import com.sun.grid.jgdi.JGDIException;
import java.io.PrintWriter;
import java.lang.annotation.Annotation;
import java.lang.reflect.Method;
import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.MissingResourceException;
import java.util.ResourceBundle;
import java.util.Set;


/**
 *
 */
public abstract class AnnotatedCommand extends AbstractCommand {
    
    /* Map holding all commands and their respective optionDesriptorMap */
    private static Map<Class<? extends Command>, Map<String, OptionDescriptor>> commandOptionMap = null;
    /* Map holding all options of selected command and methods that should be invoked for it */
    private Map<String, OptionDescriptor> optionDescriptorMap = null;
    /* List of all command options */
    private List<OptionInfo> optionList = null;
    /* Map holding OptionInfo (after paring the args) to the option string */
    private Map<String, OptionInfo> optionInfoMap = null;
    
    /* ResourceBundle containging all error messags */
    private static final ResourceBundle errorMessages = ResourceBundle.getBundle("com.sun.grid.jgdi.util.shell.ErrorMessagesResources");

    /* Extra argument list for extending commands that support it. Other have to explicitly check and throw error if not-null */
    private List<String> extraArgs = null;
    
    /**
     * Initialize the option map optionDescriptorMap if not yet created.
     * Map is created by scanning all OptionMethod annotated functions.
     * NOTE: Only options in the map will be recognized as implemented
     */
    public static void initOptionDescriptorMap(Class<? extends Command> cls, PrintWriter out, PrintWriter err) throws Exception {
        if (commandOptionMap == null) {
            commandOptionMap = new HashMap<Class<? extends Command>, Map<String, OptionDescriptor>>(30);
        }
        if (!commandOptionMap.containsKey(cls)) {
            Map<String, OptionDescriptor> optionDescriptorMap = new HashMap<String, OptionDescriptor>(140);
            
            for (Method m : cls.getMethods()) {
                for (Annotation a : m.getDeclaredAnnotations()) {
                    if (a instanceof OptionAnnotation) {
                        OptionAnnotation o = (OptionAnnotation) a;
                        if (optionDescriptorMap.containsKey(o.value())) {
                            OptionDescriptor od = optionDescriptorMap.get(o.value());
                            if (!od.getMethod().getName().equals(m.getName())) {
                                err.println("WARNING: Attempt to override " +od.getMethod().getDeclaringClass().getName()+"."+od.getMethod().getName() + " by " + cls.getName() + "." + m.getName() + "() for option \""+o.value()+"\"\n");
                            }
                        }
                        //Add method to the optionDescriptorMap
                        optionDescriptorMap.put(o.value(), new OptionDescriptor(o.value(), o.min(), o.extra(), m, out, err));
                    }
                }
            }
            
            commandOptionMap.put(cls, optionDescriptorMap);
        }
    }
    
    /**
     * Finds option info based on option name.
     * Use to do a lookahead to see if specific option was in the argument list.
     * @param option String
     * @return OptionInfo associated with the option or null does not exist
     */
    public OptionInfo getOptionInfo(String option) {
        if (optionInfoMap.containsKey(option)) {
            return optionInfoMap.get(option);
        }
        return null;
    }
    
    /**
     * Getter method
     * @return Map<String, OptionDescriptor>
     */
    public Map<String, OptionDescriptor> getOptionDescriptorMap() {
        if (this.optionDescriptorMap == null) {
            optionDescriptorMap = commandOptionMap.get(this.getClass());
        }
        return optionDescriptorMap;
    }

    /**
     * Gets the extra arguments for the command. Only certail commands actually need it: qrsub, qsub, qalter
     * @return List<String>
    */
    protected List<String> getExtraArguments() {
        return extraArgs;
    }

    /**
     * Check if there are extra arguments. If command should not support it (not a qrsub, qsub, qalter)
     * @return boolean
    */
    protected boolean hasExtraArguments() {
        return extraArgs == null || extraArgs.size()==0;
    }
    
    
    protected void parseOptions(String[] args) throws Exception {
        optionList = new ArrayList<OptionInfo>();
        optionInfoMap = new HashMap<String, OptionInfo>();
        List<String> argList = tokenizeArgs(args);
        while (!argList.isEmpty()) {
            //Get option info, token are divided by known option
            //to option and reladet arguments
            //The arguments are tested for declared multiplicity
            OptionInfo info = getOptionInfo(getOptionDescriptorMap(), argList);
            //If we got extra argument list, we skip it (info is null)
            if (info != null) {
                optionList.add(info);
                optionInfoMap.put(info.getOd().getOption(), info);
            }
        }
    }
    
    /**
     * Parse all arguments and invoke appropriate methods
     * It calls annotated functions for every recognized option.
     * @param args command line options
     * @throws java.lang.Exception an exception during the option call
     * or some runnable exception, when the options are wrong
     */
    protected void parseAndInvokeOptions(String[] args) throws Exception {
        parseOptions(args);
        invokeOptions();
    }
    
    /**
     * Invoke appropriate methods
     * It calls annotated functions for every recognized option.
     * @throws java.lang.Exception an exception during the option call
     * or some runnable exception, when the options are wrong
     */
    protected void invokeOptions() throws Exception {
        for (OptionInfo oi : optionList) {
            oi.invokeOption(this);
        }
    }
    
    /**
     * Parse the ergument array to Array list of separated tokens
     * The tokens are then divided by known options
     * @param args command line option
     * @return List of tokens
     */
    protected List<String> tokenizeArgs(String[] args) {
        ArrayList<String> argList = new ArrayList<String>();
        //Expand args to list of args, 'arg1,arg2 arg3' -> 3 args
        for (String arg : args) {
            String[] subElems = arg.split("[,]");
            for (String subElem : subElems) {
                subElem = subElem.trim();
                if (subElem.length() > 0) {
                    argList.add(subElem);
                }
            }
        }
        return argList;
    }
    
    /**
     * Lists all known (registered) options for the command
     */
    @OptionAnnotation(value = "-list", min=0)
    public void listOptions(final OptionInfo oi) throws JGDIException {
        String str = new String();
        Set<String> set = oi.getMap().keySet();
        String[] options = oi.getMap().keySet().toArray(new String[set.size()]);
        Arrays.sort(options);
        for (String option : options) {
            out.println(option);
        }
        // To avoid the continue of the command
        throw new AbortException();
    }
    
    /**
     * Gets option info. Returns correct Option object and all its arguments
     * Default behavior: 1) Read all mandatory args. Error if not complete
     *                   2) Read max optional args until end or next option found
     * Arguments have to be already expanded
     * @param optMap {@link Map} holding all options for current command.
     * @param args argument list
     * @return return the option info structure
     */
    //TODO LP: Discuss this enhancement. We now accept "arg1,arg2 arg3,arg4" as 4 valid args
    private OptionInfo getOptionInfo(final Map<String, OptionDescriptor> optMap, List<String> args) throws JGDIException {
        //Check we have a map set
        if (optMap.isEmpty()) {
            throw new UnsupportedOperationException("Cannot get OptionInfo. Option map is empty!");
        }

        List<String> extraArgs = null;
        String option = args.get(0);
        String msg;

        if (optMap.containsKey(option)) {
            //We remove single known option from the list
            args.remove(0);
        //We have unknown option (qconf -ddd, qalter 45). It's either extra arguments or really an error
        } else {
            extraArgs = new ArrayList<String>();
            //Read all extra argument to the end of the of all args.
            while (!args.isEmpty()) {
                extraArgs.add(args.remove(0));
                //If some is recognized as valid option. First extra arg is InvalidArgument
                if (optMap.containsKey(option)) {
                    extraArgs = null;
                    msg = getErrorMessage("InvalidArgument", extraArgs.get(0));
                    int exitCode = getCustomExitCode("InvalidArgument", extraArgs.get(0));
                    throw new JGDIException(msg, exitCode);
                }
            }
            //We now store the extraArgs to this command.
            this.extraArgs = extraArgs;
            return null;    //null says now you have the extra args so save them
        }
        
        OptionDescriptor od = optMap.get(option);
        List<String> argList = new ArrayList<String>();
        String arg;
        
        if (!od.isWithoutArgs()) {
            int i = 0;
            
            //Try to ge all mandatory args
            while ((i < od.getMandatoryArgCount()) && (args.size() > 0)) {
                arg = args.remove(0);
                argList.add(arg);
                i++;
            }
            
            //Check we have all mandatory args
            if (i != od.getMandatoryArgCount()) {
                final String msgType = (i == 0) ? "NoArgument" : "LessArguments";
                msg = getErrorMessage(msgType, option, argList);
                int exitCode = getCustomExitCode(msgType, option);
                throw new JGDIException(msg, exitCode);
            }
            
            //Try to get as many optional args as possible
            i = 0;
            
            while ((i < od.getOptionalArgCount()) && (args.size() > 0)) {
                arg = args.remove(0);
                
                //Not enough args?
                if (optMap.containsKey(arg)) {
                    args.add(0, arg);
                    break;
                }
                
                argList.add(arg);
                i++;
            }
        }
        
        //TODO LP: This should probably never happen - remove?
        //Check if we have more args than expected
        if (argList.size() > od.getMaxArgCount()) {
            msg = "Expected only " + od.getMaxArgCount() + " arguments for " + option + " option. Got " + argList.size() + ".";
            throw new IllegalArgumentException(msg);
        }
        
        boolean hasNoArgs = (od.getMandatoryArgCount() == 0) && (od.getOptionalArgCount() == 0);
        
        if (hasNoArgs) {
            return new OptionInfo(od, optMap);
        }
        
        //TODO: Check we have correct args
        return new OptionInfo(od, argList, optMap);
    }
    
    /** Hepler method to get the right error message */
    String getErrorMessage(String msgType, String optionString) {
        return getErrorMessage(msgType, optionString, new ArrayList<String>());
    }
    
    /** Hepler method to get the right error message */
    String getErrorMessage(String msgType, String optionString, String arg) {
        List<String> argList = new ArrayList<String>();
        argList.add(arg);
        return getErrorMessage(msgType, optionString, argList);
    }
    
    /** Hepler method to get the right error message */
    String getErrorMessage(String msgType, String optionString, List<String> args) {
        String[] tmp = this.getClass().getName().split("[.]");
        String cmdName = tmp[tmp.length-1];
        String cmdString = cmdName.split("Command")[0].toLowerCase();
        String prefix = msgType+"."+cmdName+".";
        String argString = "";
        if (args != null && args.size() > 0) {
            for (String arg : args) {
                argString += arg + " ";
            }
            argString = argString.substring(0, argString.length()-1);
        }
        //TODO LP: Remove the New generic message string
        String msg = "Missing error messages file!";
        try { //Try to get option specific message
            msg = errorMessages.getString(prefix+optionString);
        } catch (MissingResourceException ex) {
            try { //Try to get command default message
                msg = errorMessages.getString(prefix+"default");
            } catch (MissingResourceException dex) {
                msg = errorMessages.getString(prefix+"general");
            }
        }
        return MessageFormat.format(msg, optionString, argString, "Usage: "+cmdString+" -help", getUsage());
    }
    
    /** Hepler method to get the right exit code for specified error (msgType) */
    int getCustomExitCode(String msgType, String optionString) {
        String[] tmp = this.getClass().getName().split("[.]");
        String cmdName = tmp[tmp.length-1];
        String cmdString = cmdName.split("Command")[0].toLowerCase();
        String prefix = msgType+"."+cmdName+".";
        String argString = "";
        int exitCode = 0;
        try { //Try to get option specific exitCode for this message type
            exitCode = Integer.valueOf(errorMessages.getString(prefix+optionString+".exitCode"));
        } catch (MissingResourceException ex) {
            try { //Try to get command default message specific exitCode
            exitCode = Integer.valueOf(errorMessages.getString(prefix+"default.exitCode"));
            } catch (MissingResourceException dex) {}
        }
        return exitCode;
    }
}