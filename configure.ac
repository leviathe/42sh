# Initialize the project with the package's name, the version number and the
# bug-report address
AC_INIT([42sh], [1.0], [email])
# Setup automake
AM_INIT_AUTOMAKE([subdir-objects] [foreign])

# Check if a C compiler is available
AC_PROG_CC
# Check if a compiler have this C flags
AX_COMPILER_FLAGS([], [], [], [-std=c99 -pedantic -Werror -Wall -Wextra -Wvla])

# Enable 'ar' for Makefile
AM_PROG_AR
# Check if a ranlib is available
AC_PROG_RANLIB

# Inform the configurations files (generally Makefile)
AC_CONFIG_FILES([
                 Makefile
                 tests/Makefile
                 src/Makefile
                 src/ast/Makefile
                 src/functions/Makefile
                 src/backend/Makefile
                 src/builtin/Makefile
                 src/executor/Makefile
                 src/lexer/Makefile
                 src/linked_list/Makefile
                 src/parser/Makefile
                 src/variable/Makefile
                 src/vector/Makefile
                 src/hashmap/Makefile
                 ])
# Generate config.status and launch it
AC_OUTPUT
