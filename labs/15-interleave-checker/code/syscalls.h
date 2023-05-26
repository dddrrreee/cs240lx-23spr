#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

#define SYS_RESUME  1
#define SYS_TRYLOCK 2
#define SYS_TEST 3

int syscall_invoke_asm(int sysno, ...);

// resume back to user level with the current set of registers
#define sys_resume(ret)         syscall_invoke_asm(SYS_RESUME, ret)

#define sys_lock_try(addr)     syscall_invoke_asm(SYS_TRYLOCK, addr)

// a do-nothing syscall to test things.
#define sys_test(x)          syscall_invoke_asm(SYS_TEST, x)

#endif
