#include <pspgu.h>
#include <psppower.h>
#include <pspdisplay.h>
#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>

#define u8  unsigned char
#define u16 unsigned short int
#define u32 unsigned int

#define BUF_WIDTH   512
#define SCR_WIDTH   480
#define SCR_HEIGHT  272
#define FRAME_SIZE  (BUF_WIDTH * SCR_HEIGHT * 4)

#define VRAM_BASE   0
#define DRAW_BASE   VRAM_BASE
#define DISP_BASE   (DRAW_BASE + FRAME_SIZE)
#define DEPTH_BASE  (DISP_BASE + FRAME_SIZE)

struct Vertex {
  u16 u, v;
  u16 color;
  u16 x, y, z;
} __attribute__((aligned(4), packed));

Vertex surface[2] __attribute__((aligned(16))) = {
  {0, 0, 0, 0, 0, 0},
  {16, 1, 0, 64, 64, 0},
};

u8 indexes[16] __attribute__((aligned(16))) = {
  0, 1,  2,  3,  4,  5,  6,  7, 8, 9, 10, 11, 12, 13, 14, 15
};

#define CLUT_COLOR_COUNT 512
u16 clut[CLUT_COLOR_COUNT] __attribute__((aligned(16))) = {
  0xffff, 0, 0, 0, 0, 0xffff, 0, 0, 0, 0, 0xffff, 0, 0, 0, 0, 0xffff,
  0x00f8, 0, 0, 0, 0, 0x00f8, 0, 0, 0, 0, 0x00f8, 0, 0, 0, 0, 0x00f8,
  0xe007, 0, 0, 0, 0, 0xe007, 0, 0, 0, 0, 0xe007, 0, 0, 0, 0, 0xe007,
  0x1f00, 0, 0, 0, 0, 0x1f00, 0, 0, 0, 0, 0x1f00, 0, 0, 0, 0, 0x1f00,
  0xe0ff, 0, 0, 0, 0, 0xe0ff, 0, 0, 0, 0, 0xe0ff, 0, 0, 0, 0, 0xe0ff,
  0xff07, 0, 0, 0, 0, 0xff07, 0, 0, 0, 0, 0xff07, 0, 0, 0, 0, 0xff07,
  0x1ff8, 0, 0, 0, 0, 0x1ff8, 0, 0, 0, 0, 0x1ff8, 0, 0, 0, 0, 0x1ff8,
  0x0000, 0, 0, 0, 0, 0x0000, 0, 0, 0, 0, 0x0000, 0, 0, 0, 0, 0x0000,
  0x0050, 0, 0, 0, 0, 0x0050, 0, 0, 0, 0, 0x0050, 0, 0, 0, 0, 0x0050,
  0x00a0, 0, 0, 0, 0, 0x00a0, 0, 0, 0, 0, 0x00a0, 0, 0, 0, 0, 0x00a0,
  0x18c6, 0, 0, 0, 0, 0x18c6, 0, 0, 0, 0, 0x18c6, 0, 0, 0, 0, 0x18c6,
  0x00d0, 0, 0, 0, 0, 0x00d0, 0, 0, 0, 0, 0x00d0, 0, 0, 0, 0, 0x00d0,
  0x4003, 0, 0, 0, 0, 0x4003, 0, 0, 0, 0, 0x4003, 0, 0, 0, 0, 0x4003,
  0x0800, 0, 0, 0, 0, 0x0800, 0, 0, 0, 0, 0x0800, 0, 0, 0, 0, 0x0800,
  0x40e0, 0, 0, 0, 0, 0x40e0, 0, 0, 0, 0, 0x40e0, 0, 0, 0, 0, 0x40e0,
  0x1f60, 0, 0, 0, 0, 0x1f60, 0, 0, 0, 0, 0x1f60, 0, 0, 0, 0, 0x1f60,
  0x9ff0, 0, 0, 0, 0, 0x9ff0, 0, 0, 0, 0, 0x9ff0, 0, 0, 0, 0, 0x9ff0,
  0x4087, 0, 0, 0, 0, 0x4087, 0, 0, 0, 0, 0x4087, 0, 0, 0, 0, 0x4087,
  0x5f0c, 0, 0, 0, 0, 0x5f0c, 0, 0, 0, 0, 0x5f0c, 0, 0, 0, 0, 0x5f0c,
  0x2584, 0, 0, 0, 0, 0x2584, 0, 0, 0, 0, 0x2584, 0, 0, 0, 0, 0x2584,
  0x7f40, 0, 0, 0, 0, 0x7f40, 0, 0, 0, 0, 0x7f40, 0, 0, 0, 0, 0x7f40,
  0x0edc, 0, 0, 0, 0, 0x0edc, 0, 0, 0, 0, 0x0edc, 0, 0, 0, 0, 0x0edc,
  0x2044, 0, 0, 0, 0, 0x2044, 0, 0, 0, 0, 0x2044, 0, 0, 0, 0, 0x2044,
  0x0c00, 0, 0, 0, 0, 0x0c00, 0, 0, 0, 0, 0x0c00, 0, 0, 0, 0, 0x0c00,
  0x4020, 0, 0, 0, 0, 0x4020, 0, 0, 0, 0, 0x4020, 0, 0, 0, 0, 0x4020,
  0x6002, 0, 0, 0, 0, 0x6002, 0, 0, 0, 0, 0x6002, 0, 0, 0, 0, 0x6002,
  0x1040, 0, 0, 0, 0, 0x1040, 0, 0, 0, 0, 0x1040, 0, 0, 0, 0, 0x1040,
  0xa0df, 0, 0, 0, 0, 0xa0df, 0, 0, 0, 0, 0xa0df, 0, 0, 0, 0, 0xa0df,
  0xf318, 0, 0, 0, 0, 0xf318, 0, 0, 0, 0, 0xf318, 0, 0, 0, 0, 0xf318,
  0x3fe7, 0, 0, 0, 0, 0x3fe7, 0, 0, 0, 0, 0x3fe7, 0, 0, 0, 0, 0x3fe7,
  0x6af8, 0, 0, 0, 0, 0x6af8, 0, 0, 0, 0, 0x6af8, 0, 0, 0, 0, 0x6af8,
  0xdef7, 0, 0, 0, 0, 0xdef7, 0, 0, 0, 0, 0xdef7, 0, 0, 0, 0, 0xdef7,
};

PSP_MODULE_INFO("clut-csa", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

u32 list[1024] __attribute__((aligned(16))) = {0};

static void guInit() {
  sceGuInit();
  
  sceGuStart(GU_DIRECT, list);
  sceGuDrawBuffer(GU_PSM_5650, (void*)DRAW_BASE, BUF_WIDTH);
  sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void*)DISP_BASE, BUF_WIDTH);
  sceGuDepthBuffer((void*)DEPTH_BASE, BUF_WIDTH);
  sceGuClearColor(0xff303030);
  
  sceGuDisable(GU_SCISSOR_TEST);
  sceGuDisable(GU_DEPTH_TEST);
  sceGuEnable(GU_TEXTURE_2D);
  
  sceGuClutLoad(32, clut);
  sceGuTexMode(GU_PSM_T8, 0, 0, 0);

  sceGuTexWrap(GU_CLAMP, GU_CLAMP);
  sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);
  sceGuDisable(GU_SCISSOR_TEST);
  
  sceGuFinish();
  sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);

  sceGuDisplay(GU_TRUE);
}

int main(int argc, char **argv) {
  scePowerSetClockFrequency(333, 333, 166);
  
  guInit();
  
  pspDebugScreenInitEx(0x0, PSP_DISPLAY_PIXEL_FORMAT_565, 0);
  pspDebugScreenEnableBackColor(0);

  int buffer = DRAW_BASE;
  pspDebugScreenSetOffset(buffer);
  
  u64 lastTime = sceKernelGetSystemTimeWide();
  const u64 interval = 500000;

  SceCtrlData ctl;
  u32 csa = 0;
do {
    sceCtrlPeekBufferPositive(&ctl, 1);
    
    sceGuStart(GU_DIRECT, list);
    sceGuClutMode(GU_PSM_5650, 0, 15 /*255*/, csa);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
    // width & height (next higher power of 2), buffer stride (effective pixel width)
    sceGuTexImage(0, 16, 1, 16, indexes); // multiple of 16 while using 8 bits data
    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_5650 |
      GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, surface);
    
    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
    
    const u32 offset = (buffer == DRAW_BASE) ? DISP_BASE : DRAW_BASE;
    pspDebugScreenSetOffset(offset);
    
    pspDebugScreenSetXY(2, 30);
    pspDebugScreenPrintf("csa: %u   ", csa);
    
    const u64 time = sceKernelGetSystemTimeWide();
    if (time - lastTime >= interval) {
        csa = (csa + 1) % 32;
        lastTime = time;
    }
    
    sceDisplayWaitVblankStart();
    buffer = (int)sceGuSwapBuffers();
} while(!(ctl.Buttons & PSP_CTRL_HOME));
  sceGuTerm();
  sceKernelExitGame();
  return 0;
}
