#!/bin/sh

# generate a CODINE/GRD version 5.1 "startup_template"
# inherits from environment:
#     $SGE_CELL
#     $COD_CELL
# args:
#     $1 = ""
#     $2 = $COMMD_PORT value or "0" if service is used
#     $3 = file to read
#     $4 = file to write


if [ "$CODINE_ROOT" = "" -o "$COD_CELL" = "" ]; then
   echo "CODINE_ROOT and/or COD_CELL variable is not set"
   echo command failed
   exit 1
fi

if [ $# != 4 ]; then
   echo "usage: $0 codine|grd 0|$COMMD_PORT file_to_read file_to_write"
   echo command failed
   exit 1
fi

    $3 > $4
