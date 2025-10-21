#include <psppower.h>
#include <pspgu.h>
#include <pspdisplay.h>
#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>

PSP_MODULE_INFO("gu-psp-dbg", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

#define BUF_WIDTH   512
#define SCR_WIDTH   480
#define SCR_HEIGHT  272

#define DRAW_BUF_0 0
#define DRAW_BUF_1 0x88000
#define DEPTH_BUF  0x110000

struct Vertex {
  u32 color;
  u16 x, y, z;
} __attribute__((aligned(4)));

static unsigned int __attribute__((aligned(16))) list[1024] = {0};

void guInit() {
  sceGuInit();
  sceGuStart(GU_DIRECT, list);
  sceGuDrawBuffer(GU_PSM_8888, (void*)DRAW_BUF_0, BUF_WIDTH);
  sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void*)DRAW_BUF_1, BUF_WIDTH);
  sceGuDepthBuffer((void*)DEPTH_BUF, BUF_WIDTH);
  sceGuClearColor(0xFF504040);
  sceGuDisable(GU_DEPTH_TEST);
  sceGuDisable(GU_SCISSOR_TEST);
  sceGuDisplay(GU_TRUE);
  sceGuFinish();
  sceGuSync(0,0);
}

int main() {
  guInit();
  
  pspDebugScreenInitEx(0x0, PSP_DISPLAY_PIXEL_FORMAT_8888, 0);
  pspDebugScreenEnableBackColor(0);

  int buffer = DRAW_BUF_0;
  int counter1 = 0;
  int counter2 = 0;
  int counter3 = 0;
  pspDebugScreenSetOffset(buffer);
  
  int dir = 1;
  int move = 0;
  SceCtrlData ctl;
  do {
    sceCtrlPeekBufferPositive(&ctl, 1);

    sceGuStart(GU_DIRECT, list);
    sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
    
    {
      Vertex* const vertices = (Vertex*)sceGuGetMemory(sizeof(Vertex) * 2);
      move += dir;
      if(move > 64) {
        dir = -1;
      } else if(move < -64) {
        dir = 1;
      }
      vertices[0].color = 0;
      vertices[0].x = 176 + move;
      vertices[0].y = 72;
      vertices[0].z = 0;
      vertices[1].color = 0xFF0000FF;
      vertices[1].x = 128 + 176 + move;
      vertices[1].y = 128 + 72;
      vertices[1].z = 0;
      sceGuDrawArray(GU_SPRITES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, nullptr, vertices);
    }
    
    const u32 offset = (buffer == DRAW_BUF_0) ? DRAW_BUF_1 : DRAW_BUF_0;
    pspDebugScreenSetOffset(offset);
    
    pspDebugScreenSetXY(10, 10);
    pspDebugScreenPrintf("Counter 1: %u ", counter1++);
    pspDebugScreenSetXY(10, 11);
    pspDebugScreenPrintf("Counter 2: %u ", counter2+=2);
    pspDebugScreenSetXY(10, 12);
    pspDebugScreenPrintf("Counter 3: %u ", counter3+=4);

    sceGuFinish();
    sceGuSync(0,0);
    
    sceDisplayWaitVblankStart();
    buffer = (int)sceGuSwapBuffers();
  } while(!(ctl.Buttons & PSP_CTRL_HOME));
  
  pspDebugScreenClear();
  pspDebugScreenSetXY(0, 1);
  pspDebugScreenPrintf("Exiting...");
  sceKernelDelayThread(500000);
  sceKernelExitGame();
  return 0;
}
