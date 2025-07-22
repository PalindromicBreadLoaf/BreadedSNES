//
// Created by Palindromic Bread Loaf on 7/21/25.
//

#ifndef CPU_H
#define CPU_H
#include "bus.h"

// 65816 CPU implementation
class CPU {
private:
    // Registers
    uint16_t A;     // Accumulator
    uint16_t X, Y;  // Index registers
    uint16_t SP;    // Stack pointer
    uint32_t PC;    // Program counter (24-bit)
    uint8_t P;      // Processor status
    uint8_t DB;     // Data bank
    uint8_t PB;     // Program bank
    uint16_t D;     // Direct page

    Bus* bus;
    uint64_t cycles;

    // Status flags
    enum Flags {
        FLAG_C = 0x01,  // Carry
        FLAG_Z = 0x02,  // Zero
        FLAG_I = 0x04,  // IRQ disable
        FLAG_D = 0x08,  // Decimal mode
        FLAG_X = 0x10,  // Index register size (0=16-bit, 1=8-bit)
        FLAG_M = 0x20,  // Memory/Accumulator size (0=16-bit, 1=8-bit)
        FLAG_V = 0x40,  // Overflow
        FLAG_N = 0x80   // Negative
    };

public:
    CPU(Bus* memory_bus) : bus(memory_bus) {
        Reset();
    }

    void Reset();
    void Step();
    void ExecuteInstruction();
    uint64_t GetCycles() const { return cycles; }

    // Instruction implementations
    // TODO: Implement remaining instructions
    void NOP();
    void LDA();
    void STA();
    void JMP();
};

#endif //CPU_H
