#! /bin/sh
#
# $Id: errorfix.sh,v 1.3 1997/03/09 23:49:49 drd Exp $
#
# shell script to change #error and #warn cpp statements. This is necessary
# for the crippled non-ANSI compiler that HP ships with it's standard
# distribution, at least in <=9.0 for m68k
#
# this needs to be run once in gnuplot directory

if [ $# = 0 ]
then
  dir=.
else
  dir=$1
fi

if [ $dir = . ]
then
  mkdirs=false
  backup=true
else
  mkdirs=true
  backup=false
fi

for i in `cd $dir;find . \( -name "*.c" -o -name "*.h" -o -name "*.trm" \) -print`
do
  sed -e 's/^#\([ 	]*error\)/\1/' \
      -e 's@^\(#[ 	]*warning.*\)$@/* \1 */@' $dir/$i >.tmp
  if cmp -s $dir/$i .tmp
  then
    rm .tmp
  else
    if $mkdirs
    then
      mkdirs `echo $i|sed 's@/[^/]*$@@'`
    fi
    if $backup && [ ! -r $dir/$i.dist ]
    then
      mv $dir/$i $dir/$i.dist
    fi
    if cmp -s .tmp $i
    then
      rm .tmp
    else
      mv .tmp $i
      echo fixed $i
    fi
  fi
done
