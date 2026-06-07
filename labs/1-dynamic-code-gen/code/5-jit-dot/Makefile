PROGS = test-pmu.c
PROGS = 2-full-test.c
# PROGS = 1-simple-test.c

COMMON_SRC = cache-support.S  jit-dotproduct.c

CFLAGS += -I../armv6-encodings

# define this if you need to give the device for your pi
TTYUSB = 
BOOTLOADER = my-install

# uncomment if you want it to automatically run.
RUN=1

include $(CS240LX_2026_PATH)/libpi/mk/Makefile.robust-v3
