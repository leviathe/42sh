#ifndef ALIAS_H
#define ALIAS_H

char *get_alias(char *name);
/*
**
*/
int alias(char **args);
int unalias(char **args);
void free_aliases(void);

#endif /* !ALIAS_H */
