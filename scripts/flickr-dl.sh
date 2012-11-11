#!/bin/bash

# Script for downloading images with a given tag. Originally from
# http://www.stanford.edu/~acoates/snippets.html adapted for personal purposes.

FLICKR_API_KEY="490efef2630371923ce87b9c4420f717"

usage()
{
cat << EOF

Usage: `basename $0` options

This script downloads images from flickr.

OPTIONS:
  -h  Show this help
  -n  Number of images to download
  -o  Output directory (optional)
  -t  Tags to search on, comma separated (optional)

EOF
}

COUNT=
TAGS=
OUTPUT_DIR="./"

while getopts "hn:o:t:" OPTION
do
  case $OPTION in
    h)
      usage
      exit 0
      ;;
    n)
      COUNT=$OPTARG
      ;;
    o)
      OUTPUT_DIR=$OPTARG
      mkdir -vp "$OUTPUT_DIR"
      ;;
    t)
      TAGS=$OPTARG
      ;;
    ?)
      usage
      exit 1
      ;;
  esac
done

if [[ -z "$COUNT" ]]
then
  usage
  exit 1
fi

if [[ -n "$TAGS" ]]
then
  QUERY="http://www.flickr.com/services/rest/?api_key=${FLICKR_API_KEY}&method=flickr.photos.search&per_page=20&tags=${TAGS}&sort=interestingness-desc"
else
  QUERY="http://www.flickr.com/services/rest/?api_key=${FLICKR_API_KEY}&method=flickr.photos.getRecent&per_page=20&sort=interestingness-desc"
fi

OUT=$(wget -q -O - "$QUERY")
PAGE_COUNT=$(echo "$OUT" | grep 'pages=' | sed 's/.*pages="\([0-9]*\)".*/\1/g')
PAGES_NEEDED=$(( $COUNT / 20 + 1 ))
if [[ $PAGE_COUNT -gt $PAGES_NEEDED ]]
then
  PAGE_COUNT=$PAGES_NEEDED
fi

for i in `seq 1 $PAGE_COUNT`
do
  OUT=$(wget -q -O - "${QUERY}&page=${i}")
  IDS=$(echo "$OUT" | sed 's/.*id="\([0-9]*\)".*/\1/g' | grep ^[0-9])
  echo "Now on page: $i / $PAGE_COUNT"
  for id in $IDS
  do
    SIZE_QUERY="http://www.flickr.com/services/rest/?api_key=${FLICKR_API_KEY}&method=flickr.photos.getSizes&photo_id=${id}"
    SIZE_OUT=$(wget -q -O - "$SIZE_QUERY")
    FIELDS=$(echo "$SIZE_OUT" | grep 'source=' | sed 's/.*width="\([0-9]*\)".*source="\([^"]*\)".*/\1 \2/g')
    URL=$(echo "$FIELDS" | awk 'BEGIN {maxw=0; out=0;}  { if ($1 > maxw) { out=$2; maxw = $1; } } END { if (maxw >= 640) { print out; } }')
    FNAME=$(basename "$URL")
    
    if [[ -e "$OUTPUT_DIR/$FNAME" ]]
    then
      echo "Duplicate detected, skipping..."
      continue
    fi

    if [[ -n "$URL" ]]
    then
      echo -n "Downloading $FNAME..."
      wget -q -O "$OUTPUT_DIR/$FNAME" "$URL"
      [[ $? -eq 0 ]] && echo "[done]" || echo "[failed]"
    fi
  done
done

exit 0
