#! /bin/sh
#
# $Id: errorfix.sh,v 1.4 1998/04/14 00:15:18 drd Exp $
#
# shell script to change #error and #warn cpp statements. This is necessary
# for the crippled non-ANSI compiler that HP ships with it's standard
# distribution, at least in <=9.0 for m68k
#
# this needs to be run once in gnuplot directory

dir="$1" && test ${dir} || dir=.

if [ "$dir" = . ] ; then
  mkdirs=false
  backup=true
else
  mkdirs=true
  backup=false
fi

for i in `cd $dir && find . \( -name "*.c" -o -name "*.h" -o -name "*.trm" \) -print` ; do
  grep "^#[ 	]*[ew][ar]" ${dir}/${i} >/dev/null 2>&1
  if [ $? -eq 0 ]; then
    # found #error or #warning
    sed -e 's%^#\([ 	]*error\)%\1%' \
        -e 's%^\(#[ 	]*warning.*\)$%/* \1 */%' $dir/$i >.tmp
    if $mkdirs ; then
      dirnew=`echo $i | sed -n 's%^\./\([^/]*\)/.*$%\1%p'`
      if [ x"$dirnew" != x ]; then
        mkdir ${dirnew}
      fi
    fi
    if $backup && [ ! -r $dir/$i.dist ]; then
      mv $dir/$i $dir/$i.dist
    fi
    suffix=`echo $i | awk -F\. '{print $NF}'`
    if [ $suffix = h ]; then
      test -r $dir/$i && mv $dir/$i $dir/$i.dist
      mv .tmp $dir/$i
    else
      mv .tmp $i
    fi
    echo fixed $i
  fi
done
