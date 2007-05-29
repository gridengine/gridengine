<%
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
%>

<%
  java.beans.BeanInfo beanInfo = (java.beans.BeanInfo)params.get("beanInfo");
  Class beanClass = beanInfo.getBeanDescriptor().getBeanClass();
  String classname = beanClass.getName();
  
  String fullClassname = classname.replace('.','/');
  {
      int i = classname.lastIndexOf('.');
      if(i>0) {
          classname = classname.substring(i+1);
      }
  }  
  
  classname = classname.replace('$', '_');
  
  java.beans.PropertyDescriptor [] props = beanInfo.getPropertyDescriptors();
  
  com.sun.grid.javaconv.CWrapperHelper ch = new com.sun.grid.javaconv.CWrapperHelper(beanInfo.getBeanDescriptor().getBeanClass());
  
  java.util.Iterator iter = null;
%>

/*-------------------------------------------------------------------------*
 * NAME
 *   <%=classname%>_find_class - find the jclass object of <%=fullClassname%>
 *
 * PARAMETER
 *  env   - the JNI environment
 *  alpp  - answer list for error reporting
 *
 * RETURN
 *  NULL - class <%=fullClassname%> not found, check the CLASSPATH of
 *         your jvm
 *  else - the class object
 *
 * DESCRIPTION
 *
 * The jclass object for the class <%=fullClassname%> 
 * is stored in static variable of this method. The first call of this method
 * tries to find this class object. If it is found a global reference is aquired
 *
 *-------------------------------------------------------------------------*/
jclass <%=classname%>_find_class(JNIEnv *env, lList** alpp) {
   static jclass clazz = NULL;
   DENTER(BASIS_LAYER, "<%=classname%>_find_class");
   if( clazz == NULL) {
      jclass tmpclazz = (*env)->FindClass(env, "<%=fullClassname%>");
      if (test_jni_error(env, "Class <%=fullClassname%> not found", alpp)) {
          DEXIT;
          return NULL;
      }
      /* aquire a global ref for the class object */
      clazz = (jclass)(*env)->NewGlobalRef(env, tmpclazz);
      if (clazz == NULL) {
         answer_list_add(alpp, 
                         "Can not get a global reference on the <%=fullClassname%> class object", 
                         STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         DEXIT;
         return NULL;
      }
   }
   DEXIT;
   return clazz;
}

<%
  // ---------------------------------------------------------------------------
  // ------------ CONSTRUCTORS -------------------------------------------------
  // ---------------------------------------------------------------------------
   iter = ch.getConstructorNames().iterator();

   while(iter.hasNext()) {
      String constructorName = (String)iter.next();
      java.lang.reflect.Constructor constructor = ch.getConstructor(constructorName);
      Class [] parameters = constructor.getParameterTypes();
%>
/*-------------------------------------------------------------------------*
 * NAME
 *   <%=classname%>_<%=constructorName%> - Construct a new object with
 *                     the constructor "<%=constructor%>"
 *                     of class <%=fullClassname%>
 *           
 * PARAMETER
 *  env  - JNI environment
 *  obj  - pointer where the reference of the new object is stored
<%
      for(int i = 0; i < parameters.length; i++) {%>
 *  p<%=i%>
<%    } // end of fore %>
 *  alpp - answerlist for error reporting
 *
 * RETURN
 *  JGDI_SUCCESS - Object constructed
 *  else         - error occured, reason has been reported into alpp
 *
 * DESCRIPTION
 *
 *  Constructs a new object of class <%=fullClassname%>.
 *  The method id of the java constructor is stored in a static variable.
 *  On the first call of this method this static method id is initialized.
 *-------------------------------------------------------------------------*/
jgdi_result_t <%=classname%>_<%=constructorName%>(JNIEnv *env, jobject *obj <%
   
      for(int i = 0; i < parameters.length; i++) {      
         if( String.class.equals(parameters[i])) {
            /* For strings we want a const char* not a jstring */
          %>, const char* p<%=i%> <%
         } else {
         %>, <%=ch.getCType(parameters[i])%> p<%=i%> <%
         }
      } // end of fore
%>, lList **alpp) {

   jgdi_result_t ret = JGDI_SUCCESS;
   static jmethodID mid = NULL;
   jclass clazz = NULL;<%
   
  // For string parameters we need a local variable
  // which hold the jstring object
  for(int i = 0; i < parameters.length; i++) {
     if( String.class.equals(parameters[i])) {
%>
   jstring p<%=i%>_obj = NULL;<%
     }
  }
   
%>
   DENTER(BASIS_LAYER, "<%=classname%>_<%=constructorName%>");
      
   clazz = <%=classname%>_find_class(env, alpp);
   if (clazz == NULL) {
      DEXIT;
      return JGDI_ILLEGAL_STATE;
   }
   if (mid == NULL) {
      /* initialize the mid */
      mid = get_methodid(env, clazz, "<init>", "<%=ch.getSignature(constructor)%>", alpp);
      if (mid == NULL) {
         DEXIT;
         return JGDI_ILLEGAL_STATE;
      }
   } <%
   
   // All string parameters comes a const char* 
   // we have to allocate the jstring object
   for(int i = 0; i < parameters.length; i++) {
      if( String.class.equals(parameters[i])) {
%>
   if ( p<%=i%> != NULL ) {
      p<%=i%>_obj = (*env)->NewStringUTF(env, p<%=i%> ); 
   }<%
      } // end of if
  } // end of for
%>   
   *obj = (*env)->NewObject(env, clazz, mid <%
      for(int i = 0; i < parameters.length; i++) {      
         if( String.class.equals(parameters[i])) {
            // For all string parameter we pass the jstring object
            // to the method call
          %>, p<%=i%>_obj <%
         } else {
         %>, p<%=i%> <%
         }
      } // end of for
  %>);
  
   if (test_jni_error(env, "call of constructor failed", alpp)) {
      ret = JGDI_ILLEGAL_STATE;
   } 
   
   DEXIT;
   return ret;
}
<%   
   } // end of while(iter.hasNext())
   
  // ---------------------------------------------------------------------------
  // ------------ Static Fields ------------------------------------------------
  // ---------------------------------------------------------------------------

 iter = ch.getStaticFieldNames().iterator();
 while(iter.hasNext()) {
    String fieldName = (String)iter.next();
    java.lang.reflect.Field field = ch.getStaticField(fieldName);
  
    
%>
jgdi_result_t <%=classname%>_static_<%=fieldName%>(JNIEnv *env, <%=ch.getCType(field.getType())%> *res, lList **alpp) {

   jgdi_result_t ret = JGDI_SUCCESS;
   jclass clazz = NULL;
   static jfieldID mid = NULL;
   /*static <%=ch.getCType(field.getType())%> field = <%=ch.getInitializer(field.getType())%>;*/
   
   DENTER(BASIS_LAYER, "<%=classname%>_static_<%=fieldName%>");

   if (env == NULL) {
      answer_list_add(alpp, "env is NULL", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return JGDI_NULL_POINTER;
   }
   
   clazz = <%=classname%>_find_class(env, alpp);
   if (clazz == NULL) {
      answer_list_add(alpp, "class <%=fullClassname%> not found", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return JGDI_ILLEGAL_STATE;
   }
   
   if (mid == NULL) {
      mid = get_static_fieldid(env, clazz, "<%=field.getName()%>", "<%=ch.getSignature(field)%>", alpp);
      if (mid == NULL) {
         DEXIT;
         return JGDI_ILLEGAL_STATE;
      }
   }   
   *res = (*env)-><%=ch.getStaticGetFieldMethod(field)%>(env, clazz, mid);
   if (test_jni_error(env, "<%=classname%>_static_<%=fieldName%> failed", alpp)) {
      ret = JGDI_ILLEGAL_STATE;
   }      
   DEXIT;
   return ret;
}
<%    
 } // end of while
   
   
  // ---------------------------------------------------------------------------
  // ------------ Static METHODS -----------------------------------------------
  // ---------------------------------------------------------------------------

   iter = ch.getStaticMethodNames().iterator();
   while(iter.hasNext()) {
      String methodName = (String)iter.next();
      java.lang.reflect.Method method = ch.getStaticMethod(methodName);
      Class [] parameters = method.getParameterTypes();
      
%>
/*-------------------------------------------------------------------------*
 * NAME
 *   <%=classname%>_static_<%=methodName%> - Call the method "<%=method%>" 
 *                                           of class <%=fullClassname%>
 *           
 * PARAMETER
 *  env    - JNI environment<%
      for(int i = 0; i < parameters.length; i++) {%>
 *  p<%=i%>     -  <%
      } // end of fore 
      if (!Void.TYPE.equals(method.getReturnType())) {%>
 *  result -  pointer to the result<%
      }%>      
 *  alpp   - answerlist for error reporting
 *
 * RETURN
 *  JGDI_SUCCESS - method <%=method%> successfully executed<%
      if (!Void.TYPE.equals(method.getReturnType())) { %>      
 *                 return value has been stored in *result<%    }      
 %>
 *  else         - error occured, reason has been reported in alpp.
 *
 * DESCRIPTION
 *
 *  Call the method "<%=method%>"  of class 
 *  <%=fullClassname%>.
 *  The method id of the java method is stored in a static variable.
 *  On the first call of this method this static method id is initialized.
 *-------------------------------------------------------------------------*/
jgdi_result_t <%=classname%>_static_<%=methodName%>(JNIEnv *env <%
   
      for(int i = 0; i < parameters.length; i++) {         
         if( String.class.equals(parameters[i])) {
            /* For strings we want a const char* not a jstring */
          %>, const char* p<%=i%> <%
         } else {
         %>, <%=ch.getCType(parameters[i])%> p<%=i%> <%
         }
      }

      if (!Void.TYPE.equals(method.getReturnType())) {
         // Add a pointer to the result argument
         %>, <%=ch.getCType(method.getReturnType())%>* result<%
         if(method.getReturnType().isArray()) {
           %>, int* len<%  
         }
      }
    // And finally we need the answer lsit
%>, lList **alpp) {

   jgdi_result_t ret = JGDI_SUCCESS;
   static jmethodID mid = NULL; 
   jclass clazz = NULL;
<%   
  // For string parameters we need a local variable
  // which hold the jstring object
  for(int i = 0; i < parameters.length; i++) {
     if( String.class.equals(parameters[i])) {
%>
   jstring p<%=i%>_obj = NULL;<%
     }
  }
  // For non void methods we temporary store the result
  // of the java method in a local variable
  if (!Void.TYPE.equals(method.getReturnType())) {
     if(method.getReturnType().isArray()) { %>
   jobject temp = NULL;    
<%   } else {
%>
   <%=ch.getCType(method.getReturnType())%> temp = <%=ch.getInitializer(method.getReturnType())%>;
<%
    }
  } 
%>
   DENTER(BASIS_LAYER, "<%=classname%>_static_<%=methodName%>");

<%   
  if (!Void.TYPE.equals(method.getReturnType())) {
%>         
   if (result == NULL ) {
      answer_list_add(alpp, "result is NULL", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return JGDI_NULL_POINTER;
   }
   /* We set the result always to the default value */
<%   
   if(method.getReturnType().isArray()) { %>
   *result = NULL;    
<%   } else {
%>
   *result = <%=ch.getInitializer(method.getReturnType())%>;
<%
    }
  }  %>   
   
   /* Test preconditions */
   if (env == NULL) {
      answer_list_add(alpp, "env is NULL", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return JGDI_NULL_POINTER;
   }
   
   clazz = <%=classname%>_find_class(env, alpp);
   if (clazz == NULL) {
      answer_list_add(alpp, "class <%=fullClassname%> not found", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return JGDI_ILLEGAL_STATE;
   }
   
   if (mid == NULL) {
      mid = get_static_methodid(env, clazz, "<%=method.getName()%>", "<%=ch.getSignature(method)%>", alpp);
      if (mid == NULL) {
         DEXIT;
         return JGDI_ILLEGAL_STATE;
      }
   }<%
   
   // All string parameters comes a const char* 
   // we have to allocate the jstring object
   for(int i = 0; i < parameters.length; i++) {
      if( String.class.equals(parameters[i])) {
%>
   if ( p<%=i%> != NULL ) {
      p<%=i%>_obj = (*env)->NewStringUTF(env, p<%=i%> ); 
   }<%
      } // end of if
  } // end of for
      
  if (!Void.TYPE.equals(method.getReturnType())) {
%>
   temp =<%
  } else { %>
  <%
  }
%> (*env)-><%=ch.getStaticCallMethod(method.getReturnType())%>(env, clazz, mid <%
      /* Add all parameter to the method call */
      for(int i = 0; i < parameters.length; i++) {  
         if( String.class.equals(parameters[i])) {
            // For all string parameter we pass the jstring object
            // to the method call
          %>, p<%=i%>_obj <%
         } else {
         %>, p<%=i%> <%
         }
      }
  %>);  
   if (test_jni_error(env, "<%=classname%>_<%=methodName%> failed", alpp)) {
      ret = JGDI_ILLEGAL_STATE;
<%  if (!Void.TYPE.equals(method.getReturnType())) {
       if (method.getReturnType().isArray()) {
%>
      temp = NULL;
<%     } else { %>
      temp = <%=ch.getInitializer(method.getReturnType())%>;        
<%       }    
    }
%>      
   } <%
  if (!Void.TYPE.equals(method.getReturnType())) {
     // for non void method store the temporary result
     // in the result pointer
  %> else {
  <%
     if(method.getReturnType().isArray()) {
         
       Class realType = method.getReturnType().getComponentType();
       String realCType = ch.getCType(realType);
  %>
       if(temp == NULL) {
          *result = NULL;
          *len = 0;
       } else {
           jint arrayLen = (*env)->GetArrayLength(env, (jarray)temp);
           if (test_jni_error(env, "Can not get the array length of the result", alpp)) {
              ret = JGDI_ILLEGAL_STATE;
           } if (arrayLen > 0) {
               <%=realCType%> *array = NULL;
               int i = 0;
               <% if(realType.isPrimitive()) { %>
               jboolean isCopy = false;
               <%=realCType%> *tmpArray = NULL;
               
               tmpArray = (*env)-><%=ch.getGetArrayElementsMethod(realType)%>(env, (<%=realCType%>Array)temp, &isCopy);
               if (test_jni_error(env, "Can not get the array elements of the result", alpp)) {
                  ret = JGDI_ILLEGAL_STATE;
               } else {
                  array = (<%=realCType%>*)malloc(sizeof(<%=realCType%>)* arrayLen);

                  for(i = 0; i < arrayLen; i++) {
                     array[i] = tmpArray[i];
                  }
                  (*env)-><%=ch.getReleaseArrayElementsMethod(realType)%>(env, (<%=realCType%>Array)temp, tmpArray, JNI_ABORT);
                  if (test_jni_error(env, "release the array elements of the result failed", alpp)) {
                     free(array);
                  } else {
                     *result = array;
                     *len = arrayLen;
                  }
               }
               
               <% } else { %>
               
               array = (<%=realCType%>*)malloc(sizeof(<%=realCType%>)* arrayLen);
               
               for(i = 0; i < arrayLen; i++) {
                  array[i] = (*env)->GetObjectArrayElement(env, (jobjectArray)temp, i); 
                  if (test_jni_error(env, "Can not get object from array", alpp)) {
                     free(array);
                     array = NULL;
                     break;
                  }
               }
               
               if(array != NULL) {
                  *result = array;
                  *len = arrayLen;
               }
               
               <% } %>
          } else {
             *result = NULL;
             *len = 0;
          }
       }
<%     } else { %>
      *result = temp;
<%   } %>      
   }
<% } %>
   DEXIT; 
   return ret;
}<%
   } // end of while(iter.hasNext())
   
   
  // ---------------------------------------------------------------------------
  // ------------ METHODS ------------------------------------------------------
  // ---------------------------------------------------------------------------

   iter = ch.getMethodNames().iterator();
   while(iter.hasNext()) {
      String methodName = (String)iter.next();
      java.lang.reflect.Method method = ch.getMethod(methodName);
      Class [] parameters = method.getParameterTypes();
      
%>
/*-------------------------------------------------------------------------*
 * NAME
 *   <%=classname%>_<%=methodName%> - Call the method "<%=method%>" 
 *                                    of class <%=fullClassname%>
 *           
 * PARAMETER
 *  env    - JNI environment
 *  obj    - the jobject<%
      for(int i = 0; i < parameters.length; i++) {%>
 *  p<%=i%>     -  <%
      } // end of fore 
      if (!Void.TYPE.equals(method.getReturnType())) {%>
 *  result -  pointer to the result<%
      }%>      
 *  alpp   - answerlist for error reporting
 *
 * RETURN
 *  JGDI_SUCCESS - method <%=ch.getMethod(methodName)%> successfully executed<%
      if (!Void.TYPE.equals(method.getReturnType())) { %>      
 *                 return value has been stored in *result<%    }      
 %>
 *  else         - error occured, reason has been reported in alpp.
 *
 * DESCRIPTION
 *
 *  Call the method "<%=ch.getMethod(methodName)%>"  of class 
 *  <%=fullClassname%>.
 *  The method id of the java method is stored in a static variable.
 *  On the first call of this method this static method id is initialized.
 *-------------------------------------------------------------------------*/
jgdi_result_t <%=classname%>_<%=methodName%>(JNIEnv *env, <%=ch.getCType(beanClass)%> obj <%
   
      for(int i = 0; i < parameters.length; i++) {         
         if( String.class.equals(parameters[i])) {
            /* For strings we want a const char* not a jstring */
          %>, const char* p<%=i%> <%
         } else {
         %>, <%=ch.getCType(parameters[i])%> p<%=i%> <%
         }
      }

      if (!Void.TYPE.equals(method.getReturnType())) {
         // Add a pointer to the result argument
         %>, <%=ch.getCType(method.getReturnType())%>* result<%
         if(method.getReturnType().isArray()) {
           %>, int* len<%  
         }
      }
    // And finally we need the answer lsit
%>, lList **alpp) {

   jgdi_result_t ret = JGDI_SUCCESS;
   static jmethodID mid = NULL; 
   jclass clazz = NULL;
<%   
  // For string parameters we need a local variable
  // which hold the jstring object
  for(int i = 0; i < parameters.length; i++) {
     if( String.class.equals(parameters[i])) {
%>
   jstring p<%=i%>_obj = NULL;<%
     }
  }
  // For non void methods we temporary store the result
  // of the java method in a local variable
  if (!Void.TYPE.equals(method.getReturnType())) {
     if(method.getReturnType().isArray()) { %>
   jobject temp = NULL;    
<%   } else {
%>
   <%=ch.getCType(method.getReturnType())%> temp = <%=ch.getInitializer(method.getReturnType())%>;
<%
    }
  } 
%>
   DENTER(BASIS_LAYER, "<%=classname%>_<%=methodName%>");

<%   
  if (!Void.TYPE.equals(method.getReturnType())) {
%>         
   if (result == NULL ) {
      answer_list_add(alpp, "result is NULL", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return JGDI_NULL_POINTER;
   }
   /* We set the result always to the default value */
<%   
   if(method.getReturnType().isArray()) { %>
   *result = NULL;    
<%   } else {
%>
   *result = <%=ch.getInitializer(method.getReturnType())%>;
<%
    }
  }  %>   

   /* Test preconditions */
   if (env == NULL) {
      answer_list_add(alpp, "env is NULL", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return JGDI_NULL_POINTER;
   }
   if (obj == NULL) {
      answer_list_add(alpp, "obj is NULL", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return JGDI_NULL_POINTER;
   }
   
   clazz = <%=classname%>_find_class(env, alpp);
   if (clazz == NULL) {
      answer_list_add(alpp, "class <%=fullClassname%> not found", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return JGDI_ILLEGAL_STATE;
   }
   
   if (!(*env)->IsInstanceOf(env, obj, clazz)) {
      answer_list_add(alpp, "obj is not an instanceof <%=fullClassname%>", STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return JGDI_ILLEGAL_STATE;
   }

   if (mid == NULL) {
      mid = get_methodid(env, clazz, "<%=method.getName()%>", "<%=ch.getSignature(method)%>", alpp);
      if (mid == NULL) {
         DEXIT;
         return JGDI_ILLEGAL_STATE;
      }
   }<%
   
   // All string parameters comes a const char* 
   // we have to allocate the jstring object
   for(int i = 0; i < parameters.length; i++) {
      if( String.class.equals(parameters[i])) {
%>
   if ( p<%=i%> != NULL ) {
      p<%=i%>_obj = (*env)->NewStringUTF(env, p<%=i%> ); 
   }<%
      } // end of if
  } // end of for
      
  if (!Void.TYPE.equals(method.getReturnType())) {
%>
   temp =<%
  } else { %>
  <%
  }
%> (*env)-><%=ch.getCallMethod(method.getReturnType())%>(env, obj, mid <%
      // Add all parameter to the method call 
      for(int i = 0; i < parameters.length; i++) {  
         if( String.class.equals(parameters[i])) {
            // For all string parameter we pass the jstring object
            // to the method call
          %>, p<%=i%>_obj <%
         } else {
         %>, p<%=i%> <%
         }
      }
  %>);  
   if (test_jni_error(env, "<%=classname%>_<%=methodName%> failed", alpp)) {
      ret = JGDI_ILLEGAL_STATE;
<%  if (!Void.TYPE.equals(method.getReturnType())) {
       if (method.getReturnType().isArray()) {
%>
      temp = NULL;
<%     } else { %>
      temp = <%=ch.getInitializer(method.getReturnType())%>;        
<%       }    
    }
%>      
   } <%
  if (!Void.TYPE.equals(method.getReturnType())) {
     // for non void method store the temporary result
     // in the result pointer
 
     if(method.getReturnType().isArray()) {
         
       Class realType = method.getReturnType().getComponentType();
       String realCType = ch.getCType(realType);
  %>
       if(temp == NULL) {
          *result = NULL;
          *len = 0;
       } else {
           jint arrayLen = (*env)->GetArrayLength(env, (jarray)temp);
           if (test_jni_error(env, "Can not get the array length of the result", alpp)) {
              ret = JGDI_ILLEGAL_STATE;
           } if (arrayLen > 0) {
               
               <%=realCType%> *array = NULL;
               int i = 0;
               <% if(realType.isPrimitive()) { %>
               jboolean isCopy = false;
               <%=realCType%> *tmpArray = NULL;
               
               tmpArray = (*env)-><%=ch.getGetArrayElementsMethod(realType)%>(env, (<%=realCType%>Array)temp, &isCopy);
               if (test_jni_error(env, "Can not get the array elements of the result", alpp)) {
                  ret = JGDI_ILLEGAL_STATE;
               } else {
                  array = (<%=realCType%>*)malloc(sizeof(<%=realCType%>)* arrayLen);

                  for(i = 0; i < arrayLen; i++) {
                     array[i] = tmpArray[i];
                  }
                  (*env)-><%=ch.getReleaseArrayElementsMethod(realType)%>(env, (<%=realCType%>Array)temp, tmpArray, JNI_ABORT);
                  if (test_jni_error(env, "release the array elements of the result failed", alpp)) {
                     free(array);
                  } else {
                     *result = array;
                     *len = arrayLen;
                  }
               }
               
               <% } else { %>
               
               array = (<%=realCType%>*)malloc(sizeof(<%=realCType%>)* arrayLen);
               
               for(i = 0; i < arrayLen; i++) {
                  array[i] = (*env)->GetObjectArrayElement(env, (jobjectArray)temp, i); 
                  if (test_jni_error(env, "Can not get object from array", alpp)) {
                     free(array);
                     array = NULL;
                     break;
                  }
               }
               
               if(array != NULL) {
                  *result = array;
                  *len = arrayLen;
               }
               
               <% } %>
               
          } else {
             *result = NULL;
             *len = 0;
          }
       }
<%     } else { %>
      *result = temp;
<%     } %>      
<% } // end of !Void.TYPE %>
   DEXIT; 
   return ret;
}<%
   } // end of while(iter.hasNext())
%>

