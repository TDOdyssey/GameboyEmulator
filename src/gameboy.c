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

typedef struct {
    char *text;
    void (*function)(GameBoy *gb);
} opcode_table_entry;

opcode_table_entry opcode_table[256] = {0};

#define GB_OP(code, text) \
    static void OP_##code(GameBoy *gb); \
    __attribute__((constructor)) static void _register_##code(void) { opcode_table[code] = (opcode_table_entry){#text, OP_##code}; }; \
    static void OP_##code(GameBoy *gb)

GB_OP(0x00, "NOP") { /* TODO */ }
GB_OP(0x10, "STOP") { /* TODO */ }

// 8-bit load instructions
void LD_R_R(uint8_t *dst, uint8_t *src) { *dst = *src; }

GB_OP(0x02, "LD (BC), A")   { LD_R_R(&gb->cpu.memory[gb->cpu.BC], &gb->cpu.A); }
GB_OP(0x12, "LD (DE), A")   { LD_R_R(&gb->cpu.memory[gb->cpu.DE], &gb->cpu.A); }
GB_OP(0x22, "LD (HL+), A")  { LD_R_R(&gb->cpu.memory[gb->cpu.HL++], &gb->cpu.A); }
GB_OP(0x32, "LD (HL-), A")  { LD_R_R(&gb->cpu.memory[gb->cpu.HL--], &gb->cpu.A); }

GB_OP(0x06, "LD B, n")      { LD_R_R(&gb->cpu.B, &gb->cpu.memory[++gb->cpu.PC]); }
GB_OP(0x16, "LD D, n")      { LD_R_R(&gb->cpu.D, &gb->cpu.memory[++gb->cpu.PC]); }
GB_OP(0x26, "LD H, n")      { LD_R_R(&gb->cpu.H, &gb->cpu.memory[++gb->cpu.PC]); }
GB_OP(0x36, "LD (HL), n")   { LD_R_R(&gb->cpu.memory[gb->cpu.HL], &gb->cpu.memory[++gb->cpu.PC]); }

GB_OP(0x0a, "LD A, (BC)")   { LD_R_R(&gb->cpu.A, &gb->cpu.memory[gb->cpu.BC]); }
GB_OP(0x1a, "LD A, (DE)")   { LD_R_R(&gb->cpu.A, &gb->cpu.memory[gb->cpu.DE]); }
GB_OP(0x2a, "LD A, (HL+)")  { LD_R_R(&gb->cpu.A, &gb->cpu.memory[gb->cpu.HL++]); }
GB_OP(0x3a, "LD A, (HL-)")  { LD_R_R(&gb->cpu.A, &gb->cpu.memory[gb->cpu.HL--]); }

GB_OP(0x0e, "LD C, n")      { LD_R_R(&gb->cpu.C, &gb->cpu.memory[++gb->cpu.PC]); }
GB_OP(0x1e, "LD E, n")      { LD_R_R(&gb->cpu.E, &gb->cpu.memory[++gb->cpu.PC]); }
GB_OP(0x2e, "LD L, n")      { LD_R_R(&gb->cpu.L, &gb->cpu.memory[++gb->cpu.PC]); }
GB_OP(0x3e, "LD A, n")      { LD_R_R(&gb->cpu.A, &gb->cpu.memory[++gb->cpu.PC]); }

GB_OP(0x40, "LD B, B")      { LD_R_R(&gb->cpu.B, &gb->cpu.B); }
GB_OP(0x41, "LD B, C")      { LD_R_R(&gb->cpu.B, &gb->cpu.C); }
GB_OP(0x42, "LD B, D")      { LD_R_R(&gb->cpu.B, &gb->cpu.D); }
GB_OP(0x43, "LD B, E")      { LD_R_R(&gb->cpu.B, &gb->cpu.E); }
GB_OP(0x44, "LD B, H")      { LD_R_R(&gb->cpu.B, &gb->cpu.H); }
GB_OP(0x45, "LD B, L")      { LD_R_R(&gb->cpu.B, &gb->cpu.L); }
GB_OP(0x46, "LD B, (HL)")   { LD_R_R(&gb->cpu.B, &gb->cpu.memory[gb->cpu.HL]); }
GB_OP(0x47, "LD B, A")      { LD_R_R(&gb->cpu.B, &gb->cpu.A); }
GB_OP(0x48, "LD C, B")      { LD_R_R(&gb->cpu.C, &gb->cpu.B); }
GB_OP(0x49, "LD C, C")      { LD_R_R(&gb->cpu.C, &gb->cpu.C); }
GB_OP(0x4A, "LD C, D")      { LD_R_R(&gb->cpu.C, &gb->cpu.D); }
GB_OP(0x4B, "LD C, E")      { LD_R_R(&gb->cpu.C, &gb->cpu.E); }
GB_OP(0x4C, "LD C, H")      { LD_R_R(&gb->cpu.C, &gb->cpu.H); }
GB_OP(0x4D, "LD C, L")      { LD_R_R(&gb->cpu.C, &gb->cpu.L); }
GB_OP(0x4E, "LD C, (HL)")   { LD_R_R(&gb->cpu.C, &gb->cpu.memory[gb->cpu.HL]); }
GB_OP(0x4F, "LD C, A")      { LD_R_R(&gb->cpu.C, &gb->cpu.A); }

GB_OP(0x50, "LD D, B")      { LD_R_R(&gb->cpu.D, &gb->cpu.B); }
GB_OP(0x51, "LD D, C")      { LD_R_R(&gb->cpu.D, &gb->cpu.C); }
GB_OP(0x52, "LD D, D")      { LD_R_R(&gb->cpu.D, &gb->cpu.D); }
GB_OP(0x53, "LD D, E")      { LD_R_R(&gb->cpu.D, &gb->cpu.E); }
GB_OP(0x54, "LD D, H")      { LD_R_R(&gb->cpu.D, &gb->cpu.H); }
GB_OP(0x55, "LD D, L")      { LD_R_R(&gb->cpu.D, &gb->cpu.L); }
GB_OP(0x56, "LD D, (HL)")   { LD_R_R(&gb->cpu.D, &gb->cpu.memory[gb->cpu.HL]); }
GB_OP(0x57, "LD D, A")      { LD_R_R(&gb->cpu.D, &gb->cpu.A); }
GB_OP(0x58, "LD E, B")      { LD_R_R(&gb->cpu.E, &gb->cpu.B); }
GB_OP(0x59, "LD E, C")      { LD_R_R(&gb->cpu.E, &gb->cpu.C); }
GB_OP(0x5A, "LD E, D")      { LD_R_R(&gb->cpu.E, &gb->cpu.D); }
GB_OP(0x5B, "LD E, E")      { LD_R_R(&gb->cpu.E, &gb->cpu.E); }
GB_OP(0x5C, "LD E, H")      { LD_R_R(&gb->cpu.E, &gb->cpu.H); }
GB_OP(0x5D, "LD E, L")      { LD_R_R(&gb->cpu.E, &gb->cpu.L); }
GB_OP(0x5E, "LD E, (HL)")   { LD_R_R(&gb->cpu.E, &gb->cpu.memory[gb->cpu.HL]); }
GB_OP(0x5F, "LD E, A")      { LD_R_R(&gb->cpu.E, &gb->cpu.A); }

GB_OP(0x60, "LD H, B")      { LD_R_R(&gb->cpu.H, &gb->cpu.B); }
GB_OP(0x61, "LD H, C")      { LD_R_R(&gb->cpu.H, &gb->cpu.C); }
GB_OP(0x62, "LD H, D")      { LD_R_R(&gb->cpu.H, &gb->cpu.D); }
GB_OP(0x63, "LD H, E")      { LD_R_R(&gb->cpu.H, &gb->cpu.E); }
GB_OP(0x64, "LD H, H")      { LD_R_R(&gb->cpu.H, &gb->cpu.H); }
GB_OP(0x65, "LD H, L")      { LD_R_R(&gb->cpu.H, &gb->cpu.L); }
GB_OP(0x66, "LD H, (HL)")   { LD_R_R(&gb->cpu.H, &gb->cpu.memory[gb->cpu.HL]); }
GB_OP(0x67, "LD H, A")      { LD_R_R(&gb->cpu.H, &gb->cpu.A); }
GB_OP(0x68, "LD L, B")      { LD_R_R(&gb->cpu.L, &gb->cpu.B); }
GB_OP(0x69, "LD L, C")      { LD_R_R(&gb->cpu.L, &gb->cpu.C); }
GB_OP(0x6A, "LD L, D")      { LD_R_R(&gb->cpu.L, &gb->cpu.D); }
GB_OP(0x6B, "LD L, E")      { LD_R_R(&gb->cpu.L, &gb->cpu.E); }
GB_OP(0x6C, "LD L, H")      { LD_R_R(&gb->cpu.L, &gb->cpu.H); }
GB_OP(0x6D, "LD L, L")      { LD_R_R(&gb->cpu.L, &gb->cpu.L); }
GB_OP(0x6E, "LD L, (HL)")   { LD_R_R(&gb->cpu.L, &gb->cpu.memory[gb->cpu.HL]); }
GB_OP(0x6F, "LD L, A")      { LD_R_R(&gb->cpu.L, &gb->cpu.A); }

GB_OP(0x70, "LD (HL), B")   { LD_R_R(&gb->cpu.memory[gb->cpu.HL], &gb->cpu.B); }
GB_OP(0x71, "LD (HL), C")   { LD_R_R(&gb->cpu.memory[gb->cpu.HL], &gb->cpu.C); }
GB_OP(0x72, "LD (HL), D")   { LD_R_R(&gb->cpu.memory[gb->cpu.HL], &gb->cpu.D); }
GB_OP(0x73, "LD (HL), E")   { LD_R_R(&gb->cpu.memory[gb->cpu.HL], &gb->cpu.E); }
GB_OP(0x74, "LD (HL), H")   { LD_R_R(&gb->cpu.memory[gb->cpu.HL], &gb->cpu.H); }
GB_OP(0x75, "LD (HL), L")   { LD_R_R(&gb->cpu.memory[gb->cpu.HL], &gb->cpu.L); }
GB_OP(0x76, "HALT")         { /* TODO */ }
GB_OP(0x77, "LD (HL), A")   { LD_R_R(&gb->cpu.memory[gb->cpu.HL], &gb->cpu.A); }
GB_OP(0x78, "LD A, B")      { LD_R_R(&gb->cpu.A, &gb->cpu.B); }
GB_OP(0x79, "LD A, C")      { LD_R_R(&gb->cpu.A, &gb->cpu.C); }
GB_OP(0x7A, "LD A, D")      { LD_R_R(&gb->cpu.A, &gb->cpu.D); }
GB_OP(0x7B, "LD A, E")      { LD_R_R(&gb->cpu.A, &gb->cpu.E); }
GB_OP(0x7C, "LD A, H")      { LD_R_R(&gb->cpu.A, &gb->cpu.H); }
GB_OP(0x7D, "LD A, L")      { LD_R_R(&gb->cpu.A, &gb->cpu.L); }
GB_OP(0x7E, "LD A, (HL)")   { LD_R_R(&gb->cpu.A, &gb->cpu.memory[gb->cpu.HL]); }
GB_OP(0x7F, "LD A, A")      { LD_R_R(&gb->cpu.A, &gb->cpu.A); }

GB_OP(0xe0, "LDH (n), A")   { LD_R_R(&gb->cpu.memory[0xFF00 | (gb->cpu.memory[(++gb->cpu.PC)])], &gb->cpu.A); }
GB_OP(0xf0, "LDH A, (n)")   { LD_R_R(&gb->cpu.A, &gb->cpu.memory[0xFF00 | gb->cpu.memory[(++gb->cpu.PC)]]); }

GB_OP(0xe2, "LDH (C), A")   { LD_R_R(&gb->cpu.memory[0xFF00 | gb->cpu.C], &gb->cpu.A); }
GB_OP(0xf2, "LDH A, (C)")   { LD_R_R(&gb->cpu.A, &gb->cpu.memory[0xFF00 | gb->cpu.C]); }

GB_OP(0xea, "LD (nn), A")   {
    uint8_t low = gb->cpu.memory[++gb->cpu.PC];
    uint8_t high = gb->cpu.memory[++gb->cpu.PC];
    uint16_t addr = (high << 8) | low;
    LD_R_R(&gb->cpu.memory[addr], &gb->cpu.A);
}
GB_OP(0xfa, "LD A, (nn)")   {
    uint8_t low = gb->cpu.memory[++gb->cpu.PC];
    uint8_t high = gb->cpu.memory[++gb->cpu.PC];
    uint16_t addr = (high << 8) | low;
    LD_R_R(&gb->cpu.A, &gb->cpu.memory[addr]);
}

/*
x0 x1 x2 x3 x4 x5 x6 x7 x8 x9 xa xb xc xd xe xf
0x NOP LD BC,nn LD (BC),A INC BC INC B DEC B LD B,n RLCA LD (nn),SP ADD HL,BC LD A,(BC) DEC BC INC C DEC C LD C,n RRCA
1x STOP LD DE,nn LD (DE),A INC DE INC D DEC D LD D,n RLA JR e ADD HL,DE LD A,(DE) DEC DE INC E DEC E LD E,n RRA
2x JR NZ,e LD HL,nn LD (HL+),A INC HL INC H DEC H LD H,n DAA JR Z,e ADD HL,HL LD A,(HL+) DEC HL INC L DEC L LD L,n CPL
3x JR NC,e LD SP,nn LD (HL-),A INC SP INC (HL) DEC (HL) LD (HL),n SCF JR C,e ADD HL,SP LD A,(HL-) DEC SP INC A DEC A LD A,n CCF

8x ADD B ADD C ADD D ADD E ADD H ADD L ADD (HL) ADD A ADC B ADC C ADC D ADC E ADC H ADC L ADC (HL) ADC A

9x SUB B SUB C SUB D SUB E SUB H SUB L SUB (HL) SUB A SBC B SBC C SBC D SBC E SBC H SBC L SBC (HL) SBC A

ax AND B AND C AND D AND E AND H AND L AND (HL) AND A XOR B XOR C XOR D XOR E XOR H XOR L XOR (HL) XOR A

bx OR B OR C OR D OR E OR H OR L OR (HL) OR A CP B CP C CP D CP E CP H CP L CP (HL) CP A

cx RET NZ POP BC JP NZ,nn JP nn CALL NZ,nn PUSH BC ADD n RST 0x00 RET Z RET JP Z,nn CB op CALL Z,nn CALL nn ADC n RST 0x08
dx RET NC POP DE JP NC,nn - CALL NC,nn PUSH DE SUB n RST 0x10 RET C RETI JP C,nn - CALL C,nn - SBC n RST 0x18
ex LDH (n),A POP HL LDH (C),A - - PUSH HL AND n RST 0x20 ADD SP,e JP HL LD (nn),A - - - XOR n RST 0x28
fx LDH A,(n) POP AF LDH A,(C) DI - PUSH AF OR n RST 0x30 LD HL,SP+e LD SP,HL LD A,(nn) EI - - CP n RST 0x38*/

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
        #ifdef _WIN32
        system("cls");
        #else
        system("clear");
        #endif

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
