# Name of the binary
lib_LIBRARIES = liblist.a

# Source files for the binary
liblist_a_SOURCES = \
	list.c \
	list.h

# Analog to the CFLAGS and CPPFLAGS in a classic Makefile
liblist_a_CFLAGS = -std=c99 -pedantic -Werror -Wall -Wextra -Wvla
liblist_a_CPPFLAGS = -I$(top_srcdir)/src

noinst_LIBRARIES = liblist.a

debug:
	make liblist_a_CFLAGS="-std=c99 -pedantic -Werror -Wall -Wextra -Wvla -g -fsanitize=address -O0"
cover:
	make liblist_a_CFLAGS="-std=c99 -pedantic -Werror -Wall -Wextra -Wvla --coverage -fPIC -g"
