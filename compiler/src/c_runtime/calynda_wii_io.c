/*
 * calynda_wii_io.c — Wii console I/O via libogc.
 *
 * Sets up VIDEO + CON so that printf/puts write to the
 * Wii framebuffer.  Works on real hardware and Dolphin.
 */

#include "calynda_wii_io.h"

#include <ogc/system.h>
#include <ogc/video.h>
#include <ogc/console.h>
#include <ogc/consol.h>

#include <stdlib.h>
#include <string.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

void calynda_wii_console_init(void) {
    VIDEO_Init();

    rmode = VIDEO_GetPreferredMode(NULL);

    /* Configure video first. */
    VIDEO_Configure(rmode);

    /* Allocate and clear the framebuffer. */
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    VIDEO_ClearFrameBuffer(rmode, xfb, 0x00800080);

    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE) {
        VIDEO_WaitVSync();
    }

    /* Now initialise the text console on the ready framebuffer. */
    CON_Init(xfb, 20, 20,
             rmode->fbWidth, rmode->xfbHeight,
             rmode->fbWidth * VI_DISPLAY_PIX_SZ);
}

void calynda_wii_wait_forever(void) {
    while (1) {
        VIDEO_WaitVSync();
    }
}
