# Process this file with autoconf to produce a configure script.
AC_INIT(NachOS-SHELL, 0.0, ceelen@ira.uka.de)
AC_CONFIG_SRCDIR([shell.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_CHECK_TOOL([STRIP],[strip])
AC_CHECK_TOOL([LD],[ld])

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lc':
AC_CHECK_LIB([c], [main])
# FIXME: Replace `main' with a function in `-lpthread':
AC_CHECK_LIB([pthread], [main])

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/file.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
