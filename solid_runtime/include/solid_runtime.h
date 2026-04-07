#ifndef SOLID_RUNTIME_H
#define SOLID_RUNTIME_H

/*
 * solid_runtime.h — Top-level Solid/Wii runtime.
 *
 * Provides the main loop integration with libwiigui:
 *   - solid_mount() starts the render/update loop
 *   - solid_unmount() tears down
 *
 * Include this single header to get the full Solid runtime API.
 */

#include "solid_signal.h"
#include "solid_effect.h"
#include "solid_control_flow.h"
#include "solid_style.h"
#include "solid_gui_bridge.h"
#include "calynda_runtime.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mount the root GUI element and start the main loop.
 * This function does not return until solid_request_unmount() is called.
 *
 * root: pointer to the root SolidGuiWindow (from the bridge layer).
 */
void solid_mount(SolidGuiWindow *root);

/*
 * Request the main loop to exit after the current frame.
 */
void solid_request_unmount(void);

/*
 * Returns true if the runtime is currently mounted and running.
 */
bool solid_is_mounted(void);

/*
 * Initialize the Solid runtime subsystems.
 * Called automatically by solid_mount(), but can be called earlier
 * for testing purposes.
 */
void solid_runtime_init(void);

/*
 * Shutdown the Solid runtime and release all resources.
 */
void solid_runtime_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* SOLID_RUNTIME_H */
