#ifndef __MPU_6050_H__
#define __MPU_6050_H__

#include "i2c.h"
#include "bit-support.h"
#include <limits.h>


// both gyro and accel return x,y,z readings: it makes things
// a bit simpler to bundle these together.
typedef struct { int x,y,z; } imu_xyz_t;

static inline imu_xyz_t
xyz_mk(int x, int y, int z) {
    return (imu_xyz_t){.x = x, .y = y, .z = z};
}
static inline void
xyz_print(const char *msg, imu_xyz_t xyz) {
    output("%s (x=%d,y=%d,z=%d)\n", msg, xyz.x,xyz.y,xyz.z);
}

// used to talk to i2c.

// write one register.
void imu_wr(uint8_t addr, uint8_t reg, uint8_t v);
// read one register
uint8_t imu_rd(uint8_t addr, uint8_t reg);

// read n registers.
int imu_rd_n(uint8_t addr, uint8_t base_reg, uint8_t *v, uint32_t n);

// note: the names are short and overly generic: this saves typing, but 
// can cause problems later.
typedef struct {
    uint8_t addr;
    unsigned hz;    // sample rate.
    unsigned g;     // sensitivity in g. 
} accel_t;

// blocking read of accel.
imu_xyz_t accel_rd(const accel_t *h);

// passed to accel init.
enum { 
    accel_2g = 0b00,
    accel_4g = 0b01,
    accel_8g = 0b10,
    accel_16g = 0b11,
};

// init accel.
accel_t mpu6050_accel_init(uint8_t addr, unsigned accel_g);

// hard reset of the device
void mpu6050_reset(uint8_t addr);

// scale a reading returned by accel_rd
imu_xyz_t accel_scale(accel_t *h, imu_xyz_t xyz);


typedef struct {
    uint8_t addr;
    unsigned hz;

    // scale: for accel is in g, for gyro is in dps.
    unsigned dps;
} gyro_t;

enum {
    gyro_250dps  = 0b00,
    gyro_500dps  = 0b01,
    gyro_1000dps = 0b10,
    gyro_2000dps = 0b11,
};

gyro_t mpu6050_gyro_init(uint8_t addr, unsigned gyro_dps);

imu_xyz_t gyro_rd(const gyro_t *h);

imu_xyz_t gyro_scale(gyro_t *h, imu_xyz_t xyz);
#endif
