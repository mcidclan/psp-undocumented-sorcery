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
PSP_HEAP_SIZE_KB(2048);

struct Vertex {
  u32 color;
  float x, y, z;
} __attribute__((aligned(4), packed));

char done = 0;
const u32 CUBE_VERT_COUNT = 36;
void sctrlHENPatchSyscall(void *addr, void *newaddr);
u32 sctrlHENFindFunction(char *modname, char *libname, u32 nid);

void* hook(char* mod, char* lib, u32 nid, void* hf) {
  unsigned int* const f = (unsigned int*)sctrlHENFindFunction(mod, lib, nid);
  if (f) {
    sctrlHENPatchSyscall(f, hf);
    return f;
  }
  return NULL;
}

int (*_displaySetFrameBuf)(void*, int, int, int);

void* list;
struct Vertex* cube;
PspGeContext* ctx;

int displaySetFrameBuf(void *frame, int bufferwidth, int pixelformat, int sync) {
  
  pspDebugScreenSetBase((u32*)(0x40000000 | (u32)frame));
  pspDebugScreenSetXY(41, 4);
  pspDebugScreenKprintf("GU cube on VSH");

  sceGeSaveContext(ctx);

  int intr = sceKernelCpuSuspendIntr();
  sceGuStart(GU_DIRECT, list);

  sceGuDepthBuffer((void*)0x1a8000, BUF_WIDTH);
  sceGuDepthRange(65535, 0);
  sceGuClearDepth(0.0f);
  sceGuDepthFunc(GU_GEQUAL);
  sceGuEnable(GU_DEPTH_TEST);

  sceGuOffset(2048 - (SCR_WIDTH/2), 2048 - (SCR_HEIGHT/2));
  sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);
  sceGuDisable(GU_CULL_FACE);

  sceGuDrawBuffer(GU_PSM_8888, frame, BUF_WIDTH);

  // sceGuBlendFunc(GU_ADD, GU_DST_COLOR, GU_FIX, 0, 0x606060);
  // sceGuEnable(GU_BLEND);
  
  sceGuDisable(GU_BLEND);
  sceGuDisable(GU_CULL_FACE);
  
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
    const float rad = 3.14f/180.0f;
    const ScePspFVector3 t0 = {7.8f, 3.0f, -12.0f};
    sceGumTranslate(&t0);
    sceGumRotateY(rad * deg * 1.2f);
    sceGumRotateZ(rad * deg * 1.4f);
    deg += 0.25f;
  }

  sceGumDrawArray(GU_TRIANGLES, GU_COLOR_8888 |
  GU_VERTEX_32BITF | GU_TRANSFORM_3D, CUBE_VERT_COUNT, NULL, cube);

  sceGuFinish();
  sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
  
  sceKernelCpuResumeIntr(intr);
  sceGeRestoreContext(ctx);
  
  return _displaySetFrameBuf(frame, bufferwidth, pixelformat, sync);
}

int thread(SceSize ags, void *agp) {  
  SceUID listId = sceKernelAllocPartitionMemory(PSP_MEMORY_PARTITION_USER, "list_block", PSP_SMEM_Low, 2048+16, NULL);
  list = (void*)(((unsigned int)sceKernelGetBlockHeadAddr(listId) + 15) & ~15);

  SceUID ctxId = sceKernelAllocPartitionMemory(PSP_MEMORY_PARTITION_KERNEL, "ctx_block", PSP_SMEM_Low, sizeof(PspGeContext)+16, NULL);
  ctx = (void*)(((unsigned int)sceKernelGetBlockHeadAddr(ctxId) + 15) & ~15);

  const u32 cubeSize = CUBE_VERT_COUNT * sizeof(struct Vertex);
  SceUID cubeId = sceKernelAllocPartitionMemory(PSP_MEMORY_PARTITION_USER, "cube_block", PSP_SMEM_Low, cubeSize + 64, NULL);
  cube = (struct Vertex*)(((unsigned int)sceKernelGetBlockHeadAddr(cubeId) + 3) & ~3);
  
  cube[0]  = (struct Vertex){ 0xFF808080,  1.0f, -1.0f,  1.0f };
  cube[1]  = (struct Vertex){ 0xFF808080, -1.0f,  1.0f,  1.0f };
  cube[2]  = (struct Vertex){ 0xFF808080, -1.0f, -1.0f,  1.0f };
  cube[3]  = (struct Vertex){ 0xFF808080,  1.0f,  1.0f,  1.0f };
  cube[4]  = (struct Vertex){ 0xFF808080, -1.0f,  1.0f,  1.0f };
  cube[5]  = (struct Vertex){ 0xFF808080,  1.0f, -1.0f,  1.0f };
  //
  cube[6]  = (struct Vertex){ 0xFFF0FFD0, -1.0f,  1.0f, -1.0f },
  cube[7]  = (struct Vertex){ 0xFFF0FFD0,  1.0f, -1.0f, -1.0f };
  cube[8]  = (struct Vertex){ 0xFFF0FFD0, -1.0f, -1.0f, -1.0f };
  cube[9]  = (struct Vertex){ 0xFFF0FFD0, -1.0f,  1.0f, -1.0f };
  cube[10] = (struct Vertex){ 0xFFF0FFD0,  1.0f,  1.0f, -1.0f };
  cube[11] = (struct Vertex){ 0xFFF0FFD0,  1.0f, -1.0f, -1.0f };
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
  
  _displaySetFrameBuf = hook("sceDisplay_Service", "sceDisplay", 0x289D82FE, (void*)displaySetFrameBuf);
  
  do {
    sceKernelDelayThread(10000);
  } while (!done);

  sceGuTerm();
  sceKernelFreePartitionMemory(listId);
  sceKernelFreePartitionMemory(ctxId);
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
