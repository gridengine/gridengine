/*
 * Copyright 2003-2004 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 * -----------------------------------------------------------------------------
 * $Id: JobFinalUsageEvent.java,v 1.2 2007/03/26 09:26:04 rhierlmeier Exp $
 * -----------------------------------------------------------------------------
 */
package com.sun.grid.jgdi.event;

/**
 * Java Wrapper class for the JOB_FINAL_USAGE event
 *
 */
public class JobFinalUsageEvent extends JobUsageEvent implements java.io.Serializable {
    
    public JobFinalUsageEvent(long timestamp, int evtId) {
        super(timestamp, evtId);
    }
    
}
