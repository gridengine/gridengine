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
 *   Copyright: 2006 by Sun Microsystems, Inc
 *
 *   All Rights Reserved.
 *
 ************************************************************************/
/*___INFO__MARK_END__*/
package com.sun.grid.jgdi.examples.jmxeventmonitor;

import com.sun.grid.jgdi.JGDIFactory;
import com.sun.grid.jgdi.event.EventListener;
import com.sun.grid.jgdi.event.EventTypeEnum;
import com.sun.grid.jgdi.management.JGDIProxy;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.management.NotificationBroadcasterSupport;

/**
 *
 */
public class ConnectionController {

    private final static Logger log = Logger.getLogger(ConnectionController.class.getName());
    private final ExecutorService executor = Executors.newSingleThreadExecutor();
    private final List<Listener> listeners = new LinkedList<Listener>();
    private JGDIProxy jgdiProxy;
    private Set<EventTypeEnum> subscription;
    private NotificationBroadcasterSupport broadcaster = new NotificationBroadcasterSupport();

    public void connect(String host, int port, Object credentials) {
        executor.submit(new ConnectAction(host, port, credentials));
    }

    public void disconnect() {
        executor.submit(new DisconnectAction());
    }

    public void subscribe(Set<EventTypeEnum> types) {
        executor.submit(new SubscribeAction(types, true));
    }

    public void unsubscribe(Set<EventTypeEnum> types) {
        executor.submit(new SubscribeAction(types, false));
    }

    public void addListener(Listener lis) {
        synchronized (listeners) {
            listeners.add(lis);
        }
    }

    public void removeListener(Listener lis) {
        synchronized (listeners) {
            listeners.remove(lis);
        }
    }

    private List<Listener> getListeners() {
        synchronized (listeners) {
            return new ArrayList<Listener>(listeners);
        }
    }
    private final Set<EventListener> eventListeners = new HashSet<EventListener>();

    public void addEventListener(EventListener lis) {
        eventListeners.add(lis);
    }

    public void removeEventListener(EventListener lis) {
        eventListeners.remove(lis);
    }

    private class SubscribeAction implements Runnable {

        private final Set<EventTypeEnum> types;
        private final boolean subscribe;

        public SubscribeAction(Set<EventTypeEnum> types, boolean subscribe) {
            this.types = types;
            this.subscribe = subscribe;
        }

        public void run() {

            try {
                if (jgdiProxy != null) {

                    if (subscribe) {
                        jgdiProxy.getProxy().subscribe(types);
                        for (Listener lis : getListeners()) {
                            lis.subscribed(types);
                        }
                    } else {
                        jgdiProxy.getProxy().unsubscribe(types);
                        for (Listener lis : getListeners()) {
                            lis.unsubscribed(types);
                        }
                    }
                }
            } catch (Exception ex) {
                for (Listener lis : getListeners()) {
                    lis.errorOccured(ex);
                }
            }
        }
    }

    private class DisconnectAction implements Runnable {

        public void run() {

            try {
                if (jgdiProxy != null) {
                    jgdiProxy.close();
                }
            } catch (Exception ex) {
                log.log(Level.WARNING, "connector.close failed", ex);
            } finally {
                jgdiProxy = null;
                for (Listener lis : getListeners()) {
                    lis.disconnected();
                }
            }
        }
    }

    private class ConnectAction implements Runnable {

        private final String host;
        private final int port;
        private final Object credentials;

        public ConnectAction(String host, int port, Object credentials) {
            this.host = host;
            this.port = port;
            this.credentials = credentials;
        }

        public void run() {
            try {
                jgdiProxy = JGDIFactory.newJMXInstance(host, port, credentials);
                for (EventListener lis : eventListeners) {
                    jgdiProxy.addEventListener(lis);
                }
                subscription = jgdiProxy.getProxy().getSubscription();

                for (Listener lis : getListeners()) {
                    lis.connected(host, port, subscription);
                }
            } catch (Exception ex) {
                for (Listener lis : getListeners()) {
                    lis.errorOccured(ex);
                }
            }
        }
    }

    public static interface Listener {

        public void connected(String host, int port, Set<EventTypeEnum> subscription);

        public void disconnected();

        public void subscribed(Set<EventTypeEnum> types);

        public void unsubscribed(Set<EventTypeEnum> typea);

        public void errorOccured(Throwable ex);
    }
}
