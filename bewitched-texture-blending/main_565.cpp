#include "./main.h"

PSP_MODULE_INFO("bewitched-texture", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

u32 __attribute__((aligned(16))) list[2048];

void guConfig(const int id) {
  
  switch(id) {
    case 0: {
      sceGuInit();
      sceGuStart(GU_DIRECT, list);
      
      sceGuDrawBuffer(GU_PSM_8888, (void*)DRAW_BUF_0, BUF_WIDTH);
      sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void*)DRAW_BUF_1, BUF_WIDTH);
      sceGuDepthBuffer((void*)DEPTH_BUF, BUF_WIDTH);
      
      sceGuDisable(GU_SCISSOR_TEST);
      sceGuDisable(GU_STENCIL_TEST);
      sceGuDisable(GU_DEPTH_TEST);

      sceGuDisable(GU_BLEND);
      sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

      sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
      sceGuTexMode(GU_PSM_8888, 0, 1, 0);
      sceGuTexWrap(GU_CLAMP, GU_CLAMP);
      sceGuTexScale(1.0f, 1.0f);
      sceGuDisable(GU_TEXTURE_2D);
      
      sceGuFinish();
      sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);

      sceGuDisplay(GU_TRUE);
      break;
    }
    case 1: {
      sceGuStart(GU_DIRECT, list);
  
      sceGuDrawBuffer(GU_PSM_8888, (void*)DRAW_BUF_0, BUF_WIDTH);
      sceGuDisable(GU_SCISSOR_TEST);
      sceGuClearColor(0xFF504040);
      sceGuEnable(GU_TEXTURE_2D);
      sceGuEnable(GU_BLEND);
      
      sceGuTexMode(GU_PSM_8888, 0, 1, 0);

      sceGuFinish();
      sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
      break;
    }
  }
}

/*
 * Bewitched texture blend between texture 0 and texture 1.
 * The output texture preserves the alpha values of the bewitched texture.
 */
#define BEWITCHED_MIXTURE_BASE  0x178000

Vertex __attribute__((aligned(4))) bSprite[2] = {
  { 0, 0, 0, 0, 0, 0 },
  { 32, 32, 0, 32, 32, 0 }
};

u32* mixBewitchedTexture(const u32* const tex0, const u32* const tex1) {
  static u32 bBaseOffset = 0;

  const u32 buffer = BEWITCHED_MIXTURE_BASE + bBaseOffset;

  PspGeContext context __attribute__((aligned(16)));
  sceGeSaveContext(&context);
  
  sceGuStart(GU_DIRECT, list);
  
  sceGuDrawBuffer(GU_PSM_5650, (void*)(buffer), 128);
  sceGuScissor(0, 0, 32, 32);
  sceGuEnable(GU_SCISSOR_TEST);
  sceGuClearColor(0);
  
  sceGuEnable(GU_TEXTURE_2D);
  sceGuTexMode(GU_PSM_5650, 0, 1, 0);
  
  sceGuTexImage(0, 32, 32, 32, tex0);
  sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 |
  GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, bSprite);  

  sceGuDrawBuffer(GU_PSM_8888, (void*)(buffer), 64);
  sceGuScissor(0, 0, 16, 16);
  sceGuTexMode(GU_PSM_8888, 0, 1, 0);
  sceGuEnable(GU_BLEND);

  sceGuTexImage(0, 16, 16, 16, tex1);
  sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 |
  GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, bSprite);

  sceGuFinish();
  sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
  
  sceGeRestoreContext(&context);
  
  bBaseOffset += 64*64*4;
  return (u32*)(0x04000000 | buffer);
}

Vertex __attribute__((aligned(4))) sprite0[2] = {
  { 0, 0, 0, 0, 0, 0 },
  { 16, 16, 0, 64, 64, 0 }
};

Vertex __attribute__((aligned(4))) sprite1[2] = {
  { 0, 0, 0, 128, 0, 0 },
  { 16, 16, 0, 64 + 128, 64, 0 }
};

int main() {
  int w0, h0, w1, h1;
  u32* const tex0 = (u32*)stbi_load("./tex0.png", &w0, &h0, NULL, STBI_rgb_alpha);
  u32* const tex1 = (u32*)stbi_load("./tex1.png", &w1, &h1, NULL, STBI_rgb_alpha);
  if (!tex0 || !tex1) {
    sceKernelExitGame();
  }
  
  guConfig(0);
  
  int buff = DRAW_BUF_0;
  
  pspDebugScreenInitEx(0x0, PSP_DISPLAY_PIXEL_FORMAT_8888, 0);
  pspDebugScreenEnableBackColor(0);
  pspDebugScreenSetOffset(buff);
  
  u32* const bTex0 = mixBewitchedTexture(tex0, tex1);
  u32* const bTex1 = mixBewitchedTexture(tex1, tex0);
  
  guConfig(1);

  SceCtrlData ctl;  
  do {
    sceCtrlPeekBufferPositive(&ctl, 1);

    sceGuStart(GU_DIRECT, list);
    
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

    sceGuTexImage(0, 16, 16, 64, bTex0);
    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 |
    GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, sprite0);
    
    sceGuTexImage(0, 16, 16, 64, bTex1);
    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 |
    GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, sprite1);
    
    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
    
    pspDebugScreenSetXY(0, 20);
    pspDebugScreenPrintf("bTex0 pixel debug %x \n", bTex0[0]);
    pspDebugScreenPrintf("bTex1 pixel debug %x \n", bTex1[0]);

    const u32 offset = (buff == DRAW_BUF_0) ? DRAW_BUF_1 : DRAW_BUF_0;
    pspDebugScreenSetOffset(offset);
    
    sceDisplayWaitVblankStart();
    buff = (int)sceGuSwapBuffers();
  } while(!(ctl.Buttons & PSP_CTRL_HOME));
  
  sceGuTerm();
  sceKernelExitGame();
  return 0;
}
