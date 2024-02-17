# Name of the binary
bin_PROGRAMS = 42sh

# Source files for the binary
42sh_SOURCES = main.c

# Analog to the CFLAGS and CPPFLAGS in a classic Makefile
42sh_CFLAGS = -std=c99 -pedantic -Werror -Wall -Wextra -Wvla
42sh_CPPFLAGS = -I$(top_srcdir)

# Binaries or libraries needed by the binary 42sh
42sh_LDADD = \
	ast/libast.a \
	backend/libbackend.a \
	lexer/liblexer.a \
	parser/libparser.a \
	executor/libexecutor.a \
	builtin/libbuiltin.a \
	vector/libvector.a \
	functions/libfunctions.a \
	variable/libvariable.a \
	linked_list/liblist.a \
	hashmap/libhashmap.a

SUBDIRS = \
	  hashmap \
	  linked_list \
	  vector \
	  variable \
	  builtin \
	  ast \
	  functions \
	  backend \
	  lexer \
	  parser\
	  executor \
	  variable


subdebug:
	make debug -C hashmap
	make debug -C linked_list
	make debug -C variable
	make debug -C ast
	make debug -C functions
	make debug -C backend
	make debug -C lexer
	make debug -C parser
	make debug -C executor
	make debug -C vector
	make debug -C builtin

debug:
	make subdebug
	make 42sh_CFLAGS="-std=c99 -pedantic -Werror -Wall -Wextra -Wvla -O0 -g -fsanitize=address"

subcover:
	make cover -C hashmap
	make cover -C linked_list
	make cover -C variable
	make cover -C ast
	make cover -C functions
	make cover -C backend
	make cover -C lexer
	make cover -C parser
	make cover -C executor
	make cover -C vector
	make cover -C builtin

cover:
	make subcover
	make 42sh_CFLAGS="-std=c99 -pedantic -Werror -Wall -Wextra -Wvla --coverage -fPIC -g -lgcov"
