#! /bin/sh
#
# $Id: upd_make.sh,v 1.1 1996/08/05 20:01:11 drd Exp $
#
# shell script to update all makefiles which include makefile.all
# after makefile.all has changed.
# simply uses sed to delete all lines between <<<makefile.all and
# >>>makefile.all
# partially derived from errorfix.sh

for i in makefile.* Makefile.in
do
  if [ $i = makefile.all ]
  then
    continue
  fi
  sed -e '/<<<makefile.all/r makefile.all' \
      -e '/<<<makefile.all/,/>>>makefile.all/d' $i > .tmp
  if cmp -s $i .tmp
  then
    echo skipped $i
    rm .tmp
  else
   echo updated $i
   mv $i $i.dist
   mv .tmp $i
  fi
done
