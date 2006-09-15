#ifndef JGDI_EVENT_H
#define JGDI_EVENT_H


jgdi_result_t getEVC(JNIEnv *env, jobject evcobj, sge_evc_class_t **evc, lList **alpp);


jgdi_result_t process_generic_event(JNIEnv *env,  jobject *event, lListElem *ev, lList** alpp);

jgdi_result_t create_generic_event(JNIEnv *env, jobject *event_obj, const char* beanClassName, 
                                   const char* cullTypeName, lDescr *descr, int event_action, lListElem *ev, lList **alpp);

#include "jgdi_event_gen.h"

#endif
