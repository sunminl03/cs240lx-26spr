# PROGS := 0-dma-example.c
# PROGS += 1-dma-blink.c
# PROGS += 2-dma-delay.c
# PROGS += 3-dma-blink.c
PROGS := 4-dma-inc.c
PROGS += 5-dma-add.c
PROGS += 6-dma-blink.c

# PROGS = riscv-dma-harness.c
# PROGS = mul-harness.c
# PROGS = riscv-dma-demo.c
# COMMON_SRC += printf.c
# STAFF_OBJS += staff-stamps.o
# STAFF_OBJS += staff-riscv-dma.o

# if you need to override the install
# BOOTLOADER = my-install

OPT_LEVEL = -Og

# automatically run the code
RUN = 1

LIBS += $(LIBM)
CFLAGS += $(LIBM_INC)
CFLAGS += -DPRINTF_DISABLE_SUPPORT_FLOAT=1 -DPRINTF_DISABLE_SUPPORT_EXPONENTIAL=1
CFLAGS += -DPRINTF_DISABLE_SUPPORT_PTRDIFF_T=1

# Note: we use a custom staff-start to put the stack pointer below our program rather than above
# it, so that if we get Big DMA Lookup Tables, they won't interfere with the stack.
# START := objs/staff-start.o
OUR_START = staff-start.S

include $(CS240LX_2026_PATH)/libpi/mk/Makefile.robust-v3

.PHONY: FORCE
riscv_demo_hello.bin: FORCE
	riscv64-elf-gcc -mabi=ilp32 -march=rv32ia_zicsr_zifencei riscv_demo_hello.S -nostdlib -ffreestanding -o riscv_demo_hello.elf
	riscv64-elf-objcopy riscv_demo_hello.elf -O binary $@
