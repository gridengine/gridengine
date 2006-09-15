<%
   com.sun.grid.cull.JavaHelper jh = (com.sun.grid.cull.JavaHelper)params.get("javaHelper");
   com.sun.grid.cull.CullDefinition cullDef = (com.sun.grid.cull.CullDefinition)params.get("cullDef");
   com.sun.grid.cull.CullObject cullObj = (com.sun.grid.cull.CullObject)params.get("cullObj");
   
   class CGDIGenerator {
       
       String fullClassname;
       String classname;
       String listname;
       String cullname;
       
       public CGDIGenerator(String fullClassname, String classname, String listname, String cullname) {
           this.fullClassname = fullClassname.replace('.','/');
           this.classname = classname;
           this.listname = listname;
           this.cullname = cullname;
       } 
       
       
       public void genListMethod() {
           
           String methodName = "Java_com_sun_grid_jgdi_jni_JGDIImpl_fill" + classname + "List";
%>           
/*
 * Class:     com_sun_grid_jgdi_jni_JGDIImpl
 * Method:    get<%=classname%>List
 */
JNIEXPORT void JNICALL <%=methodName%>(JNIEnv *env, jobject jgdi, jobject list, jobject filter) {
   
   DENTER(TOP_LAYER, "<%=methodName%>");
   jgdi_fill(env, jgdi, list, filter, "<%=fullClassname%>", <%=listname%>, <%=cullname%>);
   DEXIT;
}
<%           
       } // end of genListMethod
       
       
       public void genAddMethod() {
         String methodName = "Java_com_sun_grid_jgdi_jni_JGDIImpl_add" + classname;
%>
/* -------------- ADD ------------------------------------------------------- */
/*
 * Class:     com_sun_grid_jgdi_jni_JGDIImpl
 * Method:    add<%=classname%>
 * Signature: (L<%=fullClassname%>;)V
 */
JNIEXPORT void JNICALL <%=methodName%>(JNIEnv *env, jobject jgdi, jobject jobj)
{
   DENTER(TOP_LAYER, "<%=methodName%>");
   jgdi_add(env, jgdi, jobj, "<%=fullClassname%>", <%=listname%>, <%=cullname%>);
   DEXIT;
}
<%
       } // end of genAddMethod     
           
       public void genDeleteMethod() {
         String methodName = "Java_com_sun_grid_jgdi_jni_JGDIImpl_delete" + classname;
%>
/*
 * Class:     com_sun_grid_jgdi_jni_JGDIImpl
 * Method:    delete<%=classname%>
 * Signature: (L<%=fullClassname%>;)V
 */
JNIEXPORT void JNICALL <%=methodName%>(JNIEnv *env, jobject jgdi, jobject jobj)
{
   DENTER(TOP_LAYER, "<%=methodName%>");
   jgdi_delete(env, jgdi, jobj, "<%=fullClassname%>", <%=listname%>, <%=cullname%>);
   DEXIT;
}
<%                      
       } // end of genDeleteMethod
       
       public void genUpdateMethod() {
           
          String methodName = "Java_com_sun_grid_jgdi_jni_JGDIImpl_update" + classname;
%>
/* -------------- Update ------------------------------------------------------- */
/*
 * Class:     com_sun_grid_jgdi_jni_JGDIImpl
 * Method:    update<%=classname%>
 * Signature: (L<%=fullClassname%>;)V
 */
JNIEXPORT void JNICALL <%=methodName%>(JNIEnv *env, jobject jgdi, jobject jobj)
{
   DENTER(TOP_LAYER, "<%=methodName%>");
   jgdi_update(env, jgdi, jobj, "<%=fullClassname%>", <%=listname%>, <%=cullname%>);
   DEXIT;
}
<%           
       } // end of genUpdateMethod
   } // end of class CGDIGenerator
   
   // -------------------------------------------------------------------------
   // Build the generators 
   // -------------------------------------------------------------------------

   java.util.List generators = new java.util.ArrayList();
   
   if (cullObj == null ) {
     throw new IllegalStateException("param cullObj not found");
   }
   if (cullObj.getIdlName() == null ) {
     throw new IllegalStateException("cullObj " + cullObj.getName() + " is has no idl name");
   }
   if (cullDef == null ) {
     throw new IllegalStateException("param cullDef not found");
   }
   if (!cullObj.isRootObject()) {
      return;
   }
   String listname = cullObj.getListName();

   if (listname == null) {
     // we not a ILISTDEF, return
     return;
   }
   {
   CGDIGenerator gen = null;
   
   if (cullObj.getParentName() != null) {      
      gen = new CGDIGenerator(jh.getFullClassName(cullObj), cullObj.getIdlName(), 
                              listname, cullObj.getParentName());
   } else {
      gen = new CGDIGenerator(jh.getFullClassName(cullObj), cullObj.getIdlName(), 
                              listname, cullObj.getName());
   }
   generators.add(gen);
   }
%>
#include <ctype.h>
#include <string.h>
#include "jni.h"
#include "basis_types.h"
#include "cull.h"
#include "commlib.h"
#include "sgermon.h"
#include "sge_all_listsL.h"
#include "sge_answer.h"
#include "sge_prog.h"
#include "sge_bootstrap.h"
#include "sge_gdi.h"
#include "sge_gdi_ctx.h"
#include "sge_gdi2.h"
#include "cl_errors.h"
#include "setup.h"
#include "sge_log.h"
#include "sge_error_class.h"
#include "jgdi_common.h"
#include "jgdi.h"

#define JGDI_DEBUG

<%
   java.util.Iterator iter = generators.iterator();
   
   while(iter.hasNext()) {
      CGDIGenerator gen = (CGDIGenerator)iter.next();
      gen.genListMethod();
      gen.genAddMethod();
      gen.genDeleteMethod();
      gen.genUpdateMethod();
   }
   
%>


