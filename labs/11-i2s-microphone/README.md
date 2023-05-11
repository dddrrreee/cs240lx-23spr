### How to configure the SPH0645LM4H-B I2S microphone.

We use the SPH0645LM4H-B I2S microphone.  Its datasheet is pretty
short and, because there is not much going on, relatively approachable.
However, one issue I had was that it was easy to make clock frequency
mistakes.  Clock and frequency mistakes are a problem
for multiple reasons, one of which is that
if the supplied clock is less than 900,000 cycles per second (900KHz)
the device remains in sleep mode and produces no output.   

Compounding this issue: the pi's PCM clock is not documented in the
Broadcom BCM2835 datasheet and you have to piece together its existence
and how to use it from the BCM2835 erata and blog posts.  (The linux
source is also useful, though there is a bunch of infrastructure
code that occludes what is going on in many places.)

A short, not-necessarily-complete cheat-sheet of key facts:
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

            1/(4996*1000) * 1000*1000*1000 = 244.140625(ns).


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
    page 7, the data format is 2's copmliment [sic]: in practice this
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

To summarize (page 8):
  - BCLK must be in the 2048KHz to 4096KHz range.
  - WS must change on the falling edge of clk (broadcom says this is default:
    p 120).
  - WS must be BCLK/64 (this means we have a 50% duty cycle where it is
    low for BCLK/32  and then high for BCLK/32).
  - The hold time must be greater than the hold time to the receiver (not
    sure).
  - The mode must be i2s with MSB delayed 1 BCLK cycle after WS changes
    (we will see this is the default when we measure).

