#ifndef SOLID_STYLE_H
#define SOLID_STYLE_H

/*
 * solid_style.h — CSS-like style property system for Solid/Wii.
 *
 * Design goal: adding a new CSS property requires only:
 *   1. Add an entry to the SolidStylePropId enum
 *   2. Add a field to SolidStyleValue union (if new value type needed)
 *   3. Add an entry to the property registry table in solid_style.c
 *   4. Add the apply function that calls the libwiigui bridge
 *
 * Properties are stored as an array of (id, value) pairs in a SolidStyleMap,
 * and applied to a SolidGuiElement by walking the registry.
 */

#include "calynda_runtime.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration */
typedef struct SolidGuiElement SolidGuiElement;

/* ---- Style value types ---- */

typedef enum {
    SOLID_STYLE_VTYPE_INT,       /* int (px, sizes)            */
    SOLID_STYLE_VTYPE_FLOAT,     /* float (scale, opacity)     */
    SOLID_STYLE_VTYPE_COLOR,     /* RGBA color (4 × uint8)     */
    SOLID_STYLE_VTYPE_STRING,    /* string values (font name)  */
    SOLID_STYLE_VTYPE_BOOL       /* boolean (visible, etc.)    */
} SolidStyleValueType;

/*
 * RGBA color — matches GXColor layout for direct passing to libwiigui.
 */
typedef struct {
    uint8_t r, g, b, a;
} SolidColor;

/*
 * A style value. Tagged union of all supported value types.
 */
typedef struct {
    SolidStyleValueType type;
    union {
        int         int_val;
        float       float_val;
        SolidColor  color_val;
        const char *string_val;
        bool        bool_val;
    } as;
} SolidStyleValue;

/* ---- Property IDs ---- */

/*
 * To add a new CSS property:
 *   1. Add an entry here (before SOLID_STYLE_PROP__COUNT)
 *   2. Add a registry entry in solid_style.c :: style_property_registry[]
 *   3. Implement the apply function
 */
typedef enum {
    SOLID_STYLE_PROP_WIDTH = 0,
    SOLID_STYLE_PROP_HEIGHT,
    SOLID_STYLE_PROP_COLOR,
    SOLID_STYLE_PROP_BACKGROUND_COLOR,

    /* --- add new properties above this line --- */
    SOLID_STYLE_PROP__COUNT
} SolidStylePropId;

/* ---- Style map (set of property values) ---- */

typedef struct {
    SolidStylePropId  id;
    SolidStyleValue   value;
} SolidStyleEntry;

typedef struct {
    SolidStyleEntry *entries;
    size_t           count;
    size_t           capacity;
} SolidStyleMap;

/*
 * Initialize an empty style map.
 */
void solid_style_map_init(SolidStyleMap *map);

/*
 * Free the style map's internal storage.
 */
void solid_style_map_free(SolidStyleMap *map);

/*
 * Set a property in the map. Overwrites if already present.
 */
void solid_style_map_set(SolidStyleMap *map, SolidStylePropId id, SolidStyleValue value);

/*
 * Get a property from the map. Returns NULL if not set.
 */
const SolidStyleValue *solid_style_map_get(const SolidStyleMap *map, SolidStylePropId id);

/*
 * Apply all properties in the map to a GUI element.
 * Walks the property registry and calls each registered apply function.
 */
void solid_style_apply(const SolidStyleMap *map, SolidGuiElement *element);

/* ---- Convenience constructors ---- */

static inline SolidStyleValue solid_style_int(int v) {
    SolidStyleValue sv;
    sv.type = SOLID_STYLE_VTYPE_INT;
    sv.as.int_val = v;
    return sv;
}

static inline SolidStyleValue solid_style_float(float v) {
    SolidStyleValue sv;
    sv.type = SOLID_STYLE_VTYPE_FLOAT;
    sv.as.float_val = v;
    return sv;
}

static inline SolidStyleValue solid_style_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    SolidStyleValue sv;
    sv.type = SOLID_STYLE_VTYPE_COLOR;
    sv.as.color_val = (SolidColor){r, g, b, a};
    return sv;
}

static inline SolidStyleValue solid_style_string(const char *s) {
    SolidStyleValue sv;
    sv.type = SOLID_STYLE_VTYPE_STRING;
    sv.as.string_val = s;
    return sv;
}

static inline SolidStyleValue solid_style_bool(bool v) {
    SolidStyleValue sv;
    sv.type = SOLID_STYLE_VTYPE_BOOL;
    sv.as.bool_val = v;
    return sv;
}

/* ---- CSS name lookup ---- */

/*
 * Look up a property ID by its CSS name (e.g., "background-color").
 * Returns true if found, false otherwise.
 */
bool solid_style_lookup_name(const char *css_name, SolidStylePropId *out_id);

/*
 * Get the CSS name for a property ID.
 */
const char *solid_style_prop_name(SolidStylePropId id);

/*
 * Get the expected value type for a property.
 */
SolidStyleValueType solid_style_prop_type(SolidStylePropId id);

#ifdef __cplusplus
}
#endif

#endif /* SOLID_STYLE_H */
