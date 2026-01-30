#include <psppower.h>
#include <pspsdk.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <psprtc.h>
#include <psputilsforkernel.h>

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

#define overclock()                                                            \
  sceKernelIcacheInvalidateAll();                                              \
  kcall((int (*)(void))(0x80000000 | (unsigned int)setOverclock));

// modify this value to compare results
#define THEORETICAL_FREQUENCY 333 /*444*/ /*433*/

// Note: Only tested on Slim, should be called from SC side
// 433 ok, 444 ok - approaching the stability limit
int setOverclock() {
  
  const u32 den = 13;
  const u32 base = 37;
  const u32 num = (den * THEORETICAL_FREQUENCY) / base;
  
  scePowerSetClockFrequency(333, 333, 166);
  
  // set all clock ratios to 1:1
  hw(0xbc200000) = 511 << 16 | 511;
  hw(0xBC200004) = 511 << 16 | 511;
  hw(0xBC200008) = 511 << 16 | 511;
  sync();
  
  //
  int intr = sceKernelCpuSuspendIntr();

  const u32 index = 0x5; // set index, to pll ratio 1
  
  // set bit bit 7 to apply index, wait until hardware clears it
  hw(0xbc100068) = 0x80 | index;
  do {
    delayPipeline();
  } while (hw(0xbc100068) != index);

  // 0x4e - 222mhz
  // 0x75 - 333mhz
  
  u32 _num = 0x75;
  const u32 msb = 0x0124;
  
  // loop until the numerator reaches the target value,
  // and so, progressively increasing clock frequencies
  while (_num <= num) {
    const u32 lsb = _num << 8 | den;
    const u32 multiplier = (msb << 16) | lsb;
    hw(0xbc1000fc) = multiplier;
    delayPipeline();
    _num++;
  }
  // 0x93 - 333mhz to ~418mhz
  // 0x9c - 333mhz to ~444mhz
  // 0xa0 - 333mhz to ~455mhz

  // base * (num / den) * ratio
  // with base = 37; num = 0x93, 0x9c, 0xa0; den = 13
  
  sceKernelCpuResumeIntrWithSync(intr);
  
  {
    // wait for clock stability, signal propagation and pipeline drain
    u32 i = 0xffff;
    while (--i) {
      delayPipeline();
    }
  }
  
  sceKernelDcacheInvalidateAll();
  sceKernelIcacheInvalidateAll();
  
  return 0;
}

int cancelOverclock() {
  sceKernelDcacheWritebackInvalidateAll();
  sceKernelIcacheInvalidateAll();
  scePowerSetClockFrequency(333, 333, 166);
  return 0;
}

int main() {
  pspDebugScreenInit();
  
  if (pspSdkLoadStartModule("./kcall.prx", PSP_MEMORY_PARTITION_KERNEL) < 0) {
    pspDebugScreenSetXY(1, 1);
    pspDebugScreenPrintf("Can't load kcall prx");
    sceKernelExitGame();
    return 0;
  }

  overclock();
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
