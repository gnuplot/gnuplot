AC_DEFUN(AC_C_STRINGIZE, [
AC_REQUIRE([AC_PROG_CPP])
AC_MSG_CHECKING([for preprocessor stringizing operator])
AC_CACHE_VAL(ac_cv_c_stringize,
AC_EGREP_CPP([#teststring],[
#define x(y) #y

char *s = x(teststring);
], ac_cv_c_stringize=no, ac_cv_c_stringize=yes))
if test "${ac_cv_c_stringize}" = yes
then
        AC_DEFINE(HAVE_STRINGIZE)
fi
AC_MSG_RESULT([${ac_cv_c_stringize}])
])dnl

