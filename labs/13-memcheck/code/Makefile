# all (non-shadow) tests.  after you verify they pass with
# staff, work through one at a time.

# sanity check that your memtrace works.
# PROGS := tests-memtrace/0-test-basic.c
# PROGS += tests-memtrace/1-test-multi-faults.c
# PROGS += tests-memtrace/2-test-multiple-sizes.c
# PROGS += tests-memtrace/3-test-struct.c
# PROGS := $(wildcard tests-memtrace/[0123456]-test-*.c)

# simple purify tests
# PROGS := tests-purify/1-purify-bug.c
# PROGS += tests-purify/2-purify-bug.c
# PROGS += tests-purify/3-purify-free-bug.c
# PROGS += tests-purify/4-purify-bug.c
# PROGS += tests-purify/5-purify-no-bug.c
# PROGS := tests-purify/6-purify-no-bug-slow.c
PROGS := $(wildcard tests-purify/[0123456]-purify-*.c)


# All tests.
# PROGS := $(wildcard tests-purify/[0123456]-purify-*.c)
# PROGS += $(wildcard tests-memtrace/[0123456]-test-*.c)


# basic common source.
COMMON_SRC += memmap-default.c
COMMON_SRC += sbrk-trap.c

# part 1: you'll have to copy your memtrace.c over from
# last lab
COMMON_SRC += memtrace.c
# STAFF_OBJS += staff-memtrace.o

# part 2: switch these
COMMON_SRC += checker-purify.c
# STAFF_OBJS += staff-checker-purify.o

# part 3: you'll have to copy your ckalloc.c over from
# the memory debug lab.
COMMON_SRC += ckalloc.c
COMMON_SRC += breakpoint.c
COMMON_SRC += watchpoint.c
# STAFF_OBJS += staff-ckalloc.o

INC += -I./includes

O = $(CS240LX_2026_PATH)/libpi/staff-objs/
STAFF_OBJS += $(O)/staff-kmalloc.o
STAFF_OBJS += $(O)/interrupts-vec-asm.o
STAFF_OBJS += $(O)/staff-pinned-vm.o
STAFF_OBJS += $(O)/staff-mmu-asm.o
STAFF_OBJS += $(O)/staff-mmu.o
STAFF_OBJS += $(O)/staff-armv6-except.o
# STAFF_OBJS += $(O)/staff-breakpoint.o
STAFF_OBJS += $(O)/staff-switchto-asm.o
STAFF_OBJS += $(O)/staff-full-except.o
STAFF_OBJS += $(O)/staff-full-except-asm.o
STAFF_OBJS += $(O)/map-user-to-staff-fn.o
# STAFF_OBJS += $(O)/staff-watchpoint.o

RUN = 1

include $(CS240LX_2026_PATH)/libpi/mk/Makefile.robust-v3
