## ------------------------------- ##
## Check for NeXT.                 ##
## From Lars Hecking               ##
## ------------------------------- ##

# serial 1

AC_DEFUN(gp_NEXT,
[AC_MSG_CHECKING(for NeXT)
AC_EGREP_CPP(yes,
[#if __NeXT__
  yes
#endif
], AC_MSG_RESULT(yes)
   LIBS="$LIBS -lsys_s -lNeXT_s"
   NEXTOBJS=epsviewe.o
   CFLAGS="$CFLAGS -ObjC",dnl
   NEXTOBJS=
   AC_MSG_RESULT(no))
])

