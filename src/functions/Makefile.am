# Name of the binary
lib_LIBRARIES = libfunctions.a

# Source files for the binary
libfunctions_a_SOURCES = \
	functions.c \
	functions.h

# Analog to the CFLAGS and CPPFLAGS in a classic Makefile
libfunctions_a_CFLAGS = -std=c99 -pedantic -Werror -Wall -Wextra -Wvla
libfunctions_a_CPPFLAGS = -I$(top_srcdir)/src

noinst_LIBRARIES = libfunctions.a

# Binaries or libraries needed by the binary 42sh
libfunctions_a_LIBADD = \
	../ast/libast.a

debug:
	make libfunctions_a_CFLAGS="-std=c99 -pedantic -Werror -Wall -Wextra -Wvla -g -fsanitize=address -O0"
cover:
	make -C ../ast
	make libfunctions_a_CFLAGS="-std=c99 -pedantic -Werror -Wall -Wextra -Wvla --coverage -fPIC -g"
