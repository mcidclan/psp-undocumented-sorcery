// Betwitched texture blending, 'Isolated Alpha' method 

#include "./main.h"
#include <malloc.h>

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
      sceGuClearColor(0xFF5a4c35);
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
 * Bewitched blending between texture 0 and texture 1.
 * The alpha of the output texture is blending.
 */
const u32 BTEX_BYTE_COUNT = 64*64*4;

Vertex __attribute__((aligned(4))) bSprites[6] = {
  // 565 shift right
  { 0,  0,  0xffffffff, 1,  0,  0 },
  { 31, 32, 0xffffffff, 32, 32, 0 },
  // 565 shift left
  { 1,  0,  0xffffffff, 0, 0,  0 },
  { 32, 32, 0xffffffff, 31, 32, 0 },
  // 8888 sprite
  { 0,  0,  0xffffffff, 0,  0,  0 },
  { 16, 16, 0xffffffff, 16, 16, 0 }
};

#define rightShift565   &(bSprites[0])
#define leftShift565    &(bSprites[2])
#define bSprite         &(bSprites[4])

u32* produceBewitchedBlending(
  const u32* const tex0, const u32* const tex1,
  const u32 width, const u32 height,
  const u32 alphaIntensity, const int alphaConstant) {
  
  const u32 width565 = width * 2;
  const u32 fbw = (width + 63) & ~63;
  const u32 fbw565 = fbw * 2;
  
  const u32 buffer[4] = {
    MIXTURE_BASE,
    MIXTURE_BASE + BTEX_BYTE_COUNT,
    MIXTURE_BASE + BTEX_BYTE_COUNT * 2,
    MIXTURE_BASE + BTEX_BYTE_COUNT * 3,
  };
  
  PspGeContext context __attribute__((aligned(16)));
  sceGeSaveContext(&context);
  
  sceGuStart(GU_DIRECT, list);
  
  sceGuClearColor(0);
  sceGuEnable(GU_SCISSOR_TEST);
  sceGuEnable(GU_TEXTURE_2D);

  // force alpha value if alphaConstant param >= 0
  if (alphaConstant < 0) {

    // shift alpha components to green channel
    sceGuTexMode(GU_PSM_5650, 0, 1, 0);
    sceGuScissor(0, 0, width565, height);

    // shifting over texture 0
    sceGuDrawBuffer(GU_PSM_5650, (void*)(buffer[0]), fbw565);
    sceGuTexImage(0, width565, height, width565, tex0);
    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 |
    GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, leftShift565);

    // shifting over texture 1
    sceGuDrawBuffer(GU_PSM_5650, (void*)(buffer[1]), fbw565);
    sceGuTexImage(0, width565, height, width565, tex1);
    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 |
    GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, leftShift565);
    
    sceGuTexSync();
      
    // forcing alpha before blending the green component as our produced real alpha
    // then blend alpha components of texture 0 and texture 1 through green channel
    sceGuTexMode(GU_PSM_8888, 0, 1, 0);
    sceGuEnable(GU_BLEND);
    const u32 fix = (alphaIntensity << 8);
    sceGuBlendFunc(GU_ADD, GU_FIX, GU_FIX, fix, fix);

    sceGuDrawBuffer(GU_PSM_8888, (void*)(buffer[2]), fbw);
    sceGuScissor(0, 0, width, height);
    sceGuClear(GU_COLOR_BUFFER_BIT);

    sceGuTexImage(0, width, height, fbw, (void*)(0x04000000 | buffer[0]));
    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 |
    GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, bSprite);

    sceGuTexImage(0, width, height, fbw, (void*)(0x04000000 | buffer[1]));
    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 |
    GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, bSprite);
    
    sceGuTexSync();


    // shift back the final alpha component
    sceGuTexMode(GU_PSM_5650, 0, 1, 0);
    sceGuDisable(GU_BLEND);
     
    sceGuDrawBuffer(GU_PSM_5650, (void*)(buffer[0]), fbw565);
    sceGuScissor(0, 0, width565, height);
    sceGuClear(GU_COLOR_BUFFER_BIT);
    
    sceGuTexImage(0, width565, height, fbw565, (void*)(0x04000000 | buffer[2]));
    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 |
    GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, rightShift565);

    sceGuTexSync();
  }

  // blend RGB components of texture 0 and texture 1
  sceGuTexMode(GU_PSM_8888, 0, 1, 0);
  sceGuEnable(GU_BLEND);
  sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

  sceGuDrawBuffer(GU_PSM_8888, (void*)(buffer[0]), fbw);
  sceGuScissor(0, 0, width, height);
  
  if (alphaConstant >= 0) {
    sceGuClearStencil(0xFF);
    sceGuClear(GU_STENCIL_BUFFER_BIT | GU_COLOR_BUFFER_BIT);
  } else {
    sceGuClear(GU_COLOR_BUFFER_BIT);
  }
  
  sceGuTexImage(0, width, height, width, tex0);
  sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 |
  GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, bSprite);

  sceGuTexImage(0, width, height, width, tex1);
  sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 |
  GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, bSprite);
  
  sceGuTexSync();

  sceGuFinish();
  sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
  
  sceGeRestoreContext(&context);
  
  return (u32*)(0x44000000 | buffer[0]);
}

#define SPRITE_SIZE 128
#define SPRITE_SPACE 16

Vertex __attribute__((aligned(4))) sprite[4] = {
  { 0,  0,  0, 0,                              0,           0 },
  { 16, 16, 0, SPRITE_SIZE,                    SPRITE_SIZE, 0 },
  { 0,  0,  0, SPRITE_SIZE + SPRITE_SPACE,     0,           0 },
  { 16, 16, 0, SPRITE_SIZE * 2 + SPRITE_SPACE, SPRITE_SIZE, 0 }
};

#define sprite0   &(sprite[0])
#define sprite1   &(sprite[2])

int main() {
  int w0, h0, w1, h1;
  u32* const tex0 = (u32*)stbi_load("./tex0.png", &w0, &h0, NULL, STBI_rgb_alpha);
  u32* const tex1 = (u32*)stbi_load("./tex1.png", &w1, &h1, NULL, STBI_rgb_alpha);
  if (!tex0 || !tex1) {
    sceKernelExitGame();
  }

  sceKernelDcacheWritebackRange(tex0, (w0 * h0 * 4 + 63) & ~63);
  sceKernelDcacheWritebackRange(tex1, (w1 * h1 * 4 + 63) & ~63);
  
  guConfig(0);
  
  int buff = DRAW_BUF_0;
  
  pspDebugScreenInitEx(0x0, PSP_DISPLAY_PIXEL_FORMAT_8888, 0);
  pspDebugScreenSetTextColor(0);
  pspDebugScreenEnableBackColor(0);
  pspDebugScreenSetOffset(buff);
  
  u32* const bTex0 = (u32*)memalign(16, BTEX_BYTE_COUNT);
  u32* const bTex = produceBewitchedBlending(tex0, tex1, w0, h0, 128, -1);
  
  memcpy(bTex0, bTex, BTEX_BYTE_COUNT);
  sceKernelDcacheWritebackInvalidateRange(bTex0, (BTEX_BYTE_COUNT + 63) & ~63);
   
  u32* const bTex1 = produceBewitchedBlending(tex1, tex0, w1, h1, 128, -1);
  
  guConfig(1);

  SceCtrlData ctl;
  do {
    sceCtrlPeekBufferPositive(&ctl, 1);

    sceGuStart(GU_DIRECT, list);
    
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

    sceGuTexImage(0, h0, w0, 64, bTex0);
    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 |
    GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, sprite0);
    
    sceGuTexImage(0, h1, w1, 64, bTex1);
    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 |
    GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, sprite1);
    
    sceGuTexSync();

    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
    
    pspDebugScreenSetXY(1, 20);
    pspDebugScreenPrintf("bTex0 pixel debug %x", bTex0[0]);
    pspDebugScreenSetXY(1, 21);
    pspDebugScreenPrintf("bTex1 pixel debug %x", bTex1[0]);

    const u32 offset = (buff == DRAW_BUF_0) ? DRAW_BUF_1 : DRAW_BUF_0;
    pspDebugScreenSetOffset(offset);
    
    sceDisplayWaitVblankStart();
    buff = (int)sceGuSwapBuffers();
  } while(!(ctl.Buttons & PSP_CTRL_HOME));
  
  sceGuTerm();
  sceKernelExitGame();
  return 0;
}
