#include <pspgu.h>
#include <pspgum.h>
#include <psppower.h>
#include <pspdisplay.h>
#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <cstring>
#include <malloc.h>

#define BUF_WIDTH   512
#define SCR_WIDTH   480
#define SCR_HEIGHT  272

PSP_MODULE_INFO("gu-multi-tex", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

template<typename T>
inline T xorshift() {
  static T state = 1;
  T x = state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return state = x;
}

unsigned short int randInRange(const unsigned short int range) {
  unsigned short int x = xorshift<unsigned int>();
  unsigned int m = (unsigned int)x * (unsigned int)range;
  return (m >> 16);
}

struct Vertex {
  float u, v;
  u32 color;
  float x, y, z;
} __attribute__((aligned(4), packed));

static unsigned int __attribute__((aligned(4))) list[1024] = {0};

constexpr u32 QUAD_VERT_COUNT = 12;
constexpr u32 CUBE_VERT_COUNT = 36 - 12;
constexpr u32 CAPS_VERT_COUNT = 6;
constexpr int TEXTURE_SIZE = 128;

/*
 * The quad is split into two bands, each composed of two triangles.
 * By distributing the UV coordinates (particularly along the U axis),
 * the hardware appears to process larger textures more efficiently.
 */
Vertex __attribute__((aligned(4))) vertices[QUAD_VERT_COUNT] = {
  // first split
  { 0.0f, 0.0f, 0xffffffff, 0.0f,              0.0f,         0.0f },
  { 0.0f, 1.0f, 0xffffffff, 0.0f,              TEXTURE_SIZE, 0.0f },
  { 0.5f, 1.0f, 0xffffffff, TEXTURE_SIZE*0.5f, TEXTURE_SIZE, 0.0f },
  { 0.0f, 0.0f, 0xffffffff, 0.0f,              0.0f,         0.0f },
  { 0.5f, 1.0f, 0xffffffff, TEXTURE_SIZE*0.5f, TEXTURE_SIZE, 0.0f },
  { 0.5f, 0.0f, 0xffffffff, TEXTURE_SIZE*0.5f, 0.0f,         0.0f },
  // second split
  { 0.5f, 0.0f, 0xffffffff, TEXTURE_SIZE*0.5f, 0.0f,         0.0f },
  { 0.5f, 1.0f, 0xffffffff, TEXTURE_SIZE*0.5f, TEXTURE_SIZE, 0.0f },
  { 1.0f, 1.0f, 0xffffffff, TEXTURE_SIZE,      TEXTURE_SIZE, 0.0f },
  { 0.5f, 0.0f, 0xffffffff, TEXTURE_SIZE*0.5f, 0.0f,         0.0f },
  { 1.0f, 1.0f, 0xffffffff, TEXTURE_SIZE,      TEXTURE_SIZE, 0.0f },
  { 1.0f, 0.0f, 0xffffffff, TEXTURE_SIZE,      0.0f,         0.0f },
};

Vertex __attribute__((aligned(4))) cube[CUBE_VERT_COUNT] = {
  { 1.0f, 0.0f, 0xFFFFFFFF,  1.0f, -1.0f,  1.0f },
  { 0.0f, 1.0f, 0xFFFFFFFF, -1.0f,  1.0f,  1.0f },
  { 0.0f, 0.0f, 0xFFFFFFFF, -1.0f, -1.0f,  1.0f },
  { 1.0f, 1.0f, 0xFFFFFFFF,  1.0f,  1.0f,  1.0f },
  { 0.0f, 1.0f, 0xFFFFFFFF, -1.0f,  1.0f,  1.0f },
  { 1.0f, 0.0f, 0xFFFFFFFF,  1.0f, -1.0f,  1.0f },
  //
  { 0.0f, 1.0f, 0xFFFFFFFF, -1.0f,  1.0f, -1.0f },
  { 1.0f, 0.0f, 0xFFFFFFFF,  1.0f, -1.0f, -1.0f },
  { 0.0f, 0.0f, 0xFFFFFFFF, -1.0f, -1.0f, -1.0f },
  { 0.0f, 1.0f, 0xFFFFFFFF, -1.0f,  1.0f, -1.0f },
  { 1.0f, 1.0f, 0xFFFFFFFF,  1.0f,  1.0f, -1.0f },
  { 1.0f, 0.0f, 0xFFFFFFFF,  1.0f, -1.0f, -1.0f },
  //
  { 0.0f, 0.0f, 0xFFFFFFFF, -1.0f, -1.0f, -1.0f },
  { 1.0f, 0.0f, 0xFFFFFFFF, -1.0f, -1.0f,  1.0f },
  { 0.0f, 1.0f, 0xFFFFFFFF, -1.0f,  1.0f, -1.0f },
  { 1.0f, 0.0f, 0xFFFFFFFF, -1.0f, -1.0f,  1.0f },
  { 1.0f, 1.0f, 0xFFFFFFFF, -1.0f,  1.0f,  1.0f },
  { 0.0f, 1.0f, 0xFFFFFFFF, -1.0f,  1.0f, -1.0f },
  //
  { 0.0f, 0.0f, 0xFFFFFFFF,  1.0f, -1.0f, -1.0f },
  { 0.0f, 1.0f, 0xFFFFFFFF,  1.0f,  1.0f, -1.0f },
  { 1.0f, 0.0f, 0xFFFFFFFF,  1.0f, -1.0f,  1.0f },
  { 1.0f, 0.0f, 0xFFFFFFFF,  1.0f, -1.0f,  1.0f },
  { 0.0f, 1.0f, 0xFFFFFFFF,  1.0f,  1.0f, -1.0f },
  { 1.0f, 1.0f, 0xFFFFFFFF,  1.0f,  1.0f,  1.0f },
};

Vertex __attribute__((aligned(4))) top[CAPS_VERT_COUNT] = {
  { 1.0f, 1.0f, 0xFF00FFFF,  1.0f, -1.0f,  1.0f },
  { 0.0f, 1.0f, 0xFF00FFFF, -1.0f, -1.0f,  1.0f },
  { 1.0f, 0.0f, 0xFF00FFFF,  1.0f, -1.0f, -1.0f },
  { 1.0f, 0.0f, 0xFF00FFFF,  1.0f, -1.0f, -1.0f },
  { 0.0f, 1.0f, 0xFF00FFFF, -1.0f, -1.0f,  1.0f },
  { 0.0f, 0.0f, 0xFF00FFFF, -1.0f, -1.0f, -1.0f },
};

Vertex __attribute__((aligned(4))) bottom[CAPS_VERT_COUNT] = {
  { 0.0f, 1.0f, 0xFFFFFFFF, -1.0f,  1.0f,  1.0f },
  { 1.0f, 0.0f, 0xFFFFFFFF,  1.0f,  1.0f, -1.0f },
  { 0.0f, 0.0f, 0xFFFFFFFF, -1.0f,  1.0f, -1.0f },
  { 0.0f, 1.0f, 0xFFFFFFFF, -1.0f,  1.0f,  1.0f },
  { 1.0f, 1.0f, 0xFFFFFFFF,  1.0f,  1.0f,  1.0f },
  { 1.0f, 0.0f, 0xFFFFFFFF,  1.0f,  1.0f, -1.0f },
};

void guInit() {
  sceGuInit();
  sceGuStart(GU_DIRECT, list);
  sceGuDrawBuffer(GU_PSM_8888, (void*)0, BUF_WIDTH);
  sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void*)0x88000, BUF_WIDTH);
  sceGuDepthBuffer((void*)0x178000, BUF_WIDTH);

  sceGuClearColor(0xff504040);
  sceGuEnable(GU_SCISSOR_TEST);

  sceGuDepthRange(65535, 0);
  sceGuClearDepth(0.0f);
  sceGuDepthFunc(GU_GEQUAL);
  
  sceGuTexScale(1.0f, 1.0f);		
  sceGuOffset(2048 - (SCR_WIDTH/2), 2048 - (SCR_HEIGHT/2));
  sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);

  sceGuFrontFace(GU_CCW);
  sceGuEnable(GU_CULL_FACE);

  sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
  sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
  sceGuTexMode(GU_PSM_8888, 0, 1, 0);
  sceGuTexWrap(GU_CLAMP, GU_CLAMP);
  sceGuDisplay(GU_TRUE);
  
  sceGumMatrixMode(GU_VIEW);
  sceGumLoadIdentity();
  sceGumMatrixMode(GU_MODEL);
  sceGumLoadIdentity();
  
  sceGuFinish();
  sceGuSync(0,0);
}

int main() {
  scePowerSetClockFrequency(333, 333, 166);

  // Generate textures for multi-texturing
  constexpr int ntex = 3;
  constexpr u32 bSize = TEXTURE_SIZE * TEXTURE_SIZE;
  u32* tex[ntex] = {nullptr};
  for (int i = 0; i < ntex; i++) {
    tex[i] = (u32*)memalign(16, sizeof(u32) * bSize);
  }
  u32 i = 0;
  while (i < bSize) {
    const u32 color = 0xff << 24 | randInRange(256) << 16 | randInRange(256) << 8 | randInRange(256);
    tex[0][i] = color;
    if (!((i/(TEXTURE_SIZE*2)) % 4)) {
      tex[1][i] = 0xd05060ff;
    }
    i++;
  }
  for (int y = 0; y < TEXTURE_SIZE; y++) {
  u8 alpha = 255 - (y * 255 / (TEXTURE_SIZE - 1));
    u32 color = (alpha << 24) | 0x0000FFFF;
    for (int x = 0; x < TEXTURE_SIZE; x++) {
      tex[2][y * TEXTURE_SIZE + x] = color;
    }
  }
  
  guInit();
  int buff = 0;
  SceCtrlData ctl;
  do {
    sceCtrlPeekBufferPositive(&ctl, 1);
    
    /*
     * Switch the rendering buffer to the multi-texturing rendering area,
     * then render the top corner of the scene defined by the scissor rectangle
     */
    sceGuStart(GU_DIRECT, list);
    sceGuScissor(0, 0, TEXTURE_SIZE, TEXTURE_SIZE);
    sceGuDrawBuffer(GU_PSM_8888, (void*)0x110000, TEXTURE_SIZE);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
    
    sceGuEnable(GU_BLEND);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuEnable(GU_TEXTURE_2D);
    sceGuTexFilter(GU_NEAREST, GU_NEAREST);
    
    sceGumMatrixMode(GU_PROJECTION);
    sceGumLoadIdentity();
    sceGumOrtho(0.0f, 480.0f, 272.0f, 0.0f, -1.0f, 1.0f);
    for (u32 i = 0; i < ntex; i++) {
      sceGumMatrixMode(GU_MODEL);
      sceGumLoadIdentity();
      if(i == 1) {
        // Apply a rotation effect to the second layer composing the multitexture
        static float deg = 0.0f;
        constexpr float rad = 3.14f/180.0f;
        constexpr float disp = TEXTURE_SIZE * 0.5f;
        constexpr ScePspFVector3 s0[] = {1.42f, 1.42f, 0.0f};
        constexpr ScePspFVector3 t0[] = {-disp, -disp, 0.0f};
        constexpr ScePspFVector3 t1[] = {disp, disp, 0.0f};
        sceGumTranslate(t1);
        sceGumRotateZ(rad * deg);
        sceGumScale(s0);
        sceGumTranslate(t0);
        deg += 2.0f;
      }
      
      sceGuTexImage(0, TEXTURE_SIZE, TEXTURE_SIZE, TEXTURE_SIZE, tex[i]);
      sceGumDrawArray(GU_TRIANGLES, GU_TEXTURE_32BITF | GU_COLOR_8888 |
        GU_VERTEX_32BITF | GU_TRANSFORM_3D, QUAD_VERT_COUNT, nullptr, vertices);
    }
    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);

    /*
     * Restore rendering/drawing buffer,
     * then use the previous rendering as a texture for the cube.
     */
    sceGuStart(GU_DIRECT, list);
    sceGuScissor(0, 0, 480, 272);
    sceGuDrawBuffer(GU_PSM_8888, (void*)buff, BUF_WIDTH);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
    
    sceGuDisable(GU_BLEND);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuTexFilter(GU_LINEAR, GU_LINEAR);
    
    sceGumMatrixMode(GU_PROJECTION);
    sceGumLoadIdentity();
    sceGumPerspective(60.0f, 16.0f/9.0f, 1.0f, 100.0f);
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
    {
      static float deg = 45.0f;
      constexpr float rad = 3.14f/180.0f;
      const ScePspFVector3 t0[] = {0.0f, 0.0f, -4.5f};
      sceGumTranslate(t0);
      sceGumRotateY(rad * deg * 1.2f);
      sceGumRotateZ(rad * deg * 1.4f);
      deg += 0.25f;
    }
    
    sceGuTexImage(0, TEXTURE_SIZE, TEXTURE_SIZE, TEXTURE_SIZE, (u32*)(0x04000000 | 0x110000));
    sceGumDrawArray(GU_TRIANGLES, GU_TEXTURE_32BITF | GU_COLOR_8888 |
      GU_VERTEX_32BITF | GU_TRANSFORM_3D, CUBE_VERT_COUNT, nullptr, cube);
    
    sceGuTexImage(0, TEXTURE_SIZE, TEXTURE_SIZE, TEXTURE_SIZE, tex[0]);
    sceGumDrawArray(GU_TRIANGLES, GU_TEXTURE_32BITF | GU_COLOR_8888 |
      GU_VERTEX_32BITF | GU_TRANSFORM_3D, CAPS_VERT_COUNT, nullptr, bottom);
    
    sceGuDisable(GU_TEXTURE_2D);
    sceGumDrawArray(GU_TRIANGLES, GU_TEXTURE_32BITF | GU_COLOR_8888 |
      GU_VERTEX_32BITF | GU_TRANSFORM_3D, CAPS_VERT_COUNT, nullptr, top);
    
    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
    
    // Sync and swap buffers
    sceDisplayWaitVblankStart();
    buff = (int)sceGuSwapBuffers();
  } while(!(ctl.Buttons & PSP_CTRL_HOME));
  
  // Clean and exit
  sceGuTerm();
  for (int i = 0; i < ntex; i++) {
    free(tex[i]);
  }
  sceKernelExitGame();
  return 0;
}
