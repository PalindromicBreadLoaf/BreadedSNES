//
// Created by Palindromic Bread Loaf on 7/21/25.
//

#include "apu.h"

#include <algorithm>

// APU Implementation
void APU::Reset() {
    A = X = Y = 0;
    SP = 0xFF;
    PC = 0x0000;
    PSW = 0x02;

    std::fill(spc_ram, spc_ram + sizeof(spc_ram), 0);
}

void APU::Step() {
    // TODO: Implement SPC700 instruction execution
}

uint8_t APU::ReadSPC(uint16_t address) {
    return spc_ram[address];
}

void APU::WriteSPC(uint16_t address, uint8_t value) {
    spc_ram[address] = value;
}