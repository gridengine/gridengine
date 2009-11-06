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

package com.sun.grid.loadsensor;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;
import java.util.logging.Logger;

/**
 *
 * @author dant
 */
public class LoadSensorManager {
    private static final Logger log = Logger.getLogger("com.sun.grid.LoadSensor");
    private final LoadSensor loadSensor;
    private final Timer timer = new Timer("Measurement Reader", true);
    private long lastRun = 0L;
    private int lastInterval = 0;
    private int timeToMeasure = 0;
    private transient boolean doMeasurement = true;
    // Where output should be sent
    private PrintStream out = System.out;
    // From where input should be read
    private InputStream in = System.in;

    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        if (args.length > 0) {
            LoadSensor instance = null;

            try {
                Class loadSensor = Class.forName(args[0]);

                instance = (LoadSensor)loadSensor.newInstance();
            } catch (ClassCastException e) {
                log.severe(args[0] + " does not implement com.sun.grid.loadsensor.LoadSensor");
                System.exit(2);
            } catch (ClassNotFoundException e) {
                log.severe("Unable to load load sensor class: " + args[0]);
                System.exit(3);
            } catch (IllegalAccessException e) {
                log.severe("Unable to access load sensor class: " + args[0]);
                log.severe(e.getMessage());
                System.exit(4);
            } catch (InstantiationException e) {
                log.severe("Unable to instantiate load sensor class: " + args[0]);
                log.severe(e.getMessage());
                System.exit(5);
            } catch (SecurityException e) {
                log.severe("Unable to instantiate load sensor class: " + args[0]);
                log.severe(e.getMessage());
                System.exit(6);
            }

            LoadSensorManager manager = new LoadSensorManager(instance);

            try {
                manager.parse();
            } catch (IOException e) {
                log.severe("Error while communicating with framework: " +
                        e.getMessage());
                System.exit(7);
            } catch (IllegalArgumentException e) {
                log.severe("Error while setting job parameters: " +
                        e.getMessage());
                System.exit(8);
            } catch (Exception e) {
                log.severe("Error while executing JSV: " +
                        e.getClass().getSimpleName() + " -- " + e.getMessage());
                System.exit(8);
            }
        } else {
            log.severe("No arguments specified");
            printUsage();
            System.exit(1);
        }
    }

    public LoadSensorManager(LoadSensor loadSensor) {
        this.loadSensor = loadSensor;

        long before = 0L;
        long after = 0L;
        
        log.finest("Collecting load values from load sensor");

        try {
            before = System.currentTimeMillis();
            // We call the LoadSensor.doMeasurement() instead of our own
            // because we don't want to confuse the time to the first load
            // report with an actual load report interval
            loadSensor.doMeasurement();
            after = System.currentTimeMillis();

            adjustTimeToMeasure((int)(after - before));
        } catch (Exception e) {
            log.warning("load sensor threw exception while measuring load values: " + e.getClass().getSimpleName() + " -- " + e.getMessage());
        }
    }
    
    /**
     * Print the usage information.
     */
    private static void printUsage() {
        log.info("java -jar loadsensor.jar load_sensor_class [arg,...]");
    }

    public void parse() throws IOException {
        BufferedReader bin = new BufferedReader(new InputStreamReader(in));
        String line = null;

        while ((line = bin.readLine()) != null) {
            log.fine(">>> " + line);

            if (line.equalsIgnoreCase("QUIT")) {
                log.fine("load sensor exiting");
                return;
            } else if (line.equals("")) {
                Map<String,Map<String,String>> values = null;

                calculateTiming();

                try {
                    values = loadSensor.getLoadValues();

                    if ((values != null) && (values.size() > 0)) {
                        send("begin");

                        for (String host : values.keySet()) {
                            if (values.get(host) != null) {
                                for (String key : values.get(host).keySet()) {
                                    send(host + ":" + key + ":" + values.get(host).get(key));
                                }
                            }
                        }

                        send("end");
                    }
                } catch (Exception e) {
                    log.warning("load sensor threw exception while retrieving load values: " + e.getClass().getSimpleName() + " -- " + e.getMessage());
                }

                boolean localMeasurement = false;

                synchronized (this) {
                    localMeasurement = doMeasurement;
                }

                if (localMeasurement) {
                    doMeasurement();
                }
            } else {
                log.warning("ignoring invalid input line: " + line);
            }
        }
    }

    private void send(String message) throws IOException {
        log.fine("<<< " + message);
        out.println(message);
    }

    private void adjustTimeToMeasure(int timeToMeasure) {
        if (this.timeToMeasure > 0) {
            this.timeToMeasure = (int)Math.ceil(((this.timeToMeasure * 7.0f / 8.0f) + (timeToMeasure / 8.0f)) / 1000);
        } else {
            this.timeToMeasure = timeToMeasure;
        }

        // Always assume it takes some time to measure
        if (this.timeToMeasure == 0) {
            this.timeToMeasure = 1;
        }
        
        log.finest("The adjusted time to measure is now " + this.timeToMeasure);
    }

    private void calculateTiming() {
        long thisRun = System.currentTimeMillis();

        if (lastRun != 0) {
            lastInterval = (int)(thisRun - lastRun) / 1000;
            log.finest("The last load report interval was " + lastInterval);
        }

        lastRun = thisRun;
    }

    private void doMeasurement() {
        int timeout = 0;

        try {
            timeout = loadSensor.getMeasurementInterval();
        } catch (Exception e) {
            log.warning("load sensor threw exception while getting the measurement interval: " + e.getClass().getSimpleName() + " -- " + e.getMessage());
        }

        // If the interval is 0, then we measure on demand
        if (timeout <= 0) {
            synchronized (this) {
                // If we were previously measuring periodically, then we have to
                // handle things a little differently
                if (!doMeasurement) {
                    lastInterval = -1;
                }
                
                doMeasurement = true;
            }
            
            if (lastInterval > 0) {
                // If we know the last interval, use it
                doDelayedMeasurement(adjustedInterval());
            } else if (lastInterval == 0) {
                long before = 0L;
                long after = 0L;
                // If this is the first load report interval, just take the
                // measurement now
                log.finest("Collecting load values from load sensor");
                
                try {
                    before = System.currentTimeMillis();
                    loadSensor.doMeasurement();
                    after = System.currentTimeMillis();

                    adjustTimeToMeasure((int)(after - before));
                } catch (Exception e) {
                    log.warning("load sensor threw exception while measuring load values: " + e.getClass().getSimpleName() + " -- " + e.getMessage());
                }
            } // Otherwise keep the last measured values and wait for the next
              // load report
        } else {
            // Otherwise, we're measuring periodically
            synchronized (this) {
                doMeasurement = false;
            }

            doPeriodicMeasurement(timeout);
        }
    }

    private int adjustedInterval() {
        int ret = 0;

        if (lastInterval >= 2 * timeToMeasure) {
            ret = lastInterval - (2 * timeToMeasure);
        }

        return ret;
    }

    private void doDelayedMeasurement(long timeout) {
        log.finest("Collecting load values again in " + timeout + " seconds");
        timer.schedule(new DelayedMeasurementTask(), timeout * 1000);
    }

    private void doPeriodicMeasurement(long timeout) {
        log.finest("Collecting load values again in " + timeout + " seconds");
        timer.schedule(new PeriodicMeasurementTask(), timeout * 1000);
    }

    /**
     * Set the destination for output.  Mostly useful for testing.
     * @param out The new destination for output
     */
    void setOut(PrintStream out) {
        if (out != null) {
            this.out = out;
        } else {
            this.out = System.out;
        }
    }

    /**
     * Set the new source for input.  Mostly useful for testing.
     * @param in The new source for input
     */
    void setIn(InputStream in) {
        if (in != null) {
            this.in = in;
        } else {
            this.in = System.in;
        }
    }

    private class DelayedMeasurementTask extends TimerTask {
        @Override
        public void run() {
            long before = 0L;
            long after = 0L;

            log.finest("Collecting load values from load sensor");
            
            try {
                before = System.currentTimeMillis();
                loadSensor.doMeasurement();
                after = System.currentTimeMillis();
                
                adjustTimeToMeasure((int)(after - before));
            } catch (Exception e) {
                log.warning("load sensor threw exception while measuring load values: " + e.getClass().getSimpleName() + " -- " + e.getMessage());
            }
        }
    }

    private class PeriodicMeasurementTask extends TimerTask {
        @Override
        public void run() {
            doMeasurement();
        }
    }
}
