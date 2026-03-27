#include <psppower.h>
#include <pspgu.h>
#include <pspdisplay.h>
#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <pspintrman.h>
#include <functional>

PSP_MODULE_INFO("gu-deffered-sync", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

#define LIST_SIZE   512
#define BUF_WIDTH   512
#define SCR_WIDTH   480
#define SCR_HEIGHT  272
#define BUF_SIZE    0x44000
#define BUF_COUNT   4
#define DEPTH_BUF   (BUF_SIZE * BUF_COUNT)

std::function<int()> displayDebug;

int readyBufferIndexes[BUF_COUNT];
int readyBufferCursor = 0;
int drawBufferIndex = 1;

typedef struct {
  int pixel_size;
  int frame_width;
  void *frame_buffer;
  void *disp_buffer;
  void *depth_buffer;
  int depth_width;
  int width;
  int height;
} GuDrawBuffer;
 
extern GuDrawBuffer gu_draw_buffer;
extern void* ge_edram_address;

struct Vertex {
  u32 color;
  u16 x, y, z;
} __attribute__((aligned(4)));

static unsigned int __attribute__((aligned(16))) list[LIST_SIZE*BUF_COUNT] = {0};

void updateDisplayBuffer() {
  
  if (readyBufferCursor > 0) {
    const int dispBufferIndex = readyBufferIndexes[0];
  
    for (int i = 0; i < readyBufferCursor - 1; i++) {
      readyBufferIndexes[i] = readyBufferIndexes[i + 1];
    }
    
    readyBufferCursor--;
    gu_draw_buffer.disp_buffer = (void*)(BUF_SIZE*dispBufferIndex);
    if (displayDebug) displayDebug();
    
    sceDisplaySetFrameBuf(
      (void *)(((unsigned int)ge_edram_address) + ((unsigned int)gu_draw_buffer.disp_buffer)),
      BUF_WIDTH, gu_draw_buffer.pixel_size, PSP_DISPLAY_SETBUF_NEXTHSYNC
    );
  }
}

void updateDrawBuffer() {
  
  if (readyBufferCursor < BUF_COUNT) {
    readyBufferIndexes[readyBufferCursor++] = drawBufferIndex;
  }
  drawBufferIndex = (drawBufferIndex + 1 >= BUF_COUNT) ? 0 : drawBufferIndex + 1;
  gu_draw_buffer.frame_buffer = (void*)(BUF_SIZE*drawBufferIndex);
}

void geIntHandler(u32 sub, u32* arg) {
  
  int intr = sceKernelCpuSuspendIntr();
  updateDisplayBuffer();
  sceKernelCpuResumeIntrWithSync(intr);
}

void guInit() {
  
  sceGuInit();
  sceGuStart(GU_DIRECT, list);
  sceGuDrawBuffer(GU_PSM_5650, (void*)BUF_SIZE, BUF_WIDTH);
  sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, 0, BUF_WIDTH);
  sceGuDepthBuffer((void*)DEPTH_BUF, BUF_WIDTH);
  sceGuClearColor(0xff504040);
  sceGuDisable(GU_DEPTH_TEST);
  sceGuDisable(GU_SCISSOR_TEST);
  sceGuDisplay(GU_TRUE);
  sceGuFinish();
  sceGuSync(0, 0);
}

void exit() {
  
  sceKernelReleaseSubIntrHandler(PSP_GE_INT, 1);
  sceKernelExitGame();
}

#define FRAME_TIME_US 1000 // 1000 FPS MAX

int main() {

  sceKernelRegisterSubIntrHandler(PSP_GE_INT, 1, (void*)geIntHandler, nullptr);
  sceKernelEnableSubIntr(PSP_GE_INT, 1);

  guInit();
  sceDisplaySetMode(PSP_DISPLAY_MODE_LCD, gu_draw_buffer.width, gu_draw_buffer.height);
  
  pspDebugScreenInitEx(0x0, PSP_DISPLAY_PIXEL_FORMAT_565, 0);
  pspDebugScreenEnableBackColor(0);
  pspDebugScreenSetOffset(0);

  int dirX = 1; int moveX = 0;
  int dirY = 1; int moveY = 0;
  int fps = 0;
  SceCtrlData ctl;
  do {
    
    u64 start = sceKernelGetSystemTimeWide();
    sceCtrlPeekBufferPositive(&ctl, 1);
    
    sceGuStart(GU_DIRECT, &(list[LIST_SIZE*drawBufferIndex]));
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
    
    {
      Vertex* const vertices = (Vertex*)sceGuGetMemory(sizeof(Vertex) * 2);
      moveX += dirX;
      if(moveX > 64) {
          dirX = -1;
      } else if(moveX < -64) {
          dirX = 1;
      }

      moveY += dirY;
      if(moveY > 64) {
          dirY = -1;
      } else if(moveY < -64) {
          dirY = 1;
      }

      vertices[0].color = 0;
      vertices[0].x = 176 + moveX;
      vertices[0].y = 72 + moveY;
      vertices[0].z = 0;
      vertices[1].color = 0xFF0000FF;
      vertices[1].x = 128 + 176 + moveX;
      vertices[1].y = 128 + 72 + moveY;
      vertices[1].z = 0;
   
      sceGuDrawArray(GU_SPRITES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, nullptr, vertices);
    }
    
    auto debug = [&fps]() -> int {
      const int debugBuffer = (int)gu_draw_buffer.disp_buffer;
      pspDebugScreenSetOffset(debugBuffer);
      pspDebugScreenSetXY(1, 30);
      pspDebugScreenPrintf("fps: %u   ", fps);
      return 0;
    };
    displayDebug = debug;
    
    sceGuFinish();
    updateDrawBuffer();
    
    // if(readyBufferCursor >= BUF_COUNT) exit();

    u64 end = sceKernelGetSystemTimeWide();
    u32 elapsed = (u32)(end - start);
    if (elapsed < FRAME_TIME_US) {
        sceKernelDelayThread(FRAME_TIME_US - elapsed);
    }
    
    end = sceKernelGetSystemTimeWide();
    elapsed = (u32)(end - start);
    fps = (elapsed >= 1) ? 1000000 / elapsed : 0;

  } while(!(ctl.Buttons & PSP_CTRL_HOME));

  pspDebugScreenInit();
  pspDebugScreenClear();
  pspDebugScreenSetXY(0, 1);
  pspDebugScreenPrintf("Exiting...");
  sceKernelDelayThread(200000);
  exit();
  return 0;
}
