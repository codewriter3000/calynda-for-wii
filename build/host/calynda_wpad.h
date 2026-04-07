/*
 * calynda_wpad.h — Wii Remote (WPAD) package for the Calynda runtime.
 *
 * Provides importable wpad.* functions: init, scan, buttons_down,
 * buttons_held, buttons_up, rumble, probe, and button-mask constants.
 */

#ifndef CALYNDA_WPAD_H
#define CALYNDA_WPAD_H

#include "calynda_runtime.h"

extern CalyndaRtPackage       __calynda_pkg_wpad;

/*
 * Register all WPAD callables as known static objects so the runtime's
 * object-validation registry recognises them.  Called once at init.
 */
void calynda_wpad_register_objects(void);

CalyndaRtWord calynda_wpad_dispatch(const CalyndaRtExternCallable *callable,
                                    size_t argument_count,
                                    const CalyndaRtWord *arguments);
CalyndaRtWord calynda_wpad_member_load(const char *member);

#endif /* CALYNDA_WPAD_H */
