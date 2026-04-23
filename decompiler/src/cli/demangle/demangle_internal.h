#ifndef CLI_DEMANGLE_INTERNAL_H
#define CLI_DEMANGLE_INTERNAL_H
#include <stddef.h>
int dmg_parse_type(const char **pp, char *out, size_t n);
int dmg_parse_name(const char **pp, char *out, size_t n);
#endif
