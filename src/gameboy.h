#ifndef GAMEBOY_H
#define GAMEBOY_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

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
    int cycles;
    int ime_delay;
    bool ime;
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
    ECHO,
    PROHIBITED,
    OAM,
    IO,
    HRAM,
    IE,
};

enum gb_ram_section gb_ram_section_from_addr(uint16_t addr);

void write8(GameBoy *gb, uint16_t addr, uint8_t w);
uint8_t read8(GameBoy *gb, uint16_t addr);

uint8_t *r8(GameBoy *gb, uint8_t val); // val is 3 bits
uint8_t read_imm8(GameBoy *gb);
uint16_t read_imm16(GameBoy *gb);
void set_flag(GameBoy *gb, enum GBCPU_flags flag, bool value);
int get_flag(GameBoy *gb, enum GBCPU_flags flag);

void execute_op(GameBoy *gb, uint8_t op_code);
void execute_cb_op(GameBoy *gb, uint8_t op);

void reset(GameBoy *gb);

bool handle_interrupts(GameBoy *gb);

void clock(GameBoy *gb);

void print_cpu_state(GameBoy *gb);

#endif
