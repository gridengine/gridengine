
/****** job_exit_rsmap/sge_rsmap_free_ids() ***************************
*  NAME
*     sge_rsmap_free_ids() --  Function of Univa extension RSMAP complex.
*
*  SYNOPSIS
*     void sge-rsmap_free_ids(u_long32 jobid,
*                             u_long32 taskid,
*                             lListElem *jatep)
*
*  FUNCTION
*     Non-opensource function of Unvia extension RSMAP complex.
*
*  INPUTS
*     u_long32 jobid     - The job id
*     u_long32 taskid    - The task id
*     lListElem *jatep   - Job array task
*
*  NOTES
*     MT-NOTE: sge_rsmap_free_ids() is MT safe
*
*******************************************************************************/
void sge_rsmap_free_ids(u_long32 jobid,
                        u_long32 taskid,
                        lListElem *jatep)
{
   DENTER(TOP_LAYER, "sge_rsmap_free_ids");

   DRETURN_VOID;
}
