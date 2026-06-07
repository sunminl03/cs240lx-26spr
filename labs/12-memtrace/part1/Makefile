PROGS := tests/0-basic.c 
PROGS := tests/1-multi.c
PROGS := tests/2-two.c
PROGS := tests/3-array.c

COMMON_SRC += trap-watchpt.c

INC += -I./includes

O = $(CS240LX_2026_PATH)/libpi/staff-objs/
STAFF_OBJS += $(O)/staff-kmalloc.o
STAFF_OBJS += $(O)/interrupts-vec-asm.o
STAFF_OBJS += $(O)/staff-pinned-vm.o
STAFF_OBJS += $(O)/staff-mmu-asm.o
STAFF_OBJS += $(O)/staff-mmu.o
STAFF_OBJS += $(O)/staff-armv6-except.o
STAFF_OBJS += $(O)/staff-breakpoint.o
STAFF_OBJS += $(O)/staff-watchpoint.o
STAFF_OBJS += $(O)/staff-switchto-asm.o
STAFF_OBJS += $(O)/staff-full-except.o
STAFF_OBJS += $(O)/staff-full-except-asm.o
STAFF_OBJS += $(O)/map-user-to-staff-fn.o



OPT_LEVEL = -O2

BOOTLOADER=my-install
RUN = 1

include $(CS240LX_2026_PATH)/libpi/mk/Makefile.robust-v3