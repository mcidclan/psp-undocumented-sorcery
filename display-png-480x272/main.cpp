#include <pspgu.h>
#include <psppower.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <pspctrl.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_THREAD_LOCALS
#include <stb_image.h>

#define BUF_WIDTH     512
#define SCR_WIDTH     480
#define SCR_HEIGHT    272

PSP_MODULE_INFO("gu-display-tex", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

struct Vertex {
  u16 u, v;
  u32 color;
  u16 x, y, z;
} __attribute__((aligned(4), packed));

//or
/*
struct VertexF {
  float u, v;
  u32 color;
  float x, y, z;
} __attribute__((aligned(4), packed));
...
sceGuDrawArray(GU_SPRITES, GU_TEXTURE_32BITF | GU_COLOR_8888 |
  GU_VERTEX_32BITF | GU_TRANSFORM_2D, 2, NULL, sprite);
*/

static unsigned int __attribute__((aligned(4))) list[1024] = {0};

void guInit() {
  sceGuInit();
  sceGuStart(GU_DIRECT, list);
  sceGuDrawBuffer(GU_PSM_8888, (void*)0, BUF_WIDTH);
  sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void*)0x88000, BUF_WIDTH);
  sceGuDepthBuffer((void*)0x110000, BUF_WIDTH);
  sceGuClearColor(0xff404040);
  sceGuDisable(GU_SCISSOR_TEST);
  sceGuDisable(GU_DEPTH_TEST);
  sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
  sceGuTexMode(GU_PSM_8888, 0, 1, 0);
  sceGuTexWrap(GU_CLAMP, GU_CLAMP);
  sceGuEnable(GU_TEXTURE_2D);
  sceGuDisplay(GU_TRUE);
  sceGuFinish();
  sceGuSync(0,0);
}

Vertex __attribute__((aligned(4))) sprite[2] = {
  { 0, 0, 0xffff00ff, 0, 0, 0 },
  { 480, 272, 0xffff00ff, 480, 272, 0 }
};

int main() {
  int w, h;
  u32* const tex = (u32*)stbi_load("./preview.png", &w, &h, NULL, STBI_rgb_alpha);
  if(!tex) {
    sceKernelExitGame();
  }
  sceKernelDcacheWritebackInvalidateAll();
  
  guInit();
  SceCtrlData ctl;
  do {
    
    sceCtrlPeekBufferPositive(&ctl, 1);
    sceGuStart(GU_DIRECT, list);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
    // width & height (next higher power of 2), buffer stride (effective pixel width)
    sceGuTexImage(0, 512, 512, 480, tex);
    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 |
      GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, sprite);
    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
  } while(!(ctl.Buttons & PSP_CTRL_HOME));
  // Clean and exit
  sceGuTerm();
  stbi_image_free(tex);
  sceKernelExitGame();
  return 0;
}
