#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspctrl.h>

#define u16 unsigned short int
#define u32 unsigned int

PSP_MODULE_INFO("multitex-alpha", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

#define SCR_WIDTH     480
#define SCR_HEIGHT    272
#define BUF_WIDTH     512

#define DRAW_BUF_0    0
#define DRAW_BUF_1    0x88000
#define DEPTH_BUF     0x110000
#define MUL_BASE_TEXT 0x178000

static unsigned int __attribute__((aligned(16))) list[1024];

struct Vertex {
  u16 u, v;
  u32 color;
  u16 x, y, z;
} __attribute__((aligned(4), packed));

u32 __attribute__((aligned(16))) tex0[4] = {
  0xFF0000FF,
  0xFF00FF00,
  0xFFFF0000,
  0xFF0050FF,
};

u32 __attribute__((aligned(16))) tex1[4] = {
  0xFF404040,
  0xFFFFFFFF,
  0xFF404040,
  0xFFFFFFFF,
};

void guInit() {
  sceGuInit();
  sceGuStart(GU_DIRECT, list);
  
  sceGuDrawBuffer(GU_PSM_8888, (void*)DRAW_BUF_0, BUF_WIDTH);
  sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void*)DRAW_BUF_1, BUF_WIDTH);
  sceGuDepthBuffer((void*)DEPTH_BUF, BUF_WIDTH);
  
  sceGuClearColor(0xFF503636);
  sceGuDisable(GU_SCISSOR_TEST);
  sceGuTexScale(1.0f, 1.0f);
  sceGuTexMode(GU_PSM_8888, 0, 0, 0);
  sceGuTexWrap(GU_CLAMP, GU_CLAMP);
  sceGuDisable(GU_STENCIL_TEST);
    
  sceGuFinish();
  sceGuSync(0,0);
  
  sceGuDisplay(GU_TRUE);
}

static u8 alpha = 255;

const struct Vertex sprite[] = {
  {0, 0, 0xFFFFFFFF, 0, 0, 0},
  {4, 1, 0xFFFFFFFF, 4, 4, 0},
};
    
        
void processGuLayers() {
  PspGeContext context __attribute__((aligned(16)));
  sceGeSaveContext(&context);
  
  sceGuStart(GU_DIRECT, list);

  // Set up canvas layers
  sceGuDrawBuffer(GU_PSM_8888, (void*)MUL_BASE_TEXT, 4);    
  sceGuScissor(0, 0, 4, 1);
  sceGuEnable(GU_SCISSOR_TEST);
  
  // Make sure to clear stencil bits (alpha channel) before applying the mask
  sceGuClearStencil(0);
  sceGuClear(GU_COLOR_BUFFER_BIT | GU_STENCIL_BUFFER_BIT);
  
  // Apply a mask on alpha bits as a demonstration. Alternatively, you can achieve
  // the same effect by directly modifying the stencil bits via sceGuClearStencil
  alpha += 2;
  sceGuPixelMask((255 - alpha) << 24);
  sceGuClearStencil(0xFF);
  sceGuClear(GU_STENCIL_BUFFER_BIT);
  
  // First pass
  sceGuDisable(GU_BLEND);
  sceGuEnable(GU_TEXTURE_2D);
  sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);    
  
  sceGuTexImage(0, 4, 1, 4, tex0);

  sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, sprite);
  sceGuTexSync();

  // Second pass  
  sceGuEnable(GU_BLEND);
  sceGuBlendFunc(GU_ADD, GU_DST_COLOR, GU_FIX, 0, 0);
  sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);

  sceGuTexImage(0, 4, 1, 4, tex1);
  sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, sprite);
  sceGuTexSync();

  sceGuFinish();
  sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
  
  sceGeRestoreContext(&context);
}

int main() {
  const struct Vertex sprite[] = {
    {0, 0, 0xFFFFFFFF, 0, 0, 0},
    {4, 1, 0xFFFFFFFF, 64, 64, 0},
  }; 
  
  sceKernelDcacheWritebackAll();
  
  guInit();
  
  int buff = DRAW_BUF_0;
  
  pspDebugScreenInitEx(0x0, PSP_DISPLAY_PIXEL_FORMAT_8888, 0);
  pspDebugScreenEnableBackColor(0);
  pspDebugScreenSetOffset(buff);
    
  SceCtrlData ctl;
  do {
    sceCtrlPeekBufferPositive(&ctl, 1);

    processGuLayers();
    u32* tex = (u32*)(0x04000000 | MUL_BASE_TEXT);
    
    sceGuStart(GU_DIRECT, list);
    sceGuDrawBuffer(GU_PSM_8888, (void*)buff, BUF_WIDTH);
    sceGuClear(GU_COLOR_BUFFER_BIT);

    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
  
    sceGuEnable(GU_TEXTURE_2D);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    sceGuTexImage(0, 4, 1, 4, tex);

    sceGumDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, sprite);
    sceGuTexSync();
    
    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
    
    const u32 offset = (buff == DRAW_BUF_0) ? DRAW_BUF_1 : DRAW_BUF_0;
    pspDebugScreenSetOffset(offset);
    pspDebugScreenSetXY(1, 20);
    pspDebugScreenPrintf("colors: %08X %08X %08X %08X alpha: %i", tex[0], tex[1], tex[2], tex[3], alpha);
    pspDebugScreenSetXY(1, 20);

    sceDisplayWaitVblankStart();
    buff = (int)sceGuSwapBuffers();
  } while(!(ctl.Buttons & PSP_CTRL_HOME));
  
  sceGuTerm();
  sceKernelExitGame();
  return 0;
}


