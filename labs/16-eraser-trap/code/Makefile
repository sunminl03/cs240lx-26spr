# What to do:
#   1. Run the 0-* tests: they should pass out of the box.
#   2. Start building the rest of the system.
#   3. Drop in your code and replace ours!

# We give the initial Eraser implementation.  It's dumb: 
# only tracks one lock, one heap location.
COMMON_SRC += checker-eraser.c

# the 0- tests require the dumb initial state machine. 
# PROGS := tests-eraser/0-eraser-test0.c
# PROGS += tests-eraser/0-eraser-test1.c
# PROGS += tests-eraser/0-eraser-test2-bug.c
# PROGS += tests-eraser/0-eraser-test3-bug.c
# PROGS += tests-eraser/0-eraser-test4.c
# PROGS += tests-eraser/0-eraser-test5-bug.c

# these tests require a sophisticated state machine:
#   STAFF_OBJS += staff-checker-eraser.o
# PROGS := tests-eraser/1-eraser-test0.c
# PROGS := tests-eraser/1-eraser-test1-bug.c
# PROGS := tests-eraser/1-eraser-test2.c
# PROGS := tests-eraser/1-eraser-test3-bug.c 
# PROGS := tests-eraser/1-eraser-test4-bug.c 

# the complete full set of tests.
PROGS := $(wildcard tests-eraser/0-eraser-test[12345]*.c)
PROGS += $(wildcard tests-eraser/1-eraser-test[12345]*.c)

# PROGS := tests-eraser/2-eraser-test0.c

# where memtrace is.
MEMT_DIR = $(CS240LX_2026_PATH)/labs/13-memcheck/code

INC += -I./includes -I$(MEMT_DIR) -I$(MEMT_DIR)/includes/ 

# basic common source.
COMMON_SRC += $(MEMT_DIR)/memmap-default.c
COMMON_SRC += $(MEMT_DIR)/sbrk-trap.c

######################################################
# swap yours in!
# COMMON_SRC += $(MEMT_DIR)/memtrace.c
STAFF_OBJS += $(MEMT_DIR)/staff-memtrace.o

# COMMON_SRC += staff-ckalloc.c
STAFF_OBJS += $(MEMT_DIR)/staff-ckalloc.o

######################################################

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
