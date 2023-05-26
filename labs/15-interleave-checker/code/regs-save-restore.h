#include "rpi-asm.h"

#define TRAMPOLINE_FULL_REGS(fn)    \
  sub sp, sp, #68;                          \
  stm sp, {r0-r14}^;                        \
  str lr, [sp, #60];                        \
  mov   r2, lr;                             \
  mrs   r1, spsr;                           \
  str r1, [sp, #64];                        \
  mov   r0, sp;                             \
  bl    fn;                                 \
  ldr r1, [sp, #64];                        \
  msr   spsr, r1;                           \
  ldm   sp, {r0-r14}^;                      \
  add sp, sp, #60;                          \
  rfe sp;                                   \
  asm_not_reached()


