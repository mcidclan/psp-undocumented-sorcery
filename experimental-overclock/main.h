#define hw(addr)                      \
  (*((volatile unsigned int*)(addr)))

#define sync()          \
  asm volatile(         \
    "sync       \n"     \
  )

#define vfpuSync()      \
  asm volatile(         \
    "vsync       \n"    \
    "vnop        \n"    \
    "vnop        \n"    \
    "vnop        \n"    \
    "vnop        \n"    \
    "vnop        \n"    \
    "vnop        \n"    \
    "vnop        \n"    \
  )

#define clearTags()       \
  asm volatile (          \
    "mtc0 $0, $28   \n"   \
    "mtc0 $0, $29   \n"   \
    "sync           \n"   \
  )
    
#define delayPipeline()                    \
  asm volatile(                            \
    "nop; nop; nop; nop; nop; nop; nop \n" \
  )

#define suspendCpuIntr(var)    \
  asm volatile(                \
    ".set push             \n" \
    ".set noreorder        \n" \
    ".set volatile         \n" \
    ".set noat             \n" \
    "mfc0  %0, $12         \n" \
    "sync                  \n" \
    "li    $t0, 0xfffffffe \n" \
    "and   $t0, %0, $t0    \n" \
    "mtc0  $t0, $12        \n" \
    "sync                  \n" \
    "nop                   \n" \
    "nop                   \n" \
    "nop                   \n" \
    ".set pop              \n" \
    : "=r"(var)                \
    :                          \
    : "$t0", "memory"          \
  )

#define resumeCpuIntr(var) \
  asm volatile(            \
    ".set push      \n"    \
    ".set noreorder \n"    \
    ".set volatile  \n"    \
    ".set noat      \n"    \
    "mtc0  %0, $12  \n"    \
    "sync           \n"    \
    "nop            \n"    \
    "nop            \n"    \
    "nop            \n"    \
    ".set pop       \n"    \
    :                      \
    : "r"(var)             \
    : "memory"             \
  )
/*
#define resetDomains()          \
  asm volatile(                 \
    ".set push              \n" \
    ".set noreorder         \n" \
    ".set nomacro           \n" \
    ".set volatile          \n" \
    ".set noat              \n" \
                                \
    "  sync                 \n" \
    "  lui  $t0, 0xBC20     \n" \
    "  lui  $t1, 0xFF       \n" \
    "  ori  $t1, $t1, 0x1FF \n" \
    "  sw   $t1, 0x00($t0)  \n" \
    "  sw   $t1, 0x04($t0)  \n" \
    "  sw   $t1, 0x08($t0)  \n" \
    "  sync                 \n" \
                                \
    ".set pop               \n" \
    :                           \
    :                           \
    : "$t0", "$t1", "memory"    \
  )
*/
#define settle()                \
  asm volatile(                 \
    ".set push              \n" \
    ".set noreorder         \n" \
    ".set nomacro           \n" \
    ".set volatile          \n" \
    ".set noat              \n" \
                                \
    "sync                   \n" \
    "lui  $t0, 0x05         \n" \
    "ori  $t0, $t0, 0x5555  \n" \
                                \
    "1:                     \n" \
    "  nop                  \n" \
    "  nop                  \n" \
    "  nop                  \n" \
    "  nop                  \n" \
    "  nop                  \n" \
    "  nop                  \n" \
    "  nop                  \n" \
    "  addiu $t0, $t0, -1   \n" \
    "  bnez  $t0, 1b        \n" \
    "  nop                  \n" \
                                \
    ".set pop               \n" \
    :                           \
    :                           \
    : "$t0", "memory"           \
  )
/*
static inline int _resetDomains() {
  resetDomains();
  return 0;
}
*/
static inline void _unlockMemory() {
  const u32 start = 0xbc000000;
  const u32 end   = 0xbc00002c;
  for (u32 reg = start; reg <= end; reg += 4) {
    hw(reg) = -1;
  }
  sync();
}
/*
#define increasePLL()           \
  asm volatile(                 \
    ".set push              \n" \
    ".set noreorder         \n" \
    ".set nomacro           \n" \
    ".set volatile          \n" \
    ".set noat              \n" \
                                \
    "1:                     \n" \
    "  sll  $t0, %0, 8      \n" \
    "  ori  $t0, $t0, %1    \n" \
    "  lui  $t1, %2         \n" \
    "  or   $t1, $t1, $t0   \n" \
                                \
    "  lui  $t0, 0xBC10     \n" \
    "  sw   $t1, 0xFC($t0)  \n" \
    "  sync                 \n" \
                                \
    "  addiu %0, %0, 1      \n" \
    "  sltu  $t0, %3, %0    \n" \
    "  beqz  $t0, 1b        \n" \
    "  nop                  \n" \
                                \
    ".set pop               \n" \
    : "+r"(_num)                                \
    : "i"(PLL_DEN), "i"(PLL_MUL_MSB), "r"(num)  \
    : "$t0", "$t1", "memory"                    \
  )
  
#define decreasePLL()           \
  asm volatile(                 \
    ".set push              \n" \
    ".set noreorder         \n" \
    ".set nomacro           \n" \
    ".set volatile          \n" \
    ".set noat              \n" \
                                \
    "1:                     \n" \
    "  sll  $t0, %0, 8      \n" \
    "  ori  $t0, $t0, %1    \n" \
    "  lui  $t1, %2         \n" \
    "  or   $t1, $t1, $t0   \n" \
                                \
    "  lui  $t0, 0xBC10     \n" \
    "  sw   $t1, 0xFC($t0)  \n" \
    "  sync                 \n" \
                                \
    "  addiu %0, %0, -1     \n" \
    "  sltu  $t0, %0, %3    \n" \
    "  beqz  $t0, 1b        \n" \
    "  nop                  \n" \
                                \
    ".set pop               \n" \
    : "+r"(_num)                               \
    : "i"(PLL_DEN), "i"(PLL_MUL_MSB), "r"(num) \
    : "$t0", "$t1", "memory"                   \
  )

#define processPLL()            \
  asm volatile(                 \
    ".set push              \n" \
    ".set noreorder         \n" \
    ".set nomacro           \n" \
    ".set volatile          \n" \
    ".set noat              \n" \
                                \
    "lui  $t0, 0xbc10       \n" \
    "ori  $t1, $zero, %0    \n" \
    "ori  $t1, $t1, 0x80    \n" \
    "sw   $t1, 0x68($t0)    \n" \
    "sync                   \n" \
                                \
    "1:                     \n" \
    "  nop                  \n" \
    "  nop                  \n" \
    "  nop                  \n" \
    "  nop                  \n" \
    "  nop                  \n" \
    "  nop                  \n" \
    "  nop                  \n" \
                                \
    "  lw    $t1, 0x68($t0) \n" \
    "  xor   $t1, $t1, %0   \n" \
    "  bnez  $t1, 1b        \n" \
    "  nop                  \n" \
                                \
    ".set pop               \n" \
    :                           \
    : "i"(PLL_RATIO_INDEX)      \
    : "$t0", "$t1", "memory"    \
  )
*/
/*
// Set clock domains to ratio 1:1
#define resetDomainRatios()          \
  sync();                            \
  hw(0xbc200000) = 511 << 16 | 511;  \
  hw(0xBC200004) = 511 << 16 | 511;  \
  hw(0xBC200008) = 511 << 16 | 511;  \
  sync();
*/

// Wait for clock stability, signal propagation and pipeline drain
/*
#define settle()            \
{                           \
  sync();                   \
  u32 i = 0x1fffff;         \
  while (--i) {             \
    delayPipeline();        \
  }                         \
}
*/
