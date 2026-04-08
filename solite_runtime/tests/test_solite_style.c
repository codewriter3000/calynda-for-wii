/*
 * test_solite_style.c — Unit tests for the CSS-like style property system.
 *
 * Tests:
 *   1. Style map init and cleanup
 *   2. Setting and getting properties
 *   3. CSS name lookup (str -> SolidStylePropId)
 *   4. Unknown CSS names return -1
 *   5. Style apply delegates to bridge callbacks (stubbed)
 *   6. Value types are correct for each property
 */

#include "solite_style.h"
#include "solite_gui_bridge.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Helper: lookup that returns int prop id or -1 */
static int style_lookup(const char *name)
{
    SolidStylePropId out;
    if (name && solite_style_lookup_name(name, &out)) return (int)out;
    return -1;
}

/* ---- Tests ---- */

static void test_style_map_init_cleanup(void)
{
    SolidStyleMap smap;
    solite_style_map_init(&smap);
    assert(smap.count == 0);
    solite_style_map_free(&smap);
    printf("  [PASS] test_style_map_init_cleanup\n");
}

static void test_style_set_get(void)
{
    SolidStyleMap smap;
    solite_style_map_init(&smap);

    SolidStyleValue v = solite_style_int(320);
    solite_style_map_set(&smap, SOLID_STYLE_PROP_WIDTH, v);

    const SolidStyleValue *got = solite_style_map_get(&smap, SOLID_STYLE_PROP_WIDTH);
    assert(got != NULL);
    assert(got->type == SOLID_STYLE_VTYPE_INT);
    assert(got->as.int_val == 320);

    /* Overwrite */
    v = solite_style_int(640);
    solite_style_map_set(&smap, SOLID_STYLE_PROP_WIDTH, v);
    got = solite_style_map_get(&smap, SOLID_STYLE_PROP_WIDTH);
    assert(got != NULL);
    assert(got->as.int_val == 640);

    solite_style_map_free(&smap);
    printf("  [PASS] test_style_set_get\n");
}

static void test_css_name_lookup(void)
{
    int id;

    id = style_lookup("width");
    assert(id == SOLID_STYLE_PROP_WIDTH);

    id = style_lookup("height");
    assert(id == SOLID_STYLE_PROP_HEIGHT);

    id = style_lookup("color");
    assert(id == SOLID_STYLE_PROP_COLOR);

    id = style_lookup("background-color");
    assert(id == SOLID_STYLE_PROP_BACKGROUND_COLOR);

    printf("  [PASS] test_css_name_lookup\n");
}

static void test_unknown_css_name(void)
{
    int id;

    id = style_lookup("border-radius");
    assert(id < 0);

    id = style_lookup("");
    assert(id < 0);

    id = style_lookup(NULL);
    assert(id < 0);

    printf("  [PASS] test_unknown_css_name\n");
}

static void test_value_constructors(void)
{
    SolidStyleValue vi = solite_style_int(42);
    assert(vi.type == SOLID_STYLE_VTYPE_INT);
    assert(vi.as.int_val == 42);

    SolidStyleValue vf = solite_style_float(3.14f);
    assert(vf.type == SOLID_STYLE_VTYPE_FLOAT);
    assert(vf.as.float_val >= 3.13f && vf.as.float_val <= 3.15f);

    SolidStyleValue vc = solite_style_color(0xFF, 0x80, 0x00, 0xFF);
    assert(vc.type == SOLID_STYLE_VTYPE_COLOR);
    assert(vc.as.color_val.r == 0xFF);
    assert(vc.as.color_val.g == 0x80);
    assert(vc.as.color_val.b == 0x00);
    assert(vc.as.color_val.a == 0xFF);

    SolidStyleValue vs = solite_style_string("hello");
    assert(vs.type == SOLID_STYLE_VTYPE_STRING);
    assert(strcmp(vs.as.string_val, "hello") == 0);

    SolidStyleValue vb = solite_style_bool(1);
    assert(vb.type == SOLID_STYLE_VTYPE_BOOL);
    assert(vb.as.bool_val == 1);

    printf("  [PASS] test_value_constructors\n");
}

static void test_style_apply_runs(void)
{
    /*
     * Uses a NULL element, which is fine for host stubs.
     * Just verifying that solite_style_apply does not crash.
     */
    SolidStyleMap smap;
    solite_style_map_init(&smap);

    solite_style_map_set(&smap, SOLID_STYLE_PROP_WIDTH,  solite_style_int(200));
    solite_style_map_set(&smap, SOLID_STYLE_PROP_HEIGHT, solite_style_int(100));
    solite_style_map_set(&smap, SOLID_STYLE_PROP_COLOR,
                        solite_style_color(0xFF, 0x00, 0x00, 0xFF));

    /* This should iterate the map and call the apply functions */
    solite_style_apply(&smap, NULL);

    solite_style_map_free(&smap);
    printf("  [PASS] test_style_apply_runs\n");
}

static void test_multiple_properties(void)
{
    SolidStyleMap smap;
    solite_style_map_init(&smap);

    solite_style_map_set(&smap, SOLID_STYLE_PROP_WIDTH,  solite_style_int(100));
    solite_style_map_set(&smap, SOLID_STYLE_PROP_HEIGHT, solite_style_int(200));
    solite_style_map_set(&smap, SOLID_STYLE_PROP_COLOR,
                        solite_style_color(0, 0, 0xFF, 0xFF));
    solite_style_map_set(&smap, SOLID_STYLE_PROP_BACKGROUND_COLOR,
                        solite_style_color(0xFF, 0xFF, 0xFF, 0xFF));

    assert(smap.count == 4);

    const SolidStyleValue *w = solite_style_map_get(&smap, SOLID_STYLE_PROP_WIDTH);
    const SolidStyleValue *h = solite_style_map_get(&smap, SOLID_STYLE_PROP_HEIGHT);
    assert(w != NULL && w->as.int_val == 100);
    assert(h != NULL && h->as.int_val == 200);

    solite_style_map_free(&smap);
    printf("  [PASS] test_multiple_properties\n");
}

/* ---- Main ---- */

int main(void)
{
    printf("=== test_solite_style ===\n");
    test_style_map_init_cleanup();
    test_style_set_get();
    test_css_name_lookup();
    test_unknown_css_name();
    test_value_constructors();
    test_style_apply_runs();
    test_multiple_properties();
    printf("=== All style tests passed ===\n");
    return 0;
}
