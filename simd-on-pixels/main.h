#pragma once

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspctrl.h>

#define u16 unsigned short int
#define u32 unsigned int

struct Vert32 {
  u32 color;
  u16 x, y, z;
} __attribute__((aligned(4), packed));

struct Vert16 {
  u16 color;
  u16 x, y, z;
} __attribute__((aligned(4), packed));

#define SCR_WIDTH       480
#define SCR_HEIGHT      272
#define BUF_WIDTH       512
#define DRAW_BUF_0      0
#define DRAW_BUF_1      0x88000
#define DEPTH_BUF       0x110000
#define DDATA_BUF_BASE  0x178000
#define DDATA_BUF_SIZE  0x2000

#define CMD_CLEAR       0xd3  
#define LOGICAL_AND     0x01
#define LOGICAL_OR      0x07
