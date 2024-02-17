# Name of the binary
lib_LIBRARIES = libbuiltin.a

# Source files for the binary
libbuiltin_a_SOURCES = \
	cd.c \
	cd.h \
	echo.c \
	echo.h \
	unset.c \
	unset.h \
	dot.c \
	dot.h \
	export.c \
	export.h \
	alias.c \
	alias.h

# Analog to the CFLAGS and CPPFLAGS in a classic Makefile
libbuiltin_a_CFLAGS = -std=c99 -pedantic -Werror -Wall -Wextra -Wvla
libbuiltin_a_CPPFLAGS = -I$(top_srcdir)/src

noinst_LIBRARIES = libbuiltin.a

# Binaries or libraries needed by the binary 42sh
libbuiltin_a_LIBADD = \
	../variable/libvariable.a \
	../hashmap/libhashmap.a

debug:
	make libbuiltin_a_CFLAGS="-std=c99 -pedantic -Werror -Wall -Wextra -Wvla -g -fsanitize=address -O0"
cover:
	make libbuiltin_a_CFLAGS="-std=c99 -pedantic -Werror -Wall -Wextra -Wvla --coverage -fPIC -g"
