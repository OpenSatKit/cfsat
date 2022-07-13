#!/bin/sh
# Assumes only one cFS porcess is running
echo "Stop cFS Script"
CFS=`pgrep core`
if ! [ -z "$var" ]
then
   echo $CFS
   kill $CFS
fi
