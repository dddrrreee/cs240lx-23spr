// engler: simplistic mpu6050 accel driver code.  the main
// action is in mpu-6050.c and mpu-6050.h
//
// KEY: document why you are doing what you are doing.
//  **** put page numbers for any device-specific things you do ***
//  **** put page numbers for any device-specific things you do ***
//  **** put page numbers for any device-specific things you do ***
//  **** put page numbers for any device-specific things you do ***
//  **** put page numbers for any device-specific things you do ***
//  **** put page numbers for any device-specific things you do ***
//  **** put page numbers for any device-specific things you do ***
// 
// also: a sentence or two will go a long way in a year when you want 
// to re-use the code.
#include "rpi.h"
#include "mpu-6050.h"

void notmain(void) {
    delay_ms(100);   // allow time for i2c/device to boot up.
    i2c_init();
    delay_ms(100);   // allow time for i2c/dev to settle after init.

    // from application note.
    uint8_t dev_addr = 0b1101000;

    enum { 
        WHO_AM_I_REG      = 0x75, 
        WHO_AM_I_VAL = 0x68,       
    };

    uint8_t v = imu_rd(dev_addr, WHO_AM_I_REG);
    if(v != WHO_AM_I_VAL)
        panic("Initial probe failed: expected %b (%x), have %b (%x)\n", 
            WHO_AM_I_VAL, WHO_AM_I_VAL, v, v);
    printk("SUCCESS: mpu-6050 acknowledged our ping: WHO_AM_I=%b!!\n", v);

    // hard reset: it won't be when your pi reboots.
    mpu6050_reset(dev_addr);

    // part 1: get the accel working.


    // initialize.
    accel_t h = mpu6050_accel_init(dev_addr, accel_2g);
    assert(h.g==2);
    
#if 0
    // can comment this out.  am curious what rate we get for everyone.
    output("computing samples per second\n");
    uint32_t sum = 0, s, e;
    enum { N = 256 };
    for(int i = 0; i < N; i++) {
        s = timer_get_usec();
        imu_xyz_t xyz_raw = accel_rd(&h);
        e = timer_get_usec();

        sum += (e - s);
    }
    uint32_t usec_per_sample = sum / N;
    uint32_t hz = (1000*1000) / usec_per_sample;
    output("usec between readings = %d usec, %d hz\n", usec_per_sample, hz);
#endif

    for(int i = 0; i < 100; i++) {
        imu_xyz_t xyz_raw = accel_rd(&h);
        output("reading %d\n", i);
        xyz_print("\traw", xyz_raw);
        xyz_print("\tscaled (milligaus: 1000=1g)", accel_scale(&h, xyz_raw));

        delay_ms(1000);
    }
}
