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

import com.sun.grid.jgdi.JGDI;
import com.sun.grid.jgdi.JGDIException;
import com.sun.grid.jgdi.configuration.AbstractManager;
import com.sun.grid.jgdi.configuration.GEObject;
import java.io.PrintWriter;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

/**
 * AbstractUserQconfOption class
 * Special handling methods for {@link AbstractUser}
 * @see {@link QConfOption}
 */
public class AbstractUserQConfOption extends QConfOption {
   
   void add(final JGDI jgdi, final String type, final List args, final PrintWriter pw) {
      modify(jgdi, "add", type, args, pw);
   }
   
   void delete(final JGDI jgdi, final String type, final List args, final PrintWriter pw) {
      modify(jgdi, "delete", type, args, pw);
   }
   
   //General to save space
   private void modify(final JGDI jgdi, final String operation, final String type, final List args, final PrintWriter pw) {
      if (args.size() == 0) {
         pw.println("error: no option argument provided to \""+args.get(0)+"\"");
         pw.println("Usage: qconf -help");
         pw.flush();
         return;
      }
      if (args.size() > 1) {
         pw.println("error: invalid option argument \"" + args.get(1) + "\"");
         pw.println("Usage: qconf -help");
         pw.flush();
         return;
      }
      List userList = parseList(args);
      String value;
      Class cls;
      Object obj;
      GEObject geObj;
      Method m;
      for (Iterator iter = userList.iterator(); iter.hasNext(); ) {
         geObj = null;
         value = (String) iter.next();
         try {
            //Try to find the existing manager/operator
            m = JGDI.class.getDeclaredMethod("get"+type+"List", new Class[] {});
            List list = (List) m.invoke(jgdi, new Object[] {});
            for (Iterator iterator = list.iterator(); iterator.hasNext(); ) {
               AbstractManager am = (AbstractManager) iterator.next();
               if (am.getName().equals(value)) {
                  geObj = am;
                  break;
               }
            }
            //Adding existing value
            if (operation.equals("add") && geObj != null) {
               pw.println(type.toLowerCase()+" \""+value+"\" already exists");
               pw.flush();
               continue;
            }
            //When adding and does not exist, we create new instance
            if (operation.equals("add") && geObj == null) {
               cls = Class.forName("com.sun.grid.jgdi.configuration." + type + "Impl");
               Constructor c = cls.getConstructor(new Class[] {boolean.class});
               obj = c.newInstance(new Object[] {Boolean.TRUE});
               if (!(obj instanceof GEObject)) {
                  throw new IllegalAccessException("Class for type "+type+" is not an instance of GEObject");
               }
               geObj = (GEObject)obj;
               Method setName = obj.getClass().getDeclaredMethod("setName", new Class[] {String.class});
               setName.invoke(geObj, new Object[] { value });
            }
            //Removing non-existing value
            if (operation.equals("delete") && geObj == null) {
               pw.println("denied: "+type.toLowerCase()+" \""+value+"\" does not exist");
               pw.flush();
               continue;
            }
            //Finally we perform the task
            Class paramClass = Class.forName("com.sun.grid.jgdi.configuration." + type);
            m = JGDI.class.getDeclaredMethod(operation + type, new Class[] {paramClass});
            m.invoke(jgdi, new Object[] {geObj});
            pw.println(jgdi.getAdminUser()+"@"+java.net.InetAddress.getLocalHost().getHostName() +
                  ((operation.equals("add")) ? " added " : " removed ") + "\""+value+"\"" +
                  ((operation.equals("add")) ? " to " : " from ") + type.toLowerCase() + " list");
         } catch (ClassNotFoundException ex) {
            ex.printStackTrace();
         } catch (SecurityException ex) {
            ex.printStackTrace();
         } catch (NoSuchMethodException ex) {
            ex.printStackTrace();
         } catch (InstantiationException ex) {
            ex.printStackTrace();
         } catch (IllegalArgumentException ex) {
            ex.printStackTrace();
         } catch (IllegalAccessException ex) {
            ex.printStackTrace();
         } catch (InvocationTargetException ex) {
            ex.printStackTrace();
         } catch (JGDIException ex) {
            pw.println(ex.getMessage());
         } catch (UnknownHostException ex) {
            ex.printStackTrace();
         } finally {
            pw.flush();
         }
      }
   }
   
   private List parseList(final List argList) {
       List list = new ArrayList();
       //TODO LP: Now we stick to the client behavior don't accept additional args
       String userList = (String) argList.get(0);
       String[] users = userList.split(",");
       String user;
       for (int i = 0; i < users.length; i++) {
          user = users[i].trim();
          if (user.length() > 0) {
             list.add(user);
          }
       }
       return (list.size() > 0) ? list : null;
    }
}
