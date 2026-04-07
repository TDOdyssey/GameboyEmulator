#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "gameboy.h"

enum gb_ram_section gb_ram_section_from_addr(uint16_t addr)
{
    if(addr >= 0x0000 && addr < 0x4000) {
        return ROM_BANK_00;
    } else if(addr >= 0x4000 && addr < 0x8000) {
        return ROM_BANK_NN;
    } else if(addr >= 0x8000 && addr < 0xA000) {
        return VRAM;
    } else if(addr >= 0xA000 && addr < 0xC000) {
        return EXT;
    } else if(addr >= 0xC000 && addr < 0xD000) {
        return WRAM;
    } else if(addr >= 0xD000 && addr < 0xE000) {
        return WRAM;
    } else if(addr >= 0xE000 && addr < 0xFE00) {
        return ECHO;
    } else if(addr >= 0xFE00 && addr < 0xFEA0) {
        return OAM;
    } else if(addr >= 0xFEA0 && addr < 0xFF00) {
        return PROHIBITED;
    } else if(addr >= 0xFF00 && addr < 0xFF80) {
        return IO;
    } else if(addr >= 0xFF80 && addr < 0xFFFF) {
        return HRAM;
    } else if(addr == 0xFFFF) {
        return IE;
    }

    return PROHIBITED;
}

void write8(GameBoy *gb, uint16_t addr, uint8_t w)
{
    // Might move this logic out of this function...
    enum gb_ram_section section = gb_ram_section_from_addr(addr);

    if(section == PROHIBITED)
        return;
    else if(section == ECHO)
        addr -= 0x2000;
    
    gb->cpu.memory[addr] = w;
}

uint8_t read8(GameBoy *gb, uint16_t addr)
{
    enum gb_ram_section section = gb_ram_section_from_addr(addr);

    if(section == PROHIBITED)
        return 0x00;
    else if(section == ECHO)
        addr -= 0x2000;

    return gb->cpu.memory[addr];
}




// MMU Abstraction






// Instructions

// Variations
uint8_t *r8(GameBoy *gb, uint8_t val) // val is 3 bits
{
    val &= 0x07; // bottom 3 bits

    switch (val)
    {
        case 0: // b
            return &gb->cpu.B;
        case 1:
            return &gb->cpu.C;
        case 2:
            return &gb->cpu.D;
        case 3:
            return &gb->cpu.E;
        case 4:
            return &gb->cpu.H;
        case 5:
            return &gb->cpu.L;
        case 6:
            return &gb->cpu.memory[gb->cpu.HL];
        case 7:
            return &gb->cpu.A;
    }

    // We should never get here...
    return 0;
}

uint8_t read_imm8(GameBoy *gb)
{
    return read8(gb, ++gb->cpu.PC);
}

uint16_t read_imm16(GameBoy *gb)
{
    uint8_t low = read_imm8(gb);
    uint8_t high = read_imm8(gb);
    return (high << 8) | low;
}

void set_flag(GameBoy *gb, enum GBCPU_flags flag, bool value)
{
    if(value)
        gb->cpu.F |= flag;
    else
        gb->cpu.F &= ~flag;
}

int get_flag(GameBoy *gb, enum GBCPU_flags flag)
{
    return (gb->cpu.F & flag) ? 1 : 0;
}

uint16_t pop(GameBoy *gb)
{
    uint8_t low = read8(gb, gb->cpu.SP++);
    uint8_t high = read8(gb, gb->cpu.SP++);
    return (high << 8) | low;
}

void push(GameBoy *gb, uint16_t val)
{
    uint8_t low = val & 0xFF;
    uint8_t high = (val >> 8) & 0xFF;
    write8(gb, --gb->cpu.SP, high);
    write8(gb, --gb->cpu.SP, low);
}

void ADD_R(GameBoy *gb, uint8_t val, bool use_carry)
{
    uint8_t carry = use_carry ? get_flag(gb, c) : 0;
    uint16_t result = (uint16_t)(gb->cpu.A) + (uint16_t)(val) + carry;

    bool half_carry = (gb->cpu.A & 0xF) + (val & 0xF) + carry > 0xF;

    gb->cpu.A = result & 0xFF;

    set_flag(gb, z, gb->cpu.A == 0);
    set_flag(gb, n, false);
    set_flag(gb, h, half_carry);
    set_flag(gb, c, result > 0xFF);

}

void ADD_HL(GameBoy *gb, uint16_t val)
{
    uint32_t result = gb->cpu.HL + val;
    
    set_flag(gb, n, 0);
    set_flag(gb, h, (gb->cpu.HL & 0xFFF) + (val & 0xFFF) > 0xFFF);
    set_flag(gb, c, result > 0xFFFF);
    
    gb->cpu.HL = result & 0xFFFF;
}

void SUB_R(GameBoy *gb, uint8_t val, bool use_carry)
{
    uint8_t carry = use_carry ? get_flag(gb, c) : 0;
    uint8_t result = (uint16_t)gb->cpu.A - (val + carry);

    bool half_carry = (gb->cpu.A & 0x0F) < ((val & 0x0F) + carry);

    set_flag(gb, z, result == 0);
    set_flag(gb, n, true);
    set_flag(gb, h, half_carry);
    set_flag(gb, c, gb->cpu.A < val + carry);

    gb->cpu.A = result;
}

void CP_R(GameBoy *gb, uint8_t val)
{
    uint8_t result = gb->cpu.A - val;

    bool half_carry = (gb->cpu.A & 0x0F) < (val & 0x0F);

    set_flag(gb, z, result == 0);
    set_flag(gb, n, true);
    set_flag(gb, h, half_carry);
    set_flag(gb, c, gb->cpu.A < val);
}

void INC_R(GameBoy *gb, uint8_t *reg)
{
    bool half_carry = ((*reg & 0x0F) + 1) > 0x0F;
    (*reg)++;
    set_flag(gb, z, *reg == 0);
    set_flag(gb, n, 0);
    set_flag(gb, h, half_carry);
}

uint8_t inc8(GameBoy *gb, uint8_t val)
{
    bool half_carry = ((val & 0x0F) + 1) > 0x0F;
    val++;
    set_flag(gb, z, val == 0);
    set_flag(gb, n, 0);
    set_flag(gb, h, half_carry);
    return val;
}

void DEC_R(GameBoy *gb, uint8_t *reg)
{
    bool half_carry = (*reg & 0x0F) == 0;
    (*reg)--;
    set_flag(gb, z, *reg == 0);
    set_flag(gb, n, 1);
    set_flag(gb, h, half_carry);
}

/*
#define X(code, cyc, text, impl) static void OP_##code(GameBoy *gb);
#include "opcodes.def"
#undef X

typedef struct {
    uint8_t op;
    void (*function)(GameBoy *gb);
    unsigned char cycles;
    const char *text;
} opcode_table_entry;
static const opcode_table_entry opcode_table[256] = {
#define X(code, cyc, text, impl)  [code] = {OP_##code, cyc, text},
#include "opcodes.def"
#undef X
};

#define X(code, cyc, text, impl) static void OP_##code(GameBoy *gb) { impl; }
#include "opcodes.def"
#undef X
*/

void execute_cb_op(GameBoy *gb, uint8_t op);

typedef struct {
    uint8_t code;
    unsigned char cycles;
    const char *text;
} opcode_table_entry;
static const opcode_table_entry opcode_table[256] = {
#define X(code, cyc, text, impl)  [code] = {code, cyc, text},
#include "opcodes.def"
#undef X
};


void execute_op(GameBoy *gb, uint8_t op_code)
{
    switch(op_code)
    {
#define X(code, cyc, text, impl) case code: { impl; } break;
#include "opcodes.def"
#undef X
        default: break;
    }
}

void execute_cb_op(GameBoy *gb, uint8_t op)
{

    /* TODO: Account for (HL) */

    uint8_t *reg = r8(gb, op); // val is 3 bits
    if(op <= 0b00001000) // rlc
    {
        uint8_t carry = ((*reg) >> 7) & 1;
        (*reg) <<= 1;
        (*reg) = ((*reg) & 0xFE) | carry;

        set_flag(gb, n | h, 0);
        set_flag(gb, z, (*reg) == 0);
        set_flag(gb, c, carry);
    }
    else if(op <= 0b00010000) // rrc
    {
        uint8_t carry = (*reg) & 1;
        (*reg) >>= 1;
        (*reg) = ((*reg) & 0x7F) | (carry << 7);

        set_flag(gb, n | h, 0);
        set_flag(gb, z, (*reg) == 0);
        set_flag(gb, c, carry);
    }
    else if(op <= 0b00011000) // rl
    {
        uint8_t carry = ((*reg) >> 7) & 1;
        (*reg) <<= 1;
        (*reg) = ((*reg) & 0x7F) | (get_flag(gb, c));

        set_flag(gb, n | h, 0);
        set_flag(gb, z, (*reg) == 0);
        set_flag(gb, c, carry);
    }
    else if(op <= 0b00100000) // rr
    {
        uint8_t carry = (*reg) & 1;
        (*reg) >>= 1;
        (*reg) = ((*reg) & 0x7F) | (get_flag(gb, c) << 7);

        set_flag(gb, n | h, 0);
        set_flag(gb, z, (*reg) == 0);
        set_flag(gb, c, carry);
    }
    else if(op <= 0b00101000) // sla
    {
        uint8_t carry = ((*reg) >> 7) & 1;
        (*reg) <<= 1;
        (*reg) &= 0xFE;

        set_flag(gb, n | h, 0);
        set_flag(gb, z, (*reg) == 0);
        set_flag(gb, c, carry);
    }
    else if(op <= 0b00110000) // sra
    {
        uint8_t carry = (*reg) & 1;
        (*reg) >>= 1;
        (*reg) = ((*reg) & 0x7F) | (((*reg) & 0x40) << 1);

        set_flag(gb, n | h, 0);
        set_flag(gb, z, (*reg) == 0);
        set_flag(gb, c, carry);
    }
    else if(op <= 0b00111000) // swap
    {
        uint8_t low = (*reg) & 0x0F;
        uint8_t high = (*reg) & 0xF0;

        *reg = (low << 4) | ((high >> 4) & 0x0F);

        set_flag(gb, n | h | c, 0);
        set_flag(gb, z, (*reg) == 0);
    }
    else if(op <= 0b01000000) // srl
    {
        uint8_t carry = (*reg) & 1;
        (*reg) >>= 1;
        (*reg) &= 0x7F;

        set_flag(gb, n | h, 0);
        set_flag(gb, z, (*reg) == 0);
        set_flag(gb, c, carry);
    }
    else if(op <= 0b10000000) // bit
    {
        uint8_t bit_index = ((*reg) & 0b00111000) >> 3;
        uint8_t bit = ((*reg) >> bit_index) & 0x01;

        set_flag(gb, n, 0);
        set_flag(gb, h, 1);
        set_flag(gb, z, bit == 0);
    }
    else if(op <= 0b11000000) // res
    {
        uint8_t bit_index = ((*reg) & 0b00111000) >> 3;
        *reg &= ~(1 << bit_index);
    }
    else                      // set
    {
        uint8_t bit_index = ((*reg) & 0b00111000) >> 3;
        *reg |= 1 << bit_index;
    }
}


void reset(GameBoy *gb)
{

}

bool handle_interrupts(GameBoy *gb)
{
    uint8_t IE = gb->cpu.memory[0xFFFF];
    uint8_t IF = gb->cpu.memory[0xFF0F];

    if((IE & IF) == 0)
        return false;

    if(!gb->ime)
        return false;
    
    gb->ime = false;

    int interrupt = -1;
    for(int i = 0; i < 5; i++)
    {
        if((IE & IF) & (1 << i))
        {
            interrupt = i;
            break;
        }
    }

    gb->cpu.memory[0xFF0F] &= ~(1 << interrupt);

    push(gb, gb->cpu.PC);

    static const uint16_t vector[5] = {
        0x40, // VBlank
        0x48, // LCD
        0x50, // Timer
        0x58, // Serial
        0x60, // Joypad
    };

    gb->cycles = 5;

    return true;
}

void clock(GameBoy *gb)
{
    if(gb->cycles > 0)
    {
        gb->cycles--;
        return;
    }

    if(handle_interrupts(gb))
        return;

    opcode_table_entry op = opcode_table[gb->cpu.memory[gb->cpu.PC]];

    gb->cycles = op.cycles - 1;
    execute_op(gb, op.code);

    if(gb->ime_delay > 0)
    {
        gb->ime_delay--;
        if(gb->ime_delay == 0)
            gb->ime = true;
    }

    gb->cpu.PC++;
}

void print_cpu_state(GameBoy *gb)
{
    system("clear");
    printf("Current instruction: %s\n", opcode_table[gb->cpu.memory[gb->cpu.PC]].text);
    printf("Cycles left: %d\n", gb->cycles);
    printf("Registers:\n");
    printf("A: 0x%02x\n", gb->cpu.A);
    printf("F: 0x%02x | Z=%d N=%d H=%d C=%d\n", gb->cpu.F, get_flag(gb, z), get_flag(gb, n), get_flag(gb, h), get_flag(gb, c));
    printf("B: 0x%02x\n", gb->cpu.B);
    printf("C: 0x%02x\n", gb->cpu.C);
    printf("D: 0x%02x\n", gb->cpu.D);
    printf("E: 0x%02x\n", gb->cpu.E);
    printf("H: 0x%02x\n", gb->cpu.H);
    printf("L: 0x%02x\n", gb->cpu.L);
    printf("PC: 0x%04x\n", gb->cpu.PC);
    printf("SP: 0x%04x\n", gb->cpu.SP);
    printf("Immediate (n): 0x%x\n", gb->cpu.memory[gb->cpu.PC + 1]);
}

