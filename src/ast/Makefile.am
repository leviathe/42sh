# Name of the binary
lib_LIBRARIES = libast.a

# Source files for the binary
libast_a_SOURCES = \
	ast.c \
	ast.h \
	arguments.c \
	arguments.h

# Analog to the CFLAGS and CPPFLAGS in a classic Makefile
libast_a_CFLAGS = -std=c99 -pedantic -Werror -Wall -Wextra -Wvla
libast_a_CPPFLAGS = -I$(top_srcdir)/src

noinst_LIBRARIES = libast.a

debug:
	make libast_a_CFLAGS="-std=c99 -pedantic -Werror -Wall -Wextra -Wvla -g -fsanitize=address -O0"
cover:
	make libast_a_CFLAGS="-std=c99 -pedantic -Werror -Wall -Wextra -Wvla --coverage -fPIC -g"
