## ------------------------------- ##
## Check for MS-DOS/djgpp.         ##
## From Lars Hecking and           ##
## Hans-Bernhard Broeker           ##
## ------------------------------- ##

# serial 1

AC_DEFUN(gp_MSDOS,
[AC_MSG_CHECKING(for MS-DOS/djgpp/libGRX)
AC_EGREP_CPP(yes,
[#if __DJGPP__ && __DJGPP__ == 2
  yes
#endif
], AC_MSG_RESULT(yes)
   LIBS="-lpc $LIBS"
   AC_DEFINE(MSDOS)
   AC_DEFINE(DOS32)
   with_linux_vga=no
   AC_CHECK_LIB(grx20,GrLine,dnl
      LIBS="-lgrx20 $LIBS"
      CFLAGS="$CFLAGS -fno-inline-functions"
      AC_DEFINE(DJSVGA)
      AC_CHECK_LIB(grx20,GrCustomLine,AC_DEFINE(GRX21))),dnl
   AC_MSG_RESULT(no)
   )dnl 
])

