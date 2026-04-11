/*
 * calynda_gfx_bridge.h — Bridge between Calynda runtime and calynda_gfx3d.
 *
 * Exposes graphics engine functions via:  import game.gfx;
 */

#ifndef CALYNDA_GFX_BRIDGE_H
#define CALYNDA_GFX_BRIDGE_H

#include "calynda_runtime.h"

extern CalyndaRtPackage __calynda_pkg_gfx;

void calynda_gfx_register_objects(void);

CalyndaRtWord calynda_gfx_dispatch(const CalyndaRtExternCallable *callable,
                                    size_t argument_count,
                                    const CalyndaRtWord *arguments);
CalyndaRtWord calynda_gfx_member_load(const char *member);

#endif /* CALYNDA_GFX_BRIDGE_H */
