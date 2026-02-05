#include <pspgu.h>
#include <psppower.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <pspctrl.h>

#define u32          unsigned int
#define u16          unsigned short int
#define hwp          volatile u32*
#define hw(addr)     (*((hwp)(addr)))

PSP_MODULE_INFO("vfpu-sr", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

u32 colors[32] __attribute__((aligned(16))) = {

  0xffff0000,
  0xffff0000,
  0xffff0000,
  0xffff0000,
  
  0xffffffff,
  0xffffffff,
  0xffffffff,
  0xffffffff,
  
  0xff00ff00,
  0xff00ff00,
  0xff00ff00,
  0xff00ff00,
  
  0xffffffff,
  0xffffffff,
  0xffffffff,
  0xffffffff,
  
  0xff0000ff,
  0xff0000ff,
  0xff0000ff,
  0xff0000ff,
  
  0xffffffff,
  0xffffffff,
  0xffffffff,
  0xffffffff,
  
  0xff00ffff,
  0xff00ffff,
  0xff00ffff,
  0xff00ffff,
  
  0xffffffff,
  0xffffffff,
  0xffffffff,
  0xffffffff,
};

void initColors() {
  // load colors to vfpu matrix using general register
  asm volatile (
    ".set push          \n"
    ".set noreorder     \n"
    ".set volatile      \n"
    
    "lv.q c000, 0x00(%0)\n"
    "lv.q c010, 0x10(%0)\n"
    "lv.q c020, 0x20(%0)\n"
    "lv.q c030, 0x30(%0)\n"
    "lv.q c100, 0x40(%0)\n"
    "lv.q c110, 0x50(%0)\n"
    "lv.q c120, 0x60(%0)\n"
    "lv.q c130, 0x70(%0)\n"
        
    ".set pop           \n"
    :
    : "r" (colors)
    : "memory"
  );
}

void genPixels() {
  
  asm volatile (
    ".set push          \n"
    ".set noreorder     \n"
    ".set volatile      \n"
    
    // swizzle all columns
    "vpfxs      Y, Z, W, X        \n" /* /!\ add brackets if [Y, Z, W, X] if compilation fails */
    "mfvc       $t0, $128; vsync  \n"
    "vmov.q     c000, c000        \n"
    "mtvc       $t0, $128; vsync  \n"
    "vmov.q     c010, c010        \n"
    "mtvc       $t0, $128; vsync  \n"
    "vmov.q     c020, c020        \n"
    "mtvc       $t0, $128; vsync  \n"
    "vmov.q     c030, c030        \n"
    "mtvc       $t0, $128; vsync  \n"
    "vmov.q     c100, c100        \n"
    "mtvc       $t0, $128; vsync  \n"
    "vmov.q     c110, c110        \n"
    "mtvc       $t0, $128; vsync  \n"
    "vmov.q     c120, c120        \n"
    "mtvc       $t0, $128; vsync  \n"
    "vmov.q     c130, c130        \n"
    
    // same as:
    /*
    "vpfxs      Y, Z, W, X  \n"
    "vmov.q     c000, c000  \n"
    "vpfxs      Y, Z, W, X  \n"
    "vmov.q     c010, c010  \n"
    "vpfxs      Y, Z, W, X  \n"
    "vmov.q     c020, c020  \n"
    "vpfxs      Y, Z, W, X  \n"
    "vmov.q     c030, c030  \n"
    "vpfxs      Y, Z, W, X  \n"
    "vmov.q     c100, c100  \n"
    "vpfxs      Y, Z, W, X  \n"
    "vmov.q     c110, c110  \n"
    "vpfxs      Y, Z, W, X  \n"
    "vmov.q     c120, c120  \n"
    "vpfxs      Y, Z, W, X  \n"
    "vmov.q     c130, c130  \n"
    */
    
    // swap: save first lower value
    "vmov.s     s300, s003  \n"
    
    // swap: move lower values
    "vmov.s     s003, s013  \n"
    "vmov.s     s013, s023  \n"
    "vmov.s     s023, s033  \n"
    "vmov.s     s033, s103  \n"
    "vmov.s     s103, s113  \n"
    "vmov.s     s113, s123  \n"
    "vmov.s     s123, s133  \n"
    
    // swap: move first lower value to the last
    "vmov.s     s133, s300  \n"
    
    "vsync                  \n"
    
    // 8888 to 565
    "vt5650.q   c200, c000  \n"
    "vt5650.q   c202, c010  \n"
    "vt5650.q   c210, c020  \n"
    "vt5650.q   c212, c030  \n"
    "vt5650.q   c220, c100  \n"
    "vt5650.q   c222, c110  \n"
    "vt5650.q   c230, c120  \n"
    "vt5650.q   c232, c130  \n"
    
    ".set pop               \n"
    :
    :
    : "$t0", "memory"
  );
  
  // copy to vram
  asm volatile(
    "li         $t0, 0x44008040     \n"
    "sync                           \n"
    
    "vzero.s    s400                \n"
    "vadd.s     s400, s400, s400[3] \n"
    
    "1:                             \n"
    "sv.q       c200, 0x00($t0)     \n"
    "sv.q       c210, 0x10($t0)     \n"
    "sv.q       c220, 0x20($t0)     \n"
    "sv.q       c230, 0x30($t0)     \n"
    
    "addiu      $t0, $t0, 0x400     \n"
    "vsub.s     s400, s400, s400[1] \n"

    "vcmp.s     NZ, s400, s400      \n"
    "vsync                          \n"
    
    "bvt        0, 1b               \n"
    "nop                            \n"
    
    :
    :
    : "$t0", "memory"
  );
  
}

int main(int argc, char **argv) {
  scePowerSetClockFrequency(333, 333, 166);

  pspDebugScreenInitEx(NULL, PSP_DISPLAY_PIXEL_FORMAT_565, 0);
  pspDebugScreenSetOffset(0);
  sceDisplaySetFrameBuf((void*)(0x44000000), 512,
    PSP_DISPLAY_PIXEL_FORMAT_565, PSP_DISPLAY_SETBUF_NEXTFRAME);

  SceCtrlData ctl;
  initColors();
  
  do {
    genPixels();
    sceCtrlPeekBufferPositive(&ctl, 1);
    pspDebugScreenSetXY(0, 1);
    pspDebugScreenPrintf("Left pixels value: 0x%04x         ", (u16)hw(0x44008040));
    sceDisplayWaitVblankStart();
    sceKernelDelayThread(100000);
  } while(!(ctl.Buttons & PSP_CTRL_HOME));
  
  pspDebugScreenClear();
  pspDebugScreenSetXY(0, 1);
  pspDebugScreenPrintf("Exiting...");
  sceKernelDelayThread(1000000);
  sceKernelExitGame();
  return 0;
}
