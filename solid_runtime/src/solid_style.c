/*
 * solid_style.c — CSS-like property registry and application.
 *
 * EXTENDING WITH A NEW PROPERTY:
 *   1. Add entry to SolidStylePropId enum in solid_style.h
 *   2. Write an apply_xxx() function below
 *   3. Add a row to style_property_registry[]
 *   That's it — the rest (parsing, map storage, application) is automatic.
 */

#include "solid_style.h"
#include "solid_gui_bridge.h"
#include "calynda_gc.h"

#include <string.h>

/* ---- Apply functions ---- */

/*
 * Each apply function takes the element and the property value,
 * and calls the appropriate libwiigui bridge function.
 */

static void apply_width(SolidGuiElement *el, const SolidStyleValue *val)
{
    if (val->type == SOLID_STYLE_VTYPE_INT) {
        solid_bridge_element_set_size_w(el, val->as.int_val);
    }
}

static void apply_height(SolidGuiElement *el, const SolidStyleValue *val)
{
    if (val->type == SOLID_STYLE_VTYPE_INT) {
        solid_bridge_element_set_size_h(el, val->as.int_val);
    }
}

static void apply_color(SolidGuiElement *el, const SolidStyleValue *val)
{
    if (val->type == SOLID_STYLE_VTYPE_COLOR) {
        solid_bridge_element_set_color(el,
            val->as.color_val.r, val->as.color_val.g,
            val->as.color_val.b, val->as.color_val.a);
    }
}

static void apply_background_color(SolidGuiElement *el, const SolidStyleValue *val)
{
    if (val->type == SOLID_STYLE_VTYPE_COLOR) {
        solid_bridge_element_set_background_color(el,
            val->as.color_val.r, val->as.color_val.g,
            val->as.color_val.b, val->as.color_val.a);
    }
}

/* ---- Property registry ---- */

typedef void (*SolidStyleApplyFn)(SolidGuiElement *el, const SolidStyleValue *val);

typedef struct {
    SolidStylePropId    id;
    const char         *css_name;
    SolidStyleValueType expected_type;
    SolidStyleApplyFn   apply;
} SolidStylePropEntry;

/*
 * THE REGISTRY TABLE — add new properties here.
 * Order must match SolidStylePropId enum values.
 */
static const SolidStylePropEntry style_property_registry[SOLID_STYLE_PROP__COUNT] = {
    /* id                            css_name             type                     apply_fn           */
    { SOLID_STYLE_PROP_WIDTH,            "width",            SOLID_STYLE_VTYPE_INT,   apply_width            },
    { SOLID_STYLE_PROP_HEIGHT,           "height",           SOLID_STYLE_VTYPE_INT,   apply_height           },
    { SOLID_STYLE_PROP_COLOR,            "color",            SOLID_STYLE_VTYPE_COLOR, apply_color            },
    { SOLID_STYLE_PROP_BACKGROUND_COLOR, "background-color", SOLID_STYLE_VTYPE_COLOR, apply_background_color },
};

/* ---- CSS name lookup ---- */

bool solid_style_lookup_name(const char *css_name, SolidStylePropId *out_id)
{
    if (!css_name) return false;
    for (int i = 0; i < SOLID_STYLE_PROP__COUNT; i++) {
        if (strcmp(style_property_registry[i].css_name, css_name) == 0) {
            if (out_id) *out_id = style_property_registry[i].id;
            return true;
        }
    }
    return false;
}

const char *solid_style_prop_name(SolidStylePropId id)
{
    if (id >= 0 && id < SOLID_STYLE_PROP__COUNT)
        return style_property_registry[id].css_name;
    return "(unknown)";
}

SolidStyleValueType solid_style_prop_type(SolidStylePropId id)
{
    if (id >= 0 && id < SOLID_STYLE_PROP__COUNT)
        return style_property_registry[id].expected_type;
    return SOLID_STYLE_VTYPE_INT;
}

/* ---- Style map ---- */

#define STYLE_MAP_INITIAL 4

void solid_style_map_init(SolidStyleMap *map)
{
    if (!map) return;
    map->count = 0;
    map->capacity = STYLE_MAP_INITIAL;
    map->entries = (SolidStyleEntry *)calynda_gc_alloc(
        sizeof(SolidStyleEntry) * STYLE_MAP_INITIAL);
}

void solid_style_map_free(SolidStyleMap *map)
{
    if (!map) return;
    if (map->entries) calynda_gc_release(map->entries);
    map->entries = NULL;
    map->count = 0;
    map->capacity = 0;
}

void solid_style_map_set(SolidStyleMap *map, SolidStylePropId id, SolidStyleValue value)
{
    if (!map) return;

    /* Update existing entry */
    for (size_t i = 0; i < map->count; i++) {
        if (map->entries[i].id == id) {
            map->entries[i].value = value;
            return;
        }
    }

    /* Grow if needed */
    if (map->count >= map->capacity) {
        size_t new_cap = map->capacity * 2;
        SolidStyleEntry *new_entries = (SolidStyleEntry *)calynda_gc_alloc(
            sizeof(SolidStyleEntry) * new_cap);
        if (!new_entries) return;
        memcpy(new_entries, map->entries, sizeof(SolidStyleEntry) * map->count);
        calynda_gc_release(map->entries);
        map->entries = new_entries;
        map->capacity = new_cap;
    }

    map->entries[map->count].id = id;
    map->entries[map->count].value = value;
    map->count++;
}

const SolidStyleValue *solid_style_map_get(const SolidStyleMap *map, SolidStylePropId id)
{
    if (!map) return NULL;
    for (size_t i = 0; i < map->count; i++) {
        if (map->entries[i].id == id)
            return &map->entries[i].value;
    }
    return NULL;
}

void solid_style_apply(const SolidStyleMap *map, SolidGuiElement *element)
{
    if (!map || !element) return;

    for (size_t i = 0; i < map->count; i++) {
        SolidStylePropId id = map->entries[i].id;
        if (id >= 0 && id < SOLID_STYLE_PROP__COUNT) {
            const SolidStylePropEntry *reg = &style_property_registry[id];
            if (reg->apply) {
                reg->apply(element, &map->entries[i].value);
            }
        }
    }
}
