$ def/user sys$input [.docs]gnuplot.doc
$ def/user sys$output []gnuplot.hlp
$ run doc2hlp
