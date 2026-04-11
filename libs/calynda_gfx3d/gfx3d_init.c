/*
 * gfx3d_init.c — Video/GX initialization and frame management for 3D.
 */

#include "gfx3d_init.h"

#include <string.h>

#ifdef CALYNDA_WII_BUILD
#include <gccore.h>
#include <ogcsys.h>
#include <wiiuse/wpad.h>

#define GFX3D_FIFO_SIZE (256 * 1024)

static u32 *xfb[2] = { NULL, NULL };
static int whichfb = 0;
static GXRModeObj *vmode = NULL;
static unsigned char gp_fifo[GFX3D_FIFO_SIZE] ATTRIBUTE_ALIGN(32);
static Gfx3dDisplayInfo display_info;
static Gfx3dColor clear_color = { 100, 149, 237, 255 }; /* cornflower blue */

void gfx3d_init(void) {
    VIDEO_Init();
    vmode = VIDEO_GetPreferredMode(NULL);

    display_info.widescreen = (CONF_GetAspectRatio() == CONF_ASPECT_16_9);
    if (display_info.widescreen) {
        vmode->viWidth = VI_MAX_WIDTH_PAL;
    }

    VIDEO_Configure(vmode);

    display_info.width = vmode->fbWidth;
    display_info.height = 480;
    display_info.aspect = display_info.widescreen ? (16.0f / 9.0f) : (4.0f / 3.0f);

    /* Allocate double-buffered framebuffers */
    xfb[0] = (u32 *)MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));
    xfb[1] = (u32 *)MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));

    VIDEO_ClearFrameBuffer(vmode, xfb[0], COLOR_BLACK);
    VIDEO_ClearFrameBuffer(vmode, xfb[1], COLOR_BLACK);
    VIDEO_SetNextFramebuffer(xfb[0]);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (vmode->viTVMode & VI_NON_INTERLACE)
        VIDEO_WaitVSync();

    /* Initialize GX */
    memset(&gp_fifo, 0, GFX3D_FIFO_SIZE);
    GX_Init(&gp_fifo, GFX3D_FIFO_SIZE);

    GXColor bg = { clear_color.r, clear_color.g, clear_color.b, clear_color.a };
    GX_SetCopyClear(bg, 0x00FFFFFF);
    GX_SetDispCopyGamma(GX_GM_1_0);
    GX_SetCullMode(GX_CULL_BACK);

    gfx3d_setup_3d_mode();
}

Gfx3dDisplayInfo gfx3d_get_display_info(void) {
    return display_info;
}

void gfx3d_set_clear_color(Gfx3dColor color) {
    clear_color = color;
    GXColor bg = { color.r, color.g, color.b, color.a };
    GX_SetCopyClear(bg, 0x00FFFFFF);
}

void gfx3d_setup_3d_mode(void) {
    f32 yscale = GX_GetYScaleFactor(vmode->efbHeight, vmode->xfbHeight);
    u32 xfbHeight = GX_SetDispCopyYScale(yscale);
    GX_SetScissor(0, 0, vmode->fbWidth, vmode->efbHeight);
    GX_SetDispCopySrc(0, 0, vmode->fbWidth, vmode->efbHeight);
    GX_SetDispCopyDst(vmode->fbWidth, xfbHeight);
    GX_SetCopyFilter(vmode->aa, vmode->sample_pattern, GX_TRUE, vmode->vfilter);
    GX_SetFieldMode(vmode->field_rendering,
                     ((vmode->viHeight == 2 * vmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));

    if (vmode->aa)
        GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
    else
        GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

    /* Vertex descriptor: pos + normal + UV + color */
    GX_ClearVtxDesc();
    GX_InvVtxCache();
    GX_InvalidateTexAll();

    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_NRM, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

    /* Z-buffer on */
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);

    /* 1 color channel (for lighting), 1 tex gen */
    GX_SetNumChans(1);
    GX_SetNumTexGens(1);

    /* Default TEV: modulate texture color with lit vertex color */
    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

    /* Blending for transparency */
    GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GX_SetAlphaUpdate(GX_TRUE);

    GX_SetViewport(0, 0, vmode->fbWidth, vmode->efbHeight, 0.0f, 1.0f);
}

void gfx3d_setup_2d_mode(void) {
    Mtx model;
    Mtx44 proj;

    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

    GX_SetZMode(GX_FALSE, GX_LEQUAL, GX_TRUE);
    GX_SetNumChans(1);
    GX_SetNumTexGens(1);
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

    guMtxIdentity(model);
    guMtxTransApply(model, model, 0.0f, 0.0f, -50.0f);
    GX_LoadPosMtxImm(model, GX_PNMTX0);

    guOrtho(proj, 0, 479, 0, 639, 0, 300);
    GX_LoadProjectionMtx(proj, GX_ORTHOGRAPHIC);
}

void gfx3d_frame_begin(void) {
    /* Clear is handled by GX_SetCopyClear — the copy at end of
       previous frame clears the EFB for the next frame. */
}

void gfx3d_frame_end(void) {
    whichfb ^= 1;
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetColorUpdate(GX_TRUE);
    GX_CopyDisp(xfb[whichfb], GX_TRUE);
    GX_DrawDone();
    VIDEO_SetNextFramebuffer(xfb[whichfb]);
    VIDEO_Flush();
    VIDEO_WaitVSync();
}

void gfx3d_shutdown(void) {
    GX_AbortFrame();
    GX_Flush();
    VIDEO_SetBlack(TRUE);
    VIDEO_Flush();
}

#else /* Host stubs */

#include <stdio.h>

static Gfx3dDisplayInfo display_info = { 640, 480, false, 4.0f / 3.0f };
static Gfx3dColor clear_color = { 100, 149, 237, 255 };

void gfx3d_init(void) {
    printf("[gfx3d] init (host stub) %dx%d\n", display_info.width, display_info.height);
}

Gfx3dDisplayInfo gfx3d_get_display_info(void) { return display_info; }
void gfx3d_set_clear_color(Gfx3dColor color) { clear_color = color; (void)clear_color; }
void gfx3d_setup_3d_mode(void) { printf("[gfx3d] setup 3D mode (host stub)\n"); }
void gfx3d_setup_2d_mode(void) { printf("[gfx3d] setup 2D mode (host stub)\n"); }
void gfx3d_frame_begin(void) {}
void gfx3d_frame_end(void) {}
void gfx3d_shutdown(void) { printf("[gfx3d] shutdown (host stub)\n"); }

#endif /* CALYNDA_WII_BUILD */
