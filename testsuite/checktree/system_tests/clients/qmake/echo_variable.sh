#!/bin/sh

variable=$1
value=`eval echo '$'$variable`
exec echo "$variable=$value"
