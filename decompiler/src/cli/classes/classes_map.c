/*
 * Requirement 4.5 — Calynda representation mapping.
 *
 * Rules:
 *   - A class with a primary vtable maps to CLASS_REPR_UNION (it has
 *     runtime-dispatched methods, which Calynda models with union +
 *     tagged discriminator).
 *   - A class with no vtable but discovered data fields maps to
 *     CLASS_REPR_BINDING (a flat struct we can express as a record).
 *   - A class with neither maps to CLASS_REPR_EXTERN (opaque type,
 *     only ever passed by pointer through extern bindings).
 */
#include "classes_internal.h"

int classes_assign_repr(Classes *c) {
    for (size_t i = 0; i < c->class_count; i++) {
        ClassInfo *ci = &c->classes[i];
        if (ci->primary_vtable) {
            ci->repr = CLASS_REPR_UNION;
        } else if (ci->field_count > 0) {
            ci->repr = CLASS_REPR_BINDING;
        } else {
            ci->repr = CLASS_REPR_EXTERN;
        }
    }
    return 0;
}
