//
// Created by Palindromic Bread Loaf on 7/21/25.
//

#ifndef CPU_H
#define CPU_H
#include "bus.h"

// 65816 CPU implementation
class CPU {
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
    bool emulation_mode = true;
    bool stopped = false;
    bool waiting_for_interrupt = false;

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

    // Addressing mode helpers
    uint8_t ReadByte(uint32_t address);
    uint16_t ReadWord(uint32_t address);
    void UpdateNZ8(uint8_t value);
    void UpdateNZ16(uint16_t value);

    // Memory write helpers
    void WriteByte(uint32_t address, uint8_t value) const;
    void WriteWord(uint32_t address, uint16_t value) const;

    // Helper methods for compare operations
    void UpdateCompareFlags8(uint8_t reg_value, uint8_t compare_value);
    void UpdateCompareFlags16(uint16_t reg_value, uint16_t compare_value);

    // Branching helper method
    void DoBranch(bool condition);

    // Helper methods for stack operations
    void PushByte(uint8_t value);
    void PushWord(uint16_t value);
    uint8_t PopByte();
    uint16_t PopWord();

    // Helper method for ADC instructions
    void DoADC(uint16_t value);

    // Helper method to check for decimal mode adjustment
    static uint16_t AdjustDecimal(uint16_t binary_result, bool is_16bit);

    // Helpers for LD* Instructions
    void LDA_Mem(uint32_t address, int base_cycles, bool addDPExtraCycle, bool addPageCrossCycle, uint16_t base,
                 uint16_t offset);

    void LD_Index(uint32_t address, bool isX, int base_cycles, bool addDPExtraCycle, bool addPageCrossCycle,
                  uint16_t base,
                  uint16_t offset);

    // General ORA Logic
    void ORA_Mem(uint32_t address, int base_cycles, bool addDPExtraCycle, bool addPageCrossCycle, uint16_t base_address,
             uint16_t offset);

    //Helper Methods for Rotate Right/Left
    uint8_t ROL8(uint8_t value);
    uint16_t ROL16(uint16_t value);
    void ROL_AtAddress(uint32_t address, int base_cycles_8bit, int base_cycles_16bit);

    uint8_t ROR8(uint8_t value);
    uint16_t ROR16(uint16_t value);
    void ROR_AtAddress(uint32_t address, int base_cycles_8bit, int base_cycles_16bit);

    // Helper methods for SBC operations
    void SBC8(uint8_t operand);
    void SBC16(uint16_t operand);
    uint8_t SBC8_Decimal(uint8_t a, uint8_t operand, bool carry);
    uint16_t SBC16_Decimal(uint16_t a, uint16_t operand, bool carry);
    void SBC_FromAddress(uint32_t address, int base_cycles_8bit, int base_cycles_16bit);
    void SBC_FromAddress_PageCross(uint32_t address, uint16_t base_address, uint16_t offset, int base_cycles_8bit,
                                   int base_cycles_16bit);

    // General STZ Logic
    void STZ_ToAddress(uint32_t address, int base_cycles_8bit, int base_cycles_16bit);

    // ST* Helpers
    void WriteWithDirectPagePenalty(uint32_t address, uint16_t value, bool isMemoryFlag, int baseCycles);
    void WriteRegisterToAddress(uint32_t address, uint16_t value, bool isMemoryFlag, int baseCycles);

    // Helper methods for ASL stuff
    void UpdateASLFlags8(uint8_t original_value, uint8_t result);
    void UpdateASLFlags16(uint16_t original_value, uint16_t result);

    // Helper methods for BIT
    void UpdateBITFlags8(uint8_t memory_value, uint8_t acc_value);
    void UpdateBITFlags16(uint16_t memory_value, uint16_t acc_value);
    void UpdateBITImmediateFlags8(uint8_t memory_value, uint8_t acc_value);
    void UpdateBITImmediateFlags16(uint16_t memory_value, uint16_t acc_value);

    void UpdateLSRFlags8(uint8_t original_value, uint8_t result);
    void UpdateLSRFlags16(uint16_t original_value, uint16_t result);

public:
    explicit CPU(Bus* memory_bus) : bus(memory_bus) {
        Reset();
    }

    void Reset();
    void Step();
    void ExecuteInstruction();
    [[nodiscard]] uint64_t GetCycles() const { return cycles; }

    // Instruction implementations
    // TODO: Implement remaining instructions
    void CMP_Immediate();
    void CMP_Absolute();
    void CMP_AbsoluteX();
    void CMP_AbsoluteY();
    void CMP_DirectPage();
    void CMP_DirectPageX();
    void CMP_IndirectDirectPage();
    void CMP_IndirectDirectPageY();
    void CMP_DirectPageIndirectX();
    void CMP_Long();
    void CMP_LongX();

    void CPX_Immediate();
    void CPX_Absolute();
    void CPX_DirectPage();

    void CPY_Immediate();
    void CPY_Absolute();
    void CPY_DirectPage();

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
    void LDA_StackRelative();
    void LDA_IndirectDirectPageLong();
    void LDA_StackRelativeIndirectY();
    void LDA_IndirectDirectPageLongY();

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
    void STA_StackRelative();
    void STA_DirectPageIndirectLong();
    void STA_StackRelativeIndirectY();
    void STA_DirectPageIndirectLongY();

    void STX_Absolute();
    void STX_DirectPage();
    void STX_DirectPageY();

    void STY_Absolute();
    void STY_DirectPage();
    void STY_DirectPageX();

    void INC_Accumulator();
    void INC_Absolute();
    void INC_AbsoluteX();
    void INC_DirectPage();
    void INC_DirectPageX();

    void DEC_Accumulator();
    void DEC_Absolute();
    void DEC_AbsoluteX();
    void DEC_DirectPage();
    void DEC_DirectPageX();

    void INX();
    void INY();
    void DEX();
    void DEY();

    void JMP_Absolute();
    void JMP_AbsoluteIndirect();
    void JMP_AbsoluteLong();
    void JMP_AbsoluteIndirectX();
    void JMP_AbsoluteIndirectLong();

    void BEQ_Relative();
    void BNE_Relative();
    void BCC_Relative();
    void BCS_Relative();
    void BMI_Relative();
    void BPL_Relative();

    void JSR_Absolute();
    void JSR_AbsoluteLong();
    void JSR_AbsoluteIndirectX();
    void RTS();
    void RTL();

    void PHA();
    void PLA();
    void PHX();
    void PLX();
    void PHY();
    void PLY();
    void PHP();
    void PLP();
    void PHB();
    void PLB();
    void PHD();
    void PLD();
    void PHK();

    void ADC_Immediate();
    void ADC_Absolute();
    void ADC_AbsoluteX();
    void ADC_AbsoluteY();
    void ADC_DirectPage();
    void ADC_DirectPageX();
    void ADC_IndirectDirectPage();
    void ADC_IndirectDirectPageY();
    void ADC_DirectPageIndirectX();
    void ADC_AbsoluteLong();
    void ADC_AbsoluteLongX();
    void ADC_DirectPageIndirectLong();
    void ADC_DirectPageIndirectLongY();
    void ADC_StackRelative();
    void ADC_StackRelativeIndirectY();

    void AND_Immediate();
    void AND_Absolute();
    void AND_AbsoluteX();
    void AND_AbsoluteY();
    void AND_DirectPage();
    void AND_DirectPageX();
    void AND_IndirectDirectPage();
    void AND_IndirectDirectPageLong();
    void AND_IndexedIndirectDirectPageX();
    void AND_IndirectDirectPageY();
    void AND_IndirectDirectPageLongY();
    void AND_AbsoluteLong();
    void AND_AbsoluteLongX();
    void AND_StackRelative();
    void AND_StackRelativeIndirectY();

    void ASL_Accumulator();
    void ASL_Absolute();
    void ASL_AbsoluteX();
    void ASL_DirectPage();
    void ASL_DirectPageX();

    void BIT_Immediate();
    void BIT_Absolute();
    void BIT_AbsoluteX();
    void BIT_DirectPage();
    void BIT_DirectPageX();

    void BRA_Relative();
    void BRL_RelativeLong();
    void BVC_Relative();
    void BVS_Relative();
    void BRK();

    void CLC();
    void CLD();
    void CLI();
    void CLV();

    void CMP_StackRelative();
    void CMP_IndirectDirectPageLong();
    void CMP_StackRelativeIndirectY();
    void CMP_IndirectDirectPageLongY();

    void EOR_Immediate();
    void EOR_Absolute();
    void EOR_AbsoluteX();
    void EOR_AbsoluteY();
    void EOR_DirectPage();
    void EOR_DirectPageX();
    void EOR_IndirectDirectPage();
    void EOR_IndirectDirectPageLong();
    void EOR_IndexedIndirectDirectPageX();
    void EOR_IndirectDirectPageY();
    void EOR_IndirectDirectPageLongY();
    void EOR_AbsoluteLong();
    void EOR_AbsoluteLongX();
    void EOR_StackRelative();
    void EOR_StackRelativeIndirectY();

    void LSR_Accumulator();
    void LSR_Absolute();
    void LSR_AbsoluteX();
    void LSR_DirectPage();
    void LSR_DirectPageX();

    void ORA_Immediate();
    void ORA_Absolute();
    void ORA_AbsoluteX();
    void ORA_AbsoluteY();
    void ORA_DirectPage();
    void ORA_DirectPageX();
    void ORA_IndirectDirectPage();
    void ORA_IndirectDirectPageLong();
    void ORA_IndexedIndirectDirectPageX();
    void ORA_IndirectDirectPageY();
    void ORA_IndirectDirectPageLongY();
    void ORA_AbsoluteLong();
    void ORA_AbsoluteLongX();
    void ORA_StackRelative();
    void ORA_StackRelativeIndirectY();

    void MVN();
    void MVP();

    void ROL_Accumulator();
    void ROL_Absolute();
    void ROL_AbsoluteX();
    void ROL_DirectPage();
    void ROL_DirectPageX();

    void ROR_Accumulator();
    void ROR_Absolute();
    void ROR_AbsoluteX();
    void ROR_DirectPage();
    void ROR_DirectPageX();

    void PEA();
    void PEI();
    void PER();
    void REP();
    void RTI();

    void SBC_Immediate();
    void SBC_Absolute();
    void SBC_AbsoluteLong();
    void SBC_AbsoluteX();
    void SBC_AbsoluteLongX();
    void SBC_AbsoluteY();
    void SBC_DirectPage();
    void SBC_DirectPageX();
    void SBC_DirectPageIndirect();
    void SBC_DirectPageIndirectLong();
    void SBC_DirectPageIndirectY();
    void SBC_DirectPageIndirectLongY();
    void SBC_DirectPageIndirectX();
    void SBC_StackRelative();
    void SBC_StackRelativeIndirectY();

    void SEC();
    void SED();
    void SEI();
    void SEP();

    void STP();

    void STZ_Absolute();
    void STZ_AbsoluteX();
    void STZ_DirectPage();
    void STZ_DirectPageX();

    void TAX();
    void TAY();
    void TCD();
    void TCS();
    void TDC();
    void TSC();
    void TSX();
    void TXA();
    void TXS();
    void TXY();
    void TYA();
    void TYX();

    void TRB_DirectPage();
    void TRB_Absolute();
    void TSB_DirectPage();
    void TSB_Absolute();

    void WAI();
    void WDM(); // Reserved for future expansion - whatever that means
    void XBA();
    void XCE();
};

#endif //CPU_H
