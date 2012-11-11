#!/bin/bash

INPUTDIR=$1
DATABASE=$2
WIDTH=6
HEIGHT=3
DIM=16
RAD=12
CBR=0.1

for IMG in ${INPUTDIR}*
do
    cmd="./../build/Hexapic --input-image=${IMG} --database=${DATABASE} --width=${WIDTH} --height=${HEIGHT} --dimensions=${DIM} --min-radius=${RAD} --cb-ratio=${CBR}"

    echo ""
    echo ${cmd}
    ${cmd}
done

zip results.zip *.jpg
