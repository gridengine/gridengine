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
#include <string.h>

#include "sge_bitfield.h"

/****** uti/bitfield/--Bitfield ****************************************
*  NAME
*     Bitfield -- A variable size bitfield implementation
*
*  SYNOPSIS
*     bitfield sge_bitfield_new(int size) 
*
*  FUNCTION
*     This module provides variable size bitfields.
*     The size of a bitfield can be defined when the bitfield is created.
*     Individual bits can be set, read and cleared.
*     The contents of a bitfield can be printed to stdout or any 
*     file handle.
*
*  EXAMPLE
*     See main program (module test) in libs/uti/sge_bitfield.c
*
*  NOTES
*     MT-NOTE: this module is MT safe
*
*  SEE ALSO
*     uti/bitfield/sge_bitfield_new()
*     uti/bitfield/sge_bitfield_free()
*     uti/bitfield/sge_bitfield_set()
*     uti/bitfield/sge_bitfield_get()
*     uti/bitfield/sge_bitfield_clear()
*     uti/bitfield/sge_bitfield_print()
*******************************************************************************/

/****** uti/bitfield/-Bitfield_Typedefs ****************************************
*  NAME
*     Bitfield_Typedefs -- type definitions for the Bitfield module
*
*  SYNOPSIS
*     typedef struct {
*        int size;
*        char *bf;
*     } _bitfield;
*     
*     typedef _bitfield *bitfield;
*
*  FUNCTION
*     The _bitfield structure is the internal representation of a bitfield.
*     All operations on bitfields use the bitfield type.
*******************************************************************************/

/****** uti/bitfield/sge_bitfield_new() ****************************************
*  NAME
*     sge_bitfield_new() -- create a new bitfield
*
*  SYNOPSIS
*     bitfield sge_bitfield_new(int size) 
*
*  FUNCTION
*     Allocates and initializes the necessary memory.
*     It is in the responsibility of the caller to free the bitfield
*     once it is no longer needed.
*
*  INPUTS
*     int size - size in bits
*
*  RESULT
*     bitfield - a new bitfield or NULL, if the creation of the bitfield 
*                failed
*
*  NOTES
*     MT-NOTE: sge_bitfield_new() is MT safe
*
*  SEE ALSO
*     uti/bitfield/sge_bitfield_free()
*******************************************************************************/
bitfield sge_bitfield_new(int size)
{
   int char_size;
   bitfield bf;

   char_size = size / 8 + ((size % 8) > 0 ? 1 : 0);
   bf = (bitfield) malloc(sizeof(_bitfield));
   if(bf != NULL) {
      bf->bf = (char *)malloc(char_size);
      if(bf->bf == NULL) {
         free(bf);
         return NULL;
      }
      memset(bf->bf, 0, char_size);
      bf->size = size;
   }
   return bf;
}

/****** sge_bitfield/sge_bitfield_copy() ***************************************
*  NAME
*     sge_bitfield_copy() -- copies a bitfield into another one. 
*
*  SYNOPSIS
*     bool sge_bitfield_copy(bitfield *source, bitfield *target) 
*
*  FUNCTION
*     The memory has to be allocated before, and source and target has to have
*     the same size. Otherwise it will return false and does not copy anything.
*
*  INPUTS
*     bitfield *source - 
*     bitfield *target - 
*
*  RESULT
*     bool - false, if the bitfield sizes are different
*
*  NOTES
*     MT-NOTE: sge_bitfield_copy() is MT safe 
*
*******************************************************************************/
bool sge_bitfield_copy(bitfield source, bitfield target)
{
   if (source != NULL && target != NULL) {
      if (source->size == target->size) {
         int char_size = source->size / 8 + ((source->size % 8) > 0 ? 1 : 0);
         memcpy(target->bf, source->bf, char_size);

         return true;    
      }
   }
   
   return false;
}

/****** sge_bitfield/sge_bitfield_bitwise_copy() *******************************
*  NAME
*     sge_bitfield_copy() -- copies a bitfield into another one. 
*
*  SYNOPSIS
*     bool sge_bitfield_bitwise_copy(bitfield *source, bitfield *target) 
*
*  FUNCTION
*     The memory has to be allocated before, but the bitfields can have
*     different sizes.  If the source is longer than the target, only the bits
*     up to target's length are copied.
*
*  INPUTS
*     bitfield *source - ??? 
*     bitfield *target - ??? 
*
*  RESULT
*     bool - false, if one of the bitfields is NULL
*
*  NOTES
*     MT-NOTE: sge_bitfield_bitwise_copy() is MT safe 
*
*******************************************************************************/
bool sge_bitfield_bitwise_copy(bitfield source, bitfield target)
{
   if (source != NULL && target != NULL) {
      int char_size = 0;
      
      if (source->size > target->size) {
         /* This may result in the target getting a few more bits than it wants
          * (if target->size isn't a multiple of 8), but that shouldn't matter
          * because sge_bitfield_get() guards against accessing those extra
          * bits. */
         char_size = target->size / 8 + ((target->size % 8) > 0 ? 1 : 0);
      }
      else {
         char_size = source->size / 8 + ((source->size % 8) > 0 ? 1 : 0);
      }
      
      memcpy(target->bf, source->bf, char_size);

      return true;    
   }
   
   return false;
}

/****** sge_bitfield/sge_bitfield_changed() ***************************************
*  NAME
*     sge_bitfield_changed() -- figures out if something was changed.
*
*  SYNOPSIS
*     bool sge_bitfield_changed(bitfield *source) 
*
*  FUNCTION
*
*  INPUTS
*     bitfield *source - 
*
*  RESULT
*     bool - true, if the bitfield has a changed bit set.
*
*  NOTES
*     MT-NOTE: sge_bitfield_copy() is MT safe 
*
*******************************************************************************/
bool sge_bitfield_changed(bitfield source) {
   if (source != NULL) {
      int i;
      int char_size = source->size / 8 + ((source->size % 8) > 0 ? 1 : 0);
      for (i = 0; i<char_size; i++) {
         if (source->bf[i] != 0) {
            return true;
         }
      }
   }
   
   return false;

}

/****** sge_bitfield/sge_bitfield_reset() ***************************************
*  NAME
*     sge_bitfield_reset() -- clears a bitfield
*
*  SYNOPSIS
*     bool sge_bitfield_reset(bitfield *source) 
*
*  FUNCTION
*
*  INPUTS
*     bitfield *source - bitfield to reset
*
*  RESULT
*     bool - false, if source is NULL
*
*  NOTES
*     MT-NOTE: sge_bitfield_copy() is MT safe 
*
*******************************************************************************/
bool sge_bitfield_reset(bitfield source){
   
   if (source != NULL) {
      int char_size = source->size / 8 + ((source->size % 8) > 0 ? 1 : 0);
      memset(source->bf, 0, char_size);
      return true;    
   }
   
   return false;
}


/****** uti/bitfield/sge_bitfield_free() ***************************************
*  NAME
*     sge_bitfield_free() -- destroy a bitfield
*
*  SYNOPSIS
*     bitfield sge_bitfield_free(bitfield bf) 
*
*  FUNCTION
*     Destroys a bitfield. Frees all memory allocated by the bitfield.
*
*  INPUTS
*     bitfield bf - the bitfield to destroy
*
*  NOTES
*     MT-NOTE: sge_bitfield_free() is MT safe
*
*  RESULT
*     bitfield - NULL
*******************************************************************************/
bitfield sge_bitfield_free(bitfield bf)
{
   if(bf != NULL) {
      if(bf->bf != NULL) {
         free(bf->bf);
      }
      free(bf);
   }

   return NULL;
}

/****** uti/bitfield/sge_bitfield_set() ****************************************
*  NAME
*     sge_bitfield_set() -- set a bit
*
*  SYNOPSIS
*     int sge_bitfield_set(bitfield bf, int bit) 
*
*  FUNCTION
*     Sets a certain bit in a bitfield to 1.
*
*  INPUTS
*     bitfield bf - the bitfield to manipulate
*     int bit     - the bit to set
*
*  NOTES
*     MT-NOTE: sge_bitfield_set() is MT safe
*
*  RESULT
*     int - 1 on success, 
*           -1 on error
*******************************************************************************/
int sge_bitfield_set(bitfield bf, int bit)
{
   int byte_offset, bit_offset;

   if(bf == NULL || bit < 0 || bit >= bf->size) {
      return -1;
   }

   byte_offset = bit / 8;
   bit_offset  = bit % 8;

   bf->bf[byte_offset] = bf->bf[byte_offset] | (1 << bit_offset);

   return 1;
}

/****** uti/bitfield/sge_bitfield_get() ****************************************
*  NAME
*     sge_bitfield_get() -- read a bit 
*
*  SYNOPSIS
*     int sge_bitfield_get(bitfield bf, int bit) 
*
*  FUNCTION
*     Reads a certain bit of a bitfield and returns it's contents.
*
*  INPUTS
*     bitfield bf - the bitfield to read from
*     int bit     - the bit to read
*
*  NOTES
*     MT-NOTE: sge_bitfield_get() is MT safe
*
*  RESULT
*     int - 0 or 1 as value of the bit,
*           -1 on error
*******************************************************************************/
int sge_bitfield_get(bitfield bf, int bit)
{
   int byte_offset, bit_offset;

   if(bf == NULL || bit < 0 || bit >= bf->size) {
      return -1;
   }

   byte_offset = bit / 8;
   bit_offset  = bit % 8;

   return (bf->bf[byte_offset] & (1 << bit_offset)) > 0;
}

/****** uti/bitfield/sge_bitfield_clear() **************************************
*  NAME
*     sge_bitfield_clear() -- clear a bit
*
*  SYNOPSIS
*     int sge_bitfield_clear(bitfield bf, int bit) 
*
*  FUNCTION
*     Clears a certain bit in a bitfield (sets its content to 0).
*
*  INPUTS
*     bitfield bf - the bitfield to manipulate
*     int bit     - the bit to clear
*
*  NOTES
*     MT-NOTE: sge_bitfield_clear() is MT safe
*
*  RESULT
*     int - 0 on success,
*           -1 on error
*******************************************************************************/
int sge_bitfield_clear(bitfield bf, int bit)
{
   int byte_offset, bit_offset;

   if(bf == NULL || bit < 0 || bit >= bf->size) {
      return -1;
   }

   byte_offset = bit / 8;
   bit_offset  = bit % 8;

   bf->bf[byte_offset] = bf->bf[byte_offset] & (0xff ^ (1 << bit_offset));

   return 0;
}

/****** uti/bitfield/sge_bitfield_print() **************************************
*  NAME
*     sge_bitfield_print() -- print contents of a bitfield
*
*  SYNOPSIS
*     void sge_bitfield_print(bitfield bf, FILE *fd) 
*
*  FUNCTION
*     Prints the contents of a bitfield.
*     For each bit one digit (0/1) is printed.
*     If NULL is passed as file descriptor, output is sent to stdout.
*
*  NOTES
*     MT-NOTE: sge_bitfield_print() is MT safe
*
*  INPUTS
*     bitfield  bf - the bitfield to output
*     FILE *fd     - filehandle or NULL
*******************************************************************************/
void sge_bitfield_print(bitfield  bf, FILE *fd)
{
   int char_size, rest_bits;
   int i, j;

   if(fd == NULL) {
      fd = stdout;
   }

   char_size = bf->size / 8;
   rest_bits = bf->size % 8;

   for(i = 0; i < char_size; i++) {
      for(j = 0; j < 8; j++) {
         fprintf(fd, "%d ", (bf->bf[i] >> j) & 0x01);
      }
   }

   if(rest_bits > 0) {
      for(j = 0; j < rest_bits; j++) {
         fprintf(fd, "%d ", (bf->bf[char_size] >> j) & 0x01);
      }
   }
}

#ifdef TEST_SGE_BITFIELD


main() 
{
   bitfield bf;
   int i;

   bf = sge_bitfield_new(10);
   printf("no bits set:      "); sge_bitfield_print(bf, NULL); printf("\n");

   sge_bitfield_set(bf, 5);
   sge_bitfield_set(bf, 100);
   sge_bitfield_set(bf, -10);
   sge_bitfield_set(bf, 0);
   printf("bits 0 and 5 set: "); sge_bitfield_print(bf, stdout); printf("\n");

   sge_bitfield_clear(bf, 0);
   sge_bitfield_clear(bf, 1);
   printf("bit 5 set:        "); sge_bitfield_print(bf, stdout); printf("\n");

   for(i = -5; i < 20; i++) {
      printf("value at bit %3d: %d\n", i, sge_bitfield_get(bf, i));
   }

   bf = sge_bitfield_free(bf);
   exit(EXIT_SUCCESS);
}

#endif
