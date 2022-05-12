#!/bin/sh
# Assumes only one cFS porcess is running
echo "Stop cFS Script"
CFS=`pgrep core`
echo $CFS
kill $CFS
