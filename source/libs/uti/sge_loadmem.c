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
#include <stdio.h>

#ifdef TEST
#include <string.h>
#include <unistd.h>
#include <errno.h>
#endif

#include "sgermon.h"
#include "sge_loadmem.h"
#include "sge_log.h"
#include "msg_utilib.h"
#include "sge_getloadavg.h"

#if !defined(LINUX) && !defined(SUN4) && !defined(HPUX) && !defined(CRAY)

#include <unistd.h>

int (*p_page2M)(int size);
void init_pageshift(void);
static int page2M_none(int size);
static int page2M_left(int size);
static int page2M_right(int size);

#define page2M(size) ((*p_page2M)(size))


static int pageshift;

static int page2M_none(
int size 
) {
   return (size);
}
 
static int page2M_left(
int size 
) {
   return (size << pageshift); 
}   
 
static int page2M_right(
int size 
) {
   return (size >> pageshift);
}   

void init_pageshift()
{
   int i;
   i = sysconf(_SC_PAGESIZE);
   pageshift = 0;
   while ((i >>= 1) > 0) {
      pageshift++;
   }

   pageshift -= 20; /* adjust for MB */


   /* now determine which pageshift function is appropriate for the
      result (have to because x << y is undefined for y < 0) */
   if (pageshift > 0)
      p_page2M = page2M_left;
   else if (pageshift == 0) 
      p_page2M = page2M_none;
   else {
      p_page2M = page2M_right;
      pageshift = -pageshift;   
   }
}

#endif

#if TEST

int main(
int argc,
char *argv[]  
) {
   sge_mem_info_t mem_info;

   memset(&mem_info, 0, sizeof(sge_mem_info_t));
   if (loadmem(&mem_info)) {
      fprintf(stderr, "error: failed retrieving memory info\n");
      return 1;
   }
      
   printf("mem_free      %fM\n", mem_info.mem_free);
   printf("mem_total     %fM\n", mem_info.mem_total);
   printf("swap_total    %fM\n", mem_info.swap_total);
   printf("swap_free     %fM\n", mem_info.swap_free);
   printf("virtual_total %fM\n", mem_info.mem_total + mem_info.swap_total);
   printf("virtual_free  %fM\n", mem_info.mem_free  + mem_info.swap_free);

   return 0;
}
#endif /* TEST */ 


/*--------------------------------------------------------------------------*/
#if defined(NECSX4) || defined(NECSX5)
#include <sys/rsg.h>
#include <sys/types.h>
#include <fcntl.h>

int loadmem_rsg(
int rsg_id,
sge_mem_info_t *mem_info_l,
sge_mem_info_t *mem_info_s 
) {
   int fd;
   memrb_t lmem, smem;
   rsg_info_t info;
   char fsg_dev_string[256];
   int lpagesize, spagesize;
  
   memset(&lmem, 0, sizeof(memrb_t)); 
   memset(&smem, 0, sizeof(memrb_t));
   sprintf(fsg_dev_string, "/dev/rsg/%d", rsg_id);
   fd = open(fsg_dev_string, O_RDONLY);
   if (fd > 0) {
      if (ioctl(fd, RSG_INFO, &info) == -1) {
         close(fd);
         return -1;
      }
      close(fd);   

      lmem.init_mem = info.lprb.init_mem;
      lmem.init_swap = info.lprb.init_swap;
      lmem.availsmem = info.lprb.availsmem;
      lmem.using_pgs = info.lprb.using_pgs;
      smem.init_mem = info.sprb.init_mem;
      smem.init_swap = info.sprb.init_swap;
      smem.availsmem = info.sprb.availsmem;
      smem.using_pgs = info.sprb.using_pgs;
   }

   /* Large Pages have a size of 1MB, 4MB or 16MB */
   lpagesize = sysconf(_SC_PAGESIZE);
   /* Small Pages have always a size of 32K */
   spagesize = 32 * 1024;

   mem_info_l->mem_total = (double)lmem.init_mem * lpagesize / (1024*1024);
   mem_info_l->mem_free = (double)(lmem.init_mem - lmem.using_pgs) *
      lpagesize / (1024*1024);
   mem_info_l->swap_total = (double)lmem.init_swap * lpagesize / (1024*1024);
   mem_info_l->swap_free = (double)(lmem.availsmem - (lmem.init_mem -
      lmem.using_pgs)) * lpagesize / (1024*1024);
   mem_info_s->mem_total = (double)smem.init_mem * spagesize / (1024*1024);
   mem_info_s->mem_free = (double)(smem.init_mem - smem.using_pgs) *
      spagesize / (1024*1024);
   mem_info_s->swap_total = (double)smem.init_swap * spagesize / (1024*1024);
   mem_info_s->swap_free = (double)(smem.availsmem - (smem.init_mem -
      smem.using_pgs)) * spagesize / (1024*1024); 

   return 0;
}

int loadmem_small_large(
sge_mem_info_t *mem_info_l,
sge_mem_info_t *mem_info_s 
) {
   int fd;
   int fsg_id;
   rsg_info_t info;
   char fsg_dev_string[256];
   double load_avg[3];
   int ret = -1;

   for (fsg_id=0; fsg_id<32; fsg_id++) {
      sge_mem_info_t mem_i_l, mem_i_s;

      memset(&mem_i_l, 0, sizeof(sge_mem_info_t));
      memset(&mem_i_s, 0, sizeof(sge_mem_info_t));
      if (loadmem_rsg(fsg_id, &mem_i_l, &mem_i_s) != -1) {
         mem_info_l->mem_total += mem_i_l.mem_total; 
         mem_info_l->mem_free += mem_i_l.mem_free; 
         mem_info_l->swap_total += mem_i_l.swap_total;
         mem_info_l->swap_free += mem_i_l.swap_free; 
         mem_info_s->mem_total += mem_i_s.mem_total; 
         mem_info_s->mem_free += mem_i_s.mem_free; 
         mem_info_s->swap_total += mem_i_s.swap_total; 
         mem_info_s->swap_free += mem_i_s.swap_free; 
         ret = 0;
      }
   }
   return ret;
}

int loadmem(
sge_mem_info_t *mem_info 
) {
   sge_mem_info_t mem_info_s, mem_info_l;
   int ret;

   ret = loadmem_small_large(&mem_info_l, &mem_info_s);
   mem_info->mem_total = mem_info_l.mem_total + mem_info_s.mem_total;
   mem_info->mem_free = mem_info_l.mem_free + mem_info_s.mem_free;
   mem_info->swap_total = mem_info_l.swap_total + mem_info_s.swap_total;
   mem_info->swap_free = mem_info_l.swap_free + mem_info_s.swap_free;
   return ret;
}

#endif /* NECSX4 || NECSX5 */
 
/*--------------------------------------------------------------------------*/
#if defined(SOLARIS)
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/swap.h>
#include <nlist.h>
#include <sys/types.h>
#include <fcntl.h>
#include <kvm.h>               

int loadmem(
sge_mem_info_t *mem_info 
) {
   long total, fr;
   register long cnt, i;
   register long t, f, l;
   struct swaptable *swt;
   struct swapent *ste;
   static char path[256];
   int sz;

   long freemem;

   DENTER(TOP_LAYER, "Solaris-loadmem");

   init_pageshift();
   
   /* get total number of swap entries */
   if ((cnt = swapctl(SC_GETNSWP, 0))<0) {
      DEXIT;
      return -1;
   }

   /* allocate enough space to hold count + n swapents */
   sz =  sizeof(long) + cnt * sizeof(struct swapent);
   swt = (struct swaptable *) malloc(sz);

   if (swt == NULL) {
      total = 0;
      fr = 0;
      DEXIT;
      return -1;
   }
   swt->swt_n = cnt;

   /* fill in ste_path pointers: we don't care about the paths, so we point
      them all to the same buffer */
   ste = &(swt->swt_ent[0]);
   i = cnt;
   while (--i >= 0) {
      ste++->ste_path = path;
   }

   /* grab all swap info */
   if (swapctl(SC_LIST, swt) != cnt) {
      DEXIT;
      return -1;
   }

   /* walk thru the structs and sum up the fields */
   t = f = l = 0;
   ste = &(swt->swt_ent[0]);

   i = cnt;
   while (--i >= 0) {
      /* dont count slots being deleted */
      if (!(ste->ste_flags & ST_INDEL) &&
          !(ste->ste_flags & ST_DOINGDEL)) {
      /* DPRINTF(("%s pages: %ld free: %ld length %ld\n", 
            ste->ste_path,
            ste->ste_pages,
            ste->ste_free,
            ste->ste_length)); */
         t += ste->ste_pages;
         f += ste->ste_free;
         l += ste->ste_length;      
      }
      ste++;
   }

   /* fill in the results */
   total = t;
   fr = f;
   free(swt);
   mem_info->swap_total = page2M(total);
   mem_info->swap_free = page2M(fr);

   if (get_freemem(&freemem)) {
      DEXIT;
      return -1;
   }

   mem_info->mem_free = page2M(freemem);
   mem_info->mem_total = page2M(sysconf(_SC_PHYS_PAGES));

   return 0;
}
#endif /* SOLARIS */

/*--------------------------------------------------------------------------*/
#if defined(HP10) || defined(HP11)
#include <sys/param.h>
#include <sys/pstat.h>

#define SWAP_BURST 10

int loadmem(
sge_mem_info_t *mem_info 
) {
   int i, cnt, idx;
   struct pst_dynamic ps_dyn;
   struct pst_swapinfo ps_swap[SWAP_BURST];
   struct pst_vminfo ps_vminfo;
   struct pst_static ps_static;
   
   unsigned long swap_total, swap_free, swap_allocated, swap_this;
   int pagesize = 0;
   
   pagesize = sysconf(_SC_PAGESIZE);

   init_pageshift();

   if (pstat_getstatic(&ps_static, sizeof(struct pst_static), 1, 0) != -1) {
      mem_info->mem_total = page2M(ps_static.physical_memory);
   }   

   if (pstat_getdynamic(&ps_dyn, sizeof(struct pst_dynamic), 1, 0) != -1) {
       mem_info->mem_free = page2M(ps_dyn.psd_free);
#if 0   
        printf("psd_proc_cnt;     /* MP: number of active processors */ %ld\n", ps_dyn.psd_proc_cnt);
        printf("psd_max_proc_cnt; /* MP: max active processors */       %ld\n", ps_dyn.psd_max_proc_cnt);
        printf("psd_last_pid;     /* last run process ID */             %ld\n", ps_dyn.psd_last_pid);
        printf("psd_rq;           /* run queue length */                %ld\n", ps_dyn.psd_rq);          
        printf("psd_dw;           /* jobs in disk wait */               %ld\n", ps_dyn.psd_dw);
        printf("psd_pw;           /* jobs in page wait */               %ld\n", ps_dyn.psd_pw);
        printf("psd_sl;           /* jobs sleeping in core */           %ld\n", ps_dyn.psd_sl);
        printf("psd_sw;           /* swapped out runnable jobs */       %ld\n", ps_dyn.psd_sw);
        printf("psd_vm;           /* total virtual memory */            %ld\n", ps_dyn.psd_vm);
        printf("psd_avm;          /* active virtual memory */           %ld\n", ps_dyn.psd_avm);
        printf("psd_rm;           /* total real memory */               %ld\n", ps_dyn.psd_rm);
        printf("psd_arm;          /* active real memory */              %ld\n", ps_dyn.psd_arm);
        printf("psd_vmtxt;        /* virt mem text */                   %ld\n", ps_dyn.psd_vmtxt);
        printf("psd_avmtxt;       /* active virt mem text */            %ld\n", ps_dyn.psd_avmtxt);
        printf("psd_rmtxt;        /* real mem text */                   %ld\n", ps_dyn.psd_rmtxt);
        printf("psd_armtxt;       /* active real mem text */            %ld\n", ps_dyn.psd_armtxt);
        printf("psd_free;         /* free memory pages */               %ld\n", ps_dyn.psd_free);
        printf("psd_avg_1_min;    /* global run queue lengths */        %f\n",  ps_dyn.psd_avg_1_min);
        printf("psd_avg_5_min;                                          %f\n",  ps_dyn.psd_avg_5_min);
        printf("psd_avg_15_min;			                        %f\n",  ps_dyn.psd_avg_15_min);
        printf("psd_global_virtual /* avail. global virt space *        %ld\n", ps_dyn.psd_global_virtual);        
#endif
   }
      
   swap_total = swap_free = swap_allocated = swap_this = 0;

   idx = 0;

   while ((cnt = pstat_getswap(&ps_swap[0], sizeof(struct pst_swapinfo), SWAP_BURST, idx)) > 0) {
      for (i = 0; i < cnt; i++) {
#if 0
         printf("-------------------------\n");
         printf("pss_idx:              %lu\n", ps_swap[i].pss_idx);
         printf("pss_flags:            %lu\n", ps_swap[i].pss_flags);
         printf("pss_priority:         %lu\n", ps_swap[i].pss_priority);
         printf("pss_nfpgs:            %lu\n", ps_swap[i].pss_nfpgs);
         printf("pss_swapchunk:        %lu\n", ps_swap[i].pss_swapchunk);
         printf("Swap is %s and type is %s dev and %s fs\n",
                ps_swap[i].pss_flags & SW_ENABLED ? "enabled" : "disabled",
                ps_swap[i].pss_flags & SW_BLOCK   ? "" : "no",
                ps_swap[i].pss_flags & SW_FS      ? "" : "no");
#endif             
         if (ps_swap[i].pss_flags & SW_BLOCK && ps_swap[i].pss_flags & SW_ENABLED) {
#if 0
            printf("   Pss_nblks:         %lu\n", ps_swap[i].pss_un.Pss_blk.Pss_nblks);
            printf("   Pss_nblksavail:    %lu\n", ps_swap[i].pss_un.Pss_blk.Pss_nblksavail);
#endif         
            swap_this      = ps_swap[i].pss_un.Pss_blk.Pss_nblksavail / (pagesize / DEV_BSIZE); 
            swap_total     += swap_this;
            swap_allocated += swap_this - ps_swap[i].pss_nfpgs;
         }
         else if (ps_swap[i].pss_flags & SW_FS && ps_swap[i].pss_flags & SW_ENABLED) {
#if 0
            printf("   Pss_allocated:    %lu\n", ps_swap[i].pss_un.Pss_fs.Pss_allocated);
            printf("   Pss_min:          %lu\n", ps_swap[i].pss_un.Pss_fs.Pss_min);
            printf("   Pss_limit:        %lu\n", ps_swap[i].pss_un.Pss_fs.Pss_limit);
#endif
            swap_this       = (ps_swap[i].pss_un.Pss_fs.Pss_limit * ps_swap[i].pss_swapchunk * DEV_BSIZE) / pagesize;
            swap_total     += swap_this;
            swap_allocated += swap_this - ps_swap[i].pss_nfpgs;
         }
#if 0
         printf("Swap this             %lu\n", swap_this);
         printf("Swap total/pages:     %lu\n", swap_total);
         printf("Swap allocated/pages: %lu\n", swap_allocated);
#endif
      }         
      idx = ps_swap[cnt-1].pss_idx + 1;
   }

   if (pstat_getvminfo(&ps_vminfo, sizeof(struct pst_vminfo), 1, 0) != -1) {
#if 0
      printf("psv_swapspc_max       %lu\n", ps_vminfo.psv_swapspc_max);
      printf("psv_swapspc_cnt       %lu\n", ps_vminfo.psv_swapspc_cnt);
      printf("Reserved:             %lu\n", ps_vminfo.psv_swapspc_max - 
                                            ps_vminfo.psv_swapspc_cnt - swap_allocated);
      printf("Used:                 %lu\n", ps_vminfo.psv_swapspc_max - ps_vminfo.psv_swapspc_cnt);                      
#endif      
   }

   mem_info->swap_total = page2M(swap_total);
   mem_info->swap_free = page2M(swap_total - (ps_vminfo.psv_swapspc_max - ps_vminfo.psv_swapspc_cnt));

   return 0;
}
#endif /* HP10 */


/*--------------------------------------------------------------------------*/
#if defined(ALPHA)
#include <nlist.h>
#include <sys/sysinfo.h>
#include <machine/hal_sysinfo.h>
#include <mach.h>
#include </sys/include/vm/vm_perf.h>
#include <paths.h>
#include <sys/table.h>

int loadmem(
sge_mem_info_t *mem_info 
) {
   struct vm_statistics vmstats;
   int swap_pages=0,swap_free=0,i;
   int physical_memory;     /* size of real mem in KB                 */
   struct tbl_swapinfo swbuf;
   static int first = 1;
   static long pagesize = 0;

#ifndef NO_SGE_COMPILE_DEBUG
   char SGE_FUNC[] = "";
#endif
         
   if (first) {
      pagesize = sysconf(_SC_PAGESIZE);
      first = 0;
      if (pagesize == -1) {
         ERROR((SGE_EVENT, MSG_SYSTEM_NOPAGESIZEASSUME8192 ));
         pagesize = 8192;
      }
   }      
      

   if (getsysinfo(GSI_PHYSMEM, (caddr_t)&physical_memory,sizeof(int),0,NULL)==-1) {
/*       fprintf(stderr, "getsysinfo(GSI_CPUS_IN_BOX) failed, %s\n", strerror(errno)); */
      return -1;
   }
   mem_info->mem_total = physical_memory/1024;

   /* memory information */
   /* this is possibly bogus - we work out total # pages by */
   /* adding up the free, active, inactive, wired down, and */
   /* zero filled. Anyone who knows a better way, TELL ME!  */
   /* Change: dont use zero filled. */

   (void) vm_statistics(current_task(),&vmstats);

   /* free mem */
   mem_info->mem_free = vmstats.free_count*pagesize/(1024*1024);

   /* swap space */
   for(i=0; table(TBL_SWAPINFO,i,&swbuf,1,sizeof(struct tbl_swapinfo))>0; i++) {
      swap_pages += swbuf.size;
      swap_free  += swbuf.free;
   }
   
   mem_info->swap_free = ((double)swap_free*pagesize)/(1024*1024);
   mem_info->swap_total = ((double)swap_pages*pagesize)/(1024*1024);

   return 0;
}
#endif /* ALPHA */


/*--------------------------------------------------------------------------*/
#if defined(IRIX6)
#include <stdio.h>
#include <sys/sysinfo.h>
#include <sys/sysmp.h>
#include <errno.h>
#include <sys/swap.h>
#include <sys/types.h>
#include <sys/time.h>

#define pagetom(size) ((size)*(((float)pagesize)/1024))


int loadmem(
sge_mem_info_t *mem_info 
) {
   struct rminfo rmi;
   struct minfo mi;
   off_t swaptot, swapfree, swaprsrv;
   static int pagesize = 0;

   if (!pagesize)
      pagesize = getpagesize()/1024;

   if (swapctl(SC_GETSWAPTOT, &swaptot)<0) 
      return -1;

   if (swapctl(SC_GETFREESWAP, &swapfree)<0)
      return -1;

   if (sysmp(MP_SAGET, MPSA_RMINFO, &rmi, sizeof(rmi))<0) 
      return -1;

   if (sysmp(MP_SAGET, MPSA_MINFO, &mi, sizeof(mi))<0)
      return -1;

   if (swapctl(SC_GETRESVSWAP, &swaprsrv) < 0)
      return -1;
      
/*    mem_info->mem_total = pagetom(rmi.availrmem); */
   mem_info->mem_total = pagetom(rmi.physmem);
   mem_info->mem_free = pagetom(rmi.freemem + rmi.chunkpages);
   mem_info->swap_total = ((double)swaptot * 512)/(1024.0*1024.0); 
   mem_info->swap_free = ((double)swapfree * 512)/(1024.0*1024.0);
   mem_info->swap_rsvd = ((double)swaprsrv * 512)/(1024.0*1024.0);
   return 0;
}
#endif /* IRIX6 */


/*--------------------------------------------------------------------------*/
#if defined(LINUX)
#include <stdio.h>
#include <string.h>

#define PROC_MEMINFO "/proc/meminfo"

#define KEY_MEMTOTAL  "MemTotal"
#define KEY_MEMFREE   "MemFree"
#define KEY_SWAPTOTAL "SwapTotal"
#define KEY_SWAPFREE  "SwapFree"

#define KEY_BUFFERS   "Buffers"
#define KEY_CACHED    "Cached"

int loadmem(
sge_mem_info_t *mem_info 
) {
   char dummy[512], buffer[1024];
   double kbytes;
   FILE *fp;
   double buffers = 0, cached = 0;

   if ((fp = fopen(PROC_MEMINFO, "r"))) {
      while (fgets(buffer, sizeof(buffer)-1, fp)) {

#define READ_VALUE(key, dest)    if (!strncmp(buffer, key, sizeof(key)-1)) { \
            sscanf(buffer, "%[^0-9]%lf", dummy, &kbytes); \
            dest = kbytes/1024; \
            continue; \
         }

         READ_VALUE(KEY_MEMTOTAL,  mem_info->mem_total);
         READ_VALUE(KEY_MEMFREE,   mem_info->mem_free);
         READ_VALUE(KEY_SWAPTOTAL, mem_info->swap_total);
         READ_VALUE(KEY_SWAPFREE,  mem_info->swap_free);
         READ_VALUE(KEY_BUFFERS,   buffers);
         READ_VALUE(KEY_CACHED,    cached);

      }
      fclose(fp);
   } else {
      return 1;
   }
   mem_info->mem_free += buffers+cached;

   return 0;
}
#endif /* LINUX */


/*--------------------------------------------------------------------------*/
/* CRAY */
#if defined(CRAY)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/param.h>
#include <sys/table.h>
#include <sys/sysinfo.h>
#include <sys/pws.h>
#include <sys/session.h>
#include <sys/cred.h>
#include <sys/aoutdata.h>
#include <sys/proc.h>
#include <sys/map.h>
#include <sys/swap.h>
#include <sys/acct.h>
#include <sys/stat.h>
#include <sys/machcons.h>

static int read_kernel_table(
char *name,
void **table,
long *size,
int *entries 
) {
   struct tbs tinfo;
   long tsize;

   if (tabinfo(name, &tinfo) < 0) {
/*       fprintf(stderr, MSG_SYSTEM_TABINFO_FAILED_SS, name, strerror(errno)); */
      return -1;
   }

   tsize = tinfo.head + (tinfo.ent * tinfo.len);
   if (tsize > *size) {
      if (*table) free(*table);
      *table = malloc(tsize);
      if (*table == NULL) {
/*          fprintf(stderr, MSG_MEMORY_MALLOCFAILED_D, tsize); */
         return -1;
      }
      memset(*table, 0, tsize);
      *size = tsize;
   }

   if (tabread(name, (char *)*table, tsize, 0) == -1) {
/*       fprintf(stderr, MSG_SYSTEM_TABINFO_FAILED_SS, name, strerror(errno)); */
      return -1;
   }

   if (entries) *entries = tinfo.ent;

   return 0;
}

int loadmem(
sge_mem_info_t *mem_info 
) {
   static struct sysinfo *si;
   static long si_size;
   static struct swapper *sw;
   static long sw_size;

   if (read_kernel_table(SINFO, (void **)&si, &si_size, NULL)<0)
      return -1;

   if (read_kernel_table(SWAPTAB, (void **)&sw, &sw_size, NULL)<0)
      return -1;
   
   mem_info->mem_total = (sysconf(_SC_CRAY_USRMEM) * 8)/(1024.0*1024.0);
   mem_info->mem_free = (sysconf(_SC_CRAY_USRMEM) * 8 - si->umemused * NBPC)/(1024.0*1024.0);
   mem_info->swap_total = (sw->swp_map.bmp_total * sw->swp_wght * 4096)/(1024.0*1024.0);
   mem_info->swap_free = (sw->swp_map.bmp_avail * sw->swp_wght * 4096)/(1024.0*1024.0);

   return 0;
}
#endif /* CRAY */
