/*
 * calynda_wii_main.c — Wii-specific main() wrapper.
 *
 * Linked into Wii/GC executables instead of the generated main().
 * Initializes the libogc console before delegating to the Calynda
 * start entry and then loops forever so the output stays visible.
 *
 * The generated C code defines calynda_program_main() which this
 * wrapper calls after console initialization.
 */

#include "calynda_wii_io.h"
#include "calynda_runtime.h"

#include <stdio.h>

/* Provided by the generated C code. */
extern int calynda_program_main(int argc, char **argv);

int main(int argc, char **argv) {
    int exit_code;

    calynda_wii_console_init();

    exit_code = calynda_program_main(argc, argv);

    printf("\n[calynda] program exited with code %d\n", exit_code);

    calynda_wii_wait_forever();
    return exit_code;
}
