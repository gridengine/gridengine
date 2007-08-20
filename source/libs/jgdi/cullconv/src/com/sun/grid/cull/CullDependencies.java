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
package com.sun.grid.cull;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.StringTokenizer;
import java.util.logging.Logger;
import javax.swing.tree.DefaultMutableTreeNode;
import org.apache.tools.ant.BuildException;

/**
 *
 */
public class CullDependencies {
   
   private Logger logger = Logger.getLogger("cullconv");
   
   Map nodeMap = new HashMap();
   
   public static final String FILTER_OBJECTS = "objects";
   public static final String FILTER_PRIMITIVE_OBJECTS = "primitives";
   public static final String FILTER_ROOT_OBJECTS = "root";
   public static final String FILTER_PRIMITIVE_ROOT_OBJECTS = "primitive_root";
   public static final String FILTER_MAPPED_OBJECTS = "mapped";
   public static final String FILTER_MAP_OBJECTS = "map";
   public static final String FILTER_DEPEND_OBJECTS = "depend";
   public static final String FILTER_EVENT_OBJECTS = "event";
   
   
   public static final int INCLUDE_ROOT_OBJECTS           = 0x0001;
   public static final int INCLUDE_OBJECTS                = 0x0002;
   public static final int INCLUDE_PRIMITIVE_OBJECTS      = 0x0004;
   public static final int INCLUDE_PRIMITIVE_ROOT_OBJECTS = 0x0008;
   public static final int INCLUDE_MAPPED_OBJECTS         = 0x0010;
   public static final int INCLUDE_MAP_OBJECTS            = 0x0020;
   public static final int INCLUDE_DEPEND_OBJECTS         = 0x0040;
   public static final int INCLUDE_EVENT_OBJECTS          = 0x0080;
   public static final int INCLUDE_ALL                    = 0xFFFF;
   private int includeMask;
   
   /** Creates a new instance of CullDependencies */
   public CullDependencies(CullDefinition culldef, Set nameSet, String objectFilter) throws BuildException {
      
      includeMask = 0;
      
      if(objectFilter == null) {
         includeMask = INCLUDE_ALL;
      } else {
         StringTokenizer st = new StringTokenizer(objectFilter);

         while(st.hasMoreTokens()) {
            String token = st.nextToken();
            if(FILTER_OBJECTS.equals(token)) {
               includeMask |= INCLUDE_OBJECTS;
            } else if (FILTER_PRIMITIVE_OBJECTS.equals(token)) {
               includeMask |= INCLUDE_PRIMITIVE_OBJECTS;
            } else if (FILTER_ROOT_OBJECTS.equals(token)) {
               includeMask |= INCLUDE_ROOT_OBJECTS;
            } else if (FILTER_EVENT_OBJECTS.equals(token)) {
               includeMask |= INCLUDE_EVENT_OBJECTS;
            } else if (FILTER_DEPEND_OBJECTS.equals(token)) {
               includeMask |= INCLUDE_DEPEND_OBJECTS;
            } else if (FILTER_MAPPED_OBJECTS.equals(token)) {
               includeMask |= INCLUDE_MAPPED_OBJECTS;
            } else if (FILTER_MAP_OBJECTS.equals(token)) {
               includeMask |= INCLUDE_MAP_OBJECTS;
            } else if (FILTER_PRIMITIVE_ROOT_OBJECTS.equals(token)) {
               includeMask |= INCLUDE_PRIMITIVE_ROOT_OBJECTS;
            } else {
               throw new BuildException("Invalid object filter '" + token + "'");
            }
         }
      }
      
      Iterator iter = nameSet.iterator();
      
      
      while(iter.hasNext()) {
         String name = (String)iter.next();
         
         CullObject obj = culldef.getCullObject(name);
         
         Node node = (Node)nodeMap.get(name);
         
         if( node == null ) {
            node = new Node(obj);
            nodeMap.put(name,node);
         }
         
         for(int i = 0; i < obj.getAttrCount(); i++) {
            CullAttr attr = obj.getAttr(i);
            
            String type = attr.getType();

            CullObject subobj = culldef.getCullObject(type);
            if( subobj != null /*&& !subobj.getName().equals(name)*/ ) {       
               node = (Node)nodeMap.get(type);
               if( node == null ) {
                  node = new Node(subobj);
                  nodeMap.put(type,node);
               }
               node.addDepend(obj);
            }            
         }
         
         if(obj.getParentObject() != null) {
            
            CullObject subobj = obj.getParentObject();
            
            if( subobj != null ) {       
               node = (Node)nodeMap.get(subobj.getName());
               if( node == null ) {
                  node = new Node(subobj);
                  nodeMap.put(subobj.getName(),node);
               }
               node.addDepend(obj);
            }            
            
         }
      }
   }
   
   public boolean includeRootObjects() {
      return (includeMask & INCLUDE_ROOT_OBJECTS) == INCLUDE_ROOT_OBJECTS;
   }
   
   public boolean includeEventObjects() {
      return (includeMask & INCLUDE_EVENT_OBJECTS) == INCLUDE_EVENT_OBJECTS;
   }
   
   public boolean includeObjects() {
      return (includeMask & INCLUDE_OBJECTS) == INCLUDE_OBJECTS;
   }
   
   public boolean includePrimitveObjects() {
      return (includeMask & INCLUDE_PRIMITIVE_OBJECTS) == INCLUDE_PRIMITIVE_OBJECTS;
   }

   public boolean includeMappedObjects() {
      return (includeMask & INCLUDE_MAPPED_OBJECTS) == INCLUDE_MAPPED_OBJECTS;
   }
   
   public boolean includePrimitveRootObjects() {
      return (includeMask & INCLUDE_PRIMITIVE_ROOT_OBJECTS) == INCLUDE_PRIMITIVE_ROOT_OBJECTS;
   }
   
   public boolean includeMapObjects() {
      return (includeMask & INCLUDE_MAP_OBJECTS) == INCLUDE_MAP_OBJECTS;
   }
   
   public boolean includeDependObjects() {
      return (includeMask & INCLUDE_DEPEND_OBJECTS ) == INCLUDE_DEPEND_OBJECTS;
   }
   
   public Node getNode(String name) {
      return (Node)nodeMap.get(name);
   }
   
   public class Node {
      
      private CullObject obj;
      
      private List dependSet = new ArrayList();
      
      
      public Node(CullObject obj) {
         this.obj = obj;
      }
      
      public void addDepend(CullObject obj) {
         dependSet.add(obj);
         needed = null;
      }
      
      public int getDependCount() {
         return dependSet.size();
      }
      
      
      private Boolean needed = null;
      private boolean isNeededArmed = false;
      public boolean isNeeded() {
         if( needed == null ) {
            if( isNeededArmed ) {
               needed = Boolean.FALSE;
            } else {
               isNeededArmed = true;
               
               if ( obj.hasEvents() && includeEventObjects()) {
                   needed = Boolean.TRUE;
               } else if( obj.getType() == CullObject.TYPE_PRIMITIVE && includePrimitveObjects() ) {
                  needed = Boolean.TRUE;
               } else if ( obj.getType() == CullObject.TYPE_MAPPED && includeMappedObjects() ) {
                  needed = Boolean.TRUE;
               } else if ( obj.getType() == CullObject.TYPE_MAP && includeMapObjects() ) {
                  needed = Boolean.TRUE;
               } else if ( obj.isRootObject() ) {
                  if (obj.getType() == CullObject.TYPE_PRIMITIVE) {
                     if(includePrimitveRootObjects()) {
                        needed = Boolean.TRUE;
                     }
                  } else {
                     if(includeRootObjects()) {
                        needed = Boolean.TRUE;
                     }
                  }
               } else if ( includeObjects()) {
                  needed = Boolean.TRUE;                  
               }
               
               if(needed == null) {
                  Iterator iter = dependSet.iterator();
                  
                  while(iter.hasNext() && needed == null ) {

                     CullObject dependObj = (CullObject)iter.next();
                     Node node = getNode(dependObj.getName());
                     if( node != null ) {                          
                        if( node.isNeeded() ) {
                           switch(obj.getType()) {
                              case CullObject.TYPE_PRIMITIVE:
                              {
                                 if(includePrimitveObjects()) {
                                   needed = Boolean.TRUE; 
                                 } else {
                                   needed = Boolean.FALSE; 
                                 }
                                 break;
                              }
                              case CullObject.TYPE_MAPPED:
                              {
                                 if(includeMappedObjects()) {
                                    needed = Boolean.TRUE;
                                 } else {
                                    needed = Boolean.FALSE;
                                 }
                                 break;
                              }
                              case CullObject.TYPE_MAP:
                              {
                                 if(includeMapObjects()) {
                                    needed = Boolean.TRUE;
                                 } else {
                                    needed = Boolean.FALSE;
                                 }
                                 break;
                              }
                              default:
                              {
                                 if (includeDependObjects()) {
                                    needed = Boolean.TRUE;
                                 }
                              }
                           }
                        }
                     }
                  }
                  if( needed == null ) {
                     needed = Boolean.FALSE;
                  }
               }
               isNeededArmed = false;
            }            
         }
         return needed.booleanValue();
         
      }
      
      boolean toStringArmed = false;
      
      public String toString() {
         StringBuffer ret = new StringBuffer();
         
         ret.append(obj.getName());
         ret.append("(");
         if(obj.isRootObject() ) {
            ret.append("R");            
         }
         CullObject testObj = obj;
         if(obj.getParentObject() != null) {
            testObj = obj.getParentObject();     
         }
         
         if(testObj.getType() == CullObject.TYPE_PRIMITIVE) {
            ret.append("P");
         }
         if(testObj.getType() == CullObject.TYPE_MAPPED) {
            ret.append("M");
         }
         
         ret.append(")");
         if( !toStringArmed ) {
            toStringArmed = true;
            if( isNeeded() ) {
               ret.append("*");
            }
            if( !dependSet.isEmpty() ) {
               ret.append(" --> [");
               Iterator iter  = dependSet.iterator();
               while(iter.hasNext()) {
                  CullObject obj = (CullObject)iter.next();
                  Node node = getNode(obj.getName());
                  ret.append(node.toString());
                  if( iter.hasNext() ) {
                     ret.append(", ");
                  }
               }
               ret.append("]");
            }
            toStringArmed = false;
         }
         return ret.toString();
      }
      
   }
   
   
   
   private static DefaultMutableTreeNode findNode(DefaultMutableTreeNode node, CullObject obj) {
      
      if( node.getUserObject().equals(obj) ) {
         return node;
      }
      
      DefaultMutableTreeNode ret = null;
      for( DefaultMutableTreeNode child = (DefaultMutableTreeNode)node.getFirstChild();
           child != null;
           child = (DefaultMutableTreeNode)child.getNextSibling() ) {
         
         ret = findNode(child,obj);
         if( ret != null ) {
            break;
         }         
      }
      return ret;
   }
   
   
   
}
