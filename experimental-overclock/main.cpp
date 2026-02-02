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
#define THEORETICAL_FREQUENCY (444 /*+ 37/4*/) /*for 2k+*/
//#define THEORETICAL_FREQUENCY (370 /*+ 37/4*/) /*for 1k*/
#define PLL_MUL_MSB           0x0124
#define PLL_BASE_FREQ         37
#define PLL_DEN               19

// Note: Only tested on Slim, should be called from SC side
// 444 ok - approaching the stability limit
// base * (num / den) * ratio, with base = 37 and ratio = 1
int _setOverclock() {
  
  // note: needs to be 333 to be able to reach 444mhz
  const int INITIAL_FREQUENCY = DEFAULT_FREQUENCY;
  scePowerSetClockFrequency(INITIAL_FREQUENCY, INITIAL_FREQUENCY, INITIAL_FREQUENCY/2);

  // set index, to pll ratio 1
  const u32 index = 5; // hw(0xbc100068) & 0xf;
  float ratio = 1.0f;
  switch (index) {
    // incomplete
    case 5: ratio = 1.0f; break;
  }
  
  const u32 num = (u32)(((float)(THEORETICAL_FREQUENCY * PLL_DEN)) / (PLL_BASE_FREQ * ratio));
  u32 _num = (u32)(((float)(INITIAL_FREQUENCY * PLL_DEN)) / (PLL_BASE_FREQ * ratio));

  int intr;
  suspendCpuIntr(intr);
  resetDomains();
  
  // set bit bit 7 to apply index, wait until hardware clears it
  hw(0xbc100068) = 0x80 | index;
  do {
    delayPipeline();
  } while (hw(0xbc100068) != index);
  
  // loop until the numerator reaches the target value,
  // and so, progressively increasing clock frequencies
  while (_num <= num) {
    const u32 lsb = _num << 8 | PLL_DEN;
    const u32 multiplier = (PLL_MUL_MSB << 16) | lsb;
    hw(0xbc1000fc) = multiplier;
    sync();
    _num++;
  }
  
  settle();
  resetDomains();
  resumeCpuIntr(intr);

  return 0;
}

void _cancelOverclock() {
  
  const int TARGET_FREQUENCY = DEFAULT_FREQUENCY;
  const float ratio = 1.0f;
  const unsigned int num = (u32)(((float)(TARGET_FREQUENCY * PLL_DEN)) / (PLL_BASE_FREQ * ratio));

  int intr;
  suspendCpuIntr(intr);
  
  const u32 pllIdx = hw(0xbc100068);
  const u32 pllMul = hw(0xbc1000fc);
  sync();
  
  const u32 msb = pllMul & 0xffff;
  const u32 _den = msb & 0xff;
  u32 _num = msb >> 8;
  
  const int overclocked = (int)(_den && ((_num / _den) > 9));
  
  if (overclocked && pllIdx == 5) {
    
    resetDomains();
    
    while (_num > num) {
      _num--;
      const u32 lsb = _num << 8 | PLL_DEN;
      const u32 multiplier = (PLL_MUL_MSB << 16) | lsb;
      hw(0xbc1000fc) = multiplier;
      sync();
    }
    
    const int index = 5;
    hw(0xbc100068) = 0x80 | index;
    do {
      delayPipeline();
    } while (hw(0xbc100068) != index);
    
    settle();
    resetDomains();
  }
  
  resumeCpuIntr(intr);

  //if (overclocked) {
  //  scePowerSetClockFrequency(TARGET_FREQUENCY, TARGET_FREQUENCY, TARGET_FREQUENCY/2);
  //}
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
  cancelOverclock();
  // setOverclock();
  // scePowerSetClockFrequency(333, 333, 166);
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
