## ------------------------------- ##
## Check for BeOS.                 ##
## From Lars Hecking               ##
## From Xavier Pianet              ##
## ------------------------------- ##

# serial 1

AC_DEFUN(gp_BEOS,
[AC_MSG_CHECKING(for BeOS)
AC_EGREP_CPP(yes,
[#if __BEOS__
  yes
#endif
], AC_MSG_RESULT(yes)
   GNUPLOT_BE=gnuplot_be,dnl
   GNUPLOT_BE=
   AC_MSG_RESULT(no))
])

