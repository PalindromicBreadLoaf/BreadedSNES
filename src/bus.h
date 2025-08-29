//
// Created by Palindromic Bread Loaf on 7/21/25.
//

#ifndef BUS_H
#define BUS_H
#include <algorithm>
#include <cstdint>
#include <vector>

// Memory Bus - handles memory mapping
class Bus {
private:
    uint8_t wram[0x20000];      // 128KB Work RAM
    uint8_t sram[0x8000];       // 32KB Save RAM
    std::vector<uint8_t>* cartridge; // Cartridge Data

public:
    Bus(std::vector<uint8_t>* cart) : cartridge(cart) {
        std::fill(wram, wram + sizeof(wram), 0);
        std::fill(sram, sram + sizeof(sram), 0);
    }

    uint8_t Read(uint32_t address);
    void Write(uint32_t address, uint8_t value);
    uint16_t Read16(uint32_t address);
    void Write16(uint32_t address, uint16_t value);
};

#endif //BUS_H
