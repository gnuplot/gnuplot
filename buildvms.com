$ ! buildvms.com  (Command file to compile/link gnuplot and doc2hlp)
$ set noon
$!
$! detect compiler - drd
$! if DECC is around, assume that, else gcc is preferred. Finally vaxc
$ its_decc = (f$search("SYS$SYSTEM:DECC$COMPILER.EXE") .nes. "")
$ its_gnuc = .not.its_decc .and. (f$trnlnm("gnu_cc").nes."")
$ its_vaxc = .not. (its_decc .or. its_gnuc)
$ its_decw = (f$trnlnm("DECW$INCLUDE") .nes. "") 
$
$! configure
$
$ if its_decc
$ then pfix = "/prefix=all"
$      rtl  = "DECCRTL"
$ else pfix = ""
$      rtl  = "VAXCRTL"
$ endif
$
$ if its_decw
$ then CFLAGS    = "/NOWARN/NOOP/DEFINE=(X11,NOGAMMA,NO_GIH,PIPES,''rtl')''pfix'"
$ else CFLAGS    = "/NOWARN/NOOP/DEFINE=(NOGAMMA,NO_GIH,PIPES,''rtl')''pfix'"
$ endif
$!
$ TERMFLAGS = "/INCLUDE=[.term]"
$
$ if its_gnuc
$ then cc := GCC/NOCASE
$      EXTRALIB = ",GNU_CC:[000000]GCCLIB/LIB"
$ else EXTRALIB = ""
$ endif
$
$ if its_decc
$ then CFLAGS="''cflags'/prefix=all"
$      LINKOPT=""
$ else LINKOPT=",sys$disk:[]linkvms.opt/opt"
$ endif
$
$ if its_decw
$ then DEFINE/NOLOG X11 DECW$INCLUDE
$      DEFINE SYS SYS$LIBRARY
$ endif
$
$ set verify
$ cc 'CFLAGS' alloc.c
$ cc 'CFLAGS' binary.c
$ cc 'CFLAGS' bitmap.c
$ cc 'CFLAGS' command.c
$ cc 'CFLAGS' contour.c
$ cc 'CFLAGS' datafile.c
$ cc 'CFLAGS' eval.c
$ cc 'CFLAGS' fit.c
$ cc 'CFLAGS' graphics.c
$ cc 'CFLAGS' graph3d.c
$ cc 'CFLAGS' hidden3d.c
$ cc 'CFLAGS' internal.c
$ cc 'CFLAGS' interpol.c
$ cc 'CFLAGS' matrix.c
$ cc 'cflags' misc.c
$ cc 'CFLAGS' parse.c
$ cc 'CFLAGS' plot.c
$ cc 'CFLAGS' plot2d.c
$ cc 'CFLAGS' plot3d.c
$ cc 'CFLAGS' scanner.c
$ cc 'CFLAGS' set.c
$ cc 'CFLAGS' show.c
$ cc 'CFLAGS' specfun.c
$ cc 'CFLAGS' standard.c
$ cc 'cflags' 'TERMFLAGS' term.c
$ cc 'cflags' time.c
$ cc 'CFLAGS' util.c
$ cc 'CFLAGS' util3d.c
$ cc 'CFLAGS' version.c
$ cc 'CFLAGS' vms.c
$ link/exe=gnuplot -
bitmap,command,contour,eval,graphics,graph3d,vms,-
binary,specfun,interpol,fit,matrix,internal,misc,parse,-
plot,plot2d,plot3d,scanner,set,show,datafile,alloc,-
standard,term,util,version,util3d,hidden3d,time'extralib''LINKOPT'
$ cc 'CFLAGS' bf_test.c
$ link /exe=bf_test bf_test,binary,alloc 'extralib''LINKOPT'
$ ren bf_test.exe [.demo]
$ CC 'CFLAGS' GPLT_X11
$ LINK /exe=GNUPLOT_X11 gplt_x11 'extralib''LINKOPT',SYS$INPUT:/OPT
SYS$SHARE:DECW$XLIBSHR/SHARE
$!
$ SET DEF [.DOCS]
$ cc/OBJ=[-]doc2hlp/include=([-],[-.term]) doc2hlp.c
$ SET DEF [-]
$ link doc2hlp 'extralib' 'LINKOPT'
$ doc2hlp := $sys$disk:[]doc2hlp
$ doc2hlp [.docs]gnuplot.doc gnuplot.hlp
$ library/create/help sys$disk:[]gnuplot.hlb gnuplot.hlp
$ run [.demo]bf_test
$ write sys$output "%define GNUPLOT_X11 :== $Disk:[directory]GNUPLOT_X11"
$ set noverify
$ deassign x11
$ exit
