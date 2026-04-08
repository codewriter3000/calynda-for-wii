#ifndef SOLITE_RUNTIME_H
#define SOLITE_RUNTIME_H

/*
 * solite_runtime.h — Top-level Solite/Wii runtime.
 *
 * Provides the main loop integration with libwiigui:
 *   - solite_mount() starts the render/update loop
 *   - solid_unmount() tears down
 *
 * Include this single header to get the full Solite runtime API.
 */

#include "solite_signal.h"
#include "solite_effect.h"
#include "solite_control_flow.h"
#include "solite_style.h"
#include "solite_gui_bridge.h"
#include "calynda_runtime.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mount the root GUI element and start the main loop.
 * This function does not return until solite_request_unmount() is called.
 *
 * root: pointer to the root SoliteGuiWindow (from the bridge layer).
 */
void solite_mount(SoliteGuiWindow *root);

/*
 * Request the main loop to exit after the current frame.
 */
void solite_request_unmount(void);

/*
 * Returns true if the runtime is currently mounted and running.
 */
bool solite_is_mounted(void);

/*
 * Initialize the Solite runtime subsystems.
 * Called automatically by solite_mount(), but can be called earlier
 * for testing purposes.
 */
void solite_runtime_init(void);

/*
 * Shutdown the Solite runtime and release all resources.
 */
void solite_runtime_shutdown(void);

/*
 * Register a GUI element as a pointer cursor that tracks Wii remote IR.
 * channel: Wii remote channel (0–3)
 * cursor:  opaque pointer to the cursor image element
 *
 * The runtime main loop will update the cursor position each frame.
 */
void solite_register_pointer(int channel, SoliteGuiElement *cursor);

#ifdef __cplusplus
}
#endif

#endif /* SOLITE_RUNTIME_H */
