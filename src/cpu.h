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
        // FLAG_E = ??? // 6502 Emulation Mode
        // FLAG_B = 0x10 //Break
    };

public:
    CPU(Bus* memory_bus) : bus(memory_bus) {
        Reset();
    }

    void Reset();
    void Step();
    void ExecuteInstruction();
    [[nodiscard]] uint64_t GetCycles() const { return cycles; }

    // Addressing mode helpers
    uint32_t GetEffectiveAddress(uint8_t mode);
    uint8_t ReadByte(uint32_t address);
    uint16_t ReadWord(uint32_t address);
    void UpdateNZ8(uint8_t value);
    void UpdateNZ16(uint16_t value);

    // Memory write helpers
    void WriteByte(uint32_t address, uint8_t value) const;
    void WriteWord(uint32_t address, uint16_t value) const;


    // Instruction implementations
    // TODO: Implement remaining instructions
    void JMP();

    static void NOP();

    void LDA_Immediate();
    void LDA_Absolute();
    void LDA_AbsoluteX();
    void LDA_AbsoluteY();
    void LDA_DirectPage();
    void LDA_DirectPageX();
    void LDA_IndirectDirectPage();
    void LDA_IndirectDirectPageY();
    void LDA_DirectPageIndirectX();
    void LDA_Long();
    void LDA_LongX();

    void LDX_Immediate();
    void LDX_Absolute();
    void LDX_AbsoluteY();
    void LDX_DirectPage();
    void LDX_DirectPageY();

    void LDY_Immediate();
    void LDY_Absolute();
    void LDY_AbsoluteX();
    void LDY_DirectPage();
    void LDY_DirectPageX();

    // STA - Store Accumulator operations
    void STA_Absolute();
    void STA_AbsoluteX();
    void STA_AbsoluteY();
    void STA_DirectPage();
    void STA_DirectPageX();
    void STA_IndirectDirectPage();
    void STA_IndirectDirectPageY();
    void STA_DirectPageIndirectX();
    void STA_Long();
    void STA_LongX();

    // STX - Store X Register operations
    void STX_Absolute();
    void STX_DirectPage();
    void STX_DirectPageY();

    // STY - Store Y Register operations
    void STY_Absolute();
    void STY_DirectPage();
    void STY_DirectPageX();


    void STA();
};

#endif //CPU_H
