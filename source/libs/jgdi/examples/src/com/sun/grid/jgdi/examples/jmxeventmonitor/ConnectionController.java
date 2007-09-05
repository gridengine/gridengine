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

import com.sun.grid.jgdi.event.EventTypeEnum;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.management.ListenerNotFoundException;
import javax.management.MBeanServerConnection;
import javax.management.Notification;
import javax.management.NotificationBroadcasterSupport;
import javax.management.NotificationFilter;
import javax.management.NotificationListener;
import javax.management.ObjectName;
import javax.management.remote.JMXConnector;
import javax.management.remote.JMXConnectorFactory;
import javax.management.remote.JMXServiceURL;

/**
 *
 */
public class ConnectionController implements NotificationListener {
    
    private final static Logger log = Logger.getLogger(ConnectionController.class.getName());
    private final ExecutorService executor = Executors.newSingleThreadExecutor();
    
    private final List<Listener> listeners = new LinkedList<Listener>();
    
    private ObjectName name;
    private JMXConnector connector;
    private MBeanServerConnection connection;
    
    private Set<EventTypeEnum> subscription;
    
    private NotificationBroadcasterSupport broadcaster = new NotificationBroadcasterSupport();
    
    public void connect(String host, int port) {
        executor.submit(new ConnectAction(host, port));
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
        synchronized(listeners) {
            listeners.add(lis);
        }
    }
    
    public void removeListener(Listener lis) {
        synchronized(listeners) {
            listeners.remove(lis);
        }
    }
    
    private List<Listener> getListeners() {
        synchronized(listeners) {
            return new ArrayList(listeners);
        }
    }

    public void addNotificationListener(NotificationListener lis, NotificationFilter filter, Object handback) {
        broadcaster.addNotificationListener(lis, filter, handback);
    }
    
    public void removeNotificationListener(NotificationListener lis) {
        try {
            broadcaster.removeNotificationListener(lis);
        } catch (ListenerNotFoundException ex) {
            for(Listener tmpLis: getListeners()) {
                tmpLis.errorOccured(ex);
            }
        }
    }
    
    public void handleNotification(Notification notification, Object handback) {
        if(notification.getType().equals("jmx.remote.connection.closed")) {
            disconnect();
        } else {
            broadcaster.sendNotification(notification);
        }
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
                if(connection != null) {

                    if(subscribe) {
                        connection.invoke(name, "subscribe", new Object [] { types }, new String [] { Set.class.getName() });
                        for(Listener lis: getListeners()) {
                            lis.subscribed(types);
                        }
                    } else {
                        connection.invoke(name, "unsubscribe", new Object [] { types }, new String [] { Set.class.getName() });
                        for(Listener lis: getListeners()) {
                            lis.unsubscribed(types);
                        }
                    }

                }
            } catch(Exception ex) {
                for(Listener lis: getListeners()) {
                    lis.errorOccured(ex);
                }
            }
            
        }
        
    }
            
    
    private class DisconnectAction implements Runnable {
        
        public void run() {
            
            try {
                if(connector != null) {
                    connector.close();
                }
                
            } catch(Exception ex) {
                log.log(Level.WARNING, "connector.close failed", ex);
            } finally {
                connection = null;
                connection = null;
                subscription = null;
                for(Listener lis: getListeners()) {
                    lis.disconnected();
                }
            }
        }
                
                
    }
    
    private class ConnectAction implements Runnable {
        
        private final String host;
        private final int port;
        
        public ConnectAction(String host, int port) {
            this.host = host;
            this.port = port;
        }
        
        public void run() {
            try {
                JMXServiceURL url = new JMXServiceURL("service:jmx:rmi:///jndi/rmi://" + host + ":" + port + "/jmxrmi");

                Map<String, Object> env = new HashMap<String, Object>();

                String [] cred = { "controlRole", "R&D" };

                env.put("jmx.remote.credentials", cred );

                name = new ObjectName("gridengine:type=JGDI");
                
                connector = JMXConnectorFactory.connect(url, env);
                connection = connector.getMBeanServerConnection();

                connection.addNotificationListener(name,ConnectionController.this, null, null);
                
                connector.addConnectionNotificationListener(ConnectionController.this, null, null);
                
                subscription = (Set<EventTypeEnum>)connection.getAttribute(name, "Subscription");
                
                for(Listener lis: getListeners()) {
                    lis.connected(host, port, subscription);
                }
            } catch(Exception ex) {
                for(Listener lis: getListeners()) {
                    lis.errorOccured(ex);
                }
            }
                    
        }
    }
    
    public interface Listener {
        
        public void connected(String host, int port, Set<EventTypeEnum> subscription);
        public void disconnected();
        
        public void subscribed(Set<EventTypeEnum> types);
        public void unsubscribed(Set<EventTypeEnum> typea);
        
        public void errorOccured(Throwable ex);
        
    }

    public ObjectName getName() {
        return name;
    }
            
}
