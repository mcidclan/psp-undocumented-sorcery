#include "./main.h"

PSP_MODULE_INFO("logical-simd", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

constexpr u32 DDATA_BUF_0 = DDATA_BUF_BASE;

u32 __attribute__((aligned(16))) list[1024];
u32 __attribute__((aligned(16))) layer0[256];
u32 __attribute__((aligned(16))) layer1[256];

const struct Vert32 data0[] = {
  {0x01101111, 0, 0, 0},
  {0x11111111, 1, 0, 0},
  {0x11111111, 2, 0, 0},
  {0x11111111, 3, 0, 0},
};

const struct Vert16 data1[] = {
  {0x1010, 1, 0, 0}, {0x1111, 0, 0, 0},
  {0x1111, 3, 0, 0}, {0x1111, 2, 0, 0},
  {0x1111, 5, 0, 0}, {0x1111, 4, 0, 0},
  {0x1111, 7, 0, 0}, {0x1111, 6, 0, 0},
};

void guInit() {
  sceGuInit();
  sceGuStart(GU_DIRECT, list);
  
  sceGuDrawBuffer(GU_PSM_8888, (void*)DRAW_BUF_0, BUF_WIDTH);
  sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void*)DRAW_BUF_1, BUF_WIDTH);
  sceGuDepthBuffer((void*)DEPTH_BUF, BUF_WIDTH);
  
  sceGuClearColor(0xFF504040);
  sceGuDisable(GU_SCISSOR_TEST);
  sceGuDisable(GU_STENCIL_TEST);
  sceGuDisable(GU_BLEND);
  sceGuDisable(GU_DEPTH_TEST);
  sceGuDisable(GU_TEXTURE_2D);
  
  sceGuFinish();
  sceGuSync(0,0);
  
  sceGuDisplay(GU_TRUE);
}

/*
*  setup pixel SIMD layers
* */
void setupGuSIMDLayers() {   
  sceGuStart(GU_CALL, layer0);
  sceGuScissor(0, 0, 4, 1);
  sceGuEnable(GU_SCISSOR_TEST);
  // apply a SIMD mask operation that masks the lower 8 bits of each 32-bit word
  sceGuPixelMask(0x000000FF);
  sceGuSendCommandi(CMD_CLEAR, (3 << 8) | 1);
  sceGuDrawArray(GU_POINTS, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 4, 0, data0);
  sceGuSendCommandi(CMD_CLEAR, 0);
  sceGuFinish();
  sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);

  sceGuStart(GU_CALL, layer1);
  sceGuScissor(0, 0, 8, 1);
  // apply SIMD logical operations
  sceGuEnable(GU_COLOR_LOGIC_OP);
  sceGuLogicalOp(LOGICAL_OR);
  sceGuDrawArray(GU_POINTS, GU_COLOR_5650 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 8, 0, data1);
  sceGuFinish();
  sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
}

/*
*  process pixel SIMD layers
* */
void processGuSIMDLayers() {
  PspGeContext context __attribute__((aligned(16)));
  sceGeSaveContext(&context);

  sceGuStart(GU_DIRECT, list);

  // Process the first layer for initial setup and SIMD masking
  // This part writes the 8888 data to the ddata buffer, in the drawing region
  // It is done through clearing mode to be able to affect all bits.
  sceGuDrawBuffer(GU_PSM_8888, (void*)DDATA_BUF_0, 64);
  sceGuCallList(layer0);
  sceGuTexSync();

  // Process the second layer for SIMD logical operations
  // This part forces the GE to switch to 565 drawing mode, to preserve all bits
  // it applies SIMD logical operations on each bit including the 8 high order bits.
  sceGuDrawBuffer(GU_PSM_5650, (void*)DDATA_BUF_0, 64);
  sceGuCallList(layer1);
  sceGuTexSync();

  sceGuFinish();
  sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);

  sceGeRestoreContext(&context);
}

int main() {  
  guInit();
  setupGuSIMDLayers();
  
  int buff = DRAW_BUF_0;
  
  pspDebugScreenInitEx(0x0, PSP_DISPLAY_PIXEL_FORMAT_8888, 0);
  pspDebugScreenEnableBackColor(0);
  pspDebugScreenSetOffset(buff);
    
  SceCtrlData ctl;
  do {
    sceCtrlPeekBufferPositive(&ctl, 1);

    processGuSIMDLayers();
    u32* data = (u32*)(0x04000000 | DDATA_BUF_0);
    
    sceGuStart(GU_DIRECT, list);
    sceGuDrawBuffer(GU_PSM_8888, (void*)buff, BUF_WIDTH);
    sceGuClear(GU_COLOR_BUFFER_BIT);
    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
    
    const u32 offset = (buff == DRAW_BUF_0) ? DRAW_BUF_1 : DRAW_BUF_0;
    
    pspDebugScreenSetOffset(offset);
    pspDebugScreenSetXY(4, 28);
    pspDebugScreenPrintf("word 0: %08X", data[0]);
    pspDebugScreenSetXY(4, 29);    
    pspDebugScreenPrintf("word 1: %08X", data[1]);
    pspDebugScreenSetXY(4, 30);
    pspDebugScreenPrintf("word 2: %08X", data[2]);
    pspDebugScreenSetXY(4, 31);
    pspDebugScreenPrintf("word 3: %08X", data[3]);

    sceDisplayWaitVblankStart();
    buff = (int)sceGuSwapBuffers();    
  } while(!(ctl.Buttons & PSP_CTRL_HOME));
  
  sceGuTerm();
  sceKernelExitGame();
  return 0;
}
