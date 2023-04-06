## Mailboxes

The pi is a bit weird in that the GPU controls a lot of the action.
As described at:

   - [mailboxes](https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface)

The pi gives a way to send messages to the GPU and receive a
response.  You can use this to query and configure the hardware.
The mailbox interface is not super-intuitive, and the main writeup uses
passive-declarative voice style that makes it hard to figure out what
to do.  (One key fact: the memory used to send the request is re-used
for replies.)  When I got the mailbox working, it also claimed 128MB.

If you look through the (unfortunately incomplete) [mailbox
writeup](https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface)
you'll see all sorts of useful facts you can query for --- model number,
serial number, ethernet address, etc.  So it's worth figuring out how
to do it.

So that's what we will do.  Some hints:

  1. As always: if you want to write the code completely from scratch,
     I think doing so is a valuable exercise.  However, in the interests
     of time I put some starter code in `code/`.  Extend it
     to query for physical memory size along with a couple of other
     things that seem useful.

Rules:
  1. Buffer must be 16-byte aligned (because the low bits are ignored).
  2. The response will overwrite the request, so the size of the buffer
     needs to be the maximum of these (but not the summation).
  3. The document states that additional tags could be returned --- it may
     be worth experimenting with sending a larger buffer and checking
     the result.

-------------------------------------------------------------------
#### Example: getting the board serial number

If you read through the mailbox document, there's a bunch of cool little
factoids you can get (and sometimes set).  One is the unique board serial
number. The main useful property of the serial number is uniqueness and
persistence --- no board in this room should have the same serial number
and a given board's serial number does not change.

Later this quarter we'll see how unique, persistent host identifiers
are useful in a networked system when you want to have a guaranteed
unique ID for ordering, statically binding programs to boards, etc.
For the moment, let's just try to figure out what ours are.

Searching through the mailbox document we see the "get serial number"
message is specified as:

<p align="center">
  <img src="images/get-serial.png" width="250" />
</p>

Where:
  - the "tag" of `0x00010004` is a unique 32-bit constant used 
    to specify the "get serial" request.  This is a message opcode
    to the GPU (the "video control" or VC) knows the request.
  - The request message size is 0 bytes: i.e., we don't send 
    anything besides the tag.
  - The  response message size is 8 bytes holding the serial number.
    (i.e., two 32-bit words). 


Ok, how do we send it?  
The start of the document has a somewhat
confusingly described message format.

The first part is fairly clear:

<p align="center">
  <img src="images/msg-format.png" width="450" />
</p>

For our get serial message the words in the message will be as followed:

  - `msg[0] = 4*8`: The first 32-bit word in the message contains the 
     size of tte message in bytes.   For us: as you'll see the 
     message has 8 words, each word is 4 bytes, so the
     total size is `4*8`.
  - `msg[1] = 0`: the second 32-bit word in the message contains
    the request code, which is always 0.  After the receiver writes
    the reply into the `msg` bytes, `msg[1]` will either change to
    `0x80000000` (success) or `0x80000001` (error).

  - We then fill in the tag message.
     - `msg[2]= 0x00010004` (our tag).
     - `msg[3] = 8`: the response size in bytes: the serial
       is two 32-bit words.
     - `msg[4] = 0`: the document states we write 0 for a request.
       After a sucessesful send and reply, `msg[4]` should hold the
       constant `0x80000008` which is has bit 31 set (as stated in the
       doc) and the response size 8 (from the doc). (i.e., `(1<<31)
       | 8)`).
     - `msg[5] = 0`: since the reply message is written into our send
       buffer (it's just the way it is) we need to pad our sent message
      so it contains enough space.
     - `msg[6] = 0`: the second word of the reply.
  - `msg[7] = 0`: The final word is `0`.  

To clean things up some:

    // declar a volatile buffer, 16 byte aligned.
    volatile uint32_t __attribute__((aligned(16))) msg[8];

    msg[0] = 8*4;         // total size in bytes.
    msg[1] = 0;           // sender: always 0.
    msg[2] = 0x00010004;  // serial tag
    msg[3] = 8;           // total bytes avail for reply
    msg[4] = 0;           // request code [0].
    msg[5] = 0;           // space for 1st word of reply 
    msg[6] = 0;           // space for 2nd word of reply 
    msg[7] = 0;           // end tag [0]

Ok, we have a buffer, how do we send it?

Well, the broadcom 2835 has no discussion of the mailbox  other than
the ominmous statement: "The Mailbox and Doorbell registers are not for
general usage." (page 109).  So you can either piece things together
from linux source, or different random web pages.  We'll use the 
valver's discussion which is pretty nice:
  - [mailbox discussion](https://www.valvers.com/open-software/raspberry-pi/bare-metal-programming-in-c-part-5/#mailboxes)

Just search for "mailbox".

There are three addresses we care about:

    #define MBOX_STATUS 0x2000B898
    #define MBOX_READ   0x2000B880
    #define MBOX_WRITE  0x2000B8A0

For status, we have two values:

    #define MAILBOX_FULL   (1<<31)
    #define MAILBOX_EMPTY  (1<<30)

The model here is that you send and receive messages to the GPU.  As you
saw in 140e, whenever sending and receiving data between different
hardware devices:

  - For send: we won't have infinite buffering and so need to check
    if there is space.  In our case we wait until:

        while(GET32(MBOX_STATUS) & MAILBOX_FULL)
            ;

    We can then send by writing the address
    of the buffer to `MBOX_WRITE` bitwise-or'd with the channel we
    send on (the document states this should be 8).  

    This is why the buffer must be aligned: so the lower 4 bits are zero
    and so can be reused for the channel id.

  - Similarly, requests are not instantanous, and so we need to
    wait until they return before reading any response.  In our case
    we wait until `MBOX_STATUS&MAILBOX_EMPTY` is 0:

        while(GET32(MBOX_STATUS)&MAILBOX_EMPTY)
            ;

    We then read the response using `GET32(MBOX_READ)`.  The value should
    have the mailbox channel (8) in the low bits.

  - Finally: when writing to different devices we need to use device
    barriers to make sure reads and writes complete in order.
    The simplest approach: do a device barrier at the start and end of
    each mailbox operation.


-------------------------------------------------------------------
#### What to do

There's a bunch of stuff to do.   Two simple things to start with:  
  1. Get your pi's revision.  You can check that this value makes sense
    from the [board revision page](https://www.raspberrypi-spy.co.uk/2012/09/checking-your-raspberry-pi-board-version/)

  2. Get the board's temperature.  I had around 90 F.  Not sure if this 
    is ok or not.

  3. Get the board's memory size: we need this later.

Fancier:
  - Overclock your pi!  See how fast you can make it.  You should get
    the current clock speed, set it, and then recheck it changed.  Then
    check that the changes actually happened: compute how many 
    cycles per second by using `cycle_cnt_read()` and `timer_get_usec()`.

    I have no idea how fast these can go.  You could see how much it
    heats up and back off.  You probably want to write some kind of
    fancy calculation that you can check and see when it breaks down.

    NOTE: I had to change the `arm_freq=700` value in my `config.txt`
    before I could make the pi run faster (otherwise it ignored the
    changes).

     There's tons of other values you can mess with.

  - Also: rewrite the interface so it's no so awkward.

Look through the doc and figure some stuff out.
