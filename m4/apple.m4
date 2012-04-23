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
     is_apple=yes
     AC_ARG_WITH(aquaterm,
     [  --with-aquaterm         include support for aquaterm on OSX],
 	[if test "$withval" == yes; then
	   AC_CHECK_LIB(aquaterm, aqtInit, 
	   [ LIBS="-laquaterm $LIBS -framework Foundation"
	     CFLAGS="$CFLAGS -ObjC"
	     AC_DEFINE(HAVE_LIBAQUATERM,1,
		     [Define to 1 if you're using the aquaterm library on Mac OS X])
	   ],[], -lobjc)
      fi])
   ],
   AC_MSG_RESULT(no)
   is_apple=no)
])

