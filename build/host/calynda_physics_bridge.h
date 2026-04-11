/*
 * calynda_physics_bridge.h — Bridge between Calynda runtime and calynda_physics.
 *
 * Exposes physics engine functions via:  import game.physics;
 */

#ifndef CALYNDA_PHYSICS_BRIDGE_H
#define CALYNDA_PHYSICS_BRIDGE_H

#include "calynda_runtime.h"

extern CalyndaRtPackage __calynda_pkg_physics;

void calynda_physics_register_objects(void);

CalyndaRtWord calynda_physics_dispatch(const CalyndaRtExternCallable *callable,
                                        size_t argument_count,
                                        const CalyndaRtWord *arguments);
CalyndaRtWord calynda_physics_member_load(const char *member);

#endif /* CALYNDA_PHYSICS_BRIDGE_H */
