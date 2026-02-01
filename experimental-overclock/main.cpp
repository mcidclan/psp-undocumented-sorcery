#include <psppower.h>
#include <pspsdk.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <psprtc.h>
#include "kcall.h"

PSP_MODULE_INFO("exp-overclock", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

#define hw(addr)                      \
  (*((volatile unsigned int*)(addr)))

#define sync()          \
  asm volatile(         \
    "sync       \n"     \
  )

#define delayPipeline()                    \
  asm volatile(                            \
    "nop; nop; nop; nop; nop; nop; nop \n" \
    "sync       \n"                        \
  )

#define suspendCpuIntr(var) \
  asm volatile(             \
    "mfic  %0, $0 \n"       \
    "mtic  $0, $0 \n"       \
    "sync         \n"       \
    : "=r"(var)             \
    :                       \
    : "$8"                  \
  )

#define resumeCpuIntr(var)  \
  asm volatile(             \
    "mtic  %0, $0 \n"       \
    "sync         \n"       \
    :                       \
    : "r"(var)              \
  )

#define setOverclock()                                                         \
  sceKernelIcacheInvalidateAll();                                              \
  kcall((int (*)(void))(0x80000000 | (unsigned int)_setOverclock));

#define cancelOverclock()                                                      \
  sceKernelIcacheInvalidateAll();                                              \
  kcall((int (*)(void))(0x80000000 | (unsigned int)_cancelOverclock));

// modify this value to compare results
#define THEORETICAL_FREQUENCY (444 /*+ 37/4*/)
#define PLL_MUL_MSB           0x0124

// Note: Only tested on Slim, should be called from SC side
// 444 ok - approaching the stability limit
// base * (num / den) * ratio, with base = 37 and ratio = 1
int _setOverclock() {
  
  // note: needs to be 333 to be able to reach 444mhz
  const int INITIAL_FREQUENCY = 333;
  
  scePowerSetClockFrequency(INITIAL_FREQUENCY, INITIAL_FREQUENCY, INITIAL_FREQUENCY/2);

  // set index, to pll ratio 1
  const u32 index = 5; // hw(0xbc100068) & 0xf;
  
  float ratio = 1.0f;
  switch (index) {
    // incomplete
    case 5: ratio = 1.0f; break;
  }
  
  const u32 den = 19;
  const float base = 37;
  
  const u32 num = (u32)(((float)(THEORETICAL_FREQUENCY * den)) / (base * ratio));
  u32 _num = (u32)(((float)(INITIAL_FREQUENCY * den)) / (base * ratio));

  int intr;
  suspendCpuIntr(intr);
  
  // throttle all clock domains before overclocking PLL
  hw(0xbc200000) = 32 << 16 | 511;
  hw(0xBC200004) = 32 << 16 | 511;
  hw(0xBC200008) = 32 << 16 | 511;
  sync();
    
  // set bit bit 7 to apply index, wait until hardware clears it
  hw(0xbc100068) = 0x80 | index;
  do {
    delayPipeline();
  } while (hw(0xbc100068) != index);

  // loop until the numerator reaches the target value,
  // and so, progressively increasing clock frequencies
  while (_num <= num) {
    const u32 lsb = _num << 8 | den;
    const u32 multiplier = (PLL_MUL_MSB << 16) | lsb;
    hw(0xbc1000fc) = multiplier;
    delayPipeline();
    _num++;
  }

  // unthrottle all clock domains after overclocking PLL
  hw(0xbc200000) = 511 << 16 | 511;
  hw(0xBC200004) = 511 << 16 | 511;
  hw(0xBC200008) = 511 << 16 | 511;
  sync();

  // wait for clock stability, signal propagation and pipeline drain
  {
    u32 i = 0xfffff;
    while (--i) {
      delayPipeline();
    }
  }
  
  resumeCpuIntr(intr);
  
  return 0;
}

void _cancelOverclock() {
  
  const int TARGET_FREQUENCY = 333;
    
  float ratio = 1.0f;
  const u32 den = 19;
  const float base = 37;

  const u32 num = (u32)(((float)(TARGET_FREQUENCY * den)) / (base * ratio));
  u32 _num = (u32)(((float)(THEORETICAL_FREQUENCY * den)) / (base * ratio));
  
  int intr;
  suspendCpuIntr(intr);
  
  u32 index = 2;
  hw(0xbc200000) = 32 << 16 | 511;
  hw(0xBC200004) = 32 << 16 | 511;
  hw(0xBC200008) = 32 << 16 | 511;
  sync();
  
  hw(0xbc100068) = 0x80 | index;
  do {
    delayPipeline();
  } while (hw(0xbc100068) != index);
  
  while (_num > num) {
    const u32 lsb = _num << 8 | den;
    const u32 multiplier = (PLL_MUL_MSB << 16) | lsb;
    hw(0xbc1000fc) = multiplier;
    delayPipeline();
    _num--;
  }

  index = 5;
  hw(0xbc100068) = 0x80 | index;
  do {
    delayPipeline();
  } while (hw(0xbc100068) != index);

  hw(0xbc200000) = 511 << 16 | 511;
  hw(0xBC200004) = 511 << 16 | 511;
  hw(0xBC200008) = 511 << 16 | 511;
  sync();

  u32 i = 0xfffff;
  while (--i) {
    delayPipeline();
  }
  
  resumeCpuIntr(intr);
  
  scePowerSetClockFrequency(TARGET_FREQUENCY, TARGET_FREQUENCY, TARGET_FREQUENCY/2);
}

int main() {
  pspDebugScreenInit();
  
  if (pspSdkLoadStartModule("./kcall.prx", PSP_MEMORY_PARTITION_KERNEL) < 0) {
    pspDebugScreenSetXY(1, 1);
    pspDebugScreenPrintf("Can't load kcall prx");
    sceKernelExitGame();
    return 0;
  }

  cancelOverclock();
  setOverclock();
  // scePowerSetClockFrequency(333, 333, 166);

  u64 prev, now, fps = 0, maxFps = 0, counter = 0;
  const u64 res = sceRtcGetTickResolution();
  
  SceCtrlData ctl;
  
  do {
    sceRtcGetCurrentTick(&prev);
    
    sceCtrlPeekBufferPositive(&ctl, 1);
    
    maxFps = fps > maxFps ? fps : maxFps;
    
    pspDebugScreenSetXY(1, 1);
    pspDebugScreenPrintf("MaxFPS: %llu, FPS: %llu    ", maxFps, fps);
    
    pspDebugScreenSetXY(1, 2);
    pspDebugScreenPrintf("Counter: %llu    ", counter++);
        
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
