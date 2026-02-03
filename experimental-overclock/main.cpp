#include <psppower.h>
#include <pspsdk.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <psprtc.h>
#include "kcall.h"
#include "main.h"

PSP_MODULE_INFO("exp-overclock", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

#define setOverclock()                                                         \
  sceKernelIcacheInvalidateAll();                                              \
  kcall((int (*)(void))(0x80000000 | (unsigned int)_setOverclock));

#define cancelOverclock()                                                      \
  sceKernelIcacheInvalidateAll();                                              \
  kcall((int (*)(void))(0x80000000 | (unsigned int)_cancelOverclock));

// modify this value to compare results
#define DEFAULT_FREQUENCY     333
#define THEORETICAL_FREQUENCY 444 /*for 2k+*/
//#define THEORETICAL_FREQUENCY 370 /*for 1k*/
#define PLL_MUL_MSB           0x0124
#define PLL_BASE_FREQ         37
#define PLL_DEN               19 /*21*/
#define PLL_RATIO_INDEX       5
#define PLL_RATIO             1.0f

// Note: Only tested on Slim, should be called from SC side
// 444 ok - approaching the stability limit
// float a = (float)(THEORETICAL_FREQUENCY / BASE)
// PLL_DEN = (int)((255.0f / a) * PLL_RATIO)
// PLL_NUM = (int)(((float)(THEORETICAL_FREQUENCY * PLL_DEN)) / (PLL_BASE_FREQ * PLL_RATIO_VALUE))
// PLL_FREQ = PLL_BASE_FREQ * (PLL_NUM / PLL_DEN) * PLL_RATIO, with PLL_BASE_FREQ = 37 and PLL_RATIO = 1.0f

int _setOverclock() {
  
  u32 _num = (u32)(((float)(DEFAULT_FREQUENCY * PLL_DEN)) / (PLL_BASE_FREQ * PLL_RATIO));
  const u32 num = (u32)(((float)(THEORETICAL_FREQUENCY * PLL_DEN)) / (PLL_BASE_FREQ * PLL_RATIO));

  // note: needs to be 333 to be able to reach 444mhz
  scePowerSetClockFrequency(DEFAULT_FREQUENCY, DEFAULT_FREQUENCY, DEFAULT_FREQUENCY/2);

  int intr;
  suspendCpuIntr(intr);
  resetDomains();
  
  // set bit bit 7 to apply index, wait until hardware clears it
  hw(0xbc100068) = 0x80 | PLL_RATIO_INDEX;
  do {
    delayPipeline();
  } while (hw(0xbc100068) != PLL_RATIO_INDEX);
  
  // loop until the numerator reaches the target value,
  // and so, progressively increasing clock frequencies
  while (_num <= num) {
    const u32 lsb = _num << 8 | PLL_DEN;
    const u32 multiplier = (PLL_MUL_MSB << 16) | lsb;
    hw(0xbc1000fc) = multiplier;
    delayPipeline(); /*sync();*/
    _num += 1;
  }
  
  hw(0xbc1000fc) |= (1 << 16);
  
  settle();
  resetDomains();
  resumeCpuIntr(intr);

  return 0;
}

void _cancelOverclock() {
  
  u32 _num = (u32)(((float)(THEORETICAL_FREQUENCY * PLL_DEN)) / (PLL_BASE_FREQ * PLL_RATIO));
  const unsigned int num = (u32)(((float)(DEFAULT_FREQUENCY * PLL_DEN)) / (PLL_BASE_FREQ * PLL_RATIO));
  
  int intr;
  suspendCpuIntr(intr);
  
  const u32 pllMul = hw(0xbc1000fc);
  sync();

  const int overclocked = pllMul & (1 << 16);
  
  if (overclocked) {
  
    resetDomains();
    
    while (_num > num) {
      _num -= 1;
      const u32 lsb = _num << 8 | PLL_DEN;
      const u32 multiplier = (PLL_MUL_MSB << 16) | lsb;
      hw(0xbc1000fc) = multiplier;
      delayPipeline(); /*sync();*/
    }
    
    hw(0xbc100068) = 0x80 | PLL_RATIO_INDEX;
    do {
      delayPipeline();
    } while (hw(0xbc100068) != PLL_RATIO_INDEX);
    
    settle();
    resetDomains();
  }
  
  resumeCpuIntr(intr);
}

#define dumpPLLRegs()                                                          \
  sceKernelIcacheInvalidateAll();                                              \
  kcall((int (*)(void))(0x80000000 | (unsigned int)_dumpPLLRegs));

unsigned int pllCtl = 0;
unsigned int pllMul = 0;
int _dumpPLLRegs() {
  pllCtl = hw(0xbc100068);
  pllMul = hw(0xbc1000fc);
  sync();
  return 0;
}

int main() {
  pspDebugScreenInit();
  pspDebugScreenSetXY(1, 1);
  
  if (pspSdkLoadStartModule("./kcall.prx", PSP_MEMORY_PARTITION_KERNEL) < 0) {
    pspDebugScreenPrintf("Can't load kcall prx");
    sceKernelExitGame();
    return 0;
  }

  pspDebugScreenPrintf("Overclock Sample");
  dumpPLLRegs();
  cancelOverclock();
  // setOverclock();
  pspDebugScreenClear();

  u32 switchOverclock = 0;
  unsigned int lastFreq = DEFAULT_FREQUENCY; //THEORETICAL_FREQUENCY;

  u64 prev, now, fps = 0, maxFps = 0, counter = 0;
  const u64 res = sceRtcGetTickResolution();
  
  SceCtrlData ctl;
  
  do {
    sceRtcGetCurrentTick(&prev);
    
    sceCtrlPeekBufferPositive(&ctl, 1);
    maxFps = (fps > maxFps) ? fps : maxFps;
    
    pspDebugScreenSetXY(1, 1);
    pspDebugScreenPrintf("MaxFPS: %llu, FPS: %llu    ", maxFps, fps);
    
    pspDebugScreenSetXY(1, 2);
    pspDebugScreenPrintf("Counter: %llu    ", counter++);
    
    if (!switchOverclock && (ctl.Buttons & PSP_CTRL_TRIANGLE)) {
      const int freq = lastFreq == DEFAULT_FREQUENCY ? THEORETICAL_FREQUENCY : DEFAULT_FREQUENCY;
      if (freq == THEORETICAL_FREQUENCY) {
        setOverclock();
        lastFreq = THEORETICAL_FREQUENCY;
      }
      else {
        cancelOverclock();
        lastFreq = DEFAULT_FREQUENCY;
      }
      switchOverclock = 1;
      maxFps = 0;
    }
    else if(!(ctl.Buttons & PSP_CTRL_TRIANGLE)) {
      switchOverclock = 0;
    }
    
    pspDebugScreenSetXY(1, 3);
    pspDebugScreenPrintf("Switch to %u MHz", lastFreq);
    
    pspDebugScreenSetXY(0, 5);
    pspDebugScreenPrintf("pllCtl: %x    \n", pllCtl);
    pspDebugScreenPrintf("pllMul: %x    \n", pllMul);
    
    //
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
