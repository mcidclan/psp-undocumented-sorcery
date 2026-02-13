#include "main.h"

PSP_MODULE_INFO("pixel-buffer", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

void fill(void *buffer, int width, int height, int stride, unsigned int color, int bpp, int offx, int offy) {
  if (bpp == 32) {
    unsigned int *buf = (unsigned int*)buffer;
    for (int y = 0; y < height; y++) {
      unsigned int *row = buf + (y + offy) * stride + offx;
      for (int x = 0; x < width; x++) {
        row[x] = color;
      }
    }
  } else if (bpp == 16) {
    unsigned short *buf = (unsigned short*)buffer;
    for (int y = 0; y < height; y++) {
      unsigned short *row = buf + (y + offy) * stride + offx;
      for (int x = 0; x < width; x++) {
        row[x] = (unsigned short)color;
      }
    }
  }
}

void* setActiveBuffer(void* buf) {
  
  sceDisplaySetFrameBuf(buf, 512,
  PSP_DISPLAY_PIXEL_FORMAT_8888, PSP_DISPLAY_SETBUF_NEXTFRAME);
  
  pspDebugScreenInitEx(buf, PSP_DISPLAY_PIXEL_FORMAT_8888, 0);
  pspDebugScreenSetOffset(0);
  return buf;
}

int main() {
  void* RAM_BUFFER = memalign(16, 512 * 512 * 4);
  u32 UNCACHED_RAM_BUFFER = UNCACHED_USER_SEG | (u32)RAM_BUFFER;
  
  void* buf = (void*)UNCACHED_RAM_BUFFER;
  pspDebugScreenEnableBackColor(1);
  setActiveBuffer(buf);
    
  u64 prev, now, fps = 0;
  const u64 res = sceRtcGetTickResolution();
  
  u32 counter = 0;
  SceCtrlData ctl;
  do {
    sceRtcGetCurrentTick(&prev);
    sceCtrlPeekBufferPositive(&ctl, 1);

    const int isInRam = (UNCACHED_RAM_BUFFER == (u32)buf) ? 1 : 0;
        
    pspDebugScreenSetXY(0, 0);
    pspDebugScreenPrintf("FPS: %llu     ", fps);
    pspDebugScreenSetXY(40, 0);
    pspDebugScreenPrintf("Using %u      ", counter++);
    pspDebugScreenSetXY(58, 0);
    
    const u32 textColor = isInRam ? 0xff30ff30 : 0xff3030ff;
    pspDebugScreenSetTextColor(textColor);
    pspDebugScreenPrintf("Using %s      ", isInRam ? "HOST" : "VRAM");

    static int switching = 0;
    if (!switching && (ctl.Buttons & PSP_CTRL_TRIANGLE)) {
      
      buf = isInRam ? (void*)UNCACHED_VRAM_BUFFER :
        (void*)(UNCACHED_USER_SEG | (u32)UNCACHED_RAM_BUFFER);
      
      setActiveBuffer(buf);
      switching = 1;
    }
    else if(!(ctl.Buttons & PSP_CTRL_TRIANGLE)) {
      switching = 0;
    }
    
    const u32 color = 0xff302020;
    fill(buf, 480, 272 - 16, 512, color, 32, 0, 16);
    pspDebugScreenSetTextColor(0xffffffff);

    sceRtcGetCurrentTick(&now);
    fps = res / (now - prev);
    
  } while(!(ctl.Buttons & PSP_CTRL_HOME));
  
  free(RAM_BUFFER);
  
  setActiveBuffer((void*)UNCACHED_VRAM_BUFFER);
  pspDebugScreenClear();
  pspDebugScreenSetXY(1, 1);
  pspDebugScreenPrintf("Exiting...");
  sceKernelDelayThread(300000);
  
  sceKernelExitGame();
  return 0;
}
