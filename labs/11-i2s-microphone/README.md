## Using an I2S to make an acoustically reactive display.

Today we mostly get an I2S microphone working and then hopefully you 
can run the output through an FFT and display on your light strip.
Everyone loves blinky stuff.

This is partially based on Parthiv's lab from 340lx:
  - [i2s lab](https://github.com/dddrrreee/cs240lx-22spr/tree/main/labs/17-i2s)

You can just use that lab directly if you like. 

This write up has more discussion of where numbers come from, but it's
more raw.  It also has one issue I'm trying to figure out: why the
loopback for WS gives different timings than expected.

Main goal:
  1. Get i2s working.
  2. Get some mic readings
  3. Run through an FFT.
  4. Play several known signals from your phone (eg 500hz) and see that it
    gets decoded correctly.
  5. Display the spectrum results somehow.

The new code is in `code`.  Parthiv's lab code is in `code-parthiv`.

---------------------------------------------------------------
### How to configure the SPH0645LM4H-B I2S microphone.

We use [adafruit breakout](https://www.adafruit.com/product/3421) for
the SPH0645LM4H-B I2S MEMS microphone.

<p align="center">
  <img src="images/adafruit-i2s-mic.jpg" width="400" />
</p>

The [SPH0645LM4H datasheet](./docs/SPH0645LM4H-datasheet.pdf) is pretty
short and, because there is not much going on, relatively approachable.

However, one issue I had is that its description is a different type
than the previous devices we've used.  Rather than, say, a list of the
specific set of I2C or SPI commands it responds to, the mic is mostly
described in terms of its input clock requirements and the data size
and format it generates.  I don't know if this is fundamentally harder
than our other devices --- as typed out it sounds easier, especially
as compared to the 70+ page NRF we did previously --- but it requires
a bit different reasoning, at least for me.

Related, it was easy to make clock frequency mistakes in terms of how
fast it ran, how the BCLK and FS (sort-of) clock are related, and how to
specify the actual clock speed on the rpi.  This latter task being made
more exciting by the expedient method of the Broadcom BCM2835 datasheet
simply not containing any description of the needed PCM clock, which you
have to piece together from the BCM2835 erata and blog posts.  (The linux
source is also useful, though there is a bunch of infrastructure code
that occludes what is going on in many places.)

Clock and frequency mistakes are a problem for multiple reasons, one of
which is that if the supplied clock is less than 900,000 cycles per second
(900KHz) the device remains in sleep mode and produces no output.

To summarize the datasheet (from page 8):
  - BCLK must be in the 2048KHz to 4096KHz range.
  - WS must change on the falling edge of BCLK (Broadcom says this is
    default: p 120).
  - WS must be BCLK/64 (this means we have a 50% duty cycle where it is
    low for BCLK/32 and then high for BCLK/32).
  - The hold time must be greater than the hold time to the receiver (not
    sure what this refers to, tbh).
  - The mode must be i2s with MSB delayed 1 BCLK cycle after WS changes
    (we will see this is the default when we measure).

A bit longer, not-necessarily-complete cheat-sheet of key facts:
  - supply voltage: 3.6max, 1.62min (current draw is small enough
    that gpio pin seems to work).  (page 2)

  - clock frequency: 2048KHz (min) and 4096KHz (max).  NOTE: the
    use of KHz (kilohertz).  This means minimum clock 2,048,000hz (cycles
    per second) *not* 2048 hz.  Similarly, the max is 4,096,000hz not
    4096.  The fact these are powers of two in hz and that the slight
    confusion that KHz implies the value must be multplied by 1000.

    You can sanity check this calculation by using the minimum clock
    period (page 4) --- the total time the clock pin is held low and then
    high (a single clock cycle).  The maximum time is 488.28 nanoseconds
    and the min is 244.14ns.  

    Just to be painfully clear, this first (maximum time) corresponds 
    to the *slowest* clock speed of 2048KHz since that will be when the
    clock pulses the longest.  You can verify their number and check your
    understanding by dividing one second
    by the number of cycles (2048Khz or 2048*1000) that occur in it
    and multiplying the result by one billion (10^9) to get nanoseconds:  

            1/(2048*1000) * 1000*1000*1000 = 488.28125 (ns).

    You can similarly verify the values for the fastest clock:

            1/(4096*1000) * 1000*1000*1000 = 244.140625(ns).

    Cross-checking and rederiving values is a good way to mechanically
    detect where you've misunderstood a datasheet or made some dumb
    mistake despite understanding it.  (Embarassingly if I did this
    particular cross check a priori rather than post, I would have saved
    me an hour or two.)

  - Power up time: 50ms (page 2).  Given our usage, I'm not sure how
    much this matters.  But if you were making real recordings, you
    would likely want to skip the first 50ms of readings.

    As discussed before, many devices are not simple switches or LEDs
    where we can immediately start using their values or controlling
    them after powerup.  The more complex the device the more work it
    may well need to do before it is in a clean, initialized state.

  - Output date is 18 bits of precision (page 3).  If the i2s master
    configures the protocol to transfer more bits per reading (discussed
    below) the low bits of the value will be 0s.    As stated on 
    page 7, the data format is 2's compliment [sic]: in practice this
    means the high bits of the readings will be 1s (a negative number).
    One consequence is that when the measured value is close to silence,
    the output result will be it's largest (`0xf...`) and it will
    dip down to a *smaller* number if you blow on it.  This is 
    counter intuitive and you might wind up wasting more time trying
    to figure out what is going on, assume you've swapped the data or
    are getting garbage.

  - As the datasheet states, the device is a i2s slave, which means we
    have to configure the i2s hardware on the pi to control it (i.e., be
    in in master mode).  Among other things this means the pi suppliies
    this clock signal as well as WS (a sort-of additional clock that
    denotes complete frames).  Since our pi doesn't magically know what
    the microphone needs for a clock we will have to figure out how to
    configure it to supply it (below).

  - the device gives a PCM output which is 64 times "decimated" (sampled"
    from a continuous PDM output.  The i2s bcm2835 section discusses
    both PCM and PDM and its easy to get confused, but keep in mind
    we have a PCM output.   

  - Key fact: from page 7: you have to have the select pin tied to
    either ground (LOW) or 3v (HIGH) to have the device output a
    single channel.  Otherwise it does a "tri-state" method to merge
    two microphones.  Whether you use LOW or HIGH determines when
    the data is output ---- either when WS is LOW or when it's HIGH.
    We'll need to keep this in mind when looking at the i2s configuration.

  - Again (page 7): frequencies below 900KHz puts the device to sleep
    (page 7) down to 0 KHz.

  - micrphone is a i2s slave (page 8).

  - Key (page 7): because of oversampling, the WS signal rate (i.e.,
    the sampling rate of the microphone) must be exactly 64 times slower
    than the clock (CLK/64).  So if we want 44.1KHz, the clock must be:

         44.1 * 1000 * 64 = 2,822,400 (Hz)

    Confusingly, as we discuss below, the pi doesn't just let you set
    a literal clock rate, you have to put it in terms of dividing the
    rate of a different (faster) clock.

  - Note: as page 7 warns: we should have a single 100K Ohms resistor
    from the data pin to ground to handle bus capacitance.  We don't
    though.  Hopefully the breakout board does.   Hopefully this is ok!


---------------------------------------------------------------------------
### How to configure the bcm2835 PCM clock.

As discussed above, the i2s microphone expects a clock signal that
should switch at a rate of the sample rate times 64.  If we want to 
sample at 44.1 kHz the clock will be:

         44.1 * 1000 * 64 = 2,822,400 (Hz)

If you come from software, an additional source of confusion is that
we can't just set the clock to this value of 2.8224Mhz (as we would
with a variable) but instead must express it as a divisor (integer and
fractional part) of some other clock.  This is a bit weird.  It also
means you have to pay attention to the period of the other clock and
find one that (ideally) divides with no remainder.

The pi provides a PCM clock for this purpose, though as the [errata
notes](https://elinux.org/BCM2835_datasheet_errata)
the broadcom 2835 datasheet doesn't discuss it.   Several sources of
information:
  - [i2s test code used by the eratta](https://github.com/arisena-com/rpi_src)
  - [the Linux i2s driver](docs/bcm2835-i2s.c)

From the errata, the PCM clock has two registers:
  1. The PCM control register at `0x7E101098` (so for us `0x20101098`).
  2. The PCM divisor register at`0x7E10109C` (so for us `0x2E10109C`).

The key bits in the control (page 107):
  - `31-24`: a byte length "password" (value `0x5a`) you must set for
    the device to perform changes.  This is a common hack to try to 
    reduce the chance of wild writes.

  - bit 7: the BUSY flag: don't change the clock while this is set.
  - bit 4: the ENAB flag.  You must disable the clock (ENAB=0)
    *and* wait til BUSY=0 before changing the clock.

  - bits 3-0: the clock source.  The basic idea is to pick a clock source
    that (1) is high enough to give a good signal and (2) hopefully
    divides evenly into the clock value you want.  From the eratta
    and the bcm2835 i2s linux driver, the recommendation is to use the
    "oscillator" (source = 0b0001) which is an XTAL crystal oscilating
    at 19.2MHz.

The key bits in the divisor register (page 108):
  - `31-24`: a byte length "password" (value `0x5a`).
  - `23-12`: DIVI: the integer part of the divisor.
  - `11-0`: DIVF: the fractional part of the divisor that results by
    (1) *multiplying it by 4096* and then (2) truncating it (taking
    the integer portion).

    This is super confusing and comes from the Broadcom formula on page
    105 Table 6-32 which (after eratta correction) gives the average
    output as:

      rate = clock source / (DIVI + DIVF / 4096)

<p align="center">
  <img src="images/clock-formula.png" width="600" />
</p>

Putting it all together: To find the fractional divider of the 
19.2Mhz clock to express a 44.1 khz sample rate:

      rate = (19.2*1000*1000) / (44.1 * 1000 * 64)
            = 6.8027210884353737

Not great, since not even.

As noted above, to express the fractional part of this result, we don't
just set DIVF to 8027 (the first four digits after the decimal).  Instead
you multiply the fractional number by 4096 and do integer truncation.  So:

    DIVF = round(.8027210884353737 * 4096) 
         = round(3287.9455782312907)
         = 3288

You can check this result by plugging the result (DIVI = 6, DIVF =
3287) back in to the corrected Table 6-32 formula:

    rate = source / (DIVI + DIVF / 4096)
         = 19.2Mhz / (6 + 3288. / 4096)
         = 2,822,394.4875107664

or a bit less than 6 cycles off of 2,822,400 Hz.

You can see that this result is as close as we can get with the 
tools we got by recomputing with DIVF + 1:

        = 19.2Mhz / (6 + 3289. / 4096)
        = 2822293.1993540283

And DIVF - 1:

        = 19.2Mhz / (6 + 3287. / 4096)
        = 2822495.7829379463


Both of which are farther away from 2,822,400.  To get even closer we
could also do MASH correction, but I'll ignore that.

#### Loopback to check the clock.

At this point it should be pretty clear that a clock isn't a wristwatch
and is easy to make mistakes with.

And, unfortunately, unlike null pointers, the microphone garbage that
results from clock mistakes will be hard to detech.  So as part of setup
we will try to check what we can to make sure that things make sense.
(I recommend this in general!)

The easiest thing to check is that i2s clock: just connect ("loopback")
a jumper wire from the BCLK clock GPIO pin to some unused input pin and
measure the time for a reading to go through a complete 0 to 1 transition.
In our case, how many cycles the pin reads 0 plus how many cycles it
reads 1.  (Or vice versa: doesn't matter.)  We expect the numer of
cycles to be roughly

A secondary check is to do the same thing for the WS GPIO pin and make
sure the time it takes to do a complete 0 to 1 transition is 64x the value
of the BCLK plus 1 for the transition period.

From the bcm2835 pin table:

<p align="center">
  <img src="images/i2s-pins.jpg" width="400" />
</p>

We see the BCLK pin is 18 and the FS pin is 19.  So just do loopback
using these.

We need to compute the expected cycles per sample:

        = ARM cycles per second / samples per second
        = 700 MHz  / 44.1khz*64
        = 700*1000*1000 / (44100*64)
        = 248.

For the FS (or WS) we expect this multiplied by 64.  Currently it is not.
I'm not sure why --- if you figure out that's an extension, ty ty.

---------------------------------------------------------------
### How to configure I2s.

Filling this in.
