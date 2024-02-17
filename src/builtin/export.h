#ifndef EXPORT_H
#define EXPORT_H

struct str_dlist
{
    char *data;
    struct str_dlist *next;
    struct str_dlist *prev;
};

int remove_to_be_exported(char *s); // Returns if deletion was successful
void destroy_to_be_exported(void);
int exportv(char *arg);

#endif /* !EXPORT_H */
