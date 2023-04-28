PROGS = driver-accel.c
PROGS += driver-gyro.c

# COMMON_SRC = mpu-6050.c
STAFF_OBJS = staff-i2c.o staff-mpu-6050.o

# define this if you need to give the device for your pi
TTYUSB = 
BOOTLOADER = my-install

# uncomment if you want it to automatically run.
RUN=1

LIBS += ./libgcc.a
LIB_POST := ./libgcc.a

include $(CS140E_2023_PATH)/libpi/mk/Makefile.template-v2
