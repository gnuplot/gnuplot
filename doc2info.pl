#!/usr/local/bin/perl
#
# doc2texinfo : Converts Gnuplot .doc files to Texinfo format.
#
# George Ferguson, ferguson@cs.rochester.edu, 11 Feb 1991.
#
# Usage:
#	% doc2texinfo gnuplot.doc >gnuplot.texinfo
#	% makeinfo +fill-column 80 gnuplot.texinfo
# Creates files "gnuplot.info" and "gnuplot.info-[123]".
#

$currentBook = $currentChapter = $currentSection = $currentSubsection = 0;
$verbatim = $table = 0;

while (<>) {
    if (/^([1-4]) (.*)$/) {	# section heading
	$table = 0;
	&endVerbatim;
	$numNodes += 1;
	$nodeLevel[$numNodes] = $1;
	$nodeTitle[$numNodes] = $2;
	$nodeText[$numNodes] = "";
	if ($1 == 1) {
	    $currentBook = $numNodes;
	} elsif ($1 == 2) {
	    $currentChapter = $numNodes;
	    $nodeMenu[$currentBook] .= "* $2::\n";
	} elsif ($1 == 3) {
	    $currentSection = $numNodes;
	    if ($nodeTitle[$currentChapter] eq "set-show") {
		$nodeTitle[$numNodes] = "set $2";	# override
		$nodeMenu[$currentChapter] .= "* set $2::\n";
	    } else {
		$nodeMenu[$currentChapter] .= "* $2::\n";
	    }
	} elsif ($1 == 4) {
	    $currentSubsection = $numNodes;
	    $nodeMenu[$currentSection] .= "* $2::\n";
	}
    } elsif (/^\?(.*)$/) {				# index entry
	if ($1 ne "") {
	    $nodeText[$numNodes] .= "\@cindex $1\n";
	}
    } elsif (/^@start table/) {				# start table
	&startVerbatim;
	$table = 1;
    } elsif (/^@end table/) {				# end table
	$table = 0;
	&endVerbatim;
    } elsif (/^#/ || /^%/) {				# table entry
	next;
    } elsif (/^ ( ?)(.*)$/) {				# text
	if ($1 eq " ") {
	    &startVerbatim;
	} else {
	    &endVerbatim;
	}
	$text = $2;
	$text =~ s/@/@@/g;
	$text =~ s/{/\@{/g;
	$text =~ s/}/\@}/g;
 	$text =~ s/\`([^`]*)\`/@code{$1}/g;
	$nodeText[$numNodes] .= "$text\n";
    } elsif (/^$/) {					# blank line
	&endVerbatim;
	$nodeText[$numNodes] .= "\n";
    }
}

# Print texinfo header
print "\\input texinfo\n";
print "@setfilename gnuplot.info\n";
print "@settitle Gnuplot: An Interactive Plotting Program\n";
print "\n";
print "@node Top\n";
print "@top Gnuplot";
print "\n";
print "$nodeText[1]";
print "\@menu\n";
print "$nodeMenu[1]";
print "* General Index::\n";
print "\@end menu\n";
print "\n";

# Now output all the nodes
for ($i=2; $i <= $numNodes; $i++) {
    print "\@node $nodeTitle[$i]\n";
    if ($nodeLevel[$i] == 2) {
	print "\@chapter $nodeTitle[$i]\n";
    } elsif ($nodeLevel[$i] == 3) {
	print "\@section $nodeTitle[$i]\n";
    } elsif ($nodeLevel[$i] == 4) {
	print "\@subsection $nodeTitle[$i]\n";
    }
    print "$nodeText[$i]";
    if ($nodeMenu[$i] ne "") {
	print "\@menu\n";
	print $nodeMenu[$i];
	print "\@end menu\n";
    }
    print "\n";
}
# Print texinfo trailer
print "\n";
print "@node General Index\n";
print "@appendix General Index\n";
print "\n";
print "@printindex cp\n";
print "\n";
print "@bye\n";

#######################

sub startVerbatim {
    if (!$verbatim) {
	$nodeText[$numNodes] .= "\@example\n";
	$verbatim = 1;
    }
}

sub endVerbatim {
    if ($verbatim && !$table) {
	$nodeText[$numNodes] .= "\@end example\n";
	$verbatim = 0;
    }
}
