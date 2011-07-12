#ifndef __JOB_EXIT_RSMAP_H
#define __JOB_EXIT_RSMAP_H

#include "basis_types.h"
#include "sge.h"
#include "sge_rmon.h"
#include "cull_list.h"
#include "sge_object.h"
#include "sge_event_master.h"

void sge_rsmap_free_ids(u_long32 jobid,
                        u_long32 taskid,
                        lListElem *jatep);

#endif /* #ifndef __JOB_EXIT_RSMAP_H */
