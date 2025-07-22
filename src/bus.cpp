//
// Created by Palindromic Bread Loaf on 7/21/25.
//

#include "bus.h"

// Bus class
uint8_t Bus::Read(uint32_t address) {
    // TODO: Map properly based on SNES memory map
    if (address < 0x2000) {
        return wram[address];
    } else if (address >= 0x7E0000 && address < 0x800000) {
        return wram[address - 0x7E0000];
    } else if (address >= 0x800000 && cartridge) {
        // ROM access
        uint32_t rom_addr = address & 0x7FFFFF;
        if (rom_addr < cartridge->size()) {
            return (*cartridge)[rom_addr];
        }
    }
    return 0x00; // Open bus
}

void Bus::Write(uint32_t address, uint8_t value) {
    if (address < 0x2000) {
        wram[address] = value;
    } else if (address >= 0x7E0000 && address < 0x800000) {
        wram[address - 0x7E0000] = value;
    }
    // TODO: Add PPU/APU register writes here
}

uint16_t Bus::Read16(uint32_t address) {
    return Read(address) | (Read(address + 1) << 8);
}

void Bus::Write16(uint32_t address, uint16_t value) {
    Write(address, value & 0xFF);
    Write(address + 1, (value >> 8) & 0xFF);
}
