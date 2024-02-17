# Name of the binary
lib_LIBRARIES = libvector.a

# Source files for the binary
libvector_a_SOURCES = \
	vector_create.c \
	vector_access.c \
	vector.h

# Analog to the CFLAGS and CPPFLAGS in a classic Makefile
libvector_a_CFLAGS = -std=c99 -pedantic -Werror -Wall -Wextra -Wvla
libvector_a_CPPFLAGS = -I$(top_srcdir)/src

noinst_LIBRARIES = libvector.a

# Rule for testing the coverage
debug:
	make libvector_a_CFLAGS="-std=c99 -pedantic -Werror -Wall -Wextra -Wvla -g -fsanitize=address -O0"
cover:
	make libvector_a_CFLAGS="-std=c99 -pedantic -Werror -Wall -Wextra -Wvla --coverage -fPIC -g"
