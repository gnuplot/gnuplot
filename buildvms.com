$ ! buildvms.com  (Command file to compile/link gnuplot and doc2hlp)
$ CFLAGS = "/NOOP/define=(NOGAMMA,MEMSET)"
$ !TERMFLAGS = "/define=()"
$ TERMFLAGS = ""
$ set verify
$ cc 'CFLAGS' bitmap.c
$ cc 'CFLAGS' command.c
$ cc 'CFLAGS' contour.c
$ cc 'CFLAGS' eval.c
$ cc 'CFLAGS' graphics.c
$ cc 'CFLAGS' graph3d.c
$ cc 'CFLAGS' internal.c
$ cc 'CFLAGS' misc.c
$ cc 'CFLAGS' parse.c
$ cc 'CFLAGS' plot.c
$ cc 'CFLAGS' scanner.c
$ cc 'CFLAGS' setshow.c
$ cc 'CFLAGS' standard.c
$ cc 'CFLAGS' 'TERMFLAGS' term.c
$ cc 'CFLAGS' util.c
$ cc 'CFLAGS' version.c
$ link /exe=gnuplot -
   bitmap.obj,command.obj,contour.obj,eval.obj,graphics.obj,graph3d.obj, -
   internal.obj,misc.obj,parse.obj,plot.obj,scanner.obj,setshow.obj, -
   standard.obj,term.obj,util.obj,version.obj ,linkopt.vms/opt
$ cc [.docs]doc2hlp.c
$ link doc2hlp,linkopt.vms/opt
$ @[.docs]doc2hlp.com
$ library/create/help gnuplot.hlb gnuplot.hlp
$ set noverify
