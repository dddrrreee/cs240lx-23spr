## Labs.

This is a partial set of tentative labs.  They've been split into
ones we'll almost certainly do and those that we might do depending
on interest.  In either case, they will change!  Some will get dropped.
Some will get added.  All will get modified significantly.

[Last year's repo has a ton of
labs](https://github.com/dddrrreee/cs240lx-22spr/tree/main/labs)
if you look through the subdirectories.  It's worth going through these
and letting us know which seem most interesting.

   - There are way more possible labs than class slots so if you can
     please look through these after the first class and see which
     you prefer.

This is the third offering of the class.  While we have done many of
the labs once before in cs240lx or cs340lx many ideas are research-paper
level and how to best implement them is still up in the air, so the only
constant will be change.

Currently looking at roughly ten projects, two labs per project.  
Some rough grouping of projects:

  1. Low level hacks. A set of projects that do low level code hacking

     - [1-dynamic-code-gen](1-dynamic-code-gen/README.md): you'll learn
       how to generate executable machine code at runtime and how to 
       use this trick to do neat stuff.  

     - Fast code. Expose and use the many armv1176 performance counters
       to make code as fast and low error as possible.  Seeing what
       causes TLB misses, cache misses, branch misses, cycles, etc really
       clarifies how the machine works.

     - Timing accurate code: Build a digital analyzer. The pi is fast
       enough, and flexible enough that we can easily build a logic
       analyzer that (as far as I can tell) is faster --- and certainly
       more flexible --- then commercial logic analyzers that cost
       hundreds of dollars.  You'll learn how to write consistently
       nanosecond accurate code.

       The digital analyzer can be viewed as a way to do `printf`
       for devices.  We'll use it to reverse engineer the SPI, I2C and
       UART protocols.  We'll also make it so you can upload code into
       it to do more interesting checks (e.g., that real time threads hit
       their schedules).

       A big challenge is getting the error / variance from one sample to
       another --- you'll use both memory protection tricks and interrupts
       to shrink the monitoring loop down far enough that it runs with an
       order of a few nanosecond error.

  2. Tools using debugging hardware.   We'll use the pi debug hardware
     to write simple versions of useful tools that either require 
     millions of lines of code using other approaches or in fact have 
     never been built.

     - [Trap-based Eraser]: Eraser is a famous race detection tool that
       uses binary-rewriting to instrumenting every load and store and
       flagging if a thread accessed the memory with inconsistent set of
       locks (its *lockset*).  Doing binary rewriting is hard.  By using
       debugging hardware and fast traps we can build a version in a few
       hundred lines of code: simply trap on every load and store and
       implement the lockset algorithm.
     
     - [Trap-based memory checking]: Valgrind dynamically rewrites binaries
       so that it can instruments every load and store.  Dynamically
       rewriting binary is harder in some ways than the static rewriting
       that Eraser does, which was already really hard.  You will use your
       traps to implement a much simpler method:
   
          - Mark all heap memory as unavailable.
          - In the trap handler, determine if the faulting address is in bounds.
          - If so: do the load or store and return.
          - If not: give an error.
      
       Given how fast our traps are, and how slow valgrind is, your approach
       may actually be faster.
      
       To get around the need for *referents* (that track which block of
       memory a pointer is supposed to point to) you'll use randomized
       allocation and multiple runs.

     - [Thorough conconcurrency checking]: we will use traps to build a simple,
       very thorough lock-free checking tool.  Given a lock free algorithm and
       two threads:

        - Mark its memory as inaccessible.
        - On each resulting fault, do the instruction but then context switch
          to another thread.
        - At the end, record the result.
        - Then rerun the thread code from a clean initial state without doing
          context switches.   For the type of code we run: Any difference
          is an error.

       To be really fancy, we can buffer up the stores that are done and
       then try all possible legal orders of them when the other thread
       does a load.  This checker should find a lot of bugs.

       To speed up checking we use state hashing to prune out redundant
       states: at each program counter value, hash all previous stores,
       if we get the same hash as before, we know we will compute the same
       results as a previous and can stop exploring.

     - [Possible: Volatile cross-checking] A common, nasty problem in
       embedded code is that programs code use raw pointers to manipulate
       device memory, but either the programmer does not use `volatile`
       correctly or the compiler has a bug.  We can detect such things with
       a simple hack:

       - We know that device references should remain the same no matter
         how the code is compiled.
       - So compile a piece of code multiple ways: with no optimization, `-O`,
       `-O2`, with fancier flags, etc.
       - Then run each different version, using ARM domain tricks to trace
         all address / values that are read and written.
       - Compare these: any difference signals a bug.
 
       This tool would have caught many errors we made when designing cs107e;
       some of them took days to track down.

   3. Device code.  140e was packed enough that we didn't' do many devices,
      which are where alot of the fun is.  We'll do a bunch of fun 
      devices across the quarter.

      Writing drivers from scratch gives a better feel for hardware.
      Device driver code makes up 90+% of OS code, so it makes sense to
      learn how to write this kind of code.   We'll likely do some amount
      of building a fake-pi implementation to check this code.

      - ws2812b: you'll write nanosecond accurate
        code to control a WS2812B light string.  The WS2812B is
        fairly common, fairly cheap.  Each pixel has 8-bits of color
        (red-green-blue) and you can turn on individual lights in
        the array.


      - [i2c-driver](i2c-driver/README.md): You'll use the broadcom
        document to write your own I2C implementation from scratch (no helper
        code) and delete ours.

        To help testing, you'll also write a driver for the popular ADS1115
        analog-to-digital converter (also from scratch).  Analog devices
        (that output a varying voltage) are typically the cheapest ones.
        Unfortunately your r/pi cannot take their input directly.  We will
        use your I2C driver to read inputs from a microphone and control
        the light array with it.

      - [spi-driver]: SPI is a common, simple protocol.  You'll implement
        it and use it to control a digital-to-analogue converter and to
        control an OLED screen.

      - A sound reactive equalizer: you'll use the ws2812b light strips
        and an I2S microphone to decode audio into frequencies and 
        display the results.  It's pretty, and pretty fun.

      - A movement reactive equalizer: you'll use the ws2812b light strips
        and an accelerometer decode movement into patterns and
        display the results.  It's pretty, and pretty fun.  Part of this
        will use a network bootloader.

   4. OS hacking.

      - Network bootloader.  One big thing missing was having a network
        bootloader so we could control multiple pi's easily.  We'll also
        write a simple, reliable FIFO protocol so we can control  the
        systems.

      - Other chips.  We've written code on the r/pi A+, which is
        fine. One thing I would love to get out of this class is having
        the first set of 140E labs ported to a bunch of other chips.
        The tentative plan is to port to a risc-v and potentially the pi
        pico chip, and get the initial 140e labs running with a version
        of our bootloader.

      - A clean FAT32 read/write implementation.

      - Some other OS stuff!  Ideally a simple distributed OS.
        
   5. Possible: PCB design.  I'm hoping we can do a simple PCB version
      using Parthiv's kicad lab from 340lx.

---------------------------------------------------------------------
Possibles:
  - In cs340lx we spun up some hifive2 riscv boards and then wrote a simple
    simulator that could run itself.  I later used this to make a symbolic
    execution engine with a few hundred lines of code.  Using riscv is a
    great method for doing binary analysis since its  a much simpler system
    than ARM or valgrind intermediate representation.  We'll do several labs
    building this.

  - have you write a simple static bug
    finding system based on "micro-grammars", a trick we came up with
    that does incomplete parsing of languages (so the parser is small
    --- a few hundred lines versus a million) but does it well enough
    that you can find many clean bugs with few false positives. It'd be
    fantastic if you guys could write your own system and go out and find
    a bunch of real bugs in the large systems you'll be dealing with.
    The main limit is me finding a few days to build this out and try
    to break it down in a way that doesn't require compiler background
    (this is feasible, I am just badly managing time).

  - Make a simple distributed system.  One use case is doing crypto
    mining on a linked set of r/pi chips.

  - As a way to help testing (among other things): do a real writeable
    file system so that we can save the binaries to run, the output,
    etc and use scripting (via FUSE) to easily interact with your pi
    through your normal shell.  We could also use the RF chip to make
    this distributed.

  - More speculatively: do very sleazy symbolic execution  to generate
    inputs to run code on (and, e.g., find differences in today's code
    versus yesterday's) by inferring what each machine instruction does by
    running it in its own address space, seeing how the registers change,
    how memory is read and written, and then matching this against known
    micro-ops (or compositions of them).  You can then use this to run
    the binary to get constraints and then use a constraint solver to
    solve them and rerun the result on the code.  If we can do this it
    will be a very powerful trick, since its simple enough to work in
    real life (unlike the current symbolic systems).

