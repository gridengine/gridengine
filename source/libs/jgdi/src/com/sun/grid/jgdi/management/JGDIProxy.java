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
package com.sun.grid.jgdi.management;

import com.sun.grid.jgdi.JGDIException;
import com.sun.grid.jgdi.event.Event;
import com.sun.grid.jgdi.event.EventListener;
import com.sun.grid.jgdi.management.mbeans.JGDIJMXMBean;
import java.io.IOException;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import javax.management.Attribute;
import javax.management.InstanceAlreadyExistsException;
import javax.management.InstanceNotFoundException;
import javax.management.MBeanException;
import javax.management.MBeanRegistrationException;
import javax.management.MBeanServerConnection;
import javax.management.MalformedObjectNameException;
import javax.management.NotCompliantMBeanException;
import javax.management.Notification;
import javax.management.NotificationListener;
import javax.management.ObjectInstance;
import javax.management.ObjectName;
import javax.management.ReflectionException;
import javax.management.remote.JMXConnector;
import javax.management.remote.JMXConnectorFactory;
import javax.management.remote.JMXServiceURL;

/**
 *  <p>
 *  This class can be used to communicate with qmaster over JMX.
 *  </p>
 */
public class JGDIProxy implements InvocationHandler, NotificationListener {
    
    private static final Map<Method, MethodInvocationHandler> handlerMap = new HashMap<Method, MethodInvocationHandler>();
    
    private ObjectName name;
    private final JMXServiceURL url;
    private JGDIJMXMBean proxy;
    private final Object credentials;
    private JMXConnector connector;
    private MBeanServerConnection connection;
    private Set<EventListener> listeners = Collections.<EventListener>emptySet();
    
    /**
     *  Create a new proxy the the jgdi MBean
     *
     *  @param url  jmx connection url to qmaster
     *  @param name name of the MBean in qmasters MBean server
     *  @param credentials credentials for authenticating user. If the MBeanServer
     *         allows username/password authentication this parameter must be a
     *         string array. The for element is the username, the second elements
     *         is the password.
     *
     */
    public JGDIProxy(JMXServiceURL url, Object credentials) {
        this.url = url;
        this.credentials = credentials;
        Class<?>[] types = new Class<?>[]{JGDIJMXMBean.class};
        proxy = (JGDIJMXMBean) Proxy.newProxyInstance(JGDIJMXMBean.class.getClassLoader(), types , this);
    }
    
    /**
     *   Get the dynamic proxy object
     *
     *   @return the dynamic proxy object
     */
    public JGDIJMXMBean getProxy() {
        return proxy;
    }
    
    /**
     *   Register an jgdi event listener.
     *
     *   @param lis the jgdi event listener
     */
    public synchronized void addEventListener(EventListener lis) {
        Set<EventListener> newListeners = new HashSet<EventListener>(listeners.size() + 1);
        newListeners.addAll(listeners);
        newListeners.add(lis);
        listeners = newListeners;
    }
    
    /**
     *   Remove a jgdi event listener.
     *
     *   @param lis the jgdi event listener
     */
    public synchronized void removeEventListener(EventListener lis) {
        Set<EventListener> newListeners = new HashSet<EventListener>(listeners);
        newListeners.remove(lis);
        listeners = newListeners;
    }
    
    /**
     *  JMX will call this method of a notification for the proxy is available.
     *
     *  If the notification contains a JGDI event it will be propagated to all
     *  registered JGDI event listeners.
     *
     *  @param notification  the notification
     *  @param handback      the handback object
     */
    public void handleNotification(Notification notification, Object handback) {
        
        if (notification.getUserData() instanceof Event) {
            Set<EventListener> tmpLis = null;
            synchronized (this) {
                tmpLis = listeners;
            }
            for (EventListener lis : tmpLis) {
                lis.eventOccured((Event) notification.getUserData());
            }
        }
    }
    
    /**
     *   Determine of the connection to the JMX MBean servers is established
     *
     *   @return <code>true</code> if the connection is established otherwise
     *           <code>false</code>
     */
    public synchronized boolean isConnected() {
        return connection != null;
    }
    
    /**
     *   Close the connection to the MBean server
     */
    public synchronized void close() {
        
        if (connection != null) {
            try {
                connection.unregisterMBean(name);
                connector.close();
            } catch (MBeanRegistrationException ex) {
                // ignore: ex.printStackTrace();
            } catch (InstanceNotFoundException ex) {
                // ignore: ex.printStackTrace();
            } catch (IOException ex) {
                // Ignore it
            } finally {
                connection = null;
                connector = null;
            }
        }
    }
    
    private void connect() throws JGDIException, InstanceAlreadyExistsException {
        if (connection == null) {
            Map<String, Object> env = new HashMap<String, Object>();
            if (credentials != null) {
                env.put("jmx.remote.credentials", credentials);
            }
            try {
                connector = JMXConnectorFactory.connect(url, env);
                connection = connector.getMBeanServerConnection();
//                Subject delegationSubject = new Subject(true,
//                    Collections.singleton(new JMXPrincipal("delegate")),
//                    Collections.EMPTY_SET,
//                    Collections.EMPTY_SET);
//                connection = connector.getMBeanServerConnection(delegationSubject);
                // we need a random number and just take the current
                // mbean count + 1
                int randomId = connection.getMBeanCount() + 1;
//                System.out.println("connection.getMBeanCount() = " + connection.getMBeanCount());
                name = new ObjectName("gridengine:type=JGDI,id=" + randomId);
                ObjectInstance jgdiMBean = connection.createMBean("com.sun.grid.jgdi.management.mbeans.JGDIJMX", name);
                connection.addNotificationListener(name, this, null, null);
                connector.addConnectionNotificationListener(this, null, null);

            } catch (MalformedObjectNameException ex) {
                close();
                throw new JGDIException("jgdi mbean malformed object name", ex);
            } catch (MBeanRegistrationException ex) {
                close();
                throw new JGDIException("jgdi mbean registration failed", ex);
            } catch (MBeanException ex) {
                close();
                throw new JGDIException("jgdi mbean failed", ex);
            } catch (NotCompliantMBeanException ex) {
                close();
                throw new JGDIException("jgdi mbean not compliant", ex);
            } catch (ReflectionException ex) {
                close();
                throw new JGDIException("jgdi mbean not active in qmaster", ex);
            } catch (NullPointerException ex) {
                close();
                throw new JGDIException("jgdi mbean null", ex);
            } catch (InstanceNotFoundException ex) {
                close();
                throw new JGDIException("jgdi mbean not active in qmaster", ex);
            } catch (IOException ex) {
                close();
                throw new JGDIException("connection to " + url + "failed", ex);
            }
        }
    }
    
    /**
     *   Invoke a method on the remote MBean.
     *
     *   @param proxy the JGDIProxy object
     *   @param method the method which should be invoked
     *   @param args   arguments for the method
     */
    public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
        connect();
        return getHandler(method).invoke(connection, name, args);
    }
    
    
    private static MethodInvocationHandler getHandler(Method m) {
        MethodInvocationHandler ret = null;
        synchronized (handlerMap) {
            ret = handlerMap.get(m);
            if (ret == null) {
                ret = createHandler(m);
                handlerMap.put(m, ret);
            }
        }
        return ret;
    }
    
    private static MethodInvocationHandler createHandler(Method method) {
        
        // Is the method an attribute getter?
        String attrname = isAttributeGetter(method);
        if (attrname != null) {
            return new AttributeGetter(attrname);
        }
        
        // Is the method an attribute setter
        attrname = isAttributeSetter(method);
        if (attrname != null) {
            return new AttributeSetter(attrname);
        }
        
        if (method.getName().equals("toString")) {
            return new ToStringMethodInvocationHandler();
        }
        
        return new MethodInvoker(method);
    }
    
    private static interface MethodInvocationHandler {
        public Object invoke(MBeanServerConnection connection, ObjectName name, Object[] args) throws Throwable;
    }
    
    private static class AttributeGetter implements MethodInvocationHandler {
        
        private String attribute;
        
        public AttributeGetter(String attribute) {
            this.attribute = attribute;
        }
        
        public Object invoke(MBeanServerConnection connection, ObjectName name, Object[] args) throws Throwable {
            return connection.getAttribute(name, attribute);
        }
    }
    
    private static class AttributeSetter implements MethodInvocationHandler {
        
        private String attribute;
        
        public AttributeSetter(String attribute) {
            this.attribute = attribute;
        }
        
        public Object invoke(MBeanServerConnection connection, ObjectName name, Object[] args) throws Throwable {
            connection.setAttribute(name, new Attribute(attribute, args[0]));
            return null;
        }
    }
    
    private static class MethodInvoker implements MethodInvocationHandler {
        
        private final String[] signature;
        private final String methodName;
        private final Class  returnType;
        
        public MethodInvoker(Method method) {
            this.methodName = method.getName();
            Class<?>[] paramTypes = method.getParameterTypes();
            this.signature = new String[paramTypes.length];
            for (int i = 0; i < paramTypes.length; i++) {
                this.signature[i] = paramTypes[i].getName();
            }
            returnType = method.getReturnType();
        }
        
        public Object invoke(MBeanServerConnection connection, ObjectName name, Object[] args) throws Throwable {
            try {
                Object ret = connection.invoke(name, methodName, args, signature);
                
                if(ret != null) {
                    try {
                        return returnType.cast(ret);
                    } catch(Exception ex) {
                        throw new IllegalStateException("return type does not match (" +
                            typeToString(returnType) + " <-> " + typeToString(ret.getClass()));
                    }
                } else {
                    return ret;
                }
            } catch (MBeanException ex) {
                // Is it a real exception (thrown be the component implementation?
                if (ex.getTargetException() instanceof InvocationTargetException) {
                    throw ((InvocationTargetException) ex.getTargetException()).getTargetException();
                } else {
                    throw ex;
                }
            }
        }
        
        private static String typeToString(Class<?> clazz) {
            if(clazz != null) {
                return String.format("%s@%s(%s)", clazz.getName(), clazz.getProtectionDomain().getCodeSource(),
                    clazz.getClassLoader().getClass().getName());
            } else {
                return "null";
            }
        }
    }
    
    private static class ToStringMethodInvocationHandler implements MethodInvocationHandler {
        
        public Object invoke(MBeanServerConnection connection, ObjectName name, Object[] args) throws Throwable {
            return String.format("[JGDIClientProxy to %s]", name.toString());
        }
    }
    
    private static String isAttributeGetter(Method method) {
        
        Class returnType = method.getReturnType();
        
        String attrName = null;
        
        if (!returnType.equals(Void.TYPE)) {
            Class[] paramTypes = method.getParameterTypes();
            if (paramTypes.length == 0) {
                String methodName = method.getName();
                if ((returnType.equals(Boolean.TYPE) || returnType.equals(Boolean.class)) && methodName.length() > 2 && methodName.startsWith("is")) {
                    attrName = methodName.substring(2);
                } else if (method.getName().length() > 3 && method.getName().startsWith("get")) {
                    attrName = methodName.substring(3);
                }
            }
        }
        
        return attrName;
    }
    
    /**
     * Get a new instanceof of <code>ComponentAttributeDescriptor</code> from an
     * attribute getter.
     *
     * @param method the attribute getter
     * @return the <code>CoComponentAttributeDescriptorcode> or <code>null</code> if <code>method</code>
     *         is not an attribute getter
     */
    public static String isAttributeSetter(Method method) {
        
        Class[] paramTypes = method.getParameterTypes();
        Class returnType = method.getReturnType();
        
        if (returnType.equals(Void.TYPE) && paramTypes.length == 1
            && method.getName().length() > 3
            && method.getName().startsWith("set")) {
            String attrName = method.getName();
            return attrName.substring(3);
        } else {
            return null;
        }
    }
}
