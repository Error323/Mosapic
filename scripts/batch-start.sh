#!/bin/bash

INPUTDIR=
DATABASE=
THREADS=2
WIDTH=20
DIM=10
RAD=20
CBR=0.8

usage()
{
cat << EOF

Usage: `basename $0` options

This script creates mosaics from images.

OPTIONS:
  -h  Show this help
  -i  Input directory, images to be mosaic-ed
  -d  Database directory
  -t  Number of threads to use, in {1,...,N} (default=$THREADS)
  -w  Width in tiles of a mosaic, in {1,...,N} (default=$WIDTH)

EOF
}

while getopts "hi:d:t:w:" OPTION
do
  case $OPTION in
    h)
      usage
      exit 0
      ;;
    i)
      INPUTDIR=$OPTARG
      ;;
    d)
      DATABASE=$OPTARG
      ;;
    t)
      THREADS=$OPTARG
      ;;
    w)
      WIDTH=$OPTARG
      ;;
    ?)
      usage
      exit 1
  esac
done

if [[ ! -d "$INPUTDIR" ]] || [[ ! -d "$DATABASE" ]]
then
  usage
  exit 1
fi

for ((i = 0; i < THREADS; i++))
do
  SLOT[i]=-1
done

num_jobs=0
cur_job=0
remaining=0
for IMG in ${INPUTDIR}*
do
  cmd="../build/hexapic --input-image=${IMG} --database=${DATABASE} --width=${WIDTH} --dimensions=${DIM} --min-radius=${RAD} --cb-ratio=${CBR}"
  JOBS[num_jobs]=$cmd
  num_jobs=$((num_jobs + 1))
done

fill_slots()
{
  for ((i = 0; i < THREADS; i++))
  do
    if ((${SLOT[i]} == -1))
    then
      tput setaf $((i + 1))
      echo ${JOBS[cur_job]}
      tput setaf sgr0
      ./${JOBS[$cur_job]} > /dev/null &
      SLOT[i]=$!
      cur_job=$((cur_job + 1))
    fi
  done
}

release_slots()
{
  remaining=0
	for ((i = 0; i < THREADS; i++)); do
		if ! kill -0 ${SLOT[i]} >/dev/null 2>&1
    then
      # Release slot again
			SLOT[i]=-1
    else
      remaining=$((remaining + 1))
		fi
	done
}

# Main loop
while (($cur_job < $num_jobs))
do
  fill_slots
  release_slots
  sleep 1
done

echo -n "Wait for $remaining remaining jobs..."
wait
echo "[done]"

exit 0
