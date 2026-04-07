/*
 * calynda_mii.h — Mii Channel data package for the Calynda runtime.
 *
 * Provides importable mii.* functions: load, count, name, creator,
 * and field accessors for all Mii attributes by index.
 */

#ifndef CALYNDA_MII_H
#define CALYNDA_MII_H

#include "calynda_runtime.h"

extern CalyndaRtPackage __calynda_pkg_mii;

/*
 * Register all Mii callables as known static objects so the runtime's
 * object-validation registry recognises them.  Called once at init.
 */
void calynda_mii_register_objects(void);

CalyndaRtWord calynda_mii_dispatch(const CalyndaRtExternCallable *callable,
                                   size_t argument_count,
                                   const CalyndaRtWord *arguments);
CalyndaRtWord calynda_mii_member_load(const char *member);

#endif /* CALYNDA_MII_H */
