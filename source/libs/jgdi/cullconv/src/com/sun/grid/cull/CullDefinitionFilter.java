/*
 * CullDefinitionFilter.java
 *
 * Created on September 21, 2006, 4:19 PM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

package com.sun.grid.cull;

import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 *
 * @author rh150277
 */
public class CullDefinitionFilter {
    
   private static Logger logger = Logger.getLogger("cullconv");
    
   private String [] includes;
   private String [] excludes;
   private String objectFilter;
   private boolean printDependencies;
    

    /** Creates a new instance of CullDefinitionFilter */
    public CullDefinitionFilter(String [] includes, String [] excludes) {
        this.includes = includes;
        this.excludes = excludes;
    }
    
    public java.util.Set getObjectNames(CullDefinition cullDef) {
        
         Set orgSet = cullDef.getObjectNames();
         HashSet filteredCullObjectNameSet = new HashSet();
         
         if( this.includes == null || this.includes.length == 0 ) {
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

         if(isPrintDependencies()) {
            logger.info("dependencies --------------------------------------");
            logger.info("object filter = '" + getObjectFilter() + "'" );
         }
         CullDependencies depend = new CullDependencies(cullDef, filteredCullObjectNameSet, getObjectFilter());
         
         String [] names = new String[filteredCullObjectNameSet.size()];
         filteredCullObjectNameSet.toArray(names);
         for(int i = 0; i < names.length; i++ ) {

            CullDependencies.Node node = depend.getNode(names[i]);
            if( node == null ) {
               logger.warning("node for obj " + names[i] + " not found");
            } else {

               boolean isNeeded = node.isNeeded();
               
               if(isPrintDependencies()) {
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
            logger.warning("No cull object matches the objectFilter '" + getObjectFilter() + "'");
         }
         return filteredCullObjectNameSet;
    }

    public String getObjectFilter() {
        return objectFilter;
    }

    public void setObjectFilter(String objectFilter) {
        this.objectFilter = objectFilter;
    }

    public boolean isPrintDependencies() {
        return printDependencies;
    }

    public void setPrintDependencies(boolean printDependencies) {
        this.printDependencies = printDependencies;
    }
    
}
