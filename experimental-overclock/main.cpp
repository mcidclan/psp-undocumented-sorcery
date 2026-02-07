#include <psppower.h>
#include <pspsdk.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <psprtc.h>
#include <pspgu.h>
#include "kcall.h"
#include "main.h"

PSP_MODULE_INFO("exp-overclock", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

#define setOverclock()                                                         \
  kcall((int (*)(void))(0x80000000 | (unsigned int)_setOverclock));

#define cancelOverclock()                                                      \
  kcall((int (*)(void))(0x80000000 | (unsigned int)_cancelOverclock));

#define unlockMemory()                                                         \
  kcall((int (*)(void))(0x80000000 | (unsigned int)_unlockMemory));

#define kernelResetDomains()                                                    \
  kcall((int (*)(void))(0x80000000 | (unsigned int)_resetDomains));

#define DELAY_AFTER_CLOCK_CHANGE 300000

// modify this value to compare results
#define DEFAULT_FREQUENCY         333
#define THEORETICAL_FREQUENCY     444 /*for 2k+*/
//#define THEORETICAL_MAX_FREQUENCY 481

//#define THEORETICAL_FREQUENCY 370 /*for 1k*/
#define PLL_MUL_MSB           0x0124
#define PLL_BASE_FREQ         37
#define PLL_DEN               19 /*20*/
#define PLL_RATIO_INDEX       5
#define PLL_RATIO             1.0f
#define PLL_CUSTOM_FLAG       27 /*16*/

// Note: Only tested on Slim, should be called from SC side
// 444 ok - approaching the stability limit
// float a = (float)(THEORETICAL_FREQUENCY / BASE)
// PLL_DEN = (int)((255.0f / a) * PLL_RATIO)
// PLL_NUM = (int)(((float)(THEORETICAL_FREQUENCY * PLL_DEN)) / (PLL_BASE_FREQ * PLL_RATIO_VALUE))
// PLL_FREQ = PLL_BASE_FREQ * (PLL_NUM / PLL_DEN) * PLL_RATIO, with PLL_BASE_FREQ = 37 and PLL_RATIO = 1.0f

int _setOverclock() {
  
  // note: needs to be 333 to be able to reach 444mhz
  scePowerSetClockFrequency(DEFAULT_FREQUENCY, DEFAULT_FREQUENCY, DEFAULT_FREQUENCY/2);
  
  //resetDomainRatios();
  //hw(0xbc200000) = 511 << 16 | 511;
  //hw(0xBC200004) = 511 << 16 | 511;
  //hw(0xBC200008) = 255 << 16 | 511;
  
  u32 _num = (u32)(((float)(DEFAULT_FREQUENCY * PLL_DEN)) / (PLL_BASE_FREQ * PLL_RATIO));
  const u32 num = (u32)(((float)(THEORETICAL_FREQUENCY * PLL_DEN)) / (PLL_BASE_FREQ * PLL_RATIO));
  //const u32 mnum = (u32)(((float)(THEORETICAL_MAX_FREQUENCY * PLL_DEN)) / (PLL_BASE_FREQ * PLL_RATIO));
  
  //sceGuFinish();
  /*sceGeDrawSync(0);
  do {
    sync();
  } while (hw(0xBD400000) & 2);
  */
  
  vfpuSync();
  sync();

  int intr, state;
  state = sceKernelSuspendDispatchThread();
  suspendCpuIntr(intr);
  
  // set bit bit 7 to apply index
  // then wait until hardware clears bit 7
  // processPLL();
  if ((hw(0xbc100068) & 0xff) != PLL_RATIO_INDEX) {
    hw(0xbc100068) = 0x80 | PLL_RATIO_INDEX; sync();
    do {
      delayPipeline();
    } while (hw(0xbc100068) != PLL_RATIO_INDEX);
  }
  
  // loop until the numerator reaches the target value,
  // and so, progressively increasing clock frequencies
  // increasePLL();
  while (_num <= num) {
    const u32 lsb = _num << 8 | PLL_DEN;
    const u32 multiplier = (PLL_MUL_MSB << 16) | lsb;
    hw(0xbc1000fc) = multiplier;
    sync();
    _num++;
  }
  settle();
  
  /*
  const u32 lsb = num << 8 | PLL_DEN;
  const u32 multiplier = (PLL_MUL_MSB << 16) | lsb;
  hw(0xbc1000fc) = multiplier;
  sync();
  
  settle();
  */
  
  // add custom overclock state flag
  hw(0xbc1000fc) |= (1 << PLL_CUSTOM_FLAG);
  sync();
  
  resumeCpuIntr(intr);
  
  /*
  hw(0xBD400000) |= 2;
  do {
    sync();
  } while(hw(0xBD400000) & 2);
  */
  
  sceKernelResumeDispatchThread(state);
  
  return 0;
}

void _cancelOverclock() {
    
  // scePowerSetClockFrequency(DEFAULT_FREQUENCY, DEFAULT_FREQUENCY, DEFAULT_FREQUENCY/2);

  u32 _num = (u32)(((float)(THEORETICAL_FREQUENCY * PLL_DEN)) / (PLL_BASE_FREQ * PLL_RATIO));
  const unsigned int num = (u32)(((float)(DEFAULT_FREQUENCY * PLL_DEN)) / (PLL_BASE_FREQ * PLL_RATIO));

  vfpuSync();
  sync();

  int intr, state;
  state = sceKernelSuspendDispatchThread();
  suspendCpuIntr(intr);
    
  const u32 pllMul = hw(0xbc1000fc); sync();
  const int overclocked = pllMul & (1 << PLL_CUSTOM_FLAG);
  
  if (overclocked) {
    
    // set bit bit 7 to apply index
    // then wait until hardware clears bit 7
    // processPLL();
    if ((hw(0xbc100068) & 0xff) != PLL_RATIO_INDEX) {
      hw(0xbc100068) = 0x80 | PLL_RATIO_INDEX; sync();
      do {
        delayPipeline();
      } while (hw(0xbc100068) != PLL_RATIO_INDEX);
    }
    // loop until the numerator reaches the target value,
    // and so, progressively increasing clock frequencies
    // decreasePLL();
    while (_num >= num) {
      const u32 lsb = _num << 8 | PLL_DEN;
      const u32 multiplier = (PLL_MUL_MSB << 16) | lsb;
      hw(0xbc1000fc) = multiplier;
      sync();
      _num--;
    }
    settle();
    
    // remove custom overclock state flag
    hw(0xbc1000fc) &= ~((1 << PLL_CUSTOM_FLAG));
    sync();
  }
  
  resumeCpuIntr(intr);
  sceKernelResumeDispatchThread(state);
}

static inline void initOverclock() {
  sceKernelIcacheInvalidateAll();
  unlockMemory();
  
  cancelOverclock();
  sceKernelDelayThread(DELAY_AFTER_CLOCK_CHANGE);
}

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
  sceGuDisable(GU_DEPTH_TEST);
  sceGuDisable(GU_SCISSOR_TEST);
  sceGuDisplay(GU_TRUE);
  sceGuFinish();
  sceGuSync(0,0);
}

int main() {
  pspDebugScreenInit();
  pspDebugScreenSetXY(1, 1);
  
  if (pspSdkLoadStartModule("./kcall.prx", PSP_MEMORY_PARTITION_KERNEL) < 0) {
    pspDebugScreenPrintf("Can't load kcall prx");
    sceKernelExitGame();
    return 0;
  }

  guInit();
  pspDebugScreenInitEx(0x0, PSP_DISPLAY_PIXEL_FORMAT_8888, 0);
  pspDebugScreenEnableBackColor(0);

  int buffer = DRAW_BUF_0;
  pspDebugScreenSetOffset(buffer);
  
  pspDebugScreenPrintf("Overclock Sample");
  initOverclock();
  pspDebugScreenClear();

  u32 switchOverclock = 0;
  unsigned int lastFreq = DEFAULT_FREQUENCY;

  u64 prev, now, fps = 0, maxFps = 0, counter = 0;
  const u64 res = sceRtcGetTickResolution();
  
  int dir = 1;
  int move = 0;
  SceCtrlData ctl;
  
  do {
    sceRtcGetCurrentTick(&prev);
    
    const u32 offset = (buffer == DRAW_BUF_0) ? DRAW_BUF_1 : DRAW_BUF_0;
    pspDebugScreenSetOffset(offset);
    
    sceCtrlPeekBufferPositive(&ctl, 1);
    maxFps = (fps > maxFps) ? fps : maxFps;
    
    pspDebugScreenSetXY(1, 1);
    pspDebugScreenPrintf("MaxFPS: %llu, FPS: %llu    ", maxFps, fps);
    
    pspDebugScreenSetXY(1, 2);
    pspDebugScreenPrintf("Counter: %llu    ", counter++);
    
    if (!switchOverclock && (ctl.Buttons & PSP_CTRL_TRIANGLE)) {
      const int freq = lastFreq == DEFAULT_FREQUENCY ? THEORETICAL_FREQUENCY : DEFAULT_FREQUENCY;
      if (freq == THEORETICAL_FREQUENCY) {
        lastFreq = THEORETICAL_FREQUENCY;
        setOverclock();
      }
      else {
        lastFreq = DEFAULT_FREQUENCY;
        cancelOverclock();
      }
      switchOverclock = 1;
      maxFps = 0;
      sceKernelDelayThread(DELAY_AFTER_CLOCK_CHANGE);
    }
    else if(!(ctl.Buttons & PSP_CTRL_TRIANGLE)) {
      switchOverclock = 0;
    }
    
    pspDebugScreenSetXY(1, 3);
    pspDebugScreenPrintf("Switch to %u MHz", lastFreq);


    sceGuStart(GU_DIRECT, list);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

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
    
    sceGuFinish();
    sceGuSync(0,0);
    
    // sceDisplayWaitVblankStart();
    buffer = (int)sceGuSwapBuffers();
    sceKernelDcacheWritebackInvalidateAll();
    
    sceRtcGetCurrentTick(&now);
    fps = res / (now - prev);
    
  } while(!(ctl.Buttons & PSP_CTRL_HOME));
  
  cancelOverclock();
  
  pspDebugScreenClear();
  pspDebugScreenSetXY(1, 1);
  pspDebugScreenPrintf("Exiting...");
  sceKernelDelayThread(300000);
  
  sceKernelExitGame();
  return 0;
}
