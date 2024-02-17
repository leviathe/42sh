# Name of the binary
lib_LIBRARIES = libhashmap.a

# Source files for the binary
libhashmap_a_SOURCES = \
	htab.c \
	htab.h

# Analog to the CFLAGS and CPPFLAGS in a classic Makefile
libhashmap_a_CFLAGS = -std=c99 -pedantic -Werror -Wall -Wextra -Wvla
libhashmap_a_CPPFLAGS = -I$(top_srcdir)/src

noinst_LIBRARIES = libhashmap.a

# Rule for testing the coverage
debug:
	make libvector_a_CFLAGS="-std=c99 -pedantic -Werror -Wall -Wextra -Wvla -g -fsanitize=address -O0"
cover:
	make libvector_a_CFLAGS="-std=c99 -pedantic -Werror -Wall -Wextra -Wvla --coverage -fPIC -g"
