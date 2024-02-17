# Name of the binary
lib_LIBRARIES = libbackend.a

# Source files for the binary
libbackend_a_SOURCES = \
	backend.c \
	backend.h

# Analog to the CFLAGS and CPPFLAGS in a classic Makefile
libbackend_a_CFLAGS = -std=c99 -pedantic -Werror -Wall -Wextra -Wvla
libbackend_a_CPPFLAGS = -I$(top_srcdir)/src

noinst_LIBRARIES = libbackend.a

debug:
	make libbackend_a_CFLAGS="-std=c99 -pedantic -Werror -Wall -Wextra -Wvla -g -fsanitize=address -O0"

cover:
	make libbackend_a_CFLAGS="-std=c99 -pedantic -Werror -Wall -Wextra -Wvla --coverage -fPIC -g"
