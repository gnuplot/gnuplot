$! make sure you SET TERM/NOWRAP before running GNUPLOT!
$ CFLAGS = "NOTHING"
$ TERMFLAGS = "AED,HP26,HP75,POSTSCRIPT,QMS,REGIS,TEK,V384,HPLJET"
$ set verify
$ cc/define=('CFLAGS') command.c
$ cc/define=('CFLAGS') eval.c
$ cc/define=('CFLAGS') graphics.c
$ cc/define=('CFLAGS') internal.c
$ cc/define=('CFLAGS') misc.c
$ cc/define=('CFLAGS') parse.c
$ cc/define=('CFLAGS') plot.c
$ cc/define=('CFLAGS') scanner.c
$ cc/define=('CFLAGS') standard.c
$ cc/define=('CFLAGS','TERMFLAGS') term.c
$ cc/define=('CFLAGS') util.c
$ cc/define=('CFLAGS') version.c
$ set noverify
