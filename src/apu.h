//
// Created by Palindromic Bread Loaf on 7/21/25.
//

#ifndef APU_H
#define APU_H
#include <cstdint>

// SPC700 APU
class APU {
private:
    std::uint8_t spc_ram[0x10000];   // 64KB SPC700 RAM

    // APU registers
    uint8_t A, X, Y, SP;
    uint16_t PC;
    uint8_t PSW;

public:
    APU() {
        Reset();
    }

    void Reset();
    void Step();

    uint8_t ReadSPC(uint16_t address);
    void WriteSPC(uint16_t address, uint8_t value);
};

#endif //APU_H
