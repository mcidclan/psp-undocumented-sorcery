#include <psppower.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <pspsdk.h>
#include <pspgu.h>
#include <pspgum.h>

static inline void unlockMemory() {
  const unsigned int start = 0xbc000000;
  const unsigned int end   = 0xbc00002c;
  for (unsigned int reg = start; reg <= end; reg += 4) {
    (*((volatile unsigned int*)(reg))) = -1;
  }
  asm volatile("sync");
}

#define BUF_WIDTH   512
#define SCR_WIDTH   480
#define SCR_HEIGHT  272

PSP_MODULE_INFO("vsh-cube-overlay", 0x1007, 1, 1);
PSP_NO_CREATE_MAIN_THREAD();
PSP_HEAP_SIZE_KB(1024*4);

struct Vertex {
  u32 color;
  float x, y, z;
} __attribute__((aligned(4), packed));


constexpr u32 CUBE_VERT_COUNT = 36;

struct Vertex __attribute__((aligned(4))) cube[CUBE_VERT_COUNT] = {
  { 0xFF808080,  1.0f, -1.0f,  1.0f },
  { 0xFF808080, -1.0f,  1.0f,  1.0f },
  { 0xFF808080, -1.0f, -1.0f,  1.0f },
  { 0xFF808080,  1.0f,  1.0f,  1.0f },
  { 0xFF808080, -1.0f,  1.0f,  1.0f },
  { 0xFF808080,  1.0f, -1.0f,  1.0f },
  //
  { 0xFFFFFFFF, -1.0f,  1.0f, -1.0f },
  { 0xFFFFFFFF,  1.0f, -1.0f, -1.0f },
  { 0xFFFFFFFF, -1.0f, -1.0f, -1.0f },
  { 0xFFFFFFFF, -1.0f,  1.0f, -1.0f },
  { 0xFFFFFFFF,  1.0f,  1.0f, -1.0f },
  { 0xFFFFFFFF,  1.0f, -1.0f, -1.0f },
  //
  { 0xFFFF00FF, -1.0f, -1.0f, -1.0f },
  { 0xFFFF00FF, -1.0f, -1.0f,  1.0f },
  { 0xFFFF00FF, -1.0f,  1.0f, -1.0f },
  { 0xFFFF00FF, -1.0f, -1.0f,  1.0f },
  { 0xFFFF00FF, -1.0f,  1.0f,  1.0f },
  { 0xFFFF00FF, -1.0f,  1.0f, -1.0f },
  //
  { 0xFFFFFF00,  1.0f, -1.0f, -1.0f },
  { 0xFFFFFF00,  1.0f,  1.0f, -1.0f },
  { 0xFFFFFF00,  1.0f, -1.0f,  1.0f },
  { 0xFFFFFF00,  1.0f, -1.0f,  1.0f },
  { 0xFFFFFF00,  1.0f,  1.0f, -1.0f },
  { 0xFFFFFF00,  1.0f,  1.0f,  1.0f },
  //
  { 0xFF00FFFF,  1.0f, -1.0f,  1.0f },
  { 0xFF00FFFF, -1.0f, -1.0f,  1.0f },
  { 0xFF00FFFF,  1.0f, -1.0f, -1.0f },
  { 0xFF00FFFF,  1.0f, -1.0f, -1.0f },
  { 0xFF00FFFF, -1.0f, -1.0f,  1.0f },
  { 0xFF00FFFF, -1.0f, -1.0f, -1.0f },
  //
  { 0xFF00D0D0, -1.0f,  1.0f,  1.0f },
  { 0xFF00D0D0,  1.0f,  1.0f, -1.0f },
  { 0xFF00D0D0, -1.0f,  1.0f, -1.0f },
  { 0xFF00D0D0, -1.0f,  1.0f,  1.0f },
  { 0xFF00D0D0,  1.0f,  1.0f,  1.0f },
  { 0xFF00D0D0,  1.0f,  1.0f, -1.0f },
};

char done = 0;
SceUID blockId;

/* Todo: manage sleep /!\ */
int thread(SceSize ags, void *agp) {
  blockId = sceKernelAllocPartitionMemory(PSP_MEMORY_PARTITION_USER, "list_block", PSP_SMEM_Low, 2048, NULL);
  void* list = (void*)sceKernelGetBlockHeadAddr(blockId);
  
  pspDebugScreenInitEx(0, PSP_DISPLAY_PIXEL_FORMAT_8888, 1);
  pspDebugScreenEnableBackColor(0);
  
  void *frame = NULL;
  int width, format;

  sceGuInit();

  do {
    sceDisplayGetFrameBuf(&frame, &width, &format, 0);

    if (frame) {
      pspDebugScreenSetBase((u32*)(0x40000000 | (u32)frame));
      
      PspGeContext context __attribute__((aligned(16)));
      sceGeSaveContext(&context);
  
      sceGuStart(GU_DIRECT, list);
      
      sceGuDepthBuffer((void*)0x178000, BUF_WIDTH);
      sceGuDepthRange(65535, 0);
      sceGuClearDepth(0.0f);
      sceGuDepthFunc(GU_GEQUAL);
      sceGuEnable(GU_DEPTH_TEST);

      sceGuOffset(2048 - (SCR_WIDTH/2), 2048 - (SCR_HEIGHT/2));
      sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);
      sceGuDisable(GU_CULL_FACE);

      sceGuDrawBuffer(GU_PSM_8888, frame, BUF_WIDTH);

      sceGuDisable(GU_BLEND);
      sceGuScissor(0, 0, 480, 272);
      sceGuEnable(GU_SCISSOR_TEST);
      sceGuClear(GU_DEPTH_BUFFER_BIT);

      
      sceGumMatrixMode(GU_PROJECTION);
      sceGumLoadIdentity();
      sceGumPerspective(50.0f, 16.0f/9.0f, 1.0f, 100.0f);

      sceGumMatrixMode(GU_VIEW);
      sceGumLoadIdentity();

      sceGumMatrixMode(GU_MODEL);
      sceGumLoadIdentity();
      
      {
        static float deg = 45.0f;
        constexpr float rad = 3.14f/180.0f;
        const ScePspFVector3 t0 = {3.0f, -1.0f, -6.0f};
        sceGumTranslate(&t0);
        sceGumRotateY(rad * deg * 1.2f);
        sceGumRotateZ(rad * deg * 1.4f);
        deg += 0.25f;
      }
      
      sceGumDrawArray(GU_TRIANGLES, GU_COLOR_8888 |
        GU_VERTEX_32BITF | GU_TRANSFORM_3D, CUBE_VERT_COUNT, nullptr, cube);
      
      sceGuFinish();
      sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);

      sceGeRestoreContext(&context);
      
      pspDebugScreenSetXY(1, 31);
      pspDebugScreenKprintf("GU cube on VSH");
    }
  
    sceDisplayWaitVblank();
    sceKernelDelayThread(1);

  } while (!done);
    
  sceGuTerm();
  sceKernelFreePartitionMemory(blockId);
  sceKernelExitDeleteThread(0);
  
  return 0;
}

int module_start(SceSize ags, void *agp) {
  unlockMemory();
  SceUID id = sceKernelCreateThread("thread", thread, 0x12, 0x10000, 0, NULL);
  if (id >= 0) {
    sceKernelStartThread(id, 0, NULL);
  }
  return 0;
}

int module_stop(SceSize args, void *argp) {
  done = 1;
  return 0;
}

void* getModuleInfo(void) {
  return (void *) &module_info;
}
