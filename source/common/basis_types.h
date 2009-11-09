#ifndef __BASIS_TYPES_H
#define __BASIS_TYPES_H
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

#include <sys/types.h>

#ifdef __SGE_COMPILE_WITH_GETTEXT__
#  include <libintl.h>
#  include <locale.h>
#  include "sge_language.h"
#  define SGE_ADD_MSG_ID(x) (sge_set_message_id_output(1),(x),sge_set_message_id_output(0),1) ? 1 : 0 
#  define _(x)               sge_gettext(x)
#  define _MESSAGE(x,y)      sge_gettext_((x),(y))
#  define _SGE_GETTEXT__(x)  sge_gettext__(x)
#else
#  define SGE_ADD_MSG_ID(x) (x)
#  define _(x)              (x)
#  define _MESSAGE(x,y)     (y)
#  define _SGE_GETTEXT__(x) (x)
#endif

#if 0
#ifndef FALSE
#   define FALSE                (0)
#endif

#ifndef TRUE
#   define TRUE                 (1)
#endif
#endif

#if !defined(__cplusplus) 
#  if defined(DARWIN9) || defined(DARWIN10)
#     include <stdbool.h>
#  else
typedef enum {
  false = 0,
  true
} bool;
#  endif
#endif

#define SGE_EPSILON 0.00001

#define FALSE_STR "FALSE"
#define TRUE_STR  "TRUE"

#define NONE_STR  "NONE"
#define NONE_LEN  4

#if defined(FREEBSD) || defined(NETBSD) || defined(LINUXAMD64) || defined(LINUXIA64)
#  define sge_U32CFormat "%u"  
#  define sge_U32CLetter "u"
#  define sge_u32c(x)  (unsigned int)(x)

#  define sge_X32CFormat "%x"
#  define sge_x32c(x)  (unsigned int)(x)
#else
#  define sge_U32CFormat "%ld"
#  define sge_U32CLetter "ld"
#  define sge_u32c(x)  (unsigned long)(x)

#  define sge_X32CFormat "%lx"
#  define sge_x32c(x)  (unsigned long)(x)
#endif


#if defined(IRIX)
#define sge_u64 "%lld"
#define sge_u64c(x)  (unsigned long long)(x)
#endif

#if defined(HP11) || defined(HP1164)
#  include <limits.h>
#else
#  if !(defined(WIN32NATIVE) || defined(WINDOWS))
#     include <sys/param.h>
#  endif
#endif

#ifdef  __cplusplus
extern "C" {
#endif

#if defined(TARGET_64BIT)
#  define u_long32 u_int
#elif defined(WIN32NATIVE)
#  define u_long32 unsigned long
#elif defined(FREEBSD) || defined(NETBSD)
#  define u_long32 uint32_t
#else
#  define u_long32 u_long
#endif

#define U_LONG32_MAX 4294967295UL
#define LONG32_MAX   2147483647

/* set sge_u32 and sge_x32 for 64 or 32 bit machines */
/* sge_uu32 for strictly unsigned, not nice, but did I use %d for an unsigned? */
#if defined(TARGET_64BIT) || defined(FREEBSD) || defined(NETBSD)
#  define sge_u32    "%d"
#  define sge_uu32   "%u"
#  define sge_x32    "%x"
#  define sge_fu32   "d"
#else
#  define sge_u32    "%ld"
#  define sge_uu32   "%lu"
#  define sge_x32    "%lx"
#  define sge_fu32   "ld"
#endif

/* -------------------------------
   solaris (who else - it's IRIX?) uses long 
   variables for uid_t, gid_t and pid_t 
*/
#if defined(FREEBSD)
#  define uid_t_fmt "%u"
#else
#  define uid_t_fmt pid_t_fmt
#endif

#if (defined(SOLARIS) && defined(TARGET_32BIT)) || defined(IRIX) || defined(INTERIX)
#  define pid_t_fmt    "%ld"
#else
#  define pid_t_fmt    "%d"
#endif

#if (defined(SOLARIS) && defined(TARGET_32BIT)) || defined(IRIX) || defined(INTERIX)
#  define gid_t_fmt    "%ld"
#elif defined(LINUX86) || defined(FREEBSD)
#  define gid_t_fmt    "%u"
#else
#  define gid_t_fmt    "%d"
#endif

/* _POSIX_PATH_MAX is only 255 and this is less than in most real systmes */
#define SGE_PATH_MAX    1024

#define MAX_STRING_SIZE 2048
typedef char stringT[MAX_STRING_SIZE];

#define MAX_VERIFY_STRING 512

#define INTSIZE     4           /* (4) 8 bit bytes */
#if defined(_UNICOS)
#define INTOFF      4           /* big endian 64-bit machines where sizeof(int) = 8 */
#else
#define INTOFF      0           /* the rest of the world; see comments in request.c */
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)<(b)?(b):(a))
#endif

/* types */
/* these are used for complexes */
#define TYPE_INT          1
#define TYPE_FIRST        TYPE_INT
#define TYPE_STR          2
#define TYPE_TIM          3
#define TYPE_MEM          4
#define TYPE_BOO          5
#define TYPE_CSTR         6
#define TYPE_HOST         7
#define TYPE_DOUBLE       8
#define TYPE_RESTR        9
#define TYPE_CE_LAST      TYPE_RESTR

/* used in config */
#define TYPE_ACC          10 
#define TYPE_LOG          11
#define TYPE_LAST         TYPE_LOG

/* save string format quoted */
#define SFQ  "\"%-.100s\""
/* save string format non-quoted */
#define SFN  "%-.100s"
#define SFN2 "%-.200s"
/* non-quoted string not limited intentionally */
#define SN_UNLIMITED  "%s"

#if defined(HPUX)
#  define seteuid(euid) setresuid(-1, euid, -1)
#  define setegid(egid) setresgid(-1, egid, -1)
#endif

#if defined(INTERIX) && !defined(INTERIX52)
#  define seteuid(euid) setreuid(-1, euid)
#  define setegid(egid) setregid(-1, egid)
#endif
    

#ifdef  __cplusplus
}
#endif

#ifndef TRUE
#  define TRUE 1
#  define FALSE !TRUE
#endif

#define GET_SPECIFIC(type, variable, init_func, key, func_name) \
   type *variable = pthread_getspecific(key); \
   if(variable == NULL) { \
      int ret; \
      variable = (type *)malloc(sizeof(type)); \
      init_func(variable); \
      ret = pthread_setspecific(key, (void*)variable); \
      if (ret != 0) { \
         fprintf(stderr, "pthread_setspecific(%s) failed: %s\n", func_name, strerror(ret)); \
         abort(); \
      } \
   }


#define HAS_GETPWNAM_R
#define HAS_GETGRNAM_R
#define HAS_GETPWUID_R
#define HAS_GETGRGID_R

#define HAS_LOCALTIME_R
#define HAS_CTIME_R

typedef enum {
   NO    = 0,
   YES   = 1,
   UNSET = 2
} ternary_t;

#endif /* __BASIS_TYPES_H */
