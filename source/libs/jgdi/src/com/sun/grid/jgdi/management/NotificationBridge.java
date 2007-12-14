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
package com.sun.grid.jgdi.management;

import com.sun.grid.jgdi.EventClient;
import com.sun.grid.jgdi.JGDIException;
import com.sun.grid.jgdi.JGDIFactory;
import com.sun.grid.jgdi.event.Event;
import com.sun.grid.jgdi.event.EventListener;
import com.sun.grid.jgdi.event.EventTypeEnum;
import com.sun.grid.jgdi.event.ShutdownEvent;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.atomic.AtomicLong;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.management.ListenerNotFoundException;
import javax.management.MBeanNotificationInfo;
import javax.management.Notification;
import javax.management.NotificationFilter;
import javax.management.NotificationListener;

/**
 *
 */
public class NotificationBridge implements EventListener {

    private static final Logger log = Logger.getLogger(NotificationBridge.class.getName());
    private final AtomicLong seqNumber = new AtomicLong();
    private final Map<Class, EventToNotification> eventMap = new HashMap<Class, EventToNotification>();
    private MBeanNotificationInfo[] notificationInfos;
    private final String url;
    private EventClient eventClient;
    private final List<NotificationListener> listeners = new LinkedList<NotificationListener>();
    private final Map<NotificationListener, Object> handbackMap = new HashMap<NotificationListener, Object>();

    public NotificationBridge(String url) {
        this.url = url;
    }

    private class EventToNotification {

        private final String eventName;
        private final Class eventClass;
        private final MBeanNotificationInfo notificationInfo;

        public EventToNotification(String eventName, Class eventClass) {
            this.eventName = eventName;
            this.eventClass = eventClass;
            this.notificationInfo = new MBeanNotificationInfo(new String[]{eventName}, eventClass.getName(), null);
        }

        public MBeanNotificationInfo getNotificationInfo() {
            return notificationInfo;
        }

        public Notification createNotification(Event event) {
            Notification notification = new Notification(eventName, EventClient.class.getName(), seqNumber.incrementAndGet());
            notification.setUserData(event);
            return notification;
        }
    }

    public void eventOccured(Event evt) {
        log.entering("NotificationBridge", "eventOccured", evt);
        Notification n = null;
        List<NotificationListener> tmpLis = null;
        synchronized (this) {
            EventToNotification bridge = eventMap.get(evt.getClass());
            if (bridge != null) {
                log.log(Level.FINE, "create notification for event {0}", evt);
                n = bridge.createNotification(evt);
                tmpLis = new ArrayList<NotificationListener>(listeners);
            } else {
                log.log(Level.WARNING, "Received unknown event {0}", evt);
            }
            if (evt instanceof ShutdownEvent) {
                subscribtion.clear();
            }
        }
        if (n != null) {
            for (NotificationListener lis : tmpLis) {
                log.log(Level.FINE, "send notification to {0}", lis);
                lis.handleNotification(n, handbackMap.get(lis));
            }
        }
        log.exiting("NotificationBridge", "eventOccured");
    }

    public synchronized void registerEvent(String eventName, Class eventClass) {
        if(log.isLoggable(Level.FINER)) {
            log.entering("NotificationBridge", "registerEvent", new Object[]{eventName, eventClass});
        }
        EventToNotification evtNot = new EventToNotification(eventName, eventClass);
        eventMap.put(eventClass, evtNot);
        log.exiting("NotificationBridge", "registerEvent");
    }

    public synchronized MBeanNotificationInfo[] getMBeanNotificationInfo() {
        log.entering("NotificationBridge", "getMBeanNotificationInfo");
        if (notificationInfos == null) {
            notificationInfos = new MBeanNotificationInfo[eventMap.size()];
            int i = 0;

            for (Map.Entry<Class, EventToNotification> entry : eventMap.entrySet()) {
                notificationInfos[i++] = entry.getValue().getNotificationInfo();
            }
        }
        log.exiting("NotificationBridge", "getMBeanNotificationInfo", notificationInfos);
        return notificationInfos;
    }

    public synchronized void removeNotificationListener(NotificationListener listener) throws JGDIException, ListenerNotFoundException {
        log.entering("NotificationBridge", "removeNotificationListener", listener);
        listeners.remove(listener);
        handbackMap.remove(listener);
        commit();
        log.exiting("NotificationBridge", "removeNotificationListener");
    }

    public synchronized void addNotificationListener(NotificationListener listener, NotificationFilter filter, Object handback) throws JGDIException {
        if(log.isLoggable(Level.FINER)) {
            log.entering("NotificationBridge", "addNotificationListener", new Object [] { listener, filter, handback });
        }
        listeners.add(listener);
        handbackMap.put(listener, handback);
        commit();
        log.exiting("NotificationBridge", "addNotificationListener");
    }
    private static final Map<EventTypeEnum, Method> subscribeMethodMap = new HashMap<EventTypeEnum, Method>();

    private static Method getSubscribeMethod(EventTypeEnum type) {
        log.entering("NotificationBridge", "getSubscribeMethod", type);
        Method ret;
        synchronized (subscribeMethodMap) {
            ret = subscribeMethodMap.get(type);
            if (ret == null) {
                try {
                    ret = EventClient.class.getMethod("subscribe" + type.toString(), Boolean.TYPE);
                } catch (Exception ex) {
                    throw new IllegalStateException("subscribe method for type " + type + " not found", ex);
                }
                subscribeMethodMap.put(type, ret);
            }
        }
        log.exiting("NotificationBridge", "getSubscribeMethod", ret);
        return ret;
    }

    private synchronized void subscribe(EventTypeEnum type, boolean subscribe) throws JGDIException {
        if (log.isLoggable(Level.FINER)) {
            log.entering("NotificationBridge", "subscribe", new Object[]{type, subscribe});
        }
        boolean callNative = false;
        if (subscribe) {
            callNative = subscribtion.add(type);
        } else {
            callNative = subscribtion.remove(type);
        }
        if (callNative) {
            subscribeNative(type, subscribe);
        }
        log.exiting("NotificationBridge", "subscribe");
    }
    
    private synchronized void subscribeNative(EventTypeEnum type, boolean subscribe) throws JGDIException {
        if (log.isLoggable(Level.FINER)) {
            log.entering("NotificationBridge", "subscribeNative", new Object[]{type, subscribe});
        }
        if (eventClient != null && !eventClient.isClosed()) {
            try {
                getSubscribeMethod(type).invoke(eventClient, subscribe);
            } catch (IllegalArgumentException ex) {
                throw new IllegalStateException("Can not invoke subscribe method", ex);
            } catch (IllegalAccessException ex) {
                throw new IllegalStateException("Can not invoke subscribe method", ex);
            } catch (InvocationTargetException ex) {
                if (ex.getTargetException() instanceof JGDIException) {
                    throw (JGDIException) ex.getTargetException();
                } else {
                    throw new IllegalStateException("Can not invoke subscribe method", ex.getTargetException());
                }
            }
        }
        log.exiting("NotificationBridge", "subscribeNative");
    }
    private Set<EventTypeEnum> subscribtion = new HashSet<EventTypeEnum>();

    private synchronized void commit() throws JGDIException {

        log.entering("NotificationBridge", "commit");

        if (log.isLoggable(Level.FINE)) {
            log.log(Level.FINE, "subscription is {0}", getSubscription());
        }
        if (subscribtion.isEmpty() || listeners.isEmpty()) {
            close();
        } else {
            boolean createNew = false;
            if (eventClient == null) {
                log.log(Level.FINER, "creating new event client");
                createNew = true;
            } else if (eventClient.isClosed()) {
                log.log(Level.FINER, "event client has been closed, create a new one");
                createNew = true;
            }          
            if (createNew) {
                eventClient = JGDIFactory.createEventClient(url, 0);
                log.log(Level.FINE, "event client to {0} created", url);
                for (EventTypeEnum type : subscribtion) {
                    subscribeNative(type, true);
                }
                log.log(Level.FINER, "adding me as event listener");
                eventClient.addEventListener(this);
                log.log(Level.FINER, "I am now a event listener");
            }
            if (!eventClient.isRunning()) {
                try {
                    log.log(Level.FINER, "starting event client [{0}]", eventClient.getId());
                    eventClient.start();
                    log.log(Level.FINE, "event client [{0}] started", eventClient.getId());
                } catch (InterruptedException ex) {
                    eventClient = null;
                    throw new JGDIException("startup of event client has been interrupted", ex);
                }
            } else {
                log.log(Level.FINER, "commiting event client [{0}]", eventClient.getId());
                eventClient.commit();
                log.log(Level.FINE, "changes at event client [{0}] commited", eventClient.getId());
            }
        }

        log.exiting("NotificationBridge", "commit");
    }

    public synchronized Set<EventTypeEnum> getSubscription() {
        log.entering("NotificationBridge", "getSubscription");
        Set<EventTypeEnum> ret = Collections.unmodifiableSet(subscribtion);
        log.exiting("NotificationBridge", "getSubscription", ret);
        return ret;
    }

    public synchronized void setSubscription(Set<EventTypeEnum> types) throws JGDIException {

        log.entering("NotificationBridge", "setSubscription", types);

        Set<EventTypeEnum> orgSubscription = new HashSet<EventTypeEnum>(subscribtion);

        for (EventTypeEnum type : types) {
            if (!orgSubscription.remove(type)) {
                // type has not been subscribed before
                subscribe(type, true);
            }
        }

        // Remove all previous subscribed events which are not contained in types
        for (EventTypeEnum type : orgSubscription) {
            subscribe(type, false);
        }
        commit();
        log.exiting("NotificationBridge", "setSubscription");
    }

    public synchronized void subscribe(EventTypeEnum type) throws JGDIException {
        log.entering("NotificationBridge", "subscribe", type);
        subscribe(type, true);
        eventClient.commit();
        log.exiting("NotificationBridge", "subscribe");
    }

    public synchronized void unsubscribe(EventTypeEnum type) throws JGDIException {
        log.entering("NotificationBridge", "unsubscribe", type);
        subscribe(type, false);
        commit();
        log.exiting("NotificationBridge", "unsubscribe");
    }

    public synchronized void subscribe(Set<EventTypeEnum> eventTypes) throws JGDIException {
        log.entering("NotificationBridge", "subscribe", eventTypes);
        for (EventTypeEnum type : eventTypes) {
            subscribe(type, true);
        }
        commit();
        log.exiting("NotificationBridge", "subscribe");
    }

    public synchronized void unsubscribe(Set<EventTypeEnum> eventTypes) throws JGDIException {
        log.entering("NotificationBridge", "unsubscribe", eventTypes);
        for (EventTypeEnum type : eventTypes) {
            subscribe(type, false);
        }
        commit();
        log.exiting("NotificationBridge", "unsubscribe");
    }

    /**
     *  Close the event notification bridge
     *
     *  The JGDI event client will be stopped.
     */
    public void close() {
        log.entering("NotificationBridge", "close");
        synchronized (this) {
            if (eventClient != null) {
                try {
                    log.log(Level.FINE, "closing event client [{0}]", eventClient.getId());
                    eventClient.close();
                } catch (Exception ex) {
                // Ignore
                } finally {
                    eventClient = null;
                    subscribtion.clear();
                }
            }
        }
        log.exiting("NotificationBridge", "close");
    }
}
