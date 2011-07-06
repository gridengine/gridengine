#ifndef __SGE_SCHED_THREAD_REMAP_H
#define __SGE_SCHED_THREAD_REMAP_H

#include "basis_types.h"
#include "sge.h"
#include "sge_rmon.h"
#include "cull_list.h"

void add_granted_resource_list(lListElem *ja_task, lListElem *job, lList *granted, lList *host_list);

#endif /* #ifndef __SGE_SCHED_THREAD_REMAP_H */
