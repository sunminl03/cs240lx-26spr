# PROGS = driver-gyro.c
# PROGS = driver-accel.c
# PROGS = driver-gyro-selftest.c
# PROGS = driver-accel-selftest.c
PROGS = driver-temp.c

# when you build yours swap this.
COMMON_SRC += mpu-6050.c
COMMON_SRC += mbox.c
# STAFF_OBJS += staff-mpu-6050.o

# set to 0 if you don't want it to automatically run.
RUN=1

# define this if you need to give the device for your pi
TTYUSB = 
BOOTLOADER = my-install

# for division
# LIBS += $(LIBM)
# CFLAGS += $(LIBM_INC)
include $(CS240LX_2026_PATH)/libpi/mk/Makefile.robust-v3
