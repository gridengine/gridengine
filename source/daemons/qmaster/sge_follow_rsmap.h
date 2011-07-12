#ifndef __SGE_FOLLOW_RSMAP_H
#define __SGE_FOLLOW_RSMAP_H

#include "basis_types.h"
#include "sge.h"
#include "sge_rmon.h"
#include "cull_list.h"

void debit_rsmap_consumable(lListElem *jep, lListElem *hep,
                                lList *centry_list, bool *ok);

void debit_rsmap_consumable_task(lListElem *jep, lListElem *hep,
                                lList *centry_list, bool *ok, u_long32 jobid,
                                u_long32 taskid);


#endif /* #ifndef __SGE_FOLLOW_RSMAP_H */
