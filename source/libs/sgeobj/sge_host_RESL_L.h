#ifndef __SGE_HOST_RESL_L_H
#define __SGE_HOST_RESL_L_H

/*___INFO__MARK_BEGIN__*/
/*___INFO__MARK_END__*/

#include "cull/cull.h"
#include "sgeobj/sge_boundaries.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* *INDENT-OFF* */

/*
 * Consumable Resource Map Resource List
 * Holds all consumable resource map identifiers
 * of a particular job.
 */
enum {
   RESL_value = RESL_LOWERBOUND,
   RESL_jobid,
   RESL_taskid
};

LISTDEF(RESL_Type)
   JGDI_OBJ(ResourceList)
   /* the ID value of the RSMAP consumable complex */
   SGE_STRING(RESL_value, CULL_DEFAULT | CULL_SPOOL)
   /* the job, which currently uses this value (in case of 0 it is unused) */
   SGE_ULONG(RESL_jobid, CULL_DEFAULT | CULL_SPOOL)
   /* the task, which currently uses this value (in case of 0 it is unused) */
   SGE_ULONG(RESL_taskid, CULL_DEFAULT | CULL_SPOOL)
LISTEND

NAMEDEF(RESLN)
   NAME("RESL_value")
   NAME("RESL_jobid")
   NAME("RESL_taskid")
NAMEEND

#define RESLS sizeof(RESLN)/sizeof(char*)

/* *INDENT-ON* */

#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_HOST_RESL_L_H */
