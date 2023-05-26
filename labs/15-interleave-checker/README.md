## Lab: using single stepping to check interleaving.

We'll use single-stepping to write a concurrency checker that is tiny
but mighty.

In many tools, we'd want to run a single instruction and then get control
back for a tool.

For this lab you'll have to read the comments. Sorry :( 
  - check-interleave.h: has the the interface.
  - check-interleave.c: has a working version.

What to do:
  - see how the tests work.  `make check` should pass.  (unless the
    number of instructions is different. aiya.)
  - write a trylock in the system call routine.
  - choose your own adventure (I'll be adding a bunch).

-----------------------------------------------------------------------------

### option: write code to switch back to supervisor mode

To keep things simple, we'll just write a simple trampoline in assembly 
`user_trampoline_ret` that behaves just like `user_trampoline_no_ret` except
that it:
   1. Allows a return from the trampoline by changing our mode from user
      to back to super.  
   2. Makes the next part a bit easier by passing a pointer to the
      called routine.

As mentioned above, the main complication is that we cannot change the 
current `cpsr` at user level.  To do this you'll have to make a simple
system call.  Recall from lab 7 of cs140e how we make a system call:
   1. To make a system call with 1, you write `swi 1` in the assembly
      (to do 2, you'd write `swi 2`, etc).
   2. In your `software_interrupt_asm` routine you have to save all caller-registers
      adjust the lr, and call a C routine.  Unlike other exception handlers,
      you should *not* load the stack pointer since it is shared with SUPER mode.
   3. In your C code, you should dereference the pc value to see the actual
      instruction and see which constant (in our case, it should be 1).
   4. Change the *spsr* (not the `cpsr` since that will just change the 
      value in the exception handler) to the new value.

As an intermediate step, I would say you should first make a "hello world"
system call that just prints and returns.

For the `user_trampoline_ret` assembly:
   1. You'll have to save the old mode.
   2. Before calling the handler don't forget to move the r2 register to the 
      first argument register (r0).
   3. When the branch and link returns you'll have to call your new system
      call to change back to the old mode.
   4. Return.

Note: the register-based indirect jump seems to be enough to cause
prefetching, so perhaps you could argue we don't need the flush.  I think
there are cases where we could have problems if we ran this code more
than once and it remembered the destination.))

The test `16-part2-test1.c` should pass (sorry there are not more!).

-----------------------------------------------------------------------------
### Extensions
  1. Don't switch until there was a load or store (can't matter otherwise).
  2. Compute the read-write set from loads and stores so you know when 
     you reach a fixed point.
  3. Have a state hash so you know when you don't have to explore further.
  4. Make a way to yield back and forth.
  5. Write an LDEX lock implementation and check it.
  6. check system call locking.

- use lock with a non-volatile to show is broken.
- Extend to check for null as well as mismatch.
