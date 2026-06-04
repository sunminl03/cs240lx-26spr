PROGS = sig_write.c

COMMON_SRC += fat32.c
COMMON_SRC += fat32-helpers.c
COMMON_SRC += fat32-lfn-helpers.c
COMMON_SRC += mbr.c
COMMON_SRC += mbr-helpers.c
COMMON_SRC += pi-sd.c
COMMON_SRC += i2c.c
COMMON_SRC += ssd1306-display-driver.c
COMMON_SRC += standard-ascii-font.c
COMMON_SRC += emmc.c
COMMON_SRC += unicode-utf8.c

O := $(CS240LX_2026_PATH)/libpi
STAFF_OBJS += $(O)/staff-objs/staff-kmalloc.o
STAFF_OBJS += $(O)/staff-objs/gpio.o

TTYUSB =
BOOTLOADER = my-install
RUN = 1

include $(CS240LX_2026_PATH)/libpi/mk/Makefile.robust-v3
