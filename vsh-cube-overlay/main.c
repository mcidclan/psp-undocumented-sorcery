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

char done = 0;
const u32 CUBE_VERT_COUNT = 36;

int thread(SceSize ags, void *agp) {  
  SceUID listId = sceKernelAllocPartitionMemory(PSP_MEMORY_PARTITION_USER, "list_block", PSP_SMEM_Low, 2048+16, NULL);
  void* list = (void*)(((unsigned int)sceKernelGetBlockHeadAddr(listId) + 15) & ~15);

  const u32 cubeSize = CUBE_VERT_COUNT * sizeof(struct Vertex);
  SceUID cubeId = sceKernelAllocPartitionMemory(PSP_MEMORY_PARTITION_USER, "cube_block", PSP_SMEM_Low, cubeSize + 64, NULL);
  struct Vertex* cube = (struct Vertex*)(((unsigned int)sceKernelGetBlockHeadAddr(cubeId) + 3) & ~3);
  
  cube[0] = (struct Vertex){ 0xFF808080,  1.0f, -1.0f,  1.0f };
  cube[1] = (struct Vertex){ 0xFF808080, -1.0f,  1.0f,  1.0f };
  cube[2] = (struct Vertex){ 0xFF808080, -1.0f, -1.0f,  1.0f };
  cube[3] = (struct Vertex){ 0xFF808080,  1.0f,  1.0f,  1.0f };
  cube[4] = (struct Vertex){ 0xFF808080, -1.0f,  1.0f,  1.0f };
  cube[5] = (struct Vertex){ 0xFF808080,  1.0f, -1.0f,  1.0f };
  //
  cube[6] = (struct Vertex){ 0xFFFFFFFF, -1.0f,  1.0f, -1.0f },
  cube[7] = (struct Vertex){ 0xFFFFFFFF,  1.0f, -1.0f, -1.0f };
  cube[8] = (struct Vertex){ 0xFFFFFFFF, -1.0f, -1.0f, -1.0f };
  cube[9] = (struct Vertex){ 0xFFFFFFFF, -1.0f,  1.0f, -1.0f };
  cube[10] = (struct Vertex){ 0xFFFFFFFF,  1.0f,  1.0f, -1.0f };
  cube[11] = (struct Vertex){ 0xFFFFFFFF,  1.0f, -1.0f, -1.0f };
  //
  cube[12] = (struct Vertex){ 0xFFFF00FF, -1.0f, -1.0f, -1.0f };
  cube[13] = (struct Vertex){ 0xFFFF00FF, -1.0f, -1.0f,  1.0f };
  cube[14] = (struct Vertex){ 0xFFFF00FF, -1.0f,  1.0f, -1.0f };
  cube[15] = (struct Vertex){ 0xFFFF00FF, -1.0f, -1.0f,  1.0f };
  cube[16] = (struct Vertex){ 0xFFFF00FF, -1.0f,  1.0f,  1.0f };
  cube[17] = (struct Vertex){ 0xFFFF00FF, -1.0f,  1.0f, -1.0f };
  //
  cube[18] = (struct Vertex){ 0xFFFFFF00,  1.0f, -1.0f, -1.0f };
  cube[19] = (struct Vertex){ 0xFFFFFF00,  1.0f,  1.0f, -1.0f };
  cube[20] = (struct Vertex){ 0xFFFFFF00,  1.0f, -1.0f,  1.0f };
  cube[21] = (struct Vertex){ 0xFFFFFF00,  1.0f, -1.0f,  1.0f };
  cube[22] = (struct Vertex){ 0xFFFFFF00,  1.0f,  1.0f, -1.0f };
  cube[23] = (struct Vertex){ 0xFFFFFF00,  1.0f,  1.0f,  1.0f };
  //
  cube[24] = (struct Vertex){ 0xFF00FFFF,  1.0f, -1.0f,  1.0f };
  cube[25] = (struct Vertex){ 0xFF00FFFF, -1.0f, -1.0f,  1.0f };
  cube[26] = (struct Vertex){ 0xFF00FFFF,  1.0f, -1.0f, -1.0f };
  cube[27] = (struct Vertex){ 0xFF00FFFF,  1.0f, -1.0f, -1.0f };
  cube[28] = (struct Vertex){ 0xFF00FFFF, -1.0f, -1.0f,  1.0f };
  cube[29] = (struct Vertex){ 0xFF00FFFF, -1.0f, -1.0f, -1.0f };
  //
  cube[30] = (struct Vertex){ 0xFF00D0D0, -1.0f,  1.0f,  1.0f };
  cube[31] = (struct Vertex){ 0xFF00D0D0,  1.0f,  1.0f, -1.0f };
  cube[32] = (struct Vertex){ 0xFF00D0D0, -1.0f,  1.0f, -1.0f };
  cube[33] = (struct Vertex){ 0xFF00D0D0, -1.0f,  1.0f,  1.0f };
  cube[34] = (struct Vertex){ 0xFF00D0D0,  1.0f,  1.0f,  1.0f };
  cube[35] = (struct Vertex){ 0xFF00D0D0,  1.0f,  1.0f, -1.0f };

  sceKernelDcacheWritebackRange(cube, (cubeSize + 63) & ~63);

  pspDebugScreenInitEx(0, PSP_DISPLAY_PIXEL_FORMAT_8888, 1);
  pspDebugScreenEnableBackColor(0);

  sceGuInit();
  sceGuDisplay(GU_FALSE);

  void *frame = NULL;
  int width, format; 
  float deg = 45.0f;

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
        const float rad = 3.14f/180.0f;
        const ScePspFVector3 t0 = {3.0f, -1.0f, -6.0f};
        sceGumTranslate(&t0);
        sceGumRotateY(rad * deg * 1.2f);
        sceGumRotateZ(rad * deg * 1.4f);
        deg += 0.25f;
      }
      
      sceGumDrawArray(GU_TRIANGLES, GU_COLOR_8888 |
        GU_VERTEX_32BITF | GU_TRANSFORM_3D, CUBE_VERT_COUNT, NULL, cube);
      
      sceGuFinish();
      sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
      
      sceGeRestoreContext(&context);
      
      pspDebugScreenSetXY(1, 31);
      pspDebugScreenKprintf("GU cube on VSH");
    }
    
    sceDisplayWaitVblank();
    sceKernelDelayThread(10);
  } while (!done);
    
  sceGuTerm();
  sceKernelFreePartitionMemory(listId);
  sceKernelFreePartitionMemory(cubeId);
  sceKernelExitDeleteThread(0);
  
  return 0;
}

int module_start(SceSize ags, void *agp) {
  unlockMemory();
  SceUID id = sceKernelCreateThread("thread", thread, 0x20, 0x10000, 0, NULL);
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
