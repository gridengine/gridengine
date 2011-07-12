#ifndef __SGE_CENTRY_RSMAP_H
#define __SGE_CENTRY_RSMAP_H

#include "basis_types.h"
#include "sge.h"
#include "cull_list.h"

bool centry_check_rsmap(lList **answer_list,
                        u_long32 status,
                        const char* attrname);

#endif /* #ifndef __SGE_CENTRY_RSMAP_H */
