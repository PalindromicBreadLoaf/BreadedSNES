//
// Created by Palindromic Bread Loaf on 7/21/25.
//

#include "ppu.h"

#include <algorithm>

// PPU Implementation
void PPU::Reset() {
    scanline = 0;
    dot = 0;
    frame_complete = false;
    brightness = 0x0F;
    bg_mode = 0;

    std::fill(vram, vram + sizeof(vram), 0);
    std::fill(oam, oam + sizeof(oam), 0);
    std::fill(cgram, cgram + sizeof(cgram), 0);
}

void PPU::Step() {
    dot++;
    if (dot >= 341) {
        dot = 0;
        scanline++;

        if (scanline >= 262) {
            scanline = 0;
            frame_complete = true;
        }
    }

    // TODO: Implement PPU renderer
}

uint8_t PPU::ReadVRAM(uint16_t address) {
    return vram[address & 0xFFFF];
}

void PPU::WriteVRAM(uint16_t address, uint8_t value) {
    vram[address & 0xFFFF] = value;
}