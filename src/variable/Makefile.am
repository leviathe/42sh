# Name of the binary
lib_LIBRARIES = libvariable.a

# Source files for the binary
libvariable_a_SOURCES = \
	variable.c \
	variable.h \
	funargs.c \
	funargs.h \
	command_expansion.c \
	command_expansion.h

# Analog to the CFLAGS and CPPFLAGS in a classic Makefile
libvariable_a_CFLAGS = -std=c99 -pedantic -Werror -Wall -Wextra -Wvla
libvariable_a_CPPFLAGS = -I$(top_srcdir)/src

noinst_LIBRARIES = libvariable.a

# Binaries or libraries needed by the binary 42sh
debug:
	make libvariable_a_CFLAGS="-std=c99 -pedantic -Werror -Wall -Wextra -Wvla -g -fsanitize=address -O0"
cover:
	make libvariable_a_CFLAGS="-std=c99 -pedantic -Werror -Wall -Wextra -Wvla --coverage -fPIC -g"
