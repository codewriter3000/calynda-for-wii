/*
 * calynda_motion_bridge.h — Bridge between Calynda runtime and calynda_motion.
 *
 * Exposes motion / gesture functions via:  import game.motion;
 */

#ifndef CALYNDA_MOTION_BRIDGE_H
#define CALYNDA_MOTION_BRIDGE_H

#include "calynda_runtime.h"

extern CalyndaRtPackage __calynda_pkg_motion;

void calynda_motion_register_objects(void);

CalyndaRtWord calynda_motion_dispatch(const CalyndaRtExternCallable *callable,
                                       size_t argument_count,
                                       const CalyndaRtWord *arguments);
CalyndaRtWord calynda_motion_member_load(const char *member);

#endif /* CALYNDA_MOTION_BRIDGE_H */
