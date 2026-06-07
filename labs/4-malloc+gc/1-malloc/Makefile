PROGS += 0-test-malloc.c
PROGS += 2-test-malloc.c

COMMON_SRC = pi-sbrk.c

# swap these
COMMON_SRC += kr-malloc.c 
# STAFF_OBJS = staff-kr-malloc.o

RUN=1

EXCLUDE ?= grep -v simple_boot
include $(CS240LX_2026_PATH)/libpi/mk/Makefile.robust-v3
