#include <stdint.h>
#include <stdio.h>

typedef struct {
    // ------ CPU ------

    // Registers
    union {
        uint16_t AF;
        struct {
            uint8_t F;
            uint8_t A;
        };
    };
    union {
        uint16_t BC;
        struct {
            uint8_t C;
            uint8_t B;
        };
    };
    union {
        uint16_t DE;
        struct {
            uint8_t E;
            uint8_t D;
        };
    };
    union {
        uint16_t HL;
        struct {
            uint8_t L;
            uint8_t H;
        };
    };

    uint16_t SP; // Stack pointer
    uint16_t PC; // Program counter

    uint8_t IR;
    uint8_t IE;

    uint8_t memory[0x10000];
} GBCPU; // Sharp SM83 CPU core

enum GBCPU_flags {
    // Zero flag
    z = (1 << 7), // Set iff the result of an operation is zero. Used by conditional jumps

    // BCD flags used by the DAA instruction only
    n = (1 << 6), // indicates whether the previous instruction has been a subtraction
    h = (1 << 5), // indicates carry for the lower 4 bits of the result

    // Carry flag
    c = (1 << 4), // Is set: 
                    // when the result of an 8-bit addition is higher than $FF;
                    // when the result of a 16-bit addition is higher than $FFFF;
                    // when the result of a subtraction or compairson is lower than zero;
                    // when a rotate/shift operation shifts out a "1" bit.
};

typedef struct {
    GBCPU cpu;
} GameBoy;

// Memory map (https://gbdev.io/pandocs/Memory_Map.html)
// Start	End	    Description	                    Notes
// 0000	    3FFF	16 KiB ROM bank 00	            From cartridge, usually a fixed bank
// 4000	    7FFF	16 KiB ROM Bank 01–NN	        From cartridge, switchable bank via mapper (if any)
// 8000	    9FFF	8 KiB Video RAM (VRAM)	        In CGB mode, switchable bank 0/1
// A000	    BFFF	8 KiB External RAM	            From cartridge, switchable bank if any
// C000	    CFFF	4 KiB Work RAM (WRAM)	
// D000	    DFFF	4 KiB Work RAM (WRAM)	        In CGB mode, switchable bank 1–7
// E000	    FDFF	Echo RAM (mirror of C000–DDFF)	Nintendo says use of this area is prohibited.
// FE00	    FE9F	Object attribute memory (OAM)	
// FEA0	    FEFF	Not Usable	                    Nintendo says use of this area is prohibited.
// FF00	    FF7F	I/O Registers	
// FF80	    FFFE	High RAM (HRAM)	
// FFFF	    FFFF	Interrupt Enable register (IE)

enum gb_ram_section {
    ROM_BANK_00, // ??
    ROM_BANK_NN,
    VRAM, // Switchable 0/1 in CGB TODO
    EXT,
    WRAM, // last 4kb/8kb switchable 1-7 in CGB mode TODO
    PROHIBITED,
    OAM,
    IO,
    HRAM,
    IE,
};

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
        return PROHIBITED;
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
    
    gb->cpu.memory[addr] = w;
}

uint8_t read8(GameBoy *gb, uint16_t addr)
{
    enum gb_ram_section section = gb_ram_section_from_addr(addr);

    if(section == PROHIBITED)
        return 0x00;

    return gb->cpu.memory[addr];
}

void reset(GameBoy *gb)
{

}

void clock(GameBoy *gb)
{

}

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
            return &gb->cpu.memory[gb->cpu.A];
    }

    // We should never get here...
    return 0;
}

void LD_R_R(uint8_t *dst, uint8_t *src) { *dst = *src; }

uint16_t nn(GameBoy *gb)
{
    uint8_t low = gb->cpu.memory[++gb->cpu.PC];
    uint8_t high = gb->cpu.memory[++gb->cpu.PC];
    return (high << 8) | low;
}

uint16_t pop(GameBoy *gb)
{
    uint8_t low = gb->cpu.memory[gb->cpu.SP++];
    uint8_t high = gb->cpu.memory[gb->cpu.SP++];
    return (high << 8) | low;
}

void push(GameBoy *gb, uint16_t val)
{
    uint8_t low = val & 0xFF;
    uint8_t high = val >> 8;
    gb->cpu.memory[--gb->cpu.SP] = high;
    gb->cpu.memory[--gb->cpu.SP] = low;
}

#define X(code, cyc, text, impl) static void OP_##code(GameBoy *gb);
#include "opcodes.def"
#undef X

typedef struct {
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

void print_cpu_state(GameBoy *gb)
{
    printf("Current instruction: %s\n", opcode_table[gb->cpu.memory[gb->cpu.PC]].text);
    printf("Registers:\n");
    printf("A: 0x%02x\n", gb->cpu.A);
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

int main()
{
    GameBoy gb = {0};

    gb.cpu.memory[0x100] = 0xfa; // 
    gb.cpu.memory[0x101] = 0x34;
    gb.cpu.memory[0x102] = 0x12;
    gb.cpu.memory[0x1234] = 0x56;
    gb.cpu.memory[0x103] = 0x47;
    gb.cpu.memory[0x104] = 0x3e;
    gb.cpu.memory[0x105] = 0x99;
    gb.cpu.memory[0x106] = 0xea;
    gb.cpu.memory[0x107] = 0x78;
    gb.cpu.memory[0x108] = 0x56;

    gb.cpu.PC = 0x100;

    while(1)
    {
        print_cpu_state(&gb);

        char c = getchar();

        if(c == 'q')
            break;

        opcode_table[gb.cpu.memory[gb.cpu.PC]].function(&gb);
        gb.cpu.PC++;
    }
    
    printf("memory[0x5678] = %x\n", gb.cpu.memory[0x5678]);

    return 0;
}
