/*
 * DOL (Wii / GameCube executable) loader.
 *
 * A DOL file has a fixed 0x100-byte header followed by up to 7 text
 * sections and up to 11 data sections. All header fields are 32-bit
 * big-endian. See README for references.
 *
 * This loader validates the header, maps sections to file regions,
 * and exposes a read-only view of the image. It does not decode
 * instructions; that is the disassembler's job.
 */
#ifndef CALYNDA_DECOMPILER_DOL_H
#define CALYNDA_DECOMPILER_DOL_H

#include <stddef.h>
#include <stdint.h>

#define DOL_HEADER_SIZE      0x100u
#define DOL_MAX_TEXT_SECTIONS 7u
#define DOL_MAX_DATA_SECTIONS 11u

typedef enum {
    DOL_OK = 0,
    DOL_ERR_NULL_ARG,
    DOL_ERR_TOO_SMALL,
    DOL_ERR_BAD_SECTION,        /* offset/size outside file */
    DOL_ERR_OVERFLOW,           /* integer overflow in offset+size */
    DOL_ERR_MISALIGNED,         /* offset or address not 4-byte aligned */
    DOL_ERR_NO_ENTRYPOINT       /* entrypoint not inside any text section */
} DolStatus;

typedef enum {
    DOL_SECTION_TEXT,
    DOL_SECTION_DATA
} DolSectionKind;

typedef struct {
    DolSectionKind kind;
    uint8_t        index;       /* 0..6 for text, 0..10 for data */
    uint32_t       file_offset;
    uint32_t       virtual_addr;
    uint32_t       size;
} DolSection;

typedef struct {
    const uint8_t *bytes;       /* borrowed; caller owns lifetime */
    size_t         size;

    uint32_t       entrypoint;
    uint32_t       bss_addr;
    uint32_t       bss_size;

    DolSection     sections[DOL_MAX_TEXT_SECTIONS + DOL_MAX_DATA_SECTIONS];
    size_t         section_count; /* only non-empty sections populated */
} DolImage;

/*
 * Parse a DOL image. `bytes` must remain valid for the lifetime of
 * `out`. On error, `out` is left in a zeroed state.
 */
DolStatus dol_parse(const uint8_t *bytes, size_t size, DolImage *out);

/* Human-readable status; never returns NULL. */
const char *dol_status_str(DolStatus s);

/*
 * Find the section containing a virtual address. Returns NULL if no
 * section covers the address.
 */
const DolSection *dol_section_for_vaddr(const DolImage *img, uint32_t vaddr);

/*
 * Translate a virtual address to a pointer inside the image, or NULL
 * if the address is not backed by file bytes (e.g. BSS or unmapped).
 * `*out_remaining` receives the number of valid bytes from the
 * returned pointer to the end of the containing section.
 */
const uint8_t *dol_vaddr_to_ptr(const DolImage *img,
                                uint32_t vaddr,
                                size_t *out_remaining);

#endif /* CALYNDA_DECOMPILER_DOL_H */
