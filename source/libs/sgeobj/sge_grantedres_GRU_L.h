#ifndef __SGE_GRANTEDRESL_H
#define __SGE_GRANTEDRESL_H

/*___INFO__MARK_BEGIN__*/
/*___INFO__MARK_END__*/

#include "cull/cull.h"

#include "sgeobj/sge_boundaries.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* *INDENT-OFF* */

/*
 * Granted resource element
 */
enum {
   GRU_type = GRU_LOWERBOUND,
   GRU_name,
   GRU_value,
   GRU_host
};

LISTDEF(GRU_Type)
   JGDI_OBJ(GrantedResourcesUsed)
   SGE_ULONG(GRU_type, CULL_DEFAULT | CULL_SPOOL)
   SGE_STRING(GRU_name, CULL_DEFAULT | CULL_SPOOL)
   SGE_STRING(GRU_value, CULL_DEFAULT | CULL_SPOOL)
   SGE_STRING(GRU_host, CULL_DEFAULT | CULL_SPOOL)
LISTEND

NAMEDEF(GRUN)
   NAME("GRU_type")
   NAME("GRU_name")
   NAME("GRU_value")
   NAME("GRU_host")
NAMEEND

#define GRUS sizeof(GRUN)/sizeof(char*)

/* *INDENT-ON* */

#ifdef  __cplusplus
}
#endif

#endif /* __SGE_GRANTEDRESL_H */
