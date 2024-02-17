# Name of the binary
lib_LIBRARIES = libexecutor.a

# Source files for the binary
libexecutor_a_SOURCES = \
	executor.c \
	executor.h

# Analog to the CFLAGS and CPPFLAGS in a classic Makefile
libexecutor_a_CFLAGS = -std=c99 -pedantic -Werror -Wall -Wextra -Wvla
libexecutor_a_CPPFLAGS = -I$(top_srcdir)/src

noinst_LIBRARIES = libexecutor.a

# Binaries or libraries needed by the binary 42sh
libexecutor_a_LIBADD = \
	../functions/libfunctions.a
	../parser/libparser.a

debug:
	make libexecutor_a_CFLAGS="-std=c99 -pedantic -Werror -Wall -Wextra -Wvla -g -fsanitize=address -O0"
cover:
	make libexecutor_a_CFLAGS="-std=c99 -pedantic -Werror -Wall -Wextra -Wvla --coverage -fPIC -g"
