#!/bin/sh

printf "petask %3d started  on host %s\n" $1 $HOSTNAME
sleep $2
printf "petask %3d finished on host %s\n" $1 $HOSTNAME
