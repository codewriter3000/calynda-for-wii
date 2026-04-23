/*
 * CodeWarrior operator overload table (requirement 5.5).
 */
#include "cli/demangle.h"
#include <string.h>

static const struct { const char *mn; const char *glyph; } kOps[] = {
    { "pl", "+" },   { "mi", "-" },   { "ml", "*" },   { "dv", "/" },
    { "md", "%" },   { "as", "=" },   { "eq", "==" },  { "ne", "!=" },
    { "lt", "<" },   { "gt", ">" },   { "le", "<=" },  { "ge", ">=" },
    { "ls", "<<" },  { "rs", ">>" },  { "or", "|" },   { "an", "&" },
    { "er", "^" },   { "aa", "&&" },  { "oo", "||" },  { "nt", "!" },
    { "co", "~" },   { "pp", "++" },  { "mm", "--" },  { "cl", "()" },
    { "vc", "[]" },  { "rf", "->" },  { "rm", "->*" }, { "nw", " new" },
    { "dl", " delete" }, { "nwa", " new[]" }, { "dla", " delete[]" },
    { "apl", "+=" }, { "ami", "-=" }, { "aml", "*=" }, { "adv", "/=" },
    { "amd", "%=" }, { "als", "<<=" },{ "ars", ">>=" },{ "aor", "|=" },
    { "aan", "&=" }, { "aer", "^=" }, { "ad", "&" },   { "cm", "," },
};

const char *cw_operator(const char *mnemonic) {
    for (size_t i = 0; i < sizeof kOps / sizeof kOps[0]; i++)
        if (strcmp(kOps[i].mn, mnemonic) == 0) return kOps[i].glyph;
    return NULL;
}
