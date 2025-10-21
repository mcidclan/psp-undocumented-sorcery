#pragma once

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspctrl.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_THREAD_LOCALS
#include <stb_image.h>

#define u8 unsigned char
#define u16 unsigned short int
#define s16 short int
#define u32 unsigned int

struct Vertex {
  u16 u, v;
  u32 color;
  u16 x, y, z;
} __attribute__((aligned(4), packed));

#if MODE == GU_POINTS_MODE
struct PointVert {
  u32 color;
  u16 x, y, z;
} __attribute__((aligned(4), packed));
#endif

#define SCR_WIDTH       480
#define SCR_HEIGHT      272
#define BUF_WIDTH       512
#define DRAW_BUF_0      0
#define DRAW_BUF_1      0x88000
#define DEPTH_BUF       0x110000
#define MIXTURE_BASE    0x178000

#define CMD_CLEAR       0xd3
