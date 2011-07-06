#include "sge_flatfile_obj_remap.h"
#include "cull_multitype.h"
#include "sgeobj/sge_centry_CE_L.h"
#include "uti/sge_log.h"

int read_CE_stringval_host(lListElem *ep, int nm, const char *buf,
                           lList **alp) {

   DENTER(TOP_LAYER, "read_CE_stringval(unimplemented)");

   lSetString(ep, nm, buf);

   DRETURN(1);
}

int write_CE_stringval_host(const lListElem *ep, int nm, dstring *buffer,
                       lList **alp) {

   const char* s = NULL;

   DENTER(TOP_LAYER, "write_CE_stringval_host(unimplemented)");

   if (lGetUlong(ep, CE_valtype) != TYPE_RSMAP) {
      if ((s=lGetString(ep, CE_stringval)) != NULL) {
         sge_dstring_append(buffer, s);
      } else {
         sge_dstring_sprintf_append(buffer, "%f", lGetDouble(ep, CE_doubleval));
      }
   } else {
      WARNING((SGE_EVENT, "RSMAP complex not implemented"));
      DRETURN(0);
   }

   DRETURN(1);
}
