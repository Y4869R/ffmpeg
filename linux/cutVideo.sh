#/bin/bash

echo $#
if [ $# -lt 3  ]; then
  echo "reutrn"
  exit
fi


INPUT=$1
STARTTIME=$2
DURATION=$3
FILENAME=${INPUT%%.*}


ffmpeg -i $INPUT -ss $STARTTIME -codec copy -t $DURATION  -y  $FILENAME'_new.mp4'
