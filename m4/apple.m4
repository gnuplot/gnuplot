## ------------------------------- ##
## Check for Apple MacOS X         ##
## From Leigh Smith                ##
## ------------------------------- ##

# serial 1

AC_DEFUN(GP_APPLE,
[AC_MSG_CHECKING(for Apple MacOS X)
AC_EGREP_CPP(yes,
[#if defined(__APPLE__) && defined(__MACH__)
  yes
#endif
], AC_MSG_RESULT(yes)
   LIBS="$LIBS -framework Foundation -framework AppKit"
   CFLAGS="$CFLAGS -ObjC",
   AC_MSG_RESULT(no))
])

