#include "gameboy.h"


// LCD Control - 0xFF40
#define LCDC_ADDR 0xFF40
enum LCDC_flags {
    LCDC_LCD_AND_PU_ENABLE              = 1 << 7,
    LCDC_WINDOW_TILE_MAP                = 1 << 6,
    LCDC_WINDOW_ENABLE                  = 1 << 5,
    LCDC_BG_AND_WINDOW_TILES            = 1 << 4,
    LCDC_BG_TILE_MAP                    = 1 << 3,
    LCDC_OBJ_SIZE                       = 1 << 2,
    LCDC_OBJ_ENABLE                     = 1 << 1,
    LCDC_BG_AND_WINDOW_ENABLE_PRIORITY  = 1 << 0,
};


// LCD & PPU enable: 0 = Off; 1 = On
// Window tile map area: 0 = 9800-9BFF; 1 = 9C00-9FFF
// Window enable: 0 = Off; 1 = On
// BG & Window tile data area: 0 = 8800-97FF; 1 = 8000-8FFF
// BG tile map area: 0 = 9800-9BFF; 1 = 9C00-9FFF
// OBJ size: 0 = 8x8; 1 = 8x16
// OBJ enable: 0 = Off; 1 = On
// BG & Window enable / priority [Different meaning in CGB Mode]: 0 = Off; 1 = On


#define VRAM_TILE_DATA_BLOCK0 0x8000 
uint16_t vram_tile_data_addr_obj(uint8_t idx)
{
    return VRAM_TILE_DATA_BLOCK0 + (idx * 16);
}

#define VRAM_TILE_DATA_BLOCK2 0x9000 
uint16_t vram_tile_data_addr_bg_win(uint8_t idx, enum LCDC_flags flags)
{
    if((flags & (1 << 4)) != 0 || idx >= 128)
        return VRAM_TILE_DATA_BLOCK0 + (idx * 16);
    
    return VRAM_TILE_DATA_BLOCK2 + (idx * 16);
}
