/*
 * SDK module classification (requirement 6.2 / 6.6).
 *
 * Each entry is a prefix that identifies an RVL SDK module.  The
 * first match wins, so order the table from most-specific to
 * least-specific.
 */
#include "cli/externs.h"
#include <string.h>

typedef struct { const char *prefix; const char *module; } SdkRule;

/* Prefix-based SDK module table.  Mostly matches the Revolution SDK
 * library layout used by CodeWarrior / ogws decomp projects. */
static const SdkRule kRules[] = {
    /* Runtime library (MetroWerks standard lib) — specific `__OS*` /
     * `__DVD*` symbols are handled first so they don't get miscast. */
    { "__",       "msl" },
    { "std::",    "msl" },
    { "_savegpr", "msl" },
    { "_restgpr", "msl" },
    { "_savefpr", "msl" },
    { "_restfpr", "msl" },
    { "memcpy",   "msl" },
    { "memset",   "msl" },
    { "strlen",   "msl" },
    { "strcpy",   "msl" },
    { "strcmp",   "msl" },
    { "strncpy",  "msl" },
    { "strncmp",  "msl" },
    { "strcat",   "msl" },
    { "sprintf",  "msl" },
    { "snprintf", "msl" },
    { "printf",   "msl" },
    { "malloc",   "msl" },
    { "free",     "msl" },
    { "qsort",    "msl" },
    { "abs",      "msl" },
    /* RVL SDK per-library prefixes. */
    { "OS",       "os" },
    { "__OS",     "os" },
    { "DB",       "os" },
    { "GX",       "gx" },
    { "PAD",      "pad" },
    { "KPAD",     "kpad" },
    { "WPAD",     "wpad" },
    { "WENC",     "wenc" },
    { "VI",       "vi" },
    { "AI",       "ai" },
    { "AX",       "ax" },
    { "MIX",      "mix" },
    { "SP",       "ax" },
    { "DVD",      "dvd" },
    { "__DVD",    "dvd" },
    { "CARD",     "card" },
    { "NAND",     "nand" },
    { "ISFS",     "isfs" },
    { "IPC",      "ipc" },
    { "ARC",      "arc" },
    { "EXI",      "exi" },
    { "SI",       "si" },
    { "DSP",      "dsp" },
    { "TPL",      "tpl" },
    { "GD",       "gd" },
    { "MTX",      "mtx" },
    { "NET",      "net" },
    { "SO",       "so" },
    { "SC",       "sc" },
    { "HBM",      "hbm" },
    { "NWC",      "nwc" },
    { "NHTTP",    "nhttp" },
    { "NSSL",     "nssl" },
    { "ESP",      "esp" },
    { "TRK",      "trk" },
    { "IOS_",     "ios" },
    { "BT",       "bte" },
    /* C++ namespaces. */
    { "nw4r::",   "nw4r" },
    { "nw4r",     "nw4r" },     /* mangled prefix like Q34nw4r... */
    { "EGG::",    "egg" },
    { "EGG",      "egg" },
    { "RP",       "revopack" },
    { "JSystem::","jsystem" },
    { "JKernel::","jkernel" },
    { "JUtility::","jutility" },
    { "JMath::",  "jmath" },
    { "homebutton", "homebutton" },
    { "HomeButton", "homebutton" },
};

const char *externs_classify(const char *raw) {
    if (!raw || !*raw) return "";
    const char *best_mod = "";
    size_t best_len = 0;
    for (size_t i = 0; i < sizeof kRules / sizeof kRules[0]; i++) {
        size_t n = strlen(kRules[i].prefix);
        if (n <= best_len) continue;
        if (strncmp(raw, kRules[i].prefix, n) == 0) {
            best_len = n;
            best_mod = kRules[i].module;
        }
    }
    return best_mod;
}
