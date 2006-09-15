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
import java.util.*;
import java.util.logging.*;

/**
 *
 * @author rh150277
 */
public class CullDefinitionFilterProxy extends CullDefinition {
   
   private static Logger logger = Logger.getLogger("cullconv");
   
   
   
   private CullDefinition cullDef;
   private String [] includes;
   private String [] excludes;
   private Set filteredCullObjectNameSet;
   private boolean printDependencies;
   private String objectFilter;
   
   /** Creates a new instance of CullDefinitionFilter */
   public CullDefinitionFilterProxy(CullDefinition cullDef, String[] includes, String []excludes, String objectFilter) {
      this.cullDef = cullDef;
      this.includes = includes;
      this.excludes = excludes;
      this.objectFilter = objectFilter;
   }

   public void setPackageName(String packageName) {

      super.setPackageName(packageName);
   }

   public java.io.File getSource(String name) {
      return cullDef.getSource(name);
   }

   public void addFile(String file) throws java.io.IOException, ParseException {
      cullDef.addFile(file);
   }

   public com.sun.grid.cull.CullNameSpace getNameSpace(String name) {
      return cullDef.getNameSpace(name);
   }

   public com.sun.grid.cull.CullObject getCullObject(String name) {
      return cullDef.getCullObject(name);
   }

   public java.io.File getNameSource(String name) {
      return cullDef.getNameSource(name);
   }

   public void addCullObject(com.sun.grid.cull.CullObject obj, java.io.File source) {
      cullDef.addCullObject(obj, source);
   }

   public void addNameSpace(com.sun.grid.cull.CullNameSpace obj, java.io.File source) {

      cullDef.addNameSpace(obj, source);
   }

   public void addFile(java.io.File file) throws java.io.IOException, ParseException {

      cullDef.addFile(file);
   }

   public String getPackageName() {
      return cullDef.getPackageName();
   }

   public java.util.Set getObjectNames() {
      if( this.filteredCullObjectNameSet == null ) {
         Set orgSet = cullDef.getObjectNames();
         filteredCullObjectNameSet = new HashSet();
         
         if( this.includes == null || includes.length == 0 ) {
            filteredCullObjectNameSet.addAll(orgSet);
         } else {

            for(int i = 0; i < includes.length; i++) {
               if( orgSet.contains(includes[i]) ) {
                  filteredCullObjectNameSet.add(includes[i]);
               }
            }

         }
         if( this.excludes != null && this.excludes.length > 0 ) {
            for(int i = 0; i < excludes.length; i++ ) {
               filteredCullObjectNameSet.remove(excludes[i]);
            }
         }

         if(printDependencies) {
            logger.info("dependencies --------------------------------------");
            logger.info("object filter = '" + objectFilter + "'" );
         }
         CullDependencies depend = new CullDependencies(cullDef, filteredCullObjectNameSet, objectFilter);
         
         String [] names = new String[filteredCullObjectNameSet.size()];
         filteredCullObjectNameSet.toArray(names);
         for(int i = 0; i < names.length; i++ ) {

            CullDependencies.Node node = depend.getNode(names[i]);
            if( node == null ) {
               logger.warning("node for obj " + names[i] + " not found");
            } else {

               boolean isNeeded = node.isNeeded();
               
               if(printDependencies) {
                  logger.info(node.toString());
               } else if( logger.isLoggable(Level.FINE ) ) {
                 logger.fine(node.toString());
               }

               if( !isNeeded ) {
                  filteredCullObjectNameSet.remove(names[i]);
               }
            }
         }
         if(filteredCullObjectNameSet.isEmpty()) {
            logger.warning("No cull object matches the objectFilter '" + objectFilter + "'");
         }
      }
      return Collections.unmodifiableSet(filteredCullObjectNameSet);
   }

   public java.util.Set getNameSpaceNameSet() {
      return cullDef.getNameSpaceNameSet();
   }

   protected com.sun.grid.cull.CullDefinition.Elem createElem() {
      return cullDef.createElem();
   }

   public boolean isPrintDependencies() {
      return printDependencies;
   }

   public void setPrintDependencies(boolean printDependencies) {
      this.printDependencies = printDependencies;
   }
   
}
