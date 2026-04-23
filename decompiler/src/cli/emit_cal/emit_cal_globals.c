/*
 * Top-level binding emitter (req 8.3).
 *
 * Walks every symbol whose address lies in a *data* (non-text) section
 * and emits a Calynda `var` / `final` binding with a literal initializer
 * derived from the raw section bytes. When the symbol's size is
 * unknown, we fall back to a commented `# unknown size` note and skip
 * the body. BSS symbols (no file backing) become `var <name>: u32` with
 * a zero initializer.
 */
#include "emit_cal_internal.h"

#include <string.h>

static int in_text(const DolImage *img, uint32_t vaddr) {
    const DolSection *s = dol_section_for_vaddr(img, vaddr);
    return s && s->kind == DOL_SECTION_TEXT;
}

static uint32_t read_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
}

int emit_globals(EmitFile *ef, const DolImage *img, const SymTable *syms) {
    if (!syms || syms->count == 0) return 0;
    int emitted = 0;

    for (size_t i = 0; i < syms->count; i++) {
        const SymEntry *e = &syms->entries[i];
        if (in_text(img, e->vaddr)) continue;                /* functions */
        if (e->name[0] == '\0')     continue;

        char ident[SYM_NAME_MAX];
        emit_safe_ident(e->name, ident, sizeof ident);

        size_t remaining = 0;
        const uint8_t *p = dol_vaddr_to_ptr(img, e->vaddr, &remaining);

        if (!emitted)
            emit_linef(ef, "# --- recovered globals (req 8.3) ---");

        if (!p) {
            /* BSS / unmapped — zero-initialized. */
            emit_linef(ef, "# 0x%08x  %s (bss %u bytes)",
                       e->vaddr, e->name, e->size);
            emit_linef(ef, "var %s: u32 = 0", ident);
        } else if (e->size == 4 && remaining >= 4) {
            uint32_t v = read_be32(p);
            emit_linef(ef, "# 0x%08x  %s", e->vaddr, e->name);
            emit_linef(ef, "final %s: u32 = 0x%08x", ident, v);
        } else if (e->size == 2 && remaining >= 2) {
            uint16_t v = (uint16_t)((p[0] << 8) | p[1]);
            emit_linef(ef, "# 0x%08x  %s", e->vaddr, e->name);
            emit_linef(ef, "final %s: u16 = 0x%04x", ident, v);
        } else if (e->size == 1 && remaining >= 1) {
            emit_linef(ef, "# 0x%08x  %s", e->vaddr, e->name);
            emit_linef(ef, "final %s: u8 = 0x%02x", ident, p[0]);
        } else if (e->size > 0 && e->size <= 64 && remaining >= e->size) {
            /* Array literal of bytes. */
            emit_linef(ef, "# 0x%08x  %s (%u bytes)",
                       e->vaddr, e->name, e->size);
            char line[512];
            size_t k = 0;
            k += (size_t)snprintf(line + k, sizeof line - k,
                                  "final %s: arr<u8> = [", ident);
            for (unsigned j = 0; j < e->size; j++) {
                k += (size_t)snprintf(line + k, sizeof line - k,
                                      "%s0x%02x",
                                      j == 0 ? "" : ", ", p[j]);
                if (k > sizeof line - 16) break;
            }
            snprintf(line + k, sizeof line - k, "]");
            emit_linef(ef, "%s", line);
        } else {
            emit_linef(ef, "# 0x%08x  %s  (size=%u — not materialized)",
                       e->vaddr, e->name, e->size);
            emit_linef(ef, "var %s: u32 = 0  # TODO: hydrate from data",
                       ident);
        }
        emitted++;
    }
    if (emitted) emit_linef(ef, "");
    return emitted;
}
