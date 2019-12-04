#/bin/bash

echo $#
if [ $# -lt 1  ]; then
  echo "reutrn"
  exit
fi


INPUT=$1
NEWDIR=${INPUT%%.*}
OUTPUT=${INPUT%%.*}.gif
GIFDURATION=8
FRAMERATE=5

echo $NEWDIR
if [ -d $NEWDIR ]; then
  ls $NEWDIR/
  rm $NEWDIR/* -rf
  mkdir $NEWDIR/image
else
  mkdir $NEWDIR
  mkdir $NEWDIR/image
fi

if [ -f $OUTPUT ]; then
  #rm $OUTPUT
  ls -lh $OUTPUT
fi

echo $NEWDIR/frames_%04d.png

#srale= 宽度为200，宽高比不变
#-r 帧率为10
#-t 只处理前10秒

#ffmpeg -i $INPUT -vf scale=200:-1 -r 2 $OUTPUT

# step1
# -r 帧率
ffmpeg -i $INPUT -r $FRAMERATE -vf scale=200:-1 -t 10 $NEWDIR/image/frames_%04d.png

#-t 时长
ffmpeg -i  $NEWDIR/image/frames_%04d.png -r $GIFDURATION -y $NEWDIR/$OUTPUT

ls -lh $NEWDIR/$OUTPUT
