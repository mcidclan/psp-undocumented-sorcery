#include <pspgu.h>
#include <psppower.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <pspgum.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_THREAD_LOCALS
#include <stb_image.h>

#define BUF_WIDTH     512
#define SCR_WIDTH     480
#define SCR_HEIGHT    272

PSP_MODULE_INFO("gu-1024", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

struct Vertex {
  float u, v;
  u32 color;
  float x, y, z;
} __attribute__((aligned(4), packed));

constexpr u32 QUAD_VERT_COUNT = 6*8;
constexpr u32 IMG_SIZE = 1024;
constexpr u32 VIEW_SIZE = 512;

/*
 * The quad is split into four bands, each composed of two triangles.
 * By distributing the UV coordinates (particularly along the U axis),
 * the hardware appears to process larger textures more efficiently.
 */
Vertex __attribute__((aligned(4))) vertices[QUAD_VERT_COUNT] = {
  // first split
  { 0.0f,  0.0f, 0xffffffff, 0.0f,           0.0f,      0.0f },
  { 0.0f,  1.0f, 0xffffffff, 0.0f,           VIEW_SIZE, 0.0f },
  { 0.25f, 1.0f, 0xffffffff, VIEW_SIZE*0.25f, VIEW_SIZE, 0.0f },
  { 0.0f,  0.0f, 0xffffffff, 0.0f,           0.0f,      0.0f },
  { 0.25f, 1.0f, 0xffffffff, VIEW_SIZE*0.25f, VIEW_SIZE, 0.0f },
  { 0.25f, 0.0f, 0xffffffff, VIEW_SIZE*0.25f, 0.0f,      0.0f },
  // second split
  { 0.25f, 0.0f, 0xffffffff, VIEW_SIZE*0.25f, 0.0f,      0.0f },
  { 0.25f, 1.0f, 0xffffffff, VIEW_SIZE*0.25f, VIEW_SIZE, 0.0f },
  { 0.5f,  1.0f, 0xffffffff, VIEW_SIZE*0.5f,  VIEW_SIZE, 0.0f },
  { 0.25f, 0.0f, 0xffffffff, VIEW_SIZE*0.25f, 0.0f,      0.0f },
  { 0.5f,  1.0f, 0xffffffff, VIEW_SIZE*0.5f,  VIEW_SIZE, 0.0f },
  { 0.5f,  0.0f, 0xffffffff, VIEW_SIZE*0.5f,  0.0f,      0.0f },
  // third split
  { 0.5f,  0.0f, 0xffffffff, VIEW_SIZE*0.5f,  0.0f,      0.0f },
  { 0.5f,  1.0f, 0xffffffff, VIEW_SIZE*0.5f,  VIEW_SIZE, 0.0f },
  { 0.75f, 1.0f, 0xffffffff, VIEW_SIZE*0.75f, VIEW_SIZE, 0.0f },
  { 0.5f,  0.0f, 0xffffffff, VIEW_SIZE*0.5f,  0.0f,      0.0f },
  { 0.75f, 1.0f, 0xffffffff, VIEW_SIZE*0.75f, VIEW_SIZE, 0.0f },
  { 0.75f, 0.0f, 0xffffffff, VIEW_SIZE*0.75f, 0.0f,      0.0f },
  // fourth split
  { 0.75f, 0.0f, 0xffffffff, VIEW_SIZE*0.75f, 0.0f,      0.0f },
  { 0.75f, 1.0f, 0xffffffff, VIEW_SIZE*0.75f, VIEW_SIZE, 0.0f },
  { 1.0f,  1.0f, 0xffffffff, VIEW_SIZE,       VIEW_SIZE, 0.0f },
  { 0.75f, 0.0f, 0xffffffff, VIEW_SIZE*0.75f, 0.0f,      0.0f },
  { 1.0f,  1.0f, 0xffffffff, VIEW_SIZE,       VIEW_SIZE, 0.0f },
  { 1.0f,  0.0f, 0xffffffff, VIEW_SIZE,       0.0f,      0.0f },
};

static unsigned int __attribute__((aligned(4))) list[1024] = {0};

void guInit() {
  sceGuInit();
  sceGuStart(GU_DIRECT, list);
  sceGuDrawBuffer(GU_PSM_8888, (void*)0, BUF_WIDTH);
  sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void*)0x88000, BUF_WIDTH);
  sceGuDepthBuffer((void*)0x110000, BUF_WIDTH);
  
  sceGuClearColor(0);
  sceGuEnable(GU_SCISSOR_TEST);
  sceGuScissor(0, 0, 480, 272);
  sceGuDisable(GU_DEPTH_TEST);
  
  sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
  sceGuTexMode(GU_PSM_8888, 0, 1, 0);
  sceGuTexWrap(GU_CLAMP, GU_CLAMP);
  sceGuEnable(GU_TEXTURE_2D);
  sceGuTexScale(1.0f, 1.0f);

  sceGuOffset(2048 - (SCR_WIDTH/2), 2048 - (SCR_HEIGHT/2));
  sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);

  sceGumMatrixMode(GU_VIEW);
  sceGumLoadIdentity();
  
  sceGumMatrixMode(GU_PROJECTION);
  sceGumLoadIdentity();
  sceGumOrtho(-240.0f, 240.0f, 136.0f, -136.0f, -1.0f, 1.0f);
  
  sceGuFinish();
  sceGuSync(0,0);
  
  sceGuDisplay(GU_TRUE);
}

static u32* getTexAddr(const u32* const tex, SceCtrlData& ctl) {
  constexpr u32 MOVE_STEP = 4; // to ensure 16-byte alignment on the x-axis
  static u32 x = IMG_SIZE / 2 - VIEW_SIZE / 2;
  static u32 y = IMG_SIZE / 2 - VIEW_SIZE / 2;
  
  if ((ctl.Buttons & PSP_CTRL_LEFT) && x >= MOVE_STEP) {
    x -= MOVE_STEP;
  }
  else if ((ctl.Buttons & PSP_CTRL_RIGHT) && x <= (IMG_SIZE - VIEW_SIZE - MOVE_STEP)) {
    x += MOVE_STEP;
  }
  
  if ((ctl.Buttons & PSP_CTRL_UP) && y >= MOVE_STEP) {
    y -= MOVE_STEP;
  }
  else if ((ctl.Buttons & PSP_CTRL_DOWN) && y <= (IMG_SIZE - VIEW_SIZE - MOVE_STEP)) {
    y += MOVE_STEP;
  }
  
  return (u32*)&(tex[x + y * IMG_SIZE]);
}

int main() {
  scePowerSetClockFrequency(333, 333, 166);
  
  int w, h;
  u32* const tex = (u32*)stbi_load("./1024.png", &w, &h, NULL, STBI_rgb_alpha);
  if(!tex) {
    sceKernelExitGame();
  }
  guInit();
  SceCtrlData ctl;
  float zoom = 0.5f;
  do {
    sceCtrlPeekBufferPositive(&ctl, 1);
    sceGuStart(GU_DIRECT, list);
    sceGuClear(GU_COLOR_BUFFER_BIT);
    
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();

    if ((ctl.Buttons & PSP_CTRL_CROSS) && zoom > 0.25f) {
      zoom -= 0.01f;
    }
    else if ((ctl.Buttons & PSP_CTRL_TRIANGLE) && zoom < 1.0f) {
      zoom += 0.01f;
    }
  
    const ScePspFVector3 translate = {-256.0f, -256.0f, 0.5f};
    const ScePspFVector3 scale = {zoom, zoom, zoom};
    sceGumScale(&scale);
    sceGumTranslate(&translate);
    
    u32* const texAddr = getTexAddr(tex, ctl);
    
    // width and height of the viewport, buffer stride (effective picture width).
    sceGuTexImage(0, VIEW_SIZE, VIEW_SIZE, IMG_SIZE, texAddr);
    sceGumDrawArray(GU_TRIANGLES, GU_TEXTURE_32BITF | GU_COLOR_8888 |
      GU_VERTEX_32BITF | GU_TRANSFORM_3D, QUAD_VERT_COUNT, NULL, vertices);

    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
  } while(!(ctl.Buttons & PSP_CTRL_HOME));
  
  sceGuTerm();
  stbi_image_free(tex);
  sceKernelExitGame();
  return 0;
}
