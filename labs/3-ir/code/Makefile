# list out the driver program source 
# PROGS := 0-dump-driver.c 
# PROGS := 1-value.c
PROGS := 2-interrupts.c
# if you have auxilary files, add them here.
COMMON_SRC :=

# for extensions if need threads.
L := $(CS240LX_2026_PATH)/libpi
STAFF_OBJS += $(L)/staff-objs/staff-rpi-thread.o
STAFF_OBJS += $(L)/staff-objs/staff-rpi-thread-asm.o
STAFF_OBJS += $(L)/staff-objs/gpio-int.o

# set RUN = 0 if you don't want the code to automatically run after building.
RUN = 1
OPT_LEVEL = -O2

include $(CS240LX_2026_PATH)/libpi/mk/Makefile.robust-v3

