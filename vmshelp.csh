#! /bin/csh
#
# vmshelp.csh /usr/help/gnuplot/* > gnuplot.hlp
# will convert the Unix help tree to VMS format,
# then use $ LIB/HELP GNUPLOT GNUPLOT under VMS to create the VMS .HLB

if (! $?level) then
	setenv level 0
endif
@ leveltmp = ($level + 1)
setenv level $leveltmp

foreach i ($*)
	if (-f $i) then
# plain file
		echo -n "$level "
		basename $i .hlp
		sed 's/^/ /' $i
	else if (-d $i) then
# directory
		echo -n "$level "
		basename $i
		sed 's/^/ /' $i/.hlp
# recurse!
		$0 $i/*
	endif
end
