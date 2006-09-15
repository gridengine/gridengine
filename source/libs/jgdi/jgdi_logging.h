#ifndef JGDI_LOGGING_H
#define JGDI_LOGGING_H


typedef enum {
   SEVERE = 0,
   WARNING,
   INFO,
   CONFIG,
   FINE,
   FINER,
   FINEST,
   LOG_LEVEL_COUNT
} log_level_t;

#define JGDI_LOGGER        "com.sun.grid.jgdi.JGDI"
#define JGDI_QSTAT_LOGGER  "com.sun.grid.jgdi.monitoring.qstat"
#define JGDI_QHOST_LOGGER  "com.sun.grid.jgdi.monitoring.qhost"
#define JGDI_EVENT_LOGGER  "com.sun.grid.jgdi.event"

jobject jgdi_get_logger(JNIEnv *env, const char* logger);
jboolean jgdi_is_loggable(JNIEnv *env, jobject logger, log_level_t level);
void jgdi_log_printf(JNIEnv *env, const char* logger, log_level_t level, const char* fmt, ...);
void jgdi_log(JNIEnv *env, jobject logger, log_level_t level, const char* msg);
void jgdi_log_list(JNIEnv *env, const char* logger, log_level_t level, lList* list);
void jgdi_log_answer_list(JNIEnv *env, const char* logger, lList *alp);

int jgdi_init_rmon_ctx(JNIEnv *env, const char* logger, rmon_ctx_t *ctx);
void jgdi_destroy_rmon_ctx(rmon_ctx_t *rmon_ctx);

#endif
