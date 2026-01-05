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

PSP_MODULE_INFO("gu-vram-tex-512", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

struct Vertex {
  u16 u, v;
  u32 color;
  u16 x, y, z;
} __attribute__((aligned(4), packed));

static unsigned int __attribute__((aligned(4))) list[1024] = {0};

void guInit() {
  sceGuInit();
  sceGuStart(GU_DIRECT, list);
  sceGuDrawBuffer(GU_PSM_8888, (void*)0, BUF_WIDTH);
  sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void*)0, BUF_WIDTH);
  
  sceGuDisable(GU_DEPTH_TEST);
  
  sceGuClearColor(0xff404040);
  sceGuDisable(GU_SCISSOR_TEST);
  
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
  { 512, 512, 0xffff00ff, 256, 256, 0 }
};

int main() {
  int w, h;
  u32* const tex = (u32*)stbi_load("./512.png", &w, &h, NULL, STBI_rgb_alpha);
  
  if(!tex) {
    sceKernelExitGame();
  }

  sceKernelDcacheWritebackAll();
  
  const u32* vram512 = (u32*)(0x44000000 + 0x88000);

  {
    sceGuStart(GU_DIRECT, list);
    sceGuCopyImage(GU_PSM_8888,
      0, 0, 512, 512, 512, (void*)tex,
      0, 0, 512, (void*)vram512
    );
    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
  }
  
  stbi_image_free(tex);

  guInit();
  SceCtrlData ctl;
  
  {
    sceGuStart(GU_DIRECT, list);
    sceGuClear(GU_COLOR_BUFFER_BIT);

    sceGuTexImage(0, 512, 512, 512, vram512);

    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 |
      GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, sprite);
    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
  }

  do {
    sceCtrlPeekBufferPositive(&ctl, 1);
  } while(!(ctl.Buttons & PSP_CTRL_HOME));

  sceGuTerm();
  sceKernelExitGame();
  return 0;
}
