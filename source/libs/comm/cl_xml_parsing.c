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
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>

#if !defined(WIN32)
#include <unistd.h>
#include <strings.h>
#endif

#include "cl_lists.h"
#include "cl_errors.h"
#include "cl_util.h"
#include "cl_xml_parsing.h"


static cl_bool_t cl_xml_parse_is_version(char* buffer, unsigned long start, unsigned long buffer_length );




#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_mih_mat_string()"
const char* cl_com_get_mih_mat_string(cl_xml_ack_type_t mat) {
   switch (mat) {
      case CL_MIH_MAT_NAK:
         return CL_MIH_MESSAGE_ACK_TYPE_NAK;
      case CL_MIH_MAT_ACK:
         return CL_MIH_MESSAGE_ACK_TYPE_ACK;
      case CL_MIH_MAT_SYNC:
         return CL_MIH_MESSAGE_ACK_TYPE_SYNC;
      case CL_MIH_MAT_UNDEFINED:
         return "undefined";
   }
   return "undefined";
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_mih_df_string()"
const char* cl_com_get_mih_df_string(cl_xml_mih_data_format_t df) {
   switch (df) {
      case CL_MIH_DF_BIN:
         return CL_MIH_MESSAGE_DATA_FORMAT_BIN;
      case CL_MIH_DF_XML:
         return CL_MIH_MESSAGE_DATA_FORMAT_XML;
      case CL_MIH_DF_AM:
         return CL_MIH_MESSAGE_DATA_FORMAT_AM;
      case CL_MIH_DF_SIM:
         return CL_MIH_MESSAGE_DATA_FORMAT_SIM;
      case CL_MIH_DF_SIRM:
         return CL_MIH_MESSAGE_DATA_FORMAT_SIRM;
      case CL_MIH_DF_CCM:
         return CL_MIH_MESSAGE_DATA_FORMAT_CCM;
      case CL_MIH_DF_CCRM:
         return CL_MIH_MESSAGE_DATA_FORMAT_CCRM;
      case CL_MIH_DF_UNDEFINED:
         return "undefined";
   }
   return "undefined";
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_free_gmsh_header()"
int cl_com_free_gmsh_header(cl_com_GMSH_t** header) {
   if (header == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*header == NULL) {
      return CL_RETVAL_PARAMS;
   }
   free(*header);
   *header = NULL;
   return CL_RETVAL_OK;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_free_cm_message()"
int cl_com_free_cm_message(cl_com_CM_t** message) {   /* CR check */
   if (message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   free( (*message)->version );
   cl_com_free_endpoint( &( (*message)->src )  );
   cl_com_free_endpoint( &( (*message)->dst )  );
   cl_com_free_endpoint( &( (*message)->rdata )  );

   free(*message);
   *message = NULL;
   return CL_RETVAL_OK;
}

int cl_com_free_mih_message(cl_com_MIH_t** message) {   /* CR check */
   if (message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   free( (*message)->version );
   free(*message);
   *message = NULL;
   return CL_RETVAL_OK;
}

int cl_com_free_am_message(cl_com_AM_t** message) {   /* CR check */
   if (message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   free( (*message)->version );
   free(*message);
   *message = NULL;
   return CL_RETVAL_OK;
}

int cl_com_free_sim_message(cl_com_SIM_t** message) {
   if (message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   free( (*message)->version );
   free(*message);
   *message = NULL;
   return CL_RETVAL_OK;
}

int cl_com_free_sirm_message(cl_com_SIRM_t** message) {
   if (message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   free( (*message)->version );
   free( (*message)->info    );
   free(*message);
   *message = NULL;
   return CL_RETVAL_OK;
}



int cl_com_free_ccm_message(cl_com_CCM_t** message) {   /* CR check */
   if (message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   free( (*message)->version );
   free(*message);
   *message = NULL;
   return CL_RETVAL_OK;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_free_ccrm_message()"
int cl_com_free_ccrm_message(cl_com_CCRM_t** message) {   /* CR check */
   if (message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   free( (*message)->version );
   free(*message);
   *message = NULL;
   return CL_RETVAL_OK;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_free_crm_message()"
int cl_com_free_crm_message(cl_com_CRM_t** message) {   /* CR check */
   if (message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   free( (*message)->version );
   free( (*message)->cs_text );
   free( (*message)->formats );
   cl_com_free_endpoint( &( (*message)->src )  );
   cl_com_free_endpoint( &( (*message)->dst )  );
   cl_com_free_endpoint( &( (*message)->rdata )  );
   free(*message);
   *message = NULL;
   return CL_RETVAL_OK;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_xml_parse_GMSH()"
int cl_xml_parse_GMSH(unsigned char* buffer, unsigned long buffer_length, cl_com_GMSH_t* header , unsigned long *used_buffer_length) {

   unsigned long i;
   char help_buf[256];
   unsigned long help_buf_pointer = 0;
   unsigned long buf_pointer = 0;
   unsigned long tag_begin = 0;
   unsigned long tag_end = 0;
   unsigned long dl_begin = 0;
   unsigned long dl_end = 0;

   if (header == NULL || buffer == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   header->dl = 0;
   *used_buffer_length = 0;

   while(buf_pointer <= buffer_length) {
      switch( buffer[buf_pointer] ) {
         case '<':
            tag_begin = buf_pointer + 1;
         break; 
         case '>':
            tag_end = buf_pointer - 1;
            if (tag_begin < tag_end && tag_begin > 0 && tag_end > 0) {
               help_buf_pointer = 0;
               for (i=tag_begin;i<=tag_end && help_buf_pointer < 254 ;i++) {
                  help_buf[help_buf_pointer++] = buffer[i];
               }
               help_buf[help_buf_pointer] = 0;
               if (strcmp(help_buf,"/gmsh") == 0) {
                  if (*used_buffer_length == 0) {
                     *used_buffer_length = buf_pointer+1;
                  }
                  buf_pointer++;
                  break;
               }
               if (strcmp(help_buf,"dl") == 0) {
                  dl_begin = buf_pointer + 1;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/dl") == 0) {
                  dl_end = buf_pointer - 5;
                  buf_pointer++;
                  continue;
               }
            }
         break;
      }
      buf_pointer++;
   }

   if (dl_begin > 0 && dl_end > 0 && dl_end >= dl_begin) {
      help_buf_pointer = 0;
      for (i=dl_begin;i<=dl_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      header->dl = cl_util_get_ulong_value(help_buf);
   }
   return CL_RETVAL_OK;
}




#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_xml_parse_CM()"
int cl_xml_parse_CM(unsigned char* buffer, unsigned long buffer_length, cl_com_CM_t** message ) {

   unsigned long i;
   char help_buf[256];
   unsigned long help_buf_pointer = 0;
   unsigned long buf_pointer = 0;
   int in_tag = 0;
   unsigned long tag_begin = 0;
   unsigned long tag_end = 0;
   unsigned long version_begin = 0;
   unsigned long df_begin = 0;
   unsigned long df_end = 0;
   unsigned long ct_begin = 0;
   unsigned long ct_end = 0;
   unsigned long src_begin = 0;
   unsigned long src_end = 0;
   unsigned long dst_begin = 0;
   unsigned long dst_end = 0;
   unsigned long rdata_begin = 0;
   unsigned long rdata_end = 0;
   unsigned long port_begin = 0;
   unsigned long port_end = 0;
   unsigned long autoclose_begin = 0;
   unsigned long autoclose_end = 0;


  
   if (message == NULL || buffer == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   if (*message != NULL) {
      return CL_RETVAL_PARAMS;
   }

   *message = (cl_com_CM_t*)malloc(sizeof(cl_com_CM_t));
   if (*message == NULL) {
      return CL_RETVAL_MALLOC;
   }
   memset((char *) (*message), 0, sizeof(cl_com_CM_t));

   (*message)->df = CL_CM_DF_UNDEFINED;
   (*message)->ct = CL_CM_CT_UNDEFINED;
   (*message)->ac = CL_CM_AC_UNDEFINED;

   while(buf_pointer < buffer_length) {
      switch( buffer[buf_pointer] ) {
         case '=':
            if (in_tag == 1) {
               if ( cl_xml_parse_is_version((char*)buffer, tag_begin, buffer_length) == CL_TRUE) {
                  version_begin = buf_pointer + 2;
               }
            }
         break;   
         case '<':
            in_tag = 1;
            tag_begin = buf_pointer + 1;
         break; 
         case '>':
            in_tag = 0;
            tag_end = buf_pointer - 1;
            if (tag_begin < tag_end && tag_begin > 0 && tag_end > 0) {
               
               help_buf_pointer = 0;
               for (i=tag_begin;i<=tag_end && help_buf_pointer < 254 ;i++) {
                  help_buf[help_buf_pointer++] = buffer[i];
               }
               help_buf[help_buf_pointer] = 0;
               if (strcmp(help_buf,"/cm") == 0) {
                  buf_pointer++;
                  continue;
               }

               if (strcmp(help_buf,"df") == 0) {
                  df_begin = buf_pointer + 1;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"ct") == 0) {
                  ct_begin = buf_pointer + 1;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"port") == 0) {
                  port_begin = buf_pointer + 1;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/port") == 0) {
                  port_end = buf_pointer - 7;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"ac") == 0) {
                  autoclose_begin = buf_pointer + 1;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/ac") == 0) {
                  autoclose_end = buf_pointer - 5;
                  buf_pointer++;
                  continue;
               }

               if (strcmp(help_buf,"/df") == 0) {
                  df_end = buf_pointer - 5;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/ct") == 0) {
                  ct_end = buf_pointer - 5;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/src") == 0) {
                  src_end = buf_pointer - 6;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/dst") == 0) {
                  dst_end = buf_pointer - 6;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/rdata") == 0) {
                  rdata_end = buf_pointer - 8;
                  buf_pointer++;
                  continue;
               }
                  
               if (strncmp(help_buf,"src", 3) == 0 && src_begin == 0) {
                  src_begin = tag_begin;
                  buf_pointer++;
                  continue;
               }
               if (strncmp(help_buf,"dst", 3) == 0 && dst_begin == 0) {
                  dst_begin = tag_begin;
                  buf_pointer++;
                  continue;
               }
               if (strncmp(help_buf,"rdata", 5) == 0 && rdata_begin == 0) {
                  rdata_begin = tag_begin;
                  buf_pointer++;
                  continue;
               }
            }
         break;
      }
      buf_pointer++;
   }

   if (df_begin > 0 && df_end > 0 && df_end >= df_begin) {
      help_buf_pointer = 0;
      for (i=df_begin;i<=df_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      if (strcmp("bin",help_buf) == 0) {
         (*message)->df = CL_CM_DF_BIN;
      }
      if (strcmp("xml",help_buf) == 0) {
         (*message)->df = CL_CM_DF_XML;
      }
   }

   if (ct_begin > 0 && ct_end > 0 && ct_end >= ct_begin) {
      help_buf_pointer = 0;
      for (i=ct_begin;i<=ct_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      if (strcmp("stream",help_buf) == 0) {
         (*message)->ct = CL_CM_CT_STREAM;
      }
      if (strcmp("message",help_buf) == 0) {
         (*message)->ct = CL_CM_CT_MESSAGE;
      }
   }

   if (port_begin > 0 && port_end > 0 && port_end >= port_begin) {
      help_buf_pointer = 0;
      for (i=port_begin;i<=port_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->port = cl_util_get_ulong_value(help_buf);
   }

   if (autoclose_begin > 0 && autoclose_end > 0 && autoclose_end >= autoclose_begin) {
      help_buf_pointer = 0;
      for (i=autoclose_begin;i<=autoclose_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      if (strcmp(CL_CONNECT_MESSAGE_AUTOCLOSE_ENABLED,help_buf) == 0) {
         (*message)->ac = CL_CM_AC_ENABLED;
      } 
      if (strcmp(CL_CONNECT_MESSAGE_AUTOCLOSE_DISABLED,help_buf) == 0) {
         (*message)->ac = CL_CM_AC_DISABLED;
      }
   }

   /* get version */
   if (version_begin > 0) {
      help_buf_pointer = 0;
      for (i=version_begin ; i<= buffer_length && buffer[i] != '\"' && help_buf_pointer < 254 ; i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->version = strdup(help_buf);
   }

   if ( (*message)->df == CL_CM_DF_UNDEFINED) {
      CL_LOG(CL_LOG_ERROR,"data formate undefined");
      return CL_RETVAL_UNKNOWN;
   }

   if ( (*message)->ct == CL_CM_CT_UNDEFINED) {
      CL_LOG(CL_LOG_ERROR,"connection type undefined");
      return CL_RETVAL_UNKNOWN;
   }

   if ( (*message)->ac == CL_CM_AC_UNDEFINED) {
      CL_LOG(CL_LOG_ERROR,"autoclose option undefined");
      return CL_RETVAL_UNKNOWN;
   }

   /* get src */
   if (src_begin > 0 && src_end > 0 && src_end >= src_begin) {
      (*message)->src = (cl_com_endpoint_t*)malloc(sizeof(cl_com_endpoint_t));
      if ((*message)->src == NULL) {
         cl_com_free_cm_message(message);
         return CL_RETVAL_MALLOC;
      }
      i = src_begin;

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=src_end && help_buf_pointer < 254) {
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->src->comp_host = strdup(help_buf);

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=src_end && help_buf_pointer < 254) {
        
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->src->comp_name = strdup(help_buf);

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=src_end && help_buf_pointer < 254) {
        
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->src->comp_id = cl_util_get_ulong_value(help_buf);
   } 
   /* get dst */
   if (dst_begin > 0 && dst_end > 0 && dst_end >= dst_begin) {
      (*message)->dst = (cl_com_endpoint_t*)malloc(sizeof(cl_com_endpoint_t));
      if ((*message)->dst == NULL) {
         cl_com_free_cm_message(message);
         return CL_RETVAL_MALLOC;
      }
      i = dst_begin;

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=dst_end && help_buf_pointer < 254) {
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->dst->comp_host = strdup(help_buf);


      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=dst_end && help_buf_pointer < 254) {
        
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->dst->comp_name = strdup(help_buf);

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=dst_end && help_buf_pointer < 254) {
        
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->dst->comp_id = cl_util_get_ulong_value(help_buf);

      
   }
   /* get rdata */
   if (rdata_begin > 0 && rdata_end > 0 && rdata_end >= rdata_begin) {
      (*message)->rdata = (cl_com_endpoint_t*)malloc(sizeof(cl_com_endpoint_t));
      if ((*message)->rdata == NULL) {
         cl_com_free_cm_message(message);
         return CL_RETVAL_MALLOC;
      }
      i = rdata_begin;

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=rdata_end && help_buf_pointer < 254) {
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->rdata->comp_host = strdup(help_buf);


      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=rdata_end && help_buf_pointer < 254) {
        
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->rdata->comp_name = strdup(help_buf);

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=rdata_end && help_buf_pointer < 254) {
        
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->rdata->comp_id = cl_util_get_ulong_value(help_buf);
   }
   
   if ( (*message)->src == NULL || (*message)->dst == NULL) {
      CL_LOG(CL_LOG_ERROR,"data formate undefined");
      return CL_RETVAL_UNKNOWN;
   }

   return CL_RETVAL_OK;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_xml_parse_CRM()"
int cl_xml_parse_CRM(unsigned char* buffer, unsigned long buffer_length, cl_com_CRM_t** message ) {

   unsigned long i;
   char help_buf[256];
   unsigned long help_buf_pointer = 0;
   unsigned long buf_pointer = 0;
   int in_tag = 0;
   unsigned long tag_begin = 0;
   unsigned long tag_end = 0;
   unsigned long version_begin = 0;
   unsigned long cs_begin = 0;
   unsigned long cs_end = 0;

   unsigned long src_begin = 0;
   unsigned long src_end = 0;
   unsigned long dst_begin = 0;
   unsigned long dst_end = 0;
   unsigned long rdata_begin = 0;
   unsigned long rdata_end = 0;


   if (message == NULL || buffer == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   if (*message != NULL) {
      return CL_RETVAL_PARAMS;
   }

   *message = (cl_com_CRM_t*)malloc(sizeof(cl_com_CRM_t));
   if (*message == NULL) {
      return CL_RETVAL_MALLOC;
   }
   memset((char *) (*message), 0, sizeof(cl_com_CRM_t));
   
   (*message)->cs_condition = CL_CRM_CS_UNDEFINED;
   
   while(buf_pointer < buffer_length) {
      switch( buffer[buf_pointer] ) {
         case '=':
            if (in_tag == 1) {
               if ( cl_xml_parse_is_version((char*)buffer, tag_begin, buffer_length) == CL_TRUE) {
                  version_begin = buf_pointer + 2;
               }
            }
         break;   
         case '<':
            in_tag = 1;
            tag_begin = buf_pointer + 1;
         break; 
         case '>':
            in_tag = 0;
            tag_end = buf_pointer - 1;
            if (tag_begin < tag_end && tag_begin > 0 && tag_end > 0) {
               
               help_buf_pointer = 0;
               for (i=tag_begin;i<=tag_end && help_buf_pointer < 254 ;i++) {
                  help_buf[help_buf_pointer++] = buffer[i];
               }
               help_buf[help_buf_pointer] = 0;



               if (strcmp(help_buf,"/crm") == 0) {
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/cs") == 0) {
                  cs_end = buf_pointer - 5;
                  buf_pointer++;
                  continue;
               }
               
               if (strcmp(help_buf,"/src") == 0) {
                  src_end = buf_pointer - 6;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/dst") == 0) {
                  dst_end = buf_pointer - 6;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/rdata") == 0) {
                  rdata_end = buf_pointer - 8;
                  buf_pointer++;
                  continue;
               }

               if (strncmp(help_buf,"cs", 2) == 0 && cs_begin == 0) {
                  cs_begin = tag_begin;
                  buf_pointer++;
                  continue;
               }

               if (strncmp(help_buf,"src", 3) == 0 && src_begin == 0) {
                  src_begin = tag_begin;
                  buf_pointer++;
                  continue;
               }
               if (strncmp(help_buf,"dst", 3) == 0 && dst_begin == 0) {
                  dst_begin = tag_begin;
                  buf_pointer++;
                  continue;
               }
               if (strncmp(help_buf,"rdata", 5) == 0 && rdata_begin == 0) {
                  rdata_begin = tag_begin;
                  buf_pointer++;
                  continue;
               }
            }
         break;
      }
      buf_pointer++;
   }


   /* get version */
   if (version_begin > 0) {
      help_buf_pointer = 0;
      for (i=version_begin ; i<= buffer_length && buffer[i] != '\"' && help_buf_pointer < 254 ; i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->version = strdup(help_buf);
   }

   /* get cs_condition */
   if (cs_begin > 0 && cs_end > 0 && cs_end >= cs_begin) {
      i = cs_begin;
      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=cs_end && help_buf_pointer < 254) {
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;

      if (strcmp(CL_CONNECT_RESPONSE_MESSAGE_CONNECTION_STATUS_OK,help_buf) == 0) {
         (*message)->cs_condition = CL_CRM_CS_CONNECTED;
      }
      if (strcmp(CL_CONNECT_RESPONSE_MESSAGE_CONNECTION_STATUS_DENIED,help_buf) == 0) {
         (*message)->cs_condition = CL_CRM_CS_DENIED;
      } 
      if (strcmp(CL_CONNECT_RESPONSE_MESSAGE_CONNECTION_UNSUP_DATA_FORMAT,help_buf) == 0) {
         (*message)->cs_condition = CL_CRM_CS_UNSUPPORTED;
      }
      if (strcmp(CL_CONNECT_RESPONSE_MESSAGE_CONNECTION_STATUS_NOT_UNIQUE ,help_buf) == 0) {
         (*message)->cs_condition = CL_CRM_CS_ENDPOINT_NOT_UNIQUE;
      }
      i++;
      help_buf_pointer = 0;
      while(i<=cs_end && help_buf_pointer < 254) {
         help_buf[help_buf_pointer++] = buffer[i];
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->cs_text = strdup(help_buf);
   }
   /* get src */
   if (src_begin > 0 && src_end > 0 && src_end >= src_begin) {
      (*message)->src = (cl_com_endpoint_t*)malloc(sizeof(cl_com_endpoint_t));
      if ((*message)->src == NULL) {
         cl_com_free_crm_message(message);
         return CL_RETVAL_MALLOC;
      }
      i = src_begin;

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=src_end && help_buf_pointer < 254) {
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->src->comp_host = strdup(help_buf);

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=src_end && help_buf_pointer < 254) {
        
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->src->comp_name = strdup(help_buf);

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=src_end && help_buf_pointer < 254) {
        
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->src->comp_id = cl_util_get_ulong_value(help_buf);
   } 
   /* get dst */
   if (dst_begin > 0 && dst_end > 0 && dst_end >= dst_begin) {
      (*message)->dst = (cl_com_endpoint_t*)malloc(sizeof(cl_com_endpoint_t));
      if ((*message)->dst == NULL) {
         cl_com_free_crm_message(message);
         return CL_RETVAL_MALLOC;
      }
      i = dst_begin;

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=dst_end && help_buf_pointer < 254) {
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->dst->comp_host = strdup(help_buf);


      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=dst_end && help_buf_pointer < 254) {
        
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->dst->comp_name = strdup(help_buf);

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=dst_end && help_buf_pointer < 254) {
        
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->dst->comp_id = cl_util_get_ulong_value(help_buf);

      
   }
   /* get rdata */
   if (rdata_begin > 0 && rdata_end > 0 && rdata_end >= rdata_begin) {
      (*message)->rdata = (cl_com_endpoint_t*)malloc(sizeof(cl_com_endpoint_t));
      if ((*message)->rdata == NULL) {
         cl_com_free_crm_message(message);
         return CL_RETVAL_MALLOC;
      }
      i = rdata_begin;

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=rdata_end && help_buf_pointer < 254) {
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->rdata->comp_host = strdup(help_buf);


      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=rdata_end && help_buf_pointer < 254) {
        
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->rdata->comp_name = strdup(help_buf);

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=rdata_end && help_buf_pointer < 254) {
        
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->rdata->comp_id = cl_util_get_ulong_value(help_buf);
   }

   (*message)->formats = strdup("not supported");

   CL_LOG_STR(CL_LOG_INFO,"version:     ", (*message)->version);
   CL_LOG_INT(CL_LOG_INFO,"cs_condition:", (int)(*message)->cs_condition);
   CL_LOG_STR(CL_LOG_INFO,"cs_text:     ", (*message)->cs_text);
   CL_LOG_STR(CL_LOG_INFO,"formats:     ", (*message)->formats);
   CL_LOG_STR(CL_LOG_INFO,"src->host:   ", (*message)->src->comp_host);
   CL_LOG_STR(CL_LOG_INFO,"src->comp:   ", (*message)->src->comp_name);
   CL_LOG_INT(CL_LOG_INFO,"src->id:     ", (int)(*message)->src->comp_id);
   CL_LOG_STR(CL_LOG_INFO,"dst->host:   ", (*message)->dst->comp_host);
   CL_LOG_STR(CL_LOG_INFO,"dst->comp:   ", (*message)->dst->comp_name);
   CL_LOG_INT(CL_LOG_INFO,"dst->id:     ", (int)(*message)->dst->comp_id);
   CL_LOG_STR(CL_LOG_INFO,"rdata->host: ", (*message)->rdata->comp_host);
   CL_LOG_STR(CL_LOG_INFO,"rdata->comp: ", (*message)->rdata->comp_name);
   CL_LOG_INT(CL_LOG_INFO,"rdata->id:   ", (int)(*message)->rdata->comp_id);


   return CL_RETVAL_OK;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_xml_parse_is_version()"
static cl_bool_t cl_xml_parse_is_version(char* buffer, unsigned long start, unsigned long buffer_length ) {
   unsigned long i = 0;
   cl_bool_t found = CL_FALSE;

   if (buffer == NULL) {
      return found;
   }
   for (i=start; ( i < buffer_length) && (buffer[i] != '>') ;i++) {
      /* check for version string in tag */
      if (buffer[i] == 'v') {
         if (strncmp(&buffer[i],"version", 7) == 0) {
            found = CL_TRUE;
            break;
         }   
      }
   }
   return found;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_xml_parse_MIH()"
int cl_xml_parse_MIH(unsigned char* buffer, unsigned long buffer_length, cl_com_MIH_t** message ) {
   unsigned long i;
   char help_buf[256];
   unsigned long help_buf_pointer = 0;
   unsigned long buf_pointer = 0;
   int in_tag = 0;
   unsigned long tag_begin = 0;
   unsigned long tag_end = 0;
   unsigned long version_begin = 0;
 
   unsigned long mid_begin = 0;
   unsigned long mid_end = 0;
   unsigned long dl_begin = 0;
   unsigned long dl_end = 0;
   unsigned long df_begin = 0;
   unsigned long df_end = 0;
   unsigned long mat_begin = 0;
   unsigned long mat_end = 0;  
   unsigned long mtag_begin = 0;
   unsigned long mtag_end = 0;  
   unsigned long rid_begin = 0;
   unsigned long rid_end = 0;



   if (message == NULL || buffer == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   if (*message != NULL) {
      return CL_RETVAL_PARAMS;
   }

   *message = (cl_com_MIH_t*)malloc(sizeof(cl_com_MIH_t));
   if (*message == NULL) {
      return CL_RETVAL_MALLOC;
   }
   memset((char *) (*message), 0, sizeof(cl_com_MIH_t));
   
   (*message)->df  = CL_MIH_DF_UNDEFINED;
   (*message)->mat = CL_MIH_MAT_UNDEFINED;
   
   while(buf_pointer < buffer_length) {
      switch( buffer[buf_pointer] ) {
         case '=':
            if (in_tag == 1) {
               if ( cl_xml_parse_is_version((char*)buffer, tag_begin, buffer_length) == CL_TRUE) {
                  version_begin = buf_pointer + 2;
               }
            }
         break;   
         case '<':
            in_tag = 1;
            tag_begin = buf_pointer + 1;
         break; 
         case '>':
            in_tag = 0;
            tag_end = buf_pointer - 1;
            if (tag_begin < tag_end && tag_begin > 0 && tag_end > 0) {
               
               help_buf_pointer = 0;
               for (i=tag_begin;i<=tag_end && help_buf_pointer < 254 ;i++) {
                  help_buf[help_buf_pointer++] = buffer[i];
               }
               help_buf[help_buf_pointer] = 0;



               if (strcmp(help_buf,"/mih") == 0) {
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"mid") == 0 ) {
                  mid_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/mid") == 0) {
                  mid_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"dl") == 0 ) {
                  dl_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/dl") == 0) {
                  dl_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"df") == 0 ) {
                  df_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/df") == 0) {
                  df_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               } 
               if (strcmp(help_buf,"mat") == 0 ) {
                  mat_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/mat") == 0) {
                  mat_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"tag") == 0 ) {
                  mtag_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/tag") == 0) {
                  mtag_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"rid") == 0 ) {
                  rid_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/rid") == 0) {
                  rid_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
            }
         break;
      }
      buf_pointer++;
   }


   /* get version */
   if (version_begin > 0) {
      help_buf_pointer = 0;
      for (i=version_begin ; i<= buffer_length && buffer[i] != '\"' && help_buf_pointer < 254 ; i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->version = strdup(help_buf);
   }


   /* get mid */
   if (mid_begin > 0 && mid_end > 0 && mid_end >= mid_begin) {
      help_buf_pointer = 0;
      for (i=mid_begin;i<=mid_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->mid = cl_util_get_ulong_value(help_buf);
   }

   /* get tag */
   if (mtag_begin > 0 && mtag_end > 0 && mtag_end >= mtag_begin) {
      help_buf_pointer = 0;
      for (i=mtag_begin;i<=mtag_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->tag = cl_util_get_ulong_value(help_buf);
   }

   /* get rid */
   if (rid_begin > 0 && rid_end > 0 && rid_end >= rid_begin) {
      help_buf_pointer = 0;
      for (i=rid_begin;i<=rid_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->rid = cl_util_get_ulong_value(help_buf);
   }

   /* get dl */
   if (dl_begin > 0 && dl_end > 0 && dl_end >= dl_begin) {
      help_buf_pointer = 0;
      for (i=dl_begin;i<=dl_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->dl = cl_util_get_ulong_value(help_buf);
   }

   /* get df */
   if (df_begin > 0 && df_end > 0 && df_end >= df_begin) {
      help_buf_pointer = 0;
      for (i=df_begin;i<=df_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      if (strcmp(CL_MIH_MESSAGE_DATA_FORMAT_BIN,help_buf) == 0) {
         (*message)->df = CL_MIH_DF_BIN;
      }
      if (strcmp(CL_MIH_MESSAGE_DATA_FORMAT_XML,help_buf) == 0) {
         (*message)->df = CL_MIH_DF_XML;
      }
      if (strcmp(CL_MIH_MESSAGE_DATA_FORMAT_AM,help_buf) == 0) {
         (*message)->df = CL_MIH_DF_AM;
      }
      if (strcmp(CL_MIH_MESSAGE_DATA_FORMAT_SIM,help_buf) == 0) {
         (*message)->df = CL_MIH_DF_SIM;
      }
      if (strcmp(CL_MIH_MESSAGE_DATA_FORMAT_SIRM,help_buf) == 0) {
         (*message)->df = CL_MIH_DF_SIRM;
      }
      if (strcmp(CL_MIH_MESSAGE_DATA_FORMAT_CCM,help_buf) == 0) {
         (*message)->df = CL_MIH_DF_CCM;
      }
      if (strcmp(CL_MIH_MESSAGE_DATA_FORMAT_CCRM,help_buf) == 0) {
         (*message)->df = CL_MIH_DF_CCRM;
      }

   }
   /* get mat */
   if (mat_begin > 0 && mat_end > 0 && mat_end >= mat_begin) {
      help_buf_pointer = 0;
      for (i=mat_begin;i<=mat_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      if (strcmp(CL_MIH_MESSAGE_ACK_TYPE_NAK,help_buf) == 0) {
         (*message)->mat = CL_MIH_MAT_NAK;
      }
      if (strcmp(CL_MIH_MESSAGE_ACK_TYPE_ACK,help_buf) == 0) {
         (*message)->mat = CL_MIH_MAT_ACK;
      }
      if (strcmp(CL_MIH_MESSAGE_ACK_TYPE_SYNC,help_buf) == 0) {
         (*message)->mat = CL_MIH_MAT_SYNC;
      }
   }


   CL_LOG_STR(CL_LOG_INFO,"version: ", (*message)->version);
   CL_LOG_INT(CL_LOG_INFO,"mid:     ", (int)(*message)->mid);
   CL_LOG_INT(CL_LOG_INFO,"dl:      ", (int)(*message)->dl);
   CL_LOG_STR(CL_LOG_INFO,"df:      ", cl_com_get_mih_df_string((*message)->df));
   CL_LOG_STR(CL_LOG_INFO,"mat:     ", cl_com_get_mih_mat_string((*message)->mat));
   CL_LOG_INT(CL_LOG_INFO,"tag:     ", (int)(*message)->tag);
   CL_LOG_INT(CL_LOG_INFO,"rid:     ", (int)(*message)->rid);


   if ( (*message)->dl > CL_DEFINE_MAX_MESSAGE_LENGTH ) {
      return CL_RETVAL_MAX_MESSAGE_LENGTH_ERROR; 
   }

   return CL_RETVAL_OK;
}
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_xml_parse_SIRM()"
int cl_xml_parse_SIRM(unsigned char* buffer, unsigned long buffer_length, cl_com_SIRM_t** message ) {
   unsigned long i;
   char help_buf[256];
   unsigned long help_buf_pointer = 0;
   unsigned long buf_pointer = 0;
   int in_tag = 0;
   unsigned long tag_begin = 0;
   unsigned long tag_end = 0;
   unsigned long version_begin = 0;
 
   unsigned long mid_begin = 0;
   unsigned long mid_end = 0;

   unsigned long starttime_begin = 0;
   unsigned long starttime_end = 0;

   unsigned long runtime_begin = 0;
   unsigned long runtime_end = 0;

   unsigned long application_messages_brm_begin = 0;
   unsigned long application_messages_brm_end = 0;

   unsigned long application_messages_bwm_begin = 0;
   unsigned long application_messages_bwm_end = 0;

   unsigned long application_connections_noc_begin = 0;
   unsigned long application_connections_noc_end = 0;

   unsigned long application_status_begin = 0;
   unsigned long application_status_end = 0;


   unsigned long info_begin = 0;
   unsigned long info_end = 0;



   if (message == NULL || buffer == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   if (*message != NULL) {
      return CL_RETVAL_PARAMS;
   }

   *message = (cl_com_SIRM_t*)malloc(sizeof(cl_com_SIRM_t));
   if (*message == NULL) {
      return CL_RETVAL_MALLOC;
   }
   memset((char *) (*message), 0, sizeof(cl_com_SIRM_t));
   
   while(buf_pointer < buffer_length) {
      switch( buffer[buf_pointer] ) {
         case '=':
            if (in_tag == 1) {
               if ( cl_xml_parse_is_version((char*)buffer, tag_begin, buffer_length) == CL_TRUE) {
                  version_begin = buf_pointer + 2;
               }
            }
         break;   
         case '<':
            in_tag = 1;
            tag_begin = buf_pointer + 1;
         break; 
         case '>':
            in_tag = 0;
            tag_end = buf_pointer - 1;
            if (tag_begin < tag_end && tag_begin > 0 && tag_end > 0) {
               
               help_buf_pointer = 0;
               for (i=tag_begin;i<=tag_end && help_buf_pointer < 254 ;i++) {
                  help_buf[help_buf_pointer++] = buffer[i];
               }
               help_buf[help_buf_pointer] = 0;



               if (strcmp(help_buf,"/sirm") == 0) {
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"mid") == 0 ) {
                  mid_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/mid") == 0) {
                  mid_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }

               if (strcmp(help_buf,"starttime") == 0 ) {
                  starttime_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/starttime") == 0) {
                  starttime_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }

               if (strcmp(help_buf,"runtime") == 0 ) {
                  runtime_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/runtime") == 0) {
                  runtime_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"brm") == 0 ) {
                  application_messages_brm_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/brm") == 0) {
                  application_messages_brm_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"bwm") == 0 ) {
                  application_messages_bwm_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/bwm") == 0) {
                  application_messages_bwm_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"noc") == 0 ) {
                  application_connections_noc_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/noc") == 0) {
                  application_connections_noc_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"status") == 0 ) {
                  application_status_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/status") == 0) {
                  application_status_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"info") == 0 ) {
                  info_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/info") == 0) {
                  info_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
            }
         break;
      }
      buf_pointer++;
   }


   /* get version */
   if (version_begin > 0) {
      help_buf_pointer = 0;
      for (i=version_begin ; i<= buffer_length && buffer[i] != '\"' && help_buf_pointer < 254 ; i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->version = strdup(help_buf);
   }

   /* get info */
   if (info_begin > 0 && info_end > 0 && info_end >= info_begin) {
      help_buf_pointer = 0;
      for (i=info_begin;i<=info_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->info = strdup(help_buf);
   }


   /* get mid */
   if (mid_begin > 0 && mid_end > 0 && mid_end >= mid_begin) {
      help_buf_pointer = 0;
      for (i=mid_begin;i<=mid_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->mid = cl_util_get_ulong_value(help_buf);
   }

   /* get starttime */
   if (starttime_begin > 0 && starttime_end > 0 && starttime_end >= starttime_begin) {
      help_buf_pointer = 0;
      for (i=starttime_begin;i<=starttime_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->starttime = cl_util_get_ulong_value(help_buf);
   }

   /* get runtime */
   if (runtime_begin > 0 && runtime_end > 0 && runtime_end >= runtime_begin) {
      help_buf_pointer = 0;
      for (i=runtime_begin;i<=runtime_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->runtime = cl_util_get_ulong_value(help_buf);
   }

   /* get application_messages_brm */
   if (application_messages_brm_begin > 0 && application_messages_brm_end > 0 && application_messages_brm_end >= application_messages_brm_begin) {
      help_buf_pointer = 0;
      for (i=application_messages_brm_begin;i<=application_messages_brm_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->application_messages_brm = cl_util_get_ulong_value(help_buf);
   }

   /* get application_messages_bwm */
   if (application_messages_bwm_begin > 0 && application_messages_bwm_end > 0 && application_messages_bwm_end >= application_messages_bwm_begin) {
      help_buf_pointer = 0;
      for (i=application_messages_bwm_begin;i<=application_messages_bwm_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->application_messages_bwm = cl_util_get_ulong_value(help_buf);
   }

   /* get application_connections_noc */
   if (application_connections_noc_begin > 0 && application_connections_noc_end > 0 && application_connections_noc_end >= application_connections_noc_begin) {
      help_buf_pointer = 0;
      for (i=application_connections_noc_begin;i<=application_connections_noc_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->application_connections_noc = cl_util_get_ulong_value(help_buf);
   }

   /* get application_connections_noc */
   if (application_status_begin > 0 && application_status_end > 0 && application_status_end >= application_status_begin) {
      help_buf_pointer = 0;
      for (i=application_status_begin;i<=application_status_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->application_status = cl_util_get_ulong_value(help_buf);
   }


   CL_LOG_STR(CL_LOG_WARNING,"version:   ", (*message)->version);
   CL_LOG_INT(CL_LOG_WARNING,"mid:       ", (int)(*message)->mid);
   CL_LOG_INT(CL_LOG_WARNING,"starttime: ", (int)(*message)->starttime);
   CL_LOG_INT(CL_LOG_WARNING,"runtime:   ", (int)(*message)->runtime);
   CL_LOG_INT(CL_LOG_WARNING,"brm:       ", (int)(*message)->application_messages_brm);
   CL_LOG_INT(CL_LOG_WARNING,"bwm:       ", (int)(*message)->application_messages_bwm);
   CL_LOG_INT(CL_LOG_WARNING,"noc:       ", (int)(*message)->application_connections_noc);
   CL_LOG_INT(CL_LOG_WARNING,"status:    ", (int)(*message)->application_status);
   CL_LOG_STR(CL_LOG_WARNING,"info:      ", (*message)->info);

   return CL_RETVAL_OK;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_xml_parse_AM()"
int cl_xml_parse_AM(unsigned char* buffer, unsigned long buffer_length, cl_com_AM_t** message ) {
   unsigned long i;
   char help_buf[256];
   unsigned long help_buf_pointer = 0;
   unsigned long buf_pointer = 0;
   int in_tag = 0;
   unsigned long tag_begin = 0;
   unsigned long tag_end = 0;
   unsigned long version_begin = 0;
 
   unsigned long mid_begin = 0;
   unsigned long mid_end = 0;


   if (message == NULL || buffer == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   if (*message != NULL) {
      return CL_RETVAL_PARAMS;
   }

   *message = (cl_com_AM_t*)malloc(sizeof(cl_com_AM_t));
   if (*message == NULL) {
      return CL_RETVAL_MALLOC;
   }
   memset((char *) (*message), 0, sizeof(cl_com_AM_t));
   
   while(buf_pointer < buffer_length) {
      switch( buffer[buf_pointer] ) {
         case '=':
            if (in_tag == 1) {
               if ( cl_xml_parse_is_version((char*)buffer, tag_begin, buffer_length) == CL_TRUE) {
                  version_begin = buf_pointer + 2;
               }
            }
         break;   
         case '<':
            in_tag = 1;
            tag_begin = buf_pointer + 1;
         break; 
         case '>':
            in_tag = 0;
            tag_end = buf_pointer - 1;
            if (tag_begin < tag_end && tag_begin > 0 && tag_end > 0) {
               
               help_buf_pointer = 0;
               for (i=tag_begin;i<=tag_end && help_buf_pointer < 254 ;i++) {
                  help_buf[help_buf_pointer++] = buffer[i];
               }
               help_buf[help_buf_pointer] = 0;



               if (strcmp(help_buf,"/am") == 0) {
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"mid") == 0 ) {
                  mid_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/mid") == 0) {
                  mid_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
            }
         break;
      }
      buf_pointer++;
   }


   /* get version */
   if (version_begin > 0) {
      help_buf_pointer = 0;
      for (i=version_begin ; i<= buffer_length && buffer[i] != '\"' && help_buf_pointer < 254 ; i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->version = strdup(help_buf);
   }


   /* get mid */
   if (mid_begin > 0 && mid_end > 0 && mid_end >= mid_begin) {
      help_buf_pointer = 0;
      for (i=mid_begin;i<=mid_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->mid = cl_util_get_ulong_value(help_buf);
   }

   CL_LOG_STR(CL_LOG_INFO,"version: ", (*message)->version);
   CL_LOG_INT(CL_LOG_INFO,"mid:     ", (int)(*message)->mid);

   return CL_RETVAL_OK;

}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_xml_parse_CCM()"
int cl_xml_parse_CCM(unsigned char* buffer, unsigned long buffer_length, cl_com_CCM_t** message ) {
   unsigned long i;
   char help_buf[256];
   unsigned long help_buf_pointer = 0;
   unsigned long buf_pointer = 0;
   int in_tag = 0;
   unsigned long tag_begin = 0;
   unsigned long tag_end = 0;
   unsigned long version_begin = 0;
 
   if (message == NULL || buffer == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   if (*message != NULL) {
      return CL_RETVAL_PARAMS;
   }

   *message = (cl_com_CCM_t*)malloc(sizeof(cl_com_CCM_t));
   if (*message == NULL) {
      return CL_RETVAL_MALLOC;
   }
   memset((char *) (*message), 0, sizeof(cl_com_CCM_t));
   
   while(buf_pointer < buffer_length) {
      switch( buffer[buf_pointer] ) {
         case '=':
            if (in_tag == 1) {
               if ( cl_xml_parse_is_version((char*)buffer, tag_begin, buffer_length) == CL_TRUE) {
                  version_begin = buf_pointer + 2;
               }
            }
         break;   
         case '<':
            in_tag = 1;
            tag_begin = buf_pointer + 1;
         break; 
         case '>':
            in_tag = 0;
            tag_end = buf_pointer - 1;
            if (tag_begin < tag_end && tag_begin > 0 && tag_end > 0) {
               
               help_buf_pointer = 0;
               for (i=tag_begin;i<=tag_end && help_buf_pointer < 254 ;i++) {
                  help_buf[help_buf_pointer++] = buffer[i];
               }
               help_buf[help_buf_pointer] = 0;


               if (strcmp(help_buf,"/ccm") == 0) {
                  buf_pointer++;
                  continue;
               }
            }
         break;
      }
      buf_pointer++;
   }


   /* get version */
   if (version_begin > 0) {
      help_buf_pointer = 0;
      for (i=version_begin ; i<= buffer_length && buffer[i] != '\"' && help_buf_pointer < 254 ; i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->version = strdup(help_buf);
   }


   CL_LOG_STR(CL_LOG_INFO,"version: ", (*message)->version);
   return CL_RETVAL_OK;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_xml_parse_CCRM()"
int cl_xml_parse_CCRM(unsigned char* buffer, unsigned long buffer_length, cl_com_CCRM_t** message ) {
   unsigned long i;
   char help_buf[256];
   unsigned long help_buf_pointer = 0;
   unsigned long buf_pointer = 0;
   int in_tag = 0;
   unsigned long tag_begin = 0;
   unsigned long tag_end = 0;
   unsigned long version_begin = 0;
 

   if (message == NULL || buffer == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   if (*message != NULL) {
      return CL_RETVAL_PARAMS;
   }

   *message = (cl_com_CCRM_t*)malloc(sizeof(cl_com_CCRM_t));
   if (*message == NULL) {
      return CL_RETVAL_MALLOC;
   }
   memset((char *) (*message), 0, sizeof(cl_com_CCRM_t));
   
   while(buf_pointer < buffer_length) {
      switch( buffer[buf_pointer] ) {
         case '=':
            if (in_tag == 1) {
               if ( cl_xml_parse_is_version((char*)buffer, tag_begin, buffer_length) == CL_TRUE) {
                  version_begin = buf_pointer + 2;
               }
            }
         break;   
         case '<':
            in_tag = 1;
            tag_begin = buf_pointer + 1;
         break; 
         case '>':
            in_tag = 0;
            tag_end = buf_pointer - 1;
            if (tag_begin < tag_end && tag_begin > 0 && tag_end > 0) {
               
               help_buf_pointer = 0;
               for (i=tag_begin;i<=tag_end && help_buf_pointer < 254 ;i++) {
                  help_buf[help_buf_pointer++] = buffer[i];
               }
               help_buf[help_buf_pointer] = 0;

               if (strcmp(help_buf,"/ccrm") == 0) {
                  buf_pointer++;
                  continue;
               }
            }
         break;
      }
      buf_pointer++;
   }


   /* get version */
   if (version_begin > 0) {
      help_buf_pointer = 0;
      for (i=version_begin ; i<= buffer_length && buffer[i] != '\"' && help_buf_pointer < 254 ; i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->version = strdup(help_buf);
   }


   CL_LOG_STR(CL_LOG_INFO,"version: ", (*message)->version);

   return CL_RETVAL_OK;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_xml_parse_SIM()"
int cl_xml_parse_SIM(unsigned char* buffer, unsigned long buffer_length, cl_com_SIM_t** message ) {
   unsigned long i;
   char help_buf[256];
   unsigned long help_buf_pointer = 0;
   unsigned long buf_pointer = 0;
   int in_tag = 0;
   unsigned long tag_begin = 0;
   unsigned long tag_end = 0;
   unsigned long version_begin = 0;
 
   if (message == NULL || buffer == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   if (*message != NULL) {
      return CL_RETVAL_PARAMS;
   }

   *message = (cl_com_SIM_t*)malloc(sizeof(cl_com_SIM_t));
   if (*message == NULL) {
      return CL_RETVAL_MALLOC;
   }
   memset((char *) (*message), 0, sizeof(cl_com_SIM_t));
   
   while(buf_pointer < buffer_length) {
      switch( buffer[buf_pointer] ) {
         case '=':
            if (in_tag == 1) {
               if ( cl_xml_parse_is_version((char*)buffer, tag_begin, buffer_length) == CL_TRUE) {
                  version_begin = buf_pointer + 2;
               }
            }
         break;   
         case '<':
            in_tag = 1;
            tag_begin = buf_pointer + 1;
         break; 
         case '>':
            in_tag = 0;
            tag_end = buf_pointer - 1;
            if (tag_begin < tag_end && tag_begin > 0 && tag_end > 0) {
               
               help_buf_pointer = 0;
               for (i=tag_begin;i<=tag_end && help_buf_pointer < 254 ;i++) {
                  help_buf[help_buf_pointer++] = buffer[i];
               }
               help_buf[help_buf_pointer] = 0;

               if (strcmp(help_buf,"/sim") == 0) {
                  buf_pointer++;
                  continue;
               }
            }
         break;
      }
      buf_pointer++;
   }


   /* get version */
   if (version_begin > 0) {
      help_buf_pointer = 0;
      for (i=version_begin ; i<= buffer_length && buffer[i] != '\"' && help_buf_pointer < 254 ; i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->version = strdup(help_buf);
   }


   CL_LOG_STR(CL_LOG_INFO,"version: ", (*message)->version);
   return CL_RETVAL_OK;
}



/* free cl_com_endpoint_t  structure 
  
   params: 
   cl_com_endpoint_t** endpoint -> address of an pointer to cl_com_endpoint_t

   return:
      - *endpoint is set to NULL
      - int - CL_RETVAL_XXXX error number
*/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_free_endpoint()"
int cl_com_free_endpoint(cl_com_endpoint_t** endpoint) { /* CR check */
   if (endpoint == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*endpoint == NULL) {
      return CL_RETVAL_PARAMS;
   }

   free((*endpoint)->comp_host);
   free((*endpoint)->comp_name);
   free(*endpoint);
   *endpoint = NULL;
   return CL_RETVAL_OK;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_dup_endpoint()"
cl_com_endpoint_t* cl_com_dup_endpoint(cl_com_endpoint_t* endpoint) {
    if (endpoint == NULL) {
       return NULL;
    }  
    if (endpoint->comp_host == NULL || endpoint->comp_name == NULL) {
       return NULL;
    }
    return cl_com_create_endpoint(endpoint->comp_host, endpoint->comp_name, endpoint->comp_id  );
}



#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_create_endpoint()"
cl_com_endpoint_t* cl_com_create_endpoint(const char* host, const char* name, unsigned long id) {  /* CR check */
   cl_com_endpoint_t* endpoint = NULL;

   if (host == NULL || name == NULL ) {
      CL_LOG(CL_LOG_ERROR,"parameter errors");
      return NULL;
   }

   endpoint = (cl_com_endpoint_t*)malloc(sizeof(cl_com_endpoint_t));
   if (endpoint == NULL) {
      CL_LOG(CL_LOG_ERROR,"malloc error");
      return NULL;
   }
   endpoint->comp_host = strdup(host);
   endpoint->comp_name = strdup(name);
   endpoint->comp_id   = id;

   if (endpoint->comp_host == NULL || endpoint->comp_name == NULL) {
      cl_com_free_endpoint(&endpoint);
      CL_LOG(CL_LOG_ERROR,"malloc error");
      return NULL;
   }
   return endpoint;
}





