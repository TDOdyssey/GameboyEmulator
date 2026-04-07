#include "gameboy.h"

int main()
{
    GameBoy *gb = malloc(sizeof(GameBoy));

    /* 
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
 
    gb.cpu.memory[0x100] = 0x3E; // LD A, 0x05
    gb.cpu.memory[0x101] = 0x05;

    gb.cpu.memory[0x102] = 0xD6; // SUB 0x03
    gb.cpu.memory[0x103] = 0x03;
        
    gb.cpu.memory[0x104] = 0xD6; // SUB 0x05 (should trigger borrow)
    gb.cpu.memory[0x105] = 0x05;
        
    gb.cpu.memory[0x106] = 0xD6; // SUB 0xFD (wrap test)
    gb.cpu.memory[0x107] = 0xFD;
        
    gb.cpu.memory[0x108] = 0x00; // NOP (end marker)*/

    gb->cpu.PC = 0x100;

    // load this into gb->cpu.memory starting at 0x100
    gb->cpu.memory[0x100] = 0x3E; // LD A, 0x15
    gb->cpu.memory[0x101] = 0x15;
    
    gb->cpu.memory[0x102] = 0x37; // SCF (set carry = 1)
    gb->cpu.memory[0x103] = 0x00; // NOP placeholder (just increment PC)
    
    gb->cpu.memory[0x104] = 0xDE; // SBC 0x07
    gb->cpu.memory[0x105] = 0x07;
    
    gb->cpu.memory[0x106] = 0xDE; // SBC 0x0F
    gb->cpu.memory[0x107] = 0x0F;
    
    gb->cpu.memory[0x108] = 0xDE; // SBC 0x15
    gb->cpu.memory[0x109] = 0x15;
    
    gb->cpu.memory[0x10A] = 0x00; // NOP / end
                                 //
                                 //
    while(1)
    {
        print_cpu_state(gb);

        char c = getchar();
        if(c == 'q')
            break;

        clock(gb);
    }
    
    //printf("memory[0x5678] = %x\n", gb.cpu.memory[0x5678]);

    return 0;
}
