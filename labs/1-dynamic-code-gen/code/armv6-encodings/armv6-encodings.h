#ifndef __ARMV6_ENCODINGS_H__
#define __ARMV6_ENCODINGS_H__

enum {
    armv6_lr = 14,
    armv6_pc = 15,
    armv6_sp = 13,
    armv6_r0 = 0,

    op_mov = 0b1101,
    armv6_mvn = 0b1111,
    armv6_orr = 0b1100,
    op_mult = 0b0000,

    cond_always = 0b1110
};

// trivial wrapper for register values so type-checking works better.
typedef struct {
    uint8_t reg;
} reg_t;
static inline reg_t reg_mk(unsigned r) {
    if(r >= 16)
        panic("illegal reg %d\n", r);
    return (reg_t){ .reg = r };
}

// how do you add a negative? do a sub?
static inline uint32_t armv6_mov(reg_t rd, reg_t rn) {
    // could return an error.  could get rid of checks.
    //         I        opcode        | rd
    return cond_always << 28
        | op_mov << 21 
        | rd.reg << 12 
        | rn.reg
        ;
}

static inline uint32_t 
armv6_mov_imm8_rot4(reg_t rd, uint32_t imm8, unsigned rot4) {
    if(imm8>>8)
        panic("immediate %d does not fit in 8 bits!\n", imm8);
    if(rot4 % 2)
        panic("rotation %d must be divisible by 2!\n", rot4);
    rot4 /= 2;
    if(rot4>>4)
        panic("rotation %d does not fit in 4 bits!\n", rot4);

    // todo("implement mov with rotate\n");
    // mov opcode = 1110 0011 1010. this instruction will automatically decode and rotate 
    return (uint32_t) (0xE3A00000 | (rd.reg << 12) | (rot4 << 8) | imm8);

}
static inline uint32_t 
armv6_mov_imm8(reg_t rd, uint32_t imm8) {
    return armv6_mov_imm8_rot4(rd,imm8,0);
}

static inline uint32_t 
armv6_mvn_imm8(reg_t rd, uint32_t imm8) {
    // todo("implement mvn\n");
    // 1110 00111110 0000 Rd imm8
    return (uint32_t) (0xE3E00000 | (rd.reg << 12) | imm8);
}

static inline uint32_t 
armv6_bx(reg_t rd) {
    // todo("implement bx\n");
    return (0b1110000100101111111111110001) << 4 | rd.reg;
}

// use 8-bit immediate imm8, with a 4-bit rotation.
// rd = rn | ROR(imm8, 2*rot4)
static inline uint32_t 
armv6_orr_imm8_rot4(reg_t rd, reg_t rn, unsigned imm8, unsigned rot4) {
    if(imm8>>8)
        panic("immediate %d does not fit in 8 bits!\n", imm8);
    if(rot4 % 2)
        panic("rotation %d must be divisible by 2!\n", rot4);
    rot4 /= 2;
    if(rot4>>4)
        panic("rotation %d does not fit in 4 bits!\n", rot4);

    // todo("implement orr with rotation\n");
    return 0xE3800000 | (rn.reg << 16)| (rd.reg << 12)| (rot4 << 8)| imm8;
}

static inline uint32_t 
armv6_orr_imm8(reg_t rd, reg_t rn, unsigned imm8) {
    if(imm8>>8)
        panic("immediate %d does not fit in 8 bits!\n", imm8);

    // todo("implement orr with immediate\n");
    return 0xE3800000 | (rn.reg << 16)| (rd.reg << 12)| imm8;
}

// a4-80
static inline uint32_t 
armv6_mult(reg_t rd, reg_t rm, reg_t rs) {
    // todo("implement mult\n");
    return 0xE0000000| rd.reg << 16 | rs.reg << 8 | 0x90 | rm.reg;
}


// load a word from memory[offset]
// ldr rd, [rn,#offset]
static inline uint32_t 
armv6_ldr_off12(reg_t rd, reg_t rn, int offset) {
    // a5-20
    // todo("implement lrd_off12\n");
    // u bit determines adding or subtracting offset
    uint32_t u = 1;
    if(offset < 0) {
        u = 0;
        // negate so we get positive value in encoding 
        offset = -offset;
    }
    return 0xE5100000 | (u << 23) | rn.reg << 16 | rd.reg << 12 | offset;
}

/**********************************************************************
 * synthetic instructions.
 * 
 * these can result in multiple instructions generated so we have to 
 * pass in a location to store them into.
 */

static inline uint32_t *
armv6_load_imm32(uint32_t *code, reg_t rd, uint32_t imm32) {
    uint32_t part0 = (imm32 >> 0)  & 0xFF;
    uint32_t part1 = (imm32 >> 8)  & 0xFF;
    uint32_t part2 = (imm32 >> 16) & 0xFF;
    uint32_t part3 = (imm32 >> 24) & 0xFF;

    code[0] = armv6_mov_imm8_rot4(rd, part0, 0);

    int n = 1;
    
    // shift left by 8 
    if(part1) { 
        code[n++] = armv6_orr_imm8_rot4(rd, rd, part1, 24);
    }
    // shift left by 16
    if(part2) {
        code[n++] = armv6_orr_imm8_rot4(rd, rd, part2, 16);
    }
    if(part3) {
        code[n++] = armv6_orr_imm8_rot4(rd, rd, part3, 8);
    }
    
    return code + n;
}

// MLA (Multiply Accumulate) multiplies two signed or unsigned 32-bit
// values, and adds a third 32-bit value.
//
// a4-66: multiply accumulate.
//      rd = rm * rs + rn.
static inline uint32_t
armv6_mla(reg_t rd, reg_t rm, reg_t rs, reg_t rn) {    
    // todo("implement multiply accumulate\n");
    return 0xE0200000| rd.reg << 16 | rn.reg << 12 | rs.reg << 8 | 0x90 | rm.reg;
}

#endif
