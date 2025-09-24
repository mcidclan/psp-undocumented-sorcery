/*
 * Allegrex LL/SC implementation seems to use a global LLbit flag without cache
 * coherency tracking. The LLbit is cleared on any ERET instruction, occurring during:
 * - Exception returns (including syscalls)
 * - Interrupt handling returns
 * - Context switches during thread yield
 * Observed LLbit clearing during prolonged loops (possible hardware timeout)
 * The usage of sceKernelDelayThread(0) here is meant to force a syscall and LLbit reset.
 */

static inline u32 mutexTrylock(volatile u32* mutex) {
  u32 result;
  asm volatile(
    ".set push               \n"
    ".set noreorder          \n"
    ".set volatile           \n"
    "ll    %0, %1            \n"
    "bnez  %0, 1f            \n"
    "nop                     \n"
    "li    %0, 1             \n" 
    "sc    %0, %1            \n"
    "j     2f                \n"
    "nop                     \n"
    "1: li %0, 0             \n"
    "2: sync                 \n"
    ".set pop                \n"
    : "=&r" (result)
    : "m" (*mutex)
    : "memory"
  );
  sceKernelDelayThread(0); // making sure LLbit is reset
  return result;
}

static inline void mutexLock(volatile u32* mutex) {
  do {
  } while(!mutexTrylock(mutex));
}

static inline void mutexUnlock(volatile u32* mutex) {
  asm volatile(
    ".set push               \n"
    ".set noreorder          \n"
    ".set volatile           \n"
    "sync                    \n"
    "sw    $zero, %0         \n"
    "sync                    \n"
    ".set pop                \n"
    :
    : "m" (*mutex)
    : "memory"
  );
}

static inline void atomicWrite(volatile u32* addr, u32 value) {
  asm volatile(
    ".set push               \n"
    ".set noreorder          \n"
    ".set volatile           \n"
    "1:                      \n"
    "sync                    \n"
    "ll    $t0, 0(%0)        \n"
    "move  $t0, %1           \n"
    "sync                    \n"
    "sc    $t0, 0(%0)        \n"
    "beq   $t0, $zero, 1b    \n"
    "sync                    \n"
    ".set pop                \n"
    :
    : "r" (addr), "r" (value)
    : "t0", "memory"
  );
  sceKernelDelayThread(0); // making sure LLbit is reset
}

static inline u32 atomicRead(volatile u32* addr) {
  u32 value;
  asm volatile(
    ".set push               \n"
    ".set noreorder          \n"
    ".set volatile           \n"
    "sync                    \n"
    "ll    %0, %1            \n"
    "sync                    \n"
    ".set pop                \n"
    : "=&r" (value)
    : "m" (*addr)
    : "memory"
  );
  return value;
}
