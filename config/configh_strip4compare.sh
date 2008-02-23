#!/bin/bash
# $Id: $
#
# Description:
# This script is a part of gnuplot. From config.{?|??|???} files it
# creates new files config.xxx.new with stripped definitions so they can
# be compared easily via diff, kdiff3, kompare etc.
#
# Use:
# You should copy ./configure'd config.h and an investigated config/config.???
# into an empty directory and run this script there. Then, compare the
# *.new files with config.h.new, and synchronize the differences.

AWKPROG='
# In #define XXX, #undef XXX, /* #define XXX ... */ and /* #undef XXX */
# lines of gnuplot config.aaa, replace them by  #def XXX.
# That makes it easy to compare two different config files, to check for
# potential inconsistencies.

$0 ~ "Don@t change it here" { next }
$0 == "#endif" { next }
$1 == "#if" { next }

$0 ~/def/ {
    if ($1=="/*") $2 = $3
    if ($1=="#")  $2 = $3
    print "#def " $2
    next
}

{ print }
'

# replace @ by '
AWKPROG=${AWKPROG/@/"'"}

for a in config.? config.?? config.??? ; do
    if [ -f $a ] ; then
	echo "Processing $a"
	cp $a $a.orig_
	# awk -f config_strip.awk $a >$a.new
	awk "$AWKPROG" $a >$a.new
    fi
done

# eof
