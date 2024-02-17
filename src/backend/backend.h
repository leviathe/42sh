#ifndef BACKEND_H
#define BACKEND_H

#include <stdio.h>

FILE *get_stream(int argc, char **argv);
void close_stream(FILE *stream);

#endif /* !BACKEND_H */
