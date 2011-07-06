#ifndef __SGE_FLATFILE_OBJ_REMAP_H
#define __SGE_FLATFILE_OBJ_REMAP_H

/*#include "basis_types.h"
#include "sge.h" */
#include "sge_rmon.h"
#include "cull_list.h"

int read_CE_stringval_host(lListElem *ep, int nm, const char *buf,
                           lList **alp);

int write_CE_stringval_host(const lListElem *ep, int nm, dstring *buffer,
                       lList **alp);

#endif /* #ifndef __SGE_FLATFILE_OBJ_REMAP_H */
