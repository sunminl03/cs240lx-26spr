# run all tests
# PROGS += tests-memtrace/0-test-basic.c
PROGS += tests-memtrace/1-test-multi-faults.c

# basic common source.
COMMON_SRC += memmap-default.c
COMMON_SRC += sbrk-trap.c

# part 1: switch
COMMON_SRC += memtrace.c
# STAFF_OBJS += staff-memtrace.o

INC += -I./includes

O = $(CS240LX_2026_PATH)/libpi/staff-objs/
STAFF_OBJS += $(O)/staff-kmalloc.o
STAFF_OBJS += $(O)/interrupts-vec-asm.o
STAFF_OBJS += $(O)/staff-pinned-vm.o
STAFF_OBJS += $(O)/staff-mmu-asm.o
STAFF_OBJS += $(O)/staff-mmu.o
STAFF_OBJS += $(O)/staff-armv6-except.o
STAFF_OBJS += $(O)/staff-breakpoint.o
STAFF_OBJS += $(O)/staff-switchto-asm.o
STAFF_OBJS += $(O)/staff-full-except.o
STAFF_OBJS += $(O)/staff-full-except-asm.o
STAFF_OBJS += $(O)/map-user-to-staff-fn.o
STAFF_OBJS += $(O)/staff-watchpoint.o

RUN = 1

include $(CS240LX_2026_PATH)/libpi/mk/Makefile.robust-v3
