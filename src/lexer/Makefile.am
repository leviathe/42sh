# Name of the binary
lib_LIBRARIES = liblexer.a

# Source files for the binary
liblexer_a_SOURCES = \
	data_free.c \
	data_free.h \
	lexer.c \
	lexer.h \
	token.h

# Analog to the CFLAGS and CPPFLAGS in a classic Makefile
liblexer_a_CFLAGS = -std=c99 -pedantic -Werror -Wall -Wextra -Wvla
liblexer_a_CPPFLAGS = -I$(top_srcdir)/src

noinst_LIBRARIES = liblexer.a

# Binaries or libraries needed by the binary 42sh

liblexer_a_LIBADD = ../linked_list/liblist.a ../vector/libvector.a

debug:
	make debug -C ../vector
	make debug -C ../linked_list
	make liblexer_a_CFLAGS="-std=c99 -pedantic -Werror -Wall -Wextra -Wvla -g -fsanitize=address -O0"
cover:
	make cover -C ../vector
	make cover -C ../linked_list
	make liblexer_a_CFLAGS="-std=c99 -pedantic -Werror -Wall -Wextra -Wvla --coverage -fPIC -g"
