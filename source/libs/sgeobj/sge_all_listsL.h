#ifndef __SGE_ALL_LISTSL_H
#define __SGE_ALL_LISTSL_H

/*___INFO__MARK_BEGIN__*/
/*************************************************************************
 * 
 *  The Contents of this file are made available subject to the terms of
 *  the Sun Industry Standards Source License Version 1.2
 * 
 *  Sun Microsystems Inc., March, 2001
 * 
 * 
 *  Sun Industry Standards Source License Version 1.2
 *  =================================================
 *  The contents of this file are subject to the Sun Industry Standards
 *  Source License Version 1.2 (the "License"); You may not use this file
 *  except in compliance with the License. You may obtain a copy of the
 *  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
 * 
 *  Software provided under this License is provided on an "AS IS" basis,
 *  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
 *  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
 *  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
 *  See the License for the specific provisions governing your rights and
 *  obligations concerning the Software.
 * 
 *   The Initial Developer of the Original Code is: Sun Microsystems, Inc.
 * 
 *   Copyright: 2001 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/
#include "cull.h"

/* Definition of new names */
#include "sge_answerL.h"
#include "sge_ckptL.h"
#include "sge_eventL.h"
#include "sge_identL.h"
#include "sge_jobL.h"
#include "sge_job_refL.h"
#include "sge_krbL.h"
#include "sge_orderL.h"
#include "sge_path_aliasL.h"
#include "sge_peL.h"
#include "sge_ptfL.h"
#include "sge_reportL.h"
#include "sge_schedd_conf.h"
#include "sge_sharetreeL.h"
#include "sge_stringL.h"
#include "sge_usageL.h"
#include "sge_usersetL.h"
#include "parse_qsubL.h"
#include "sge_multiL.h"
#include "sge_time_eventL.h"
#include "sge_qexecL.h"
#include "sge_messageL.h"
#include "sge_ja_task.h"
#include "sge_ctL.h"
#include "sge_access_treeL.h"
#include "sge_eejobL.h"
#include "sge_ulongL.h"
#include "qmon_prefL.h"
#include "sge_cuserL.h"
#include "sge_permissionL.h"
#include "sge_loadsensorL.h"
#include "sge_featureL.h"
#include "sge_suserL.h"
#include "sge_secL.h"
#include "sge_pe_taskL.h"
#include "sge_varL.h"
#include "sge_rangeL.h"
#include "sge_queueL.h"
#include "sge_userprjL.h"
#include "sge_hostL.h"
#include "sge_complexL.h"
#include "sge_manopL.h"
#include "sge_calendarL.h"
#include "sge_hgroupL.h"
#include "sge_confL.h"
#include "spool/sge_spoolingL.h"
#include "sge_mailrecL.h"
#include "sge_japiL.h"
#include "sge_hrefL.h"
#include "sge_attrL.h"
#include "sge_helperL.h"

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef  __cplusplus
}
#endif
#if defined(__SGE_GDI_LIBRARY_HOME_OBJECT_FILE__)
#ifdef  __cplusplus
extern "C" {
#endif

   lNameSpace nmv[] = {

/*  
   1. unique keq of the first element in the descriptor 
   2. number of elements in the descriptor 
   3. array with names describing the fields of the descriptor
          1.        2.           3.
*/
      {JB_LOWERBOUND, JBS, JBN},        /* job */
      {QU_LOWERBOUND, QUS, QUN},        /* queue */
      {EH_LOWERBOUND, EHS, EHN},        /* exec host */
      {AH_LOWERBOUND, AHS, AHN},        /* admin host */
      {SH_LOWERBOUND, SHS, SHN},        /* submit host */
      {AN_LOWERBOUND, ANS, ANN},        /* gdi acknowledge format */
      {HL_LOWERBOUND, HLS, HLN},        /* load of an exec host */
      {HS_LOWERBOUND, HSS, HSN},        /* scaling of and exec host */
      {EV_LOWERBOUND, EVS, EVN},        /* event client */
      {ET_LOWERBOUND, ETS, ETN},        /* event */
      {CE_LOWERBOUND, CES, CEN},        /* complex entity */
      {LR_LOWERBOUND, LRS, LRN},        /* load report */
      {OR_LOWERBOUND, ORS, ORN},        /* ?? */
      {OQ_LOWERBOUND, OQS, OQN},        /* ?? */
      {US_LOWERBOUND, USES, USEN},      /* user set */
      {UE_LOWERBOUND, UES, UEN},        /* user set entry */
      {RN_LOWERBOUND, RNS, RNN},        /* range list */
      {PN_LOWERBOUND, PNS, PNN},        /* path name list */
      {AT_LOWERBOUND, ATS, ATN},        /* account list */
      {VA_LOWERBOUND, VAS, VAN},        /* variable list */
      {MR_LOWERBOUND, MRS, MRN},        /* mail recipiants list */
      {MO_LOWERBOUND, MOS, MON},        /* manager and operator list */
      {PE_LOWERBOUND, PES, PEN},        /* parallel environment object */
      {QR_LOWERBOUND, QRS, QRN},        /* queue reference used in PE object */
      {JC_LOWERBOUND, JCS, JCN},        /* job couter used in schedd */
      {CONF_LOWERBOUND, CONFS, CONFN},  /* config */
      {CF_LOWERBOUND, CFS, CFN},        /* config list */
      {ST_LOWERBOUND, STS, STN},        /* string list */
      {JG_LOWERBOUND, JGS, JGN},        /* jobs sublist of granted destinatin 
                                         * identifiers */
      {SO_LOWERBOUND, SOS, SON},        /* subordinate configuration list */
      {QAJ_LOWERBOUND, QAJS, QAJN},     /* list for qacct special purpose */
      {SPA_LOWERBOUND, SPAS, SPAN},     /* option parse struct */
      {REP_LOWERBOUND, REPS, REPN},     /* report list */
      {UA_LOWERBOUND, UAS, UAN},        /* usage list */
      {UP_LOWERBOUND, UPS, UPN},        /* SGEEE - user/project */
      {STN_LOWERBOUND, STNS, STNN},     /* SGEEE - share tree node */
      {SC_LOWERBOUND, SCS, SCN},        /* scheduler config */
      {PET_LOWERBOUND, PETS, PETN},     /* PE Task object */
      {PETR_LOWERBOUND, PETRS, PETRN},  /* PE Task request object */
      {FPET_LOWERBOUND, FPETS, FPETN},  /* finished PE Task reference */
      {JR_LOWERBOUND, JRS, JRN},        /* Job report */
      {LIC_LOWERBOUND, LICS, LICN},     /* structure of license report */
      {CK_LOWERBOUND, CKS, CKN},        /* checkpointing object */
      {UPU_LOWERBOUND, UPUS, UPUN},     /* SGEEE - sublist of user/project for
                                         * storing jobs old usage */
      {KRB_LOWERBOUND, KRBS, KRBN},     /* Kerberos connection list */
      {PA_LOWERBOUND, PAS, PAN},        /* Path alias list */
      {JRE_LOWERBOUND, JRES, JREN},     /* job reference */
      {ID_LOWERBOUND, IDS, IDN},        /* id struct used for qmod requests */
      {MA_LOWERBOUND, MAS, MAN},        /* ma struct used for multi gdi
                                         * requests */

      {TE_LOWERBOUND, TES, TEN},        /* time event struct used for timer
                                         * in qmaster */
      {CAL_LOWERBOUND, CALS, CALN},     /* calendar week/year */
      {CA_LOWERBOUND, CAS, CAN},        /* calendar week/year */
      {TMR_LOWERBOUND, TMRS, TMRN},     /* time range */
      {TM_LOWERBOUND, TMS, TMN},        /* cullified struct tm */

      {RT_LOWERBOUND, RTS, RTN},        /* remote task (qrexec) */
      {UPP_LOWERBOUND, UPPS, UPPN},     /* SGEEE - sublist of user/project for
                                         * storing project usage */
      {KTGT_LOWERBOUND, KTGTS, KTGTN},  /* Kerberos TGT list */
      {SME_LOWERBOUND, SMES, SMEN},     /* scheduler message structure */
      {MES_LOWERBOUND, MESS, MESN},     /* scheduler job info */
      {JAT_LOWERBOUND, JATS, JATN},     /* JobArray task structure contains
                                         * the dynamic elements of a Job */
      {CT_LOWERBOUND, CTS, CTN},        /* scheduler job category */

      {PGR_LOWERBOUND, PGRS, PGRN},     /* scheduler access tree: priority
                                         * group */
      {USR_LOWERBOUND, USRS, USRN},     /* scheduler access tree: user */
      {JRL_LOWERBOUND, JRLS, JRLN},     /* scheduler access tree: job
                                         * reference */

      /* former ptf_nmv entries */
      {JL_LOWERBOUND, JLS, JLN},        /* ptf job list */
      {JO_LOWERBOUND, JOS, JON},        /* ptf O.S. job list */

      {HGRP_LOWERBOUND, HGRPS, HGRPN},  /* hostgroup list */
      {HR_LOWERBOUND, HRS, HRN},        /* host/group reference list */
      {PERM_LOWERBOUND, PERMS, PERMN},  /* permission list */
      {CU_LOWERBOUND, CUS, CUN},        /* usermap entry list for
                                         * administrator mapping */
      {LS_LOWERBOUND, LSS, LSN},        /* load sensor list */
      {FES_LOWERBOUND, FESS, FESN},     /* feature list */
      {FE_LOWERBOUND, FES, FEN},        /* feature list */
      {SU_LOWERBOUND, SUS, SUN},        /* submit user */
      {SEC_LOWERBOUND, SecurityS, SecurityN},   /* Certificate security */

      {SPC_LOWERBOUND, SPCS, SPCN},     /* Spooling context */
      {SPR_LOWERBOUND, SPRS, SPRN},     /* Spooling rule */
      {SPT_LOWERBOUND, SPTS, SPTN},     /* Spooling object type */
      {SPTR_LOWERBOUND, SPTRS, SPTRN},  /* Spooling rules for object type */
      {SPM_LOWERBOUND, SPMS, SPMN},     /* Mapping object->id */

      {JJ_LOWERBOUND, JJS, JJN},        /* JAPI job */
      {JJAT_LOWERBOUND, JJATS, JJATN},  /* JAPI array task */
      {ASTR_LOWERBOUND, ASTRS, ASTRN},       /* CQ string sublist */
      {AULNG_LOWERBOUND, AULNGS, AULNGN},    /* CQ u_long32 sublist */
      {ABOOL_LOWERBOUND, ABOOLS, ABOOLN},    /* CQ bool sublist */
      {ATIME_LOWERBOUND, ATIMES, ATIMEN},    /* CQ time limit sublist */
      {AMEM_LOWERBOUND, AMEMS, AMEMN},       /* CQ memory limit sublist */
      {AINTER_LOWERBOUND, AINTERS, AINTERN}, /* CQ interval sublist */
      {0, 0, NULL}
   };

#ifdef  __cplusplus
}
#endif
#else
#ifdef __SGE_GDI_LIBRARY_SUBLIST_FILE__
#else
#ifdef  __cplusplus
extern "C" {
#endif

   extern lNameSpace nmv[];
   extern lNameSpace ptf_nmv[];

#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_GDI_LIBRARY_HOME_OBJECT_FILE__ */
#endif                          /* __SGE_GDI_LIBRARY_SUBLIST_FILE__     */
#endif                          /* __SGE_ALL_LISTSL_H */
