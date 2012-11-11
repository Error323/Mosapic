#!/bin/bash

INPUTDIR=$1
DATABASE=$2
WIDTH=20
HEIGHT=18
DIM=16
RAD=12

for IMG in ${INPUTDIR}*
do
    cmd="./../build/Hexapic --input-image=${IMG} --database=${DATABASE} --width=${WIDTH} --height=${HEIGHT} --dimensions=${DIM} --min-radius=${RAD}"

    echo ""
    echo ${cmd}
    ${cmd}
done

zip results.zip *.jpg
