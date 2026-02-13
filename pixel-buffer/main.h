#include <psppower.h>
#include <pspsdk.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <psprtc.h>
#include <malloc.h>

#define u32 unsigned int
#define UNCACHED_VRAM_BUFFER 0x44000000
#define UNCACHED_USER_SEG    0x40000000

#define hw(addr)                      \
  (*((volatile unsigned int*)(addr)))

#define sync()          \
  asm volatile(         \
    "sync       \n"     \
  )
