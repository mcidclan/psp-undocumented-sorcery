// Betwitched texture blending, 'Isolated Alpha' method 
// Copyright mcidclan

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

      sceGuEnable(GU_BLEND);
      sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

      sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
      sceGuTexMode(GU_PSM_8888, 0, 1, 0);
      sceGuTexWrap(GU_CLAMP, GU_CLAMP);
      sceGuEnable(GU_TEXTURE_2D);
      
      sceGuFinish();
      sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);

      sceGuDisplay(GU_TRUE);
      break;
    }
    
    case 1: {
      sceGuStart(GU_DIRECT, list);
      sceGuDrawBuffer(GU_PSM_8888, (void*)DRAW_BUF_0, BUF_WIDTH);
      sceGuClearColor(0xFF5a4c35);
      sceGuFinish();
      sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
      break;
    }
  }
}

/*
 * Bewitched blending between texture 0 and texture 1.
 * The alpha of the output texture is blended.
 */
struct BewitchedBlend {
  u16 width;
  u16 height;
  u32* texture0;
  u32* texture1;
  u16 intensity;
  u16 bufferWidth;
  u32 vramOffset;
  u32 byteCount;
  u32* buffer;
  void* list;
};

void produceBewitchedBlending(BewitchedBlend* const blend) {
  
  const u16 width = blend->width;
  const u16 height = blend->height;
  const u32* const tex0 = blend->texture0;
  const u32* const tex1 = blend->texture1;
  
  const u32 fbw = blend->bufferWidth = (width + 63) & ~63;
  const u32 width565 = width * 2;
  const u32 fbw565 = fbw * 2;
  
  blend->byteCount = fbw * height * 4;
  
  const u32 buffer[3] = {
    0x04000000 | (blend->vramOffset),
    0x04000000 | (blend->vramOffset + blend->byteCount),
    0x04000000 | (blend->vramOffset + blend->byteCount * 2),
  };

  blend->buffer = (u32*)(0x40000000 | buffer[0]);
  
  const Vertex __attribute__((aligned(4))) bSprite[2] = {
    { 0,     0,      0xffffffff, 0,     0,      0 },
    { width, height, 0xffffffff, width, height, 0 }
  };
  sceKernelDcacheWritebackRange(bSprite, (sizeof(Vertex) * 2 + 63) & ~63);
  
  PspGeContext context __attribute__((aligned(16)));
  sceGeSaveContext(&context);
  
  sceGuStart(GU_DIRECT, blend->list);
  
  // shift alpha components to green channel
  sceGuCopyImage(GU_PSM_5650,
    1, 0, width565 - 1, height, width565, (void*)tex0,
    0, 0, fbw565, (void*)buffer[0]
  );
  sceGuCopyImage(GU_PSM_5650,
    1, 0, width565 - 1, height, width565, (void*)tex1,
    0, 0, fbw565, (void*)buffer[1]
  );

  // enable common states
  sceGuClearColor(0);
  sceGuEnable(GU_BLEND);
  sceGuEnable(GU_SCISSOR_TEST);
  sceGuEnable(GU_TEXTURE_2D);
  sceGuScissor(0, 0, width, height);
  sceGuTexMode(GU_PSM_8888, 0, 1, 0);
  
  // forcing alpha before blending the green component as our produced real alpha
  // then blend alpha components of texture 0 and texture 1 through green channel  
  const u32 fix = (blend->intensity << 8);
  sceGuBlendFunc(GU_ADD, GU_FIX, GU_FIX, fix, fix);

  sceGuDrawBuffer(GU_PSM_8888, (void*)(buffer[2]), fbw);
  sceGuClear(GU_COLOR_BUFFER_BIT);

  sceGuTexImage(0, width, height, fbw, (void*)buffer[0]);
  sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 |
  GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, bSprite);

  sceGuTexImage(0, width, height, fbw, (void*)buffer[1]);
  sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 |
  GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, bSprite);
  
  sceGuTexSync();

  // shift back the final alpha component
  sceGuCopyImage(GU_PSM_5650,
    0, 0, width565 - 1, height, fbw565, (void*)buffer[2],
    1, 0, fbw565, (void*)buffer[0]
  );

  // blend RGB components of texture 0 and texture 1
  sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

  sceGuDrawBuffer(GU_PSM_8888, (void*)buffer[0], fbw);
  sceGuClear(GU_COLOR_BUFFER_BIT);
  
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
}

#define BEWITCHED_MIXTURE_BASE  0x178000

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

  BewitchedBlend blend0 = {
    (u16)w0, (u16)h0, tex0, tex1, 255, 0, BEWITCHED_MIXTURE_BASE, 0, NULL, list,
  };
  produceBewitchedBlending(&blend0);
  
  u32* const bTex0 = (u32*)memalign(16, blend0.byteCount);
  memcpy(bTex0, blend0.buffer, blend0.byteCount);
  sceKernelDcacheWritebackInvalidateRange(bTex0, (blend0.byteCount + 63) & ~63);
  
  BewitchedBlend blend1 = {
    (u16)w1, (u16)h1, tex1, tex0, 255, 0, BEWITCHED_MIXTURE_BASE, 0, NULL, list,
  };
  produceBewitchedBlending(&blend1);
  u32* const bTex1 = blend1.buffer;
  
  guConfig(1);

  SceCtrlData ctl;
  do {
    sceCtrlPeekBufferPositive(&ctl, 1);

    sceGuStart(GU_DIRECT, list);
    
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

    sceGuTexImage(0, h0, w0, blend0.bufferWidth, bTex0);
    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 |
    GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, sprite0);
    
    sceGuTexImage(0, h1, w1, blend1.bufferWidth, bTex1);
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
