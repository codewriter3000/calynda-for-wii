#include "dol.h"

#include <string.h>

/* DOL header layout (all fields big-endian, 32-bit):
 *   0x00..0x1B  text section file offsets  (7 entries)
 *   0x1C..0x47  data section file offsets  (11 entries)
 *   0x48..0x63  text section virtual addrs (7 entries)
 *   0x64..0x8F  data section virtual addrs (11 entries)
 *   0x90..0xAB  text section sizes         (7 entries)
 *   0xAC..0xD7  data section sizes         (11 entries)
 *   0xD8        BSS virtual address
 *   0xDC        BSS size
 *   0xE0        entrypoint
 *   0xE4..0xFF  reserved / padding
 */

#define TEXT_OFFSETS_AT   0x00
#define DATA_OFFSETS_AT   0x1C
#define TEXT_ADDRS_AT     0x48
#define DATA_ADDRS_AT     0x64
#define TEXT_SIZES_AT     0x90
#define DATA_SIZES_AT     0xAC
#define BSS_ADDR_AT       0xD8
#define BSS_SIZE_AT       0xDC
#define ENTRYPOINT_AT     0xE0

static uint32_t read_u32_be(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] <<  8) |
           ((uint32_t)p[3]);
}

static int add_overflow_u32(uint32_t a, uint32_t b, uint32_t *out) {
    if (a > UINT32_MAX - b) return 1;
    *out = a + b;
    return 0;
}

const char *dol_status_str(DolStatus s) {
    switch (s) {
        case DOL_OK:                 return "ok";
        case DOL_ERR_NULL_ARG:       return "null argument";
        case DOL_ERR_TOO_SMALL:      return "file smaller than DOL header";
        case DOL_ERR_BAD_SECTION:    return "section extends outside file";
        case DOL_ERR_OVERFLOW:       return "section offset+size overflow";
        case DOL_ERR_MISALIGNED:     return "section offset or address not 4-byte aligned";
        case DOL_ERR_NO_ENTRYPOINT:  return "entrypoint not inside any text section";
    }
    return "unknown";
}

static DolStatus push_section(DolImage *img,
                              DolSectionKind kind,
                              uint8_t index,
                              uint32_t off,
                              uint32_t addr,
                              uint32_t size,
                              size_t file_size) {
    /* Empty slots in DOL are encoded as size == 0; skip. */
    if (size == 0) return DOL_OK;

    if ((off & 0x3u) != 0 || (addr & 0x3u) != 0 || (size & 0x3u) != 0) {
        return DOL_ERR_MISALIGNED;
    }

    uint32_t end;
    if (add_overflow_u32(off, size, &end)) return DOL_ERR_OVERFLOW;
    if ((size_t)end > file_size)            return DOL_ERR_BAD_SECTION;

    /* Virtual address range must also not wrap. */
    uint32_t vend;
    if (add_overflow_u32(addr, size, &vend)) return DOL_ERR_OVERFLOW;

    DolSection *s = &img->sections[img->section_count++];
    s->kind         = kind;
    s->index        = index;
    s->file_offset  = off;
    s->virtual_addr = addr;
    s->size         = size;
    return DOL_OK;
}

DolStatus dol_parse(const uint8_t *bytes, size_t size, DolImage *out) {
    if (!bytes || !out) return DOL_ERR_NULL_ARG;
    memset(out, 0, sizeof(*out));
    if (size < DOL_HEADER_SIZE) return DOL_ERR_TOO_SMALL;

    out->bytes = bytes;
    out->size  = size;

    for (uint8_t i = 0; i < DOL_MAX_TEXT_SECTIONS; i++) {
        uint32_t off  = read_u32_be(bytes + TEXT_OFFSETS_AT + 4u * i);
        uint32_t addr = read_u32_be(bytes + TEXT_ADDRS_AT   + 4u * i);
        uint32_t sz   = read_u32_be(bytes + TEXT_SIZES_AT   + 4u * i);
        DolStatus st = push_section(out, DOL_SECTION_TEXT, i, off, addr, sz, size);
        if (st != DOL_OK) { memset(out, 0, sizeof(*out)); return st; }
    }
    for (uint8_t i = 0; i < DOL_MAX_DATA_SECTIONS; i++) {
        uint32_t off  = read_u32_be(bytes + DATA_OFFSETS_AT + 4u * i);
        uint32_t addr = read_u32_be(bytes + DATA_ADDRS_AT   + 4u * i);
        uint32_t sz   = read_u32_be(bytes + DATA_SIZES_AT   + 4u * i);
        DolStatus st = push_section(out, DOL_SECTION_DATA, i, off, addr, sz, size);
        if (st != DOL_OK) { memset(out, 0, sizeof(*out)); return st; }
    }

    out->bss_addr   = read_u32_be(bytes + BSS_ADDR_AT);
    out->bss_size   = read_u32_be(bytes + BSS_SIZE_AT);
    out->entrypoint = read_u32_be(bytes + ENTRYPOINT_AT);

    /* Entrypoint must land inside a text section. */
    const DolSection *ep_sec = dol_section_for_vaddr(out, out->entrypoint);
    if (!ep_sec || ep_sec->kind != DOL_SECTION_TEXT) {
        memset(out, 0, sizeof(*out));
        return DOL_ERR_NO_ENTRYPOINT;
    }
    return DOL_OK;
}

const DolSection *dol_section_for_vaddr(const DolImage *img, uint32_t vaddr) {
    if (!img) return NULL;
    for (size_t i = 0; i < img->section_count; i++) {
        const DolSection *s = &img->sections[i];
        if (vaddr >= s->virtual_addr && vaddr < s->virtual_addr + s->size) {
            return s;
        }
    }
    return NULL;
}

const uint8_t *dol_vaddr_to_ptr(const DolImage *img,
                                uint32_t vaddr,
                                size_t *out_remaining) {
    if (out_remaining) *out_remaining = 0;
    const DolSection *s = dol_section_for_vaddr(img, vaddr);
    if (!s) return NULL;
    uint32_t delta = vaddr - s->virtual_addr;
    if (out_remaining) *out_remaining = s->size - delta;
    return img->bytes + s->file_offset + delta;
}
