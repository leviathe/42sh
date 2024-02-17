# Name of the binary
lib_LIBRARIES = libparser.a

# Source files for the binary
libparser_a_SOURCES = \
	parser.c \
	parser.h

# Analog to the CFLAGS and CPPFLAGS in a classic Makefile
libparser_a_CFLAGS = -std=c99 -pedantic -Werror -Wall -Wextra -Wvla
libparser_a_CPPFLAGS = -I$(top_srcdir)/src

noinst_LIBRARIES = libparser.a

# Binaries or libraries needed by the binary 42sh
libparser_a_LIBADD = \
	../lexer/liblexer.a

debug:
	make libparser_a_CFLAGS="-std=c99 -pedantic -Werror -Wall -Wextra -Wvla -g -fsanitize=address -O0"
cover:
	make libparser_a_CFLAGS="-std=c99 -pedantic -Werror -Wall -Wextra -Wvla --coverage -fPIC -g"
