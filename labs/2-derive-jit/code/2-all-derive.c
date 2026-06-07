#include "rpi.h"

#define SENTINAL ".word 0x12345678;"

static uint32_t *find_sentinal(uint32_t *p, unsigned max_len) {
    for(int i = 0; i < max_len; i++)
        if(p[i] == 0x12345678)
            return &p[i];
    panic("did not find sentinal after %d instructions\n", max_len);
    return 0;
}

#define BITS_STR(_x) ({                            \
    uint32_t b = _x;                                \
    static char buf[33]={0};                        \
    for(uint32_t i = 0; i < 32; i++)                \
        buf[i] = "01"[((b) >> i) & 1];               \
    buf;                                            \
})

// return 1 if x has (1) at least one bit set to 1, (2) all 
// one bits are contiguous.
static inline int iscontig32(uint32_t x) {
    if(!x)
        return 0;

    // cute trick to test for contige bits from "hacker's delight"
    // https://stackoverflow.com/a/62714182
    //
    // let lb = lowest 1 bit.
    // let ub = highest 1 bit.
    // 1. x & -x ==> get low bit lb
    //    - common trick to detect if power-of-2: (x&-x == x)
    // 2. if x only has contig bits, adding (1) to x clears all bits
    //    in the contig block, leaving only a 1 at ub+1.
    // 3. x & (2) = 0, then the 1s were all together.  otherwise there
    //    will be at least one a 1 bit.
    return (x & (x + (x & -x))) == 0;
}

// static void derive_mov_src_reg(void) {
//     asm volatile (
//         SENTINAL
//         "mov r0, r0;"
//         "mov r0, r1;"
//         "mov r0, r2;"
//         "mov r0, r3;"
//         "mov r0, r4;"
//         "mov r0, r5;"
//         "mov r0, r6;"
//         "mov r0, r7;"
//         "mov r0, r8;"
//         "mov r0, r9;"
//         "mov r0, r10;"
//         "mov r0, r11;"
//         "mov r0, r12;"
//         "mov r0, r13;"
//         "mov r0, r14;"
//         "mov r0, r15;"
//         SENTINAL
//     );
// }

static void derive_mov_src_imm8(void) {
    asm volatile (
        SENTINAL
        "mov r0, #0;"
        "mov r0, #255;"
        SENTINAL
    );
}
#define FOR_EACH_REG(M) \
    M(0)  M(1)  M(2)  M(3)  M(4)  M(5)  M(6)  M(7) \
    M(8)  M(9)  M(10) M(11) M(12) M(13) M(14) M(15)

#define FOR_EACH_REG_14(M) \
    M(0)  M(1)  M(2)  M(3)  M(4)  M(5)  M(6)  M(7) \
    M(8)  M(9)  M(10) M(11) M(12) M(13) M(14)

#define MOV_DST_CASE(r) "mov r" #r ", r0;\n"
static void derive_mov_dst(void) {
    asm volatile (
        SENTINAL
        FOR_EACH_REG(MOV_DST_CASE)
        SENTINAL
    );
}
#define MOV_IMM8_ROT4_DST_CASE(r) "mov r" #r ", r0;\n"
static void derive_mov_imm8_rot4_rd(void) {
    asm volatile (
        SENTINAL
        FOR_EACH_REG(MOV_IMM8_ROT4_DST_CASE)
        SENTINAL
    );
}

static void derive_mov_imm8_rot4_imm8(void) {
    // 60 inputs 
    asm volatile (  
        SENTINAL
        "mov r0, #0x00;"
        "mov r0, #0xFF;"
        SENTINAL
    );
}

static void derive_mov_imm8_rot4_rot4(void) {
    // 60 inputs 
    asm volatile (  
        SENTINAL
        "mov r0, #1, 0;"    // rot4 = 0
        "mov r0, #1, 30;"   // rot4 = 15 (since rot amount = 2*rot4)
        SENTINAL
    );
}

static void derive_mvn_imm8_imm8(void) {
    asm volatile (
        SENTINAL
        "mvn r0, #0;"
        "mvn r0, #0xFF;"
        SENTINAL
    );
}

#define MVN_DST_CASE(r) "mvn r" #r ", #0;\n"
static void derive_mvn_imm8_rd(void) {
    asm volatile (
        SENTINAL
        FOR_EACH_REG(MVN_DST_CASE)
        SENTINAL
    );
}

static void derive_orr_imm8_rot4_imm8(void) {
    asm volatile (
        SENTINAL
        "orr r0, r0, #0;"
        "orr r0, r0, #0xFF;"
        SENTINAL
    );
}

static void derive_orr_imm8_rot4_rot4(void) {
    asm volatile (
        SENTINAL
        "orr r0, r0, #1, 0;" 
        "orr r0, r0, #1, 30;"
        SENTINAL
    );
}

#define ORR_DST_CASE(r) "orr r" #r ", r0, #0;\n"    
static void derive_orr_imm8_rot4_rd(void) {
    asm volatile (
        SENTINAL
        FOR_EACH_REG(ORR_DST_CASE)
        SENTINAL
    );
}
#define ORR_RN_CASE(r)  "orr r0, r" #r ", #0;\n"
static void derive_orr_imm8_rot4_rn(void) {
    asm volatile (
        SENTINAL
        FOR_EACH_REG(ORR_RN_CASE)
        SENTINAL
    );
}

#define MUL_DST_CASE(r) "mul r" #r ", r0, r0;\n"  
static void derive_mul_rd(void) {
    asm volatile (
        SENTINAL
        FOR_EACH_REG_14(MUL_DST_CASE)
        SENTINAL
    );
}

#define MUL_RM_CASE(r) "mul r0, r" #r ", r0;\n"
static void derive_mul_rm(void) {
    asm volatile (
        SENTINAL
        FOR_EACH_REG_14(MUL_RM_CASE)
        SENTINAL
    );
}

#define MUL_RS_CASE(r) "mul r0, r0, r" #r ";\n"
static void derive_mul_rs(void) {
    asm volatile (
        SENTINAL
        FOR_EACH_REG_14(MUL_RS_CASE)
        SENTINAL
    );
}

#define MLA_DST_CASE(r) "mla r" #r ", r0, r0, r0;\n"
static void derive_mla_rd(void) {
    asm volatile (
        SENTINAL
        FOR_EACH_REG_14(MLA_DST_CASE)
        SENTINAL
    );
}

#define MLA_RM_CASE(r) "mla r0, r" #r ", r0, r0;\n"
static void derive_mla_rm(void) {
    asm volatile (
        SENTINAL
        FOR_EACH_REG_14(MLA_RM_CASE)
        SENTINAL
    );
}

#define MLA_RS_CASE(r) "mla r0, r0, r" #r ", r0;\n"
static void derive_mla_rs(void) {
    asm volatile (
        SENTINAL
        FOR_EACH_REG_14(MLA_RS_CASE)
        SENTINAL
    );
}

#define MLA_RN_CASE(r) "mla r0, r0, r0, r" #r ";\n"
static void derive_mla_rn(void) {
    asm volatile (
        SENTINAL
        FOR_EACH_REG_14(MLA_RN_CASE)
        SENTINAL
    );
}

#define LDR_OFF12_RD_CASE(r) "ldr r" #r ", [r0, #0];\n"
static void derive_ldr_off12_rd(void) {
    asm volatile (
        SENTINAL
        FOR_EACH_REG(LDR_OFF12_RD_CASE)
        SENTINAL
    );
}

#define LDR_OFF12_RN_CASE(r) "ldr r0, [r" #r ", #0];\n"
static void derive_ldr_off12_rn(void) {
    asm volatile (
        SENTINAL
        FOR_EACH_REG(LDR_OFF12_RN_CASE)
        SENTINAL
    );
}

#define LDR_OFF12_OFF12_CASE(r) "ldr r0, [r0, #" #r "];\n"
static void derive_ldr_off12_off12(void) {
    asm volatile (
        SENTINAL
        "ldr r0, [r0, #0];"
        "ldr r0, [r0, #4095];"
        SENTINAL
    );
}

#define BX_RD_CASE(r) "bx r" #r ";\n"
static void derive_bx_rd(void) {
    asm volatile (
        SENTINAL
        FOR_EACH_REG(BX_RD_CASE)
        SENTINAL
    );
}

static void derive_bl_imm24(void) {
    asm volatile (
        // backward
        ".Lb0: nop;"
        ".Lb1: nop;"
        ".Lb2: nop;"
        SENTINAL
        // scanned region: BL only
        "bl .L0;"
        "bl .Lb0;"      // backward
        "bl .Lb1;"      // backward, 
        "bl .Lb2;"      // backward,
        "bl .Lf0;"      // forward
        "bl .Lf1;"      // forward
        "bl .Lf2;"      // forward
        SENTINAL
        // forward 
        ".L0:"
        ".Lf0: nop;"
        ".Lf1: nop; nop;"
        ".Lf2: nop; nop; nop;"
    );
}

#define SUBI_RD_CASE(r) "sub r" #r ", r0, #0;\n"
static void derive_sub_imm8_rd(void) {
    asm volatile (
        SENTINAL
        FOR_EACH_REG(SUBI_RD_CASE)
        SENTINAL
    );
}

#define SUBI_RN_CASE(r) "sub r0, r" #r ", #0;\n"
static void derive_sub_imm8_rn(void) {
    asm volatile (
        SENTINAL
        FOR_EACH_REG(SUBI_RN_CASE)
        SENTINAL
    );
}
static void derive_sub_imm8_imm8(void) {
    asm volatile (
        SENTINAL 
        "sub r0, r0, #0;"
        "sub r0, r0, #255;"
        SENTINAL
    );
}

#define STR_IMM8_RD_CASE(r) "str r" #r ", [r0, #0];\n"
static void derive_str_imm8_rd(void) {
    asm volatile (
        SENTINAL
        FOR_EACH_REG(STR_IMM8_RD_CASE)
        SENTINAL
    );
}

#define STR_IMM8_RN_CASE(r) "str r0, [r" #r ", #0];\n"
static void derive_str_imm8_rn(void) {
    asm volatile (
        SENTINAL
        FOR_EACH_REG(STR_IMM8_RN_CASE)
        SENTINAL
    );
}

static void derive_str_imm8_imm8(void) {
    asm volatile (
        SENTINAL
        "str r0, [r0, #0];"
        "str r0, [r0, #4095];"
        SENTINAL
    );
}

#define AND_IMM8_RD_CASE(r) "and r" #r ", r0, #0;\n"
static void derive_and_imm8_rd(void) {
    asm volatile (
        SENTINAL
        FOR_EACH_REG(AND_IMM8_RD_CASE)
        SENTINAL
    );
}

#define AND_IMM8_RN_CASE(r) "and r0, r" #r ", #0;\n"
static void derive_and_imm8_rn(void) {
    asm volatile (
        SENTINAL
        FOR_EACH_REG(AND_IMM8_RN_CASE)
        SENTINAL
    );
}

static void derive_and_imm8_imm8(void) {
    asm volatile (
        SENTINAL
        "and r0, r0, #0;"
        "and r0, r0, #255;"
        SENTINAL
    );
}

#define CMP_IMM8_RD_CASE(r) "cmp r" #r ", #0;\n"
static void derive_cmp_imm8_rd(void) {
    asm volatile (
        SENTINAL
        FOR_EACH_REG(CMP_IMM8_RD_CASE)
        SENTINAL
    );
}

static void derive_cmp_imm8_imm8(void) {
    asm volatile (
        SENTINAL
        "cmp r0, #0;"
        "cmp r0, #255;"
        SENTINAL
    );
}



static uint32_t 
solve_for_reg(const char *name, 
              void (*fn)(void), 
              uint32_t expected_inst) 
{
    enum { MAX_INST = 128 };
    uint32_t * inst = find_sentinal((void*)fn, MAX_INST) + 1;
    uint32_t * e = find_sentinal(inst, MAX_INST);
    unsigned n = e-inst;
    assert(n == expected_inst);

    output("------------------------------------------------\n");
    output("%s: %d instructions total\n", name, n);
    output("inst\thex encoding\tbinary encoding\n");
    for(int i = 0; i < n; i++) 
        output("%d:\t[%x]\t[%s]\n", i,inst[i], BITS_STR(inst[i]));

    // solve.
    output("-------------------------------------\n");
    output("solve for changed bits\n");

    // solve for register bits by computing which
    // bits change.
    //
    // we do so by computing which bits never change:
    // everything else must belong to the register
    // field.
    //
    // - compute which bits are always 0.
    // - compute which bits are always 1.
    // - bits that change (the negation of these) are
    //   the bits used to encode the register
    uint32_t always_0 = ~0;
    uint32_t always_1 = ~0;
    for(int i = 0; i < n; i++) {
        always_0 &= ~inst[i];
        always_1 &= inst[i];
    }

    // bits that changed were (definitionally): not always
    // always 0 or always 1.
    uint32_t changed = ~(always_0 | always_1);
    // bits that never changed: union of always 0 and always 1.
    uint32_t unchanged = (always_0 | always_1);

    // various tautologies (but good to check to ensure they are :).
    assert(unchanged == ~changed);
    assert(changed == ~unchanged);
    assert((changed & unchanged) == 0);
    assert((changed | unchanged) == ~0);

    output("always_0   = [%s]\n", BITS_STR(always_0));
    output("always_1   = [%s]\n", BITS_STR(always_1));
    output("changed    = [%s]\n", BITS_STR(changed));
    output("unchanged  = [%s]\n", BITS_STR(unchanged));

    // 32 bits: print which ones changed.
    for(unsigned i = 0; i < 32; i++) {
        if(changed & (1<<i))
            output("bit=%d changed: part of %s\n",i, name);
    }

    if(fn != derive_bl_imm24 && !iscontig32(changed))
        panic("reg should be contig: is [%s]\n", BITS_STR(changed));
    return changed;
}

uint32_t lowest_bit(uint32_t x) {
    for (int i = 0; i < 32; i++) {
        if (x & 1) {
            return i;
        }
        x >>= 1;
    }
    return 32;
}

/**
GEN FUNCTIONS FOR EACH INSTRUCTION
**/

void gen_mov(uint32_t opcode, uint32_t dst, uint32_t src) {
    output ("static inline uint32_t armv6_mov(uint32_t dst, uint32_t src) { \n");
    output ("   return %x | dst << %x | src << %x \n", opcode, dst, src);
    output ("}\n");
}

void gen_mov_imm8_rot4(uint32_t opcode, uint32_t rd, uint32_t imm8, uint32_t rot4) {
    output ("static inline uint32_t armv6_mov_imm8_rot4(uint32_t rd, uint32_t imm8, uint32_t rot4) { \n");
    output ("   return %x | rd << %x | imm8 << %x | rot4 << %x \n", opcode, rd, imm8, rot4);
    output ("}\n");
}

void gen_mvn_imm8(uint32_t opcode, uint32_t rd, uint32_t imm8) {
    output ("static inline uint32_t armv6_mvn_imm8(uint32_t rd, uint32_t imm8) { \n");
    output ("   return %x | rd << %x | imm8 << %x \n", opcode, rd, imm8);
    output ("}\n");
}

void gen_orr_imm8_rot4(uint32_t opcode, uint32_t rd, uint32_t rn, uint32_t imm8, uint32_t rot4) {
    output ("static inline uint32_t armv6_orr_imm8_rot4(uint32_t rd, uint32_t rn, uint32_t imm8, uint32_t rot4) { \n");
    output ("   return %x | rd << %x | rn << %x | rot4 << %x | imm8 << %x  \n", opcode, rd, rn, rot4, imm8);
    output ("}\n");
}

void gen_mul(uint32_t opcode, uint32_t rd, uint32_t rm, uint32_t rs) {
    output ("static inline uint32_t armv6_mul(uint32_t rd, uint32_t rm, uint32_t rs) { \n");
    output ("   return %x | rd << %x | rm << %x | rs << %x \n", opcode, rd, rm, rs);
    output ("}\n");
}

void gen_mla(uint32_t opcode, uint32_t rd, uint32_t rm, uint32_t rs, uint32_t rn) {
    output ("static inline uint32_t armv6_mla(uint32_t rd, uint32_t rm, uint32_t rs, uint32_t rn) { \n");
    output ("   return %x | rd << %x | rm << %x | rs << %x | rn << %x \n", opcode, rd, rm, rs, rn);
    output ("}\n");
}

void gen_ldr_off12(uint32_t opcode, uint32_t rd, uint32_t rn, uint32_t off12) {
    output ("static inline uint32_t armv6_ldr_off12(uint32_t rd, uint32_t rn, uint32_t off12) { \n");
    output ("   return %x | rd << %x | rn << %x | off12 << %x \n", opcode, rd, rn, off12);
    output ("}\n");
}

void gen_bx(uint32_t opcode, uint32_t rd) {
    output ("static inline uint32_t armv6_bx(uint32_t rd) { \n");
    output ("   return %x | rd << %x \n", opcode, rd);
    output ("}\n");
}

void gen_bl_imm24(uint32_t opcode, uint32_t imm24) {
    output ("static inline uint32_t armv6_bl_imm24(uint32_t imm24) { \n");
    output ("   return %x | imm24 << %x \n", opcode, imm24);
    output ("}\n");
}

void gen_sub(uint32_t opcode, uint32_t rd, uint32_t rn, uint32_t imm8) {
    output ("static inline uint32_t armv6_sub(uint32_t rd, uint32_t rn, uint32_t imm8) { \n");
    output ("   return %x | rd << %x | rn << %x | imm8 << %x \n", opcode, rd, rn, imm8);
    output ("}\n");
}

void gen_str(uint32_t opcode, uint32_t rd, uint32_t rn, uint32_t imm8) {
    output ("static inline uint32_t armv6_str(uint32_t rd, uint32_t rn, uint32_t imm8) { \n");
    output ("   return %x | rd << %x | rn << %x | imm8 << %x \n", opcode, rd, rn, imm8);
    output ("}\n");
}

void gen_and(uint32_t opcode, uint32_t rd, uint32_t rn, uint32_t imm8) {
    output ("static inline uint32_t armv6_and(uint32_t rd, uint32_t rn, uint32_t imm8) { \n");
    output ("   return %x | rd << %x | rn << %x | imm8 << %x \n", opcode, rd, rn, imm8);
    output ("}\n");
}

void gen_cmp(uint32_t opcode, uint32_t rd, uint32_t imm8) {
    output ("static inline uint32_t armv6_cmp(uint32_t rd, uint32_t imm8) { \n");
    output ("   return %x | rd << %x | imm8 << %x \n", opcode, rd, imm8);
    output ("}\n");
}

void notmain() { 
    // 1. 
    /* mov dst, src */

    // uint32_t src_reg = solve_for_reg("src_reg", derive_mov_src_reg, 16);
    // uint32_t src_imm8 = solve_for_reg("src_imm8", derive_mov_src_imm8, 2);
    // uint32_t dst  = solve_for_reg("dst", derive_mov_dst, 16);
    // uint32_t opcode = ~(src_imm8|dst);

    // uint32_t *inst = find_sentinal((void*)derive_mov_dst, 128) + 1;
    // uint32_t opcode_bits = inst[0] & opcode;

    // // output("src_reg    = [%s]\n", BITS_STR(src_reg));
    // output("src_imm8   = [%s]\n", BITS_STR(src_imm8));
    // output("dst     = [%s]\n", BITS_STR(dst));
    // output("opcode  = [%s]\n", BITS_STR(opcode));

    // gen_mov(opcode_bits, lowest_bit(dst), lowest_bit(src_imm8));

    // 2. 
    /* mov rd, imm8, rot4*/

    // uint32_t rd = solve_for_reg("rd", derive_mov_imm8_rot4_rd, 16);
    // uint32_t imm8 = solve_for_reg("imm8", derive_mov_imm8_rot4_imm8, 2);
    // uint32_t rot4 = solve_for_reg("rot4", derive_mov_imm8_rot4_rot4, 2);
    // uint32_t opcode = ~(rd|imm8|rot4);

    // uint32_t *inst = find_sentinal((void*)derive_mov_imm8_rot4_imm8, 128) + 1;
    // uint32_t opcode_bits = inst[0] & opcode;

    // output("rd      = [%s]\n", BITS_STR(rd));
    // output("imm8    = [%s]\n", BITS_STR(imm8));
    // output("rot4    = [%s]\n", BITS_STR(rot4));
    // output("opcode  = [%s]\n", BITS_STR(opcode));

    // gen_mov_imm8_rot4(opcode_bits, lowest_bit(rd), lowest_bit(imm8), lowest_bit(rot4));

    // 3. 
    /* mvn_imm8 rd, imm8*/

    // uint32_t rd = solve_for_reg("rd", derive_mvn_imm8_rd, 16);
    // uint32_t imm8 = solve_for_reg("imm8", derive_mvn_imm8_imm8, 2);
    // uint32_t opcode = ~(rd|imm8);

    // uint32_t *inst = find_sentinal((void*)derive_mvn_imm8_rd, 128) + 1;
    // uint32_t opcode_bits = inst[0] & opcode;

    // output("rd      = [%s]\n", BITS_STR(rd));
    // output("imm8    = [%s]\n", BITS_STR(imm8));
    // output("opcode  = [%s]\n", BITS_STR(opcode));

    // gen_mvn_imm8(opcode_bits, lowest_bit(rd), lowest_bit(imm8));

    // 4. 
    /* orr_imm8_rot4 rd, rn, imm8, rot4*/

    // uint32_t rd = solve_for_reg("rd", derive_orr_imm8_rot4_rd, 16);
    // uint32_t rn = solve_for_reg("rn", derive_orr_imm8_rot4_rn, 16);
    // uint32_t imm8 = solve_for_reg("imm8", derive_orr_imm8_rot4_imm8, 2);
    // uint32_t rot4 = solve_for_reg("rot4", derive_orr_imm8_rot4_rot4, 2);
    // uint32_t opcode = ~(rd|rn|imm8|rot4);

    // uint32_t *inst = find_sentinal((void*)derive_orr_imm8_rot4_rd, 128) + 1;
    // uint32_t opcode_bits = inst[0] & opcode;

    // output("rd      = [%s]\n", BITS_STR(rd));
    // output("rn      = [%s]\n", BITS_STR(rn));
    // output("imm8    = [%s]\n", BITS_STR(imm8));
    // output("rot4    = [%s]\n", BITS_STR(rot4));
    // output("opcode  = [%s]\n", BITS_STR(opcode));

    // gen_orr_imm8_rot4(opcode_bits, lowest_bit(rd), lowest_bit(rn), lowest_bit(imm8), lowest_bit(rot4));


    // 5. 
    /* mult rd, rm, rs*/

    // uint32_t rd = solve_for_reg("rd", derive_mul_rd, 15);
    // uint32_t rm = solve_for_reg("rm", derive_mul_rm, 15);
    // uint32_t rs = solve_for_reg("rs", derive_mul_rs, 15);
    // uint32_t opcode = ~(rd|rm|rs);

    // uint32_t *inst = find_sentinal((void*)derive_mul_rd, 128) + 1;
    // uint32_t opcode_bits = inst[0] & opcode;

    // output("rd      = [%s]\n", BITS_STR(rd));
    // output("rm      = [%s]\n", BITS_STR(rm));
    // output("rs      = [%s]\n", BITS_STR(rs));
    // output("opcode  = [%s]\n", BITS_STR(opcode));

    // gen_mul(opcode_bits, lowest_bit(rd), lowest_bit(rm), lowest_bit(rs));

    // 6. 
    /* mla rd, rm, rs, rn*/

    // uint32_t rd = solve_for_reg("rd", derive_mla_rd, 15);
    // uint32_t rm = solve_for_reg("rm", derive_mla_rm, 15);
    // uint32_t rs = solve_for_reg("rs", derive_mla_rs, 15);
    // uint32_t rn = solve_for_reg("rn", derive_mla_rn, 15);
    // uint32_t opcode = ~(rd|rm|rs|rn);

    // uint32_t *inst = find_sentinal((void*)derive_mla_rd, 128) + 1;
    // uint32_t opcode_bits = inst[0] & opcode;

    // output("rd      = [%s]\n", BITS_STR(rd));
    // output("rm      = [%s]\n", BITS_STR(rm));
    // output("rs      = [%s]\n", BITS_STR(rs));
    // output("rn      = [%s]\n", BITS_STR(rn));
    // output("opcode  = [%s]\n", BITS_STR(opcode));

    // gen_mla(opcode_bits, lowest_bit(rd), lowest_bit(rm), lowest_bit(rs), lowest_bit(rn));

    // 7. 
    /* ldr_off12 rd, rn, off12 */

    // uint32_t rd = solve_for_reg("rd", derive_ldr_off12_rd, 16);
    // uint32_t rn = solve_for_reg("rn", derive_ldr_off12_rn, 16);
    // uint32_t off12 = solve_for_reg("off12", derive_ldr_off12_off12, 2);
    // uint32_t opcode = ~(rd|rn|off12);

    // uint32_t *inst = find_sentinal((void*)derive_ldr_off12_rd, 128) + 1;
    // uint32_t opcode_bits = inst[0] & opcode;

    // output("rd      = [%s]\n", BITS_STR(rd));
    // output("rn      = [%s]\n", BITS_STR(rn));
    // output("off12   = [%s]\n", BITS_STR(off12));
    // output("opcode  = [%s]\n", BITS_STR(opcode));

    // gen_ldr_off12(opcode_bits, lowest_bit(rd), lowest_bit(rn), lowest_bit(off12));

    // 8.
    /* bx rd */
    // uint32_t rd = solve_for_reg("rd", derive_bx_rd, 16);
    // uint32_t opcode = ~(rd);

    // uint32_t *inst = find_sentinal((void*)derive_bx_rd, 128) + 1;
    // uint32_t opcode_bits = inst[0] & opcode;

    // output("rd      = [%s]\n", BITS_STR(rd));
    // output("opcode  = [%s]\n", BITS_STR(opcode));

    // gen_bx(opcode_bits, lowest_bit(rd));

    // 9.
    /* bl imm24 */

    // uint32_t imm24 = solve_for_reg("imm24", derive_bl_imm24, 7);
    // uint32_t opcode = ~(imm24);

    // uint32_t *inst = find_sentinal((void*)derive_bl_imm24, 128) + 1;
    // uint32_t opcode_bits = inst[0] & opcode;

    // output("imm24   = [%s]\n", BITS_STR(imm24));
    // output("opcode  = [%s]\n", BITS_STR(opcode));

    // gen_bl_imm24(opcode_bits, lowest_bit(imm24));

    // 10. 
    /* sub rd, rn, imm8*/

    // uint32_t rd = solve_for_reg("rd", derive_sub_imm8_rd, 16);
    // uint32_t rn = solve_for_reg("rn", derive_sub_imm8_rn, 16);
    // uint32_t imm8 = solve_for_reg("imm8", derive_sub_imm8_imm8, 2);
    // uint32_t opcode = ~(rd|rn|imm8);

    // uint32_t *inst = find_sentinal((void*)derive_sub_imm8_rd, 128) + 1;
    // uint32_t opcode_bits = inst[0] & opcode;

    // output("rd      = [%s]\n", BITS_STR(rd));
    // output("rn      = [%s]\n", BITS_STR(rn));
    // output("imm8    = [%s]\n", BITS_STR(imm8));
    // output("opcode  = [%s]\n", BITS_STR(opcode));

    // gen_sub(opcode_bits, lowest_bit(rd), lowest_bit(rn), lowest_bit(imm8));

    // 11. 
    // str rd, [rn, #imm8]

    // uint32_t rd = solve_for_reg("rd", derive_str_imm8_rd, 16);
    // uint32_t rn = solve_for_reg("rn", derive_str_imm8_rn, 16);
    // uint32_t imm8 = solve_for_reg("imm8", derive_str_imm8_imm8, 2);
    // uint32_t opcode = ~(rd|rn|imm8);

    // uint32_t *inst = find_sentinal((void*)derive_str_imm8_rd, 128) + 1;
    // uint32_t opcode_bits = inst[0] & opcode;

    // output("rd      = [%s]\n", BITS_STR(rd));
    // output("rn      = [%s]\n", BITS_STR(rn));
    // output("imm8    = [%s]\n", BITS_STR(imm8));
    // output("opcode  = [%s]\n", BITS_STR(opcode));

    // gen_str(opcode_bits, lowest_bit(rd), lowest_bit(rn), lowest_bit(imm8));

    // 12. 
    // and rd, rn, imm8
    // uint32_t rd = solve_for_reg("rd", derive_and_imm8_rd, 16);
    // uint32_t rn = solve_for_reg("rn", derive_and_imm8_rn, 16);
    // uint32_t imm8 = solve_for_reg("imm8", derive_and_imm8_imm8, 2);
    // uint32_t opcode = ~(rd|rn|imm8);

    // uint32_t *inst = find_sentinal((void*)derive_and_imm8_rd, 128) + 1;
    // uint32_t opcode_bits = inst[0] & opcode;

    // output("rd      = [%s]\n", BITS_STR(rd));
    // output("rn      = [%s]\n", BITS_STR(rn));
    // output("imm8    = [%s]\n", BITS_STR(imm8));
    // output("opcode  = [%s]\n", BITS_STR(opcode));

    // gen_and(opcode_bits, lowest_bit(rd), lowest_bit(rn), lowest_bit(imm8));

    // 13.
    // cmp rd, imm8
    uint32_t rd = solve_for_reg("rd", derive_cmp_imm8_rd, 16);
    uint32_t imm8 = solve_for_reg("imm8", derive_cmp_imm8_imm8, 2);
    uint32_t opcode = ~(rd|imm8);

    uint32_t *inst = find_sentinal((void*)derive_cmp_imm8_rd, 128) + 1;
    uint32_t opcode_bits = inst[0] & opcode;

    output("rd      = [%s]\n", BITS_STR(rd));
    output("imm8    = [%s]\n", BITS_STR(imm8));
    output("opcode  = [%s]\n", BITS_STR(opcode));

    gen_cmp(opcode_bits, lowest_bit(rd), lowest_bit(imm8));

}




