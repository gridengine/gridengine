#include "basis_types.h"
#include "sge.h"
#include "sge_sched_thread_remap.h"
#include "sge_rmon.h"
#include "sge_centry_CE_L.h"
#include "sge_grantedres_GRU_L.h"
#include "sge_ja_task_JAT_L.h"
#include "sge_job_JB_L.h"
#include "sge_ulong.h"
#include "sgeobj/sge_job_JG_L.h"
#include "sgeobj/sge_host.h"
#include "sgeobj/sge_host_RESL_L.h"
#include "sgeobj/sge_centry.h"
#include "uti/sge_string.h"
#include "uti/sge_rmon.h"
#include "uti/sge_log.h"

/****** sge_sched_thread_remap/add_granted_resource_list() *********************
*  NAME
*     get_remap_ids() --  Function of Univa extension RSMAP complex.
*
*  SYNOPSIS
*     void add_granted_resource_list(lListElem *ja_task,
*                                    lListElem *job,
*                                    lList *granted,
*                                    lList *host_list)
*
*  FUNCTION
*     Non-opensource function of Unvia extension RSMAP complex.
*
*  INPUTS
*     lListElem *ja_task - The job array task
*     lListElem *job     - The job
*     lList *granted     - The granted destination identifier list
*     lList *host_list   - The host list
*
*  NOTES
*     MT-NOTE: add_granted_resource_list() is MT safe
*
*******************************************************************************/
void add_granted_resource_list(
            lListElem *ja_task,
            lListElem *job,
            lList *granted,
            lList *host_list) {

   DENTER(TOP_LAYER, "add_granted_resource_list(unimplemented)");

   DRETURN_VOID;
}

