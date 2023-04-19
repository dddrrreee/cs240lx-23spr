## Speed and measurement: the PMU

Today has two short labs, this one ---
a quick side-quest to write a simple performance monitor code --- and 
the next `5-machine-code`.

This lab: The arm1176 has a bunch of interesting performance counters
we can use to see what is going on, such as cache misses, TLB misses,
prediction misses, procedure calls.  You'll write a simple library that
exposes these, which will make it much easier to optimize code.

The arm1176 describes the performance monitor unit (PMU) on pages 3-133
--- 3-140.  It has many useful performance counters, though with the limit
that only two can be enabled at any time in addition to the "always on"
cycle counter.  We'll write a simple interface to expose these.

These counters make it much easier to speed up your code --- it's hard to
know the right optimization when you don't know what the bottleneck is.
In addition, they can be used to test your understanding of the hardware
--- if you believe you understand how the BTB, TLB, or cache works, write
code based on this understanding and measure if the expected result is
the actual.

##### Rant: no PMU interrupts on the bcm2835

The PMU also has another useful (common) feature:
  1. You can set the counter value to any initial constant you want.
  2. You can set up an exception that triggers when the counter overflows.

You can use this to detect immediately when a miss or other event occurs,
making it easy to find performance issues.  For our purposes, this can
also be used to run untrusted code with interrupts disabled:  if you
want to allow code to run for at most 1000 cycles, subtract 1000 from
`UINT_MAX` and store it in the counter and run the code.  If the code
runs more than this budget, it will get interrupted with an exception.

***Unfortunately***, the broadcom bcm3835 used on our r/pi A+  does not
(appear to) expose this interrupt!  Very, very irritating.   It does
appear to do so for the broadcom bcm3836/bcm3837 chips used in later pi's.
Of course, the broadcom document is not always reliable --- if you
look in chapter 7 there are many blank entries in the interrupt table.
It's possible that one of these is for the PMU (I should have just checked
but did not).  So: If anyone can find a way to make the PMU work I'll
offer  $200 bounty.  There's a bunch of cool tricks we can do.

### Part 1:  implement `code/armv6-pmu.h`

Fill in the `unimplemented` routines in `armv6-pmu.h`.  I'd suggest
using the `cp_asm` macros in

        #include "asm-helpers.h"

What you need:
   1. Performance monitor control register (3-133): write this to
      select which performance counters to use (the values are in tabel
      3-137) and to enable the PMU at all.
   2. Cycyle counter register (3-137): read this to get the current
      cycle count.
   3. Count register 0 (3-138): read this to get the 32-bit 0-event
      counter (set in step 1).
   4. Count register 1 (3-139): read this to get the 32-bit 1-event
      counter (set in step 1).

Note you can also count interrupts by using the system validation counter
register (3-140) but we won't use this today.


When I run `code/test-pmu.c` I get:

    no cache: doesn't work?:
	    total calls=0, total returns =0
    cache [expect 10]:
	    total calls=10, total returns =10
    cache [expect 10]:
	    total call count=10, total return count=10
    disabling caches again
    no cache branches:
	    total branch mispredict=10, total branch executed=22
    cache branches:
	    total branch mispredict=1, total branch executed=22

----------------------------------------------------------------------
### Part 2: write code that shows off some of the other performance counters.

This is a choose-your-own adventure:  look through the counters and
write some code that shows off something they can measure (or run on
your 140e code!).  The easiest way is to copy the header files
into the 140e `libpi/include` directory and they should just work.
