## ------------------------------- ##
## Check for Apple MacOs X         ##
## From Leigh Smith                ##
## ------------------------------- ##

# serial 1

AC_DEFUN(gp_APPLE,
[AC_MSG_CHECKING(for Apple MacOs X)
AC_EGREP_CPP(yes,
[#if defined(__APPLE__) && defined(__MACH__)
  yes
#endif
], AC_MSG_RESULT(yes)
   LIBS="$LIBS -framework Foundation -framework AppKit"
   CFLAGS="$CFLAGS -ObjC",dnl
   LIBS="$LIBS -lm"
   AC_MSG_RESULT(no))
])

