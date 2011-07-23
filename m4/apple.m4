## ------------------------------- ##
## Check for Apple Mac OS X        ##
## From Leigh Smith                ##
## ------------------------------- ##

# serial 1

AC_DEFUN([GP_APPLE],
[AC_MSG_CHECKING(for Apple MacOS X)
AC_EGREP_CPP(yes,
[#if defined(__APPLE__) && defined(__MACH__)
  yes
#endif
], 
   [ AC_MSG_RESULT(yes)
     AC_CHECK_LIB(aquaterm, aqtInit, 
     [ LIBS="-Wl,-framework -Wl,AquaTerm $LIBS -framework Foundation"
       CFLAGS="$CFLAGS -ObjC"
       AC_DEFINE(HAVE_LIBAQUATERM,1,
                 [Define to 1 if you're using the aquaterm library on Mac OS X])
     ],[], -lobjc)
     is_apple=yes
   ],
   AC_MSG_RESULT(no)
   is_apple=no)
])

