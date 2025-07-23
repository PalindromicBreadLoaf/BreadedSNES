//
// Created by Palindromic Bread Loaf on 7/21/25.
//

#include "cpu.h"

#include <iostream>

// CPU Implementation
void CPU::Reset() {
    A = X = Y = 0;
    SP = 0x01FF;
    PC = 0x8000;  // Will be loaded from reset vector
    P = 0x34;     // Start in emulation mode
    DB = PB = 0;
    D = 0;
    cycles = 0;
}

void CPU::Step() {
    ExecuteInstruction();
}

// CPU Helper Methods
uint8_t CPU::ReadByte(const uint32_t address) {
    cycles++;
    return bus->Read(address);
}

uint16_t CPU::ReadWord(const uint32_t address) {
    const uint8_t low = ReadByte(address);
    const uint8_t high = ReadByte(address + 1);
    return (high << 8) | low;
}

void CPU::UpdateNZ8(const uint8_t value) {
    P = (P & ~FLAG_N) | (value & 0x80);  // Set N flag if bit 7 is set
    P = (P & ~FLAG_Z) | (value == 0 ? FLAG_Z : 0);  // Set Z flag if value is zero
}

void CPU::UpdateNZ16(const uint16_t value) {
    P = (P & ~FLAG_N) | ((value & 0x8000) ? FLAG_N : 0);  // Set N flag if bit 15 is set
    P = (P & ~FLAG_Z) | (value == 0 ? FLAG_Z : 0);  // Set Z flag if value is zero
}

// Helper method to write bytes/words to memory
void CPU::WriteByte(const uint32_t address, const uint8_t value) const {
    bus->Write(address, value);
}

void CPU::WriteWord(const uint32_t address, const uint16_t value) const {
    bus->Write(address, value & 0xFF);         // Low byte
    bus->Write(address + 1, (value >> 8) & 0xFF); // High byte
}

// Helper function to update flags after compare operation
void CPU::UpdateCompareFlags8(const uint8_t reg_value, const uint8_t compare_value) {
    const uint16_t result = reg_value - compare_value;

    // Set/clear flags
    if (result & 0x100) {
        P &= ~FLAG_C;
    } else {
        P |= FLAG_C;
    }

    if ((result & 0xFF) == 0) {
        P |= FLAG_Z;
    } else {
        P &= ~FLAG_Z;
    }

    if (result & 0x80) {
        P |= FLAG_N;
    } else {
        P &= ~FLAG_N;
    }
}

void CPU::UpdateCompareFlags16(const uint16_t reg_value, const uint16_t compare_value) {
    const uint32_t result = reg_value - compare_value;

    // Set/clear flags
    if (result & 0x10000) {
        P &= ~FLAG_C;
    } else {
        P |= FLAG_C;
    }

    if ((result & 0xFFFF) == 0) {
        P |= FLAG_Z;
    } else {
        P &= ~FLAG_Z;
    }

    if (result & 0x8000) {
        P |= FLAG_N;
    } else {
        P &= ~FLAG_N;
    }
}

// General Branching Code
void CPU::DoBranch(const bool condition) {
    const auto displacement = static_cast<int8_t>(ReadByte(PC));
    PC++;

    if (condition) {
        const uint32_t old_pc = PC; // Save the current PC for page boundary check

        PC = static_cast<uint32_t>(static_cast<int32_t>(PC) + displacement);

        cycles++;

        if ((old_pc & 0xFF00) != (PC & 0xFF00)) cycles++; // Add one cycle for crossing a page boundary
    }
}

// Stack helper methods:
void CPU::PushByte(const uint8_t value) {
    WriteByte(SP, value);
    SP--;
    cycles++;
}

void CPU::PushWord(const uint16_t value) {
    // Push high then low
    PushByte(value >> 8);    // High byte
    PushByte(value & 0xFF);  // Low byte
}

uint8_t CPU::PopByte() {
    SP++;
    cycles++;
    return ReadByte(SP);
}

uint16_t CPU::PopWord() {
    // Pop low then high
    const uint8_t low = PopByte();
    const uint8_t high = PopByte();
    return (high << 8) | low;
}

void CPU::DoADC(uint16_t value) {
    uint32_t result;

    if (P & FLAG_M) {
        // 8-bit mode
        const uint8_t acc_low = A & 0xFF;
        const uint8_t val_low = value & 0xFF;

        if (P & FLAG_D) {
            // Decimal mode
            result = acc_low + val_low + (P & FLAG_C ? 1 : 0);
            result = AdjustDecimal(result, false);
        } else {
            // Binary mode
            result = acc_low + val_low + (P & FLAG_C ? 1 : 0);
        }

        P = (P & ~FLAG_C) | (result > 0xFF ? FLAG_C : 0);
        P = (P & ~FLAG_V) | (((acc_low ^ result) & (val_low ^ result) & 0x80) ? FLAG_V : 0);

        A = (A & 0xFF00) | (result & 0xFF);
        UpdateNZ8(A & 0xFF);

    } else {
        // 16-bit mode
        if (P & FLAG_D) {
            // Decimal mode
            result = A + value + (P & FLAG_C ? 1 : 0);
            result = AdjustDecimal(result, true);
        } else {
            // Binary mode
            result = A + value + (P & FLAG_C ? 1 : 0);
        }

        P = (P & ~FLAG_C) | (result > 0xFFFF ? FLAG_C : 0);
        P = (P & ~FLAG_V) | (((A ^ result) & (value ^ result) & 0x8000) ? FLAG_V : 0);

        A = result & 0xFFFF;
        UpdateNZ16(A);
    }
}

uint16_t CPU::AdjustDecimal(const uint16_t binary_result, const bool is_16bit) {
    //TODO: Make sure this works? I'm not sure I understand.
    if (is_16bit) {
        // 16-bit decimal adjustment
        uint16_t result = binary_result;
        if ((result & 0x0F) > 0x09) result += 0x06;
        if ((result & 0xF0) > 0x90) result += 0x60;
        if ((result & 0x0F00) > 0x0900) result += 0x0600;
        if ((result & 0xF000) > 0x9000) result += 0x6000;
        return result;
    }
    // 8-bit decimal adjustment
    uint16_t result = binary_result;
    if ((result & 0x0F) > 0x09) result += 0x06;
    if ((result & 0xF0) > 0x90) result += 0x60;
    return result;
}

void CPU::ExecuteInstruction() {
    // TODO: Actual Opcode decoding
    switch (const uint8_t opcode = bus->Read(PC++)) {
        // Bitwise AND Instructions
        case 0x29: AND_Immediate(); break;                    // AND #$nn or #$nnnn
        case 0x2D: AND_Absolute(); break;                     // AND $nnnn
        case 0x3D: AND_AbsoluteX(); break;                    // AND $nnnn,X
        case 0x39: AND_AbsoluteY(); break;                    // AND $nnnn,Y
        case 0x25: AND_DirectPage(); break;                   // AND $nn
        case 0x35: AND_DirectPageX(); break;                  // AND $nn,X
        case 0x32: AND_IndirectDirectPage(); break;           // AND ($nn)
        case 0x27: AND_IndirectDirectPageLong(); break;       // AND [$nn]
        case 0x21: AND_IndexedIndirectDirectPageX(); break;   // AND ($nn,X)
        case 0x31: AND_IndirectDirectPageY(); break;          // AND ($nn),Y
        case 0x37: AND_IndirectDirectPageLongY(); break;      // AND [$nn],Y
        case 0x2F: AND_AbsoluteLong(); break;                 // AND $nnnnnn
        case 0x3F: AND_AbsoluteLongX(); break;                // AND $nnnnnn,X
        case 0x23: AND_StackRelative(); break;                // AND $nn,S
        case 0x33: AND_StackRelativeIndirectY(); break;       // AND ($nn,S),Y

        // ADC - Add with Carry
        case 0x69: ADC_Immediate(); break;              // ADC #$nn/#$nnnn
        case 0x6D: ADC_Absolute(); break;               // ADC $nnnn
        case 0x7D: ADC_AbsoluteX(); break;              // ADC $nnnn,X
        case 0x79: ADC_AbsoluteY(); break;              // ADC $nnnn,Y
        case 0x6F: ADC_AbsoluteLong(); break;           // ADC $nnnnnn
        case 0x7F: ADC_AbsoluteLongX(); break;          // ADC $nnnnnn,X
        case 0x65: ADC_DirectPage(); break;             // ADC $nn
        case 0x75: ADC_DirectPageX(); break;            // ADC $nn,X
        case 0x72: ADC_IndirectDirectPage(); break;     // ADC ($nn)
        case 0x71: ADC_IndirectDirectPageY(); break;    // ADC ($nn),Y
        case 0x61: ADC_DirectPageIndirectX(); break;    // ADC ($nn,X)
        case 0x67: ADC_DirectPageIndirectLong(); break; // ADC [$nn]
        case 0x77: ADC_DirectPageIndirectLongY(); break;// ADC [$nn],Y
        case 0x63: ADC_StackRelative(); break;          // ADC $nn,S
        case 0x73: ADC_StackRelativeIndirectY(); break; // ADC ($nn,S),Y

        // Branch Instructions
        case 0xF0: BEQ_Relative(); break;      // BEQ $nn
        case 0xD0: BNE_Relative(); break;      // BNE $nn
        case 0x90: BCC_Relative(); break;      // BCC $nn
        case 0xB0: BCS_Relative(); break;      // BCS $nn
        case 0x30: BMI_Relative(); break;      // BMI $nn
        case 0x10: BPL_Relative(); break;      // BPL $nn

        // CMP - Compare Accumulator
        case 0xC9: CMP_Immediate(); break;             // CMP #$nn or #$nnnn
        case 0xCD: CMP_Absolute(); break;              // CMP $nnnn
        case 0xDD: CMP_AbsoluteX(); break;             // CMP $nnnn,X
        case 0xD9: CMP_AbsoluteY(); break;             // CMP $nnnn,Y
        case 0xC5: CMP_DirectPage(); break;            // CMP $nn
        case 0xD5: CMP_DirectPageX(); break;           // CMP $nn,X
        case 0xD2: CMP_IndirectDirectPage(); break;    // CMP ($nn)
        case 0xD1: CMP_IndirectDirectPageY(); break;   // CMP ($nn),Y
        case 0xC1: CMP_DirectPageIndirectX(); break;   // CMP ($nn,X)
        case 0xCF: CMP_Long(); break;                  // CMP $nnnnnn
        case 0xDF: CMP_LongX(); break;                 // CMP $nnnnnn,X

        // CPX - Compare X Register
        case 0xE0: CPX_Immediate(); break;         // CPX #$nn or #$nnnn
        case 0xEC: CPX_Absolute(); break;          // CPX $nnnn
        case 0xE4: CPX_DirectPage(); break;        // CPX $nn

        // CPY - Compare Y Register
        case 0xC0: CPY_Immediate(); break;         // CPY #$nn or #$nnnn
        case 0xCC: CPY_Absolute(); break;          // CPY $nnnn
        case 0xC4: CPY_DirectPage(); break;        // CPY $nn

        // DEC - Decrement Memory
        case 0x3A: DEC_Accumulator(); break;       // DEC A
        case 0xCE: DEC_Absolute(); break;          // DEC $nnnn
        case 0xDE: DEC_AbsoluteX(); break;         // DEC $nnnn,X
        case 0xC6: DEC_DirectPage(); break;        // DEC $nn
        case 0xD6: DEC_DirectPageX(); break;       // DEC $nn,X

        // JMP - Jump to an Address
        case 0x4C: JMP_Absolute(); break;           // JMP $nnnn
        case 0x6C: JMP_AbsoluteIndirect(); break;   // JMP ($nnnn)
        case 0x5C: JMP_AbsoluteLong(); break;       // JMP $nnnnnn
        case 0x7C: JMP_AbsoluteIndirectX(); break;  // JMP ($nnnn,X)

        // Subroutine instructions
        case 0x20: JSR_Absolute(); break;           // JSR $nnnn
        case 0x22: JSR_AbsoluteLong(); break;       // JSR $nnnnnn
        case 0xFC: JSR_AbsoluteIndirectX(); break;  // JSR ($nnnn,X)

        // Single Register Decrement
        case 0xCA: DEX(); break;                   // DEX - Decrement X Register
        case 0x88: DEY(); break;                   // DEY - Decrement Y Register

        // INC - Increment Memory
        case 0x1A: INC_Accumulator(); break;       // INC A
        case 0xEE: INC_Absolute(); break;          // INC $nnnn
        case 0xFE: INC_AbsoluteX(); break;         // INC $nnnn,X
        case 0xE6: INC_DirectPage(); break;        // INC $nn
        case 0xF6: INC_DirectPageX(); break;       // INC $nn,X

        // Single Register Increment
        case 0xE8: INX(); break;                   // INX - Increment X Register
        case 0xC8: INY(); break;                   // INY - Increment Y Register

        // No Operation
        case 0xEA: NOP(); break;                    //NOP

        // LDA - Load Accumulator
        case 0xA9: LDA_Immediate(); break;              // LDA #$nn or #$nnnn
        case 0xAD: LDA_Absolute(); break;               // LDA $nnnn
        case 0xBD: LDA_AbsoluteX(); break;              // LDA $nnnn,X
        case 0xB9: LDA_AbsoluteY(); break;              // LDA $nnnn,Y
        case 0xA5: LDA_DirectPage(); break;             // LDA $nn
        case 0xB5: LDA_DirectPageX(); break;            // LDA $nn,X
        case 0xB2: LDA_IndirectDirectPage(); break;     // LDA ($nn)
        case 0xB1: LDA_IndirectDirectPageY(); break;    // LDA ($nn),Y
        case 0xA1: LDA_DirectPageIndirectX(); break;    // LDA ($nn,X)
        case 0xAF: LDA_Long(); break;                   // LDA $nnnnnn
        case 0xBF: LDA_LongX(); break;                  // LDA $nnnnnn,X

        // LDX - Load X Register
        case 0xA2: LDX_Immediate(); break;          // LDX #$nn or LDX #$nnnn
        case 0xAE: LDX_Absolute(); break;           // LDX $nnnn
        case 0xBE: LDX_AbsoluteY(); break;          // LDX $nnnn,Y
        case 0xA6: LDX_DirectPage(); break;         // LDX $nn
        case 0xB6: LDX_DirectPageY(); break;        // LDX $nn,Y

        // LDY - Load Y Register
        case 0xA0: LDY_Immediate(); break;          // LDY #$nn or LDY #$nnnn
        case 0xAC: LDY_Absolute(); break;           // LDY $nnnn
        case 0xBC: LDY_AbsoluteX(); break;          // LDY $nnnn,X
        case 0xA4: LDY_DirectPage(); break;         // LDY $nn
        case 0xB4: LDY_DirectPageX(); break;        // LDY $nn,X

        // Stack operations
        case 0x48: PHA(); break;  // PHA
        case 0x68: PLA(); break;  // PLA
        case 0xDA: PHX(); break;  // PHX
        case 0xFA: PLX(); break;  // PLX
        case 0x5A: PHY(); break;  // PHY
        case 0x7A: PLY(); break;  // PLY
        case 0x08: PHP(); break;  // PHP
        case 0x28: PLP(); break;  // PLP
        case 0x8B: PHB(); break;  // PHB
        case 0xAB: PLB(); break;  // PLB
        case 0x0B: PHD(); break;  // PHD
        case 0x2B: PLD(); break;  // PLD
        case 0x4B: PHK(); break;  // PHK

        // More Subroutines
        case 0x60: RTS(); break;                    // RTS
        case 0x6B: RTL(); break;                    // RTL

        //SDA - Store Accumulator
        case 0x8D: STA_Absolute(); break;               // STA $nnnn
        case 0x9D: STA_AbsoluteX(); break;              // STA $nnnn,X
        case 0x99: STA_AbsoluteY(); break;              // STA $nnnn,Y
        case 0x85: STA_DirectPage(); break;             // STA $nn
        case 0x95: STA_DirectPageX(); break;            // STA $nn,X
        case 0x92: STA_IndirectDirectPage(); break;     // STA ($nn)
        case 0x91: STA_IndirectDirectPageY(); break;    // STA ($nn),Y
        case 0x81: STA_DirectPageIndirectX(); break;    // STA ($nn,X)
        case 0x8F: STA_Long(); break;                   // STA $nnnnnn
        case 0x9F: STA_LongX(); break;                  // STA $nnnnnn,X

        //SDX - Store X Register
        case 0x8E: STX_Absolute(); break;           // STX $nnnn
        case 0x86: STX_DirectPage(); break;         // STX $nn
        case 0x96: STX_DirectPageY(); break;        // STX $nn,Y

        // SDY - Store Y Register
        case 0x8C: STY_Absolute();  break;          // STY $nnnn
        case 0x84: STY_DirectPage();  break;        // STY $nn
        case 0x94: STY_DirectPageX(); break;        // STY $nn,X

        default:
            std::cout << "Unknown opcode: 0x" << std::hex << static_cast<int>(opcode) << std::endl;
            break;
    }
}

void CPU::NOP() {
    // No operation
}

// Load Accumulator Instructions
void CPU::LDA_Immediate() {
    if (P & FLAG_M) {
        // 8-bit accumulator mode
        const uint8_t value = ReadByte(PC++);
        A = (A & 0xFF00) | value;  // Keep high byte, update low byte
        UpdateNZ8(value);
        cycles += 2;
    } else {
        // 16-bit accumulator mode
        A = ReadWord(PC);
        PC += 2;
        UpdateNZ16(A);
        cycles += 3;
    }
}

void CPU::LDA_Absolute() {
    const uint16_t address = ReadWord(PC);
    PC += 2;

    if (P & FLAG_M) {
        // 8-bit mode
        const uint8_t value = ReadByte(address);
        A = (A & 0xFF00) | value;
        UpdateNZ8(value);
        cycles += 4;
    } else {
        // 16-bit mode
        A = ReadWord(address);
        UpdateNZ16(A);
        cycles += 5;
    }
}

void CPU::LDA_AbsoluteX() {
    const uint16_t base_address = ReadWord(PC);
    PC += 2;
    const uint32_t address = base_address + X;

    // Page boundary crossing adds a cycle in some cases
    if ((base_address & 0xFF00) != (address & 0xFF00)) {
        cycles++;
    }

    if (P & FLAG_M) {
        // 8-bit mode
        const uint8_t value = ReadByte(address);
        A = (A & 0xFF00) | value;
        UpdateNZ8(value);
        cycles += 4;
    } else {
        // 16-bit mode
        A = ReadWord(address);
        UpdateNZ16(A);
        cycles += 5;
    }
}

void CPU::LDA_AbsoluteY() {
    uint16_t base_address = ReadWord(PC);
    PC += 2;
    uint32_t address = base_address + Y;

    // Page boundary crossing adds a cycle
    if ((base_address & 0xFF00) != (address & 0xFF00)) {
        cycles++;
    }

    if (P & FLAG_M) {
        // 8-bit mode
        uint8_t value = ReadByte(address);
        A = (A & 0xFF00) | value;
        UpdateNZ8(value);
        cycles += 4;
    } else {
        // 16-bit mode
        A = ReadWord(address);
        UpdateNZ16(A);
        cycles += 5;
    }
}

void CPU::LDA_DirectPage() {
    const uint8_t offset = ReadByte(PC++);
    const uint32_t address = D + offset;

    if (P & FLAG_M) {
        // 8-bit mode
        uint8_t value = ReadByte(address);
        A = (A & 0xFF00) | value;
        UpdateNZ8(value);
        cycles += 3;
    } else {
        // 16-bit mode
        A = ReadWord(address);
        UpdateNZ16(A);
        cycles += 4;
    }

    // Extra cycle if D register is not page-aligned
    if (D & 0xFF) cycles++;
}

void CPU::LDA_DirectPageX() {
    const uint8_t offset = ReadByte(PC++);
    const uint32_t address = D + offset + (P & FLAG_X ? (X & 0xFF) : X);

    if (P & FLAG_M) {
        // 8-bit mode
        const uint8_t value = ReadByte(address);
        A = (A & 0xFF00) | value;
        UpdateNZ8(value);
        cycles += 4;
    } else {
        // 16-bit mode
        A = ReadWord(address);
        UpdateNZ16(A);
        cycles += 5;
    }

    // Extra cycle if D register is not page-aligned
    if (D & 0xFF) cycles++;
}

void CPU::LDA_IndirectDirectPage() {
    const uint8_t offset = ReadByte(PC++);
    const uint32_t pointer_address = D + offset;
    const uint16_t address = ReadWord(pointer_address);

    if (P & FLAG_M) {
        // 8-bit mode
        const uint8_t value = ReadByte(address);
        A = (A & 0xFF00) | value;
        UpdateNZ8(value);
        cycles += 5;
    } else {
        // 16-bit mode
        A = ReadWord(address);
        UpdateNZ16(A);
        cycles += 6;
    }

    // Extra cycle if D register is not page-aligned
    if (D & 0xFF) cycles++;
}

void CPU::LDA_IndirectDirectPageY() {
    const uint8_t offset = ReadByte(PC++);
    const uint32_t pointer_address = D + offset;
    const uint16_t base_address = ReadWord(pointer_address);
    const uint32_t address = base_address + Y;

    // Page boundary crossing adds a cycle
    if ((base_address & 0xFF00) != (address & 0xFF00)) {
        cycles++;
    }

    if (P & FLAG_M) {
        // 8-bit mode
        const uint8_t value = ReadByte(address);
        A = (A & 0xFF00) | value;
        UpdateNZ8(value);
        cycles += 5;
    } else {
        // 16-bit mode
        A = ReadWord(address);
        UpdateNZ16(A);
        cycles += 6;
    }

    // Extra cycle if D register is not page-aligned
    if (D & 0xFF) cycles++;
}

void CPU::LDA_DirectPageIndirectX() {
    const uint8_t offset = ReadByte(PC++);
    const uint32_t pointer_address = D + offset + (P & FLAG_X ? (X & 0xFF) : X);
    const uint16_t address = ReadWord(pointer_address);

    if (P & FLAG_M) {
        // 8-bit mode
        uint8_t value = ReadByte(address);
        A = (A & 0xFF00) | value;
        UpdateNZ8(value);
        cycles += 6;
    } else {
        // 16-bit mode
        A = ReadWord(address);
        UpdateNZ16(A);
        cycles += 7;
    }

    // Extra cycle if D register is not page-aligned
    if (D & 0xFF) cycles++;
}

void CPU::LDA_Long() {
    const uint32_t address = ReadByte(PC++) | (ReadByte(PC++) << 8) | (ReadByte(PC++) << 16);

    if (P & FLAG_M) {
        // 8-bit mode
        const uint8_t value = ReadByte(address);
        A = (A & 0xFF00) | value;
        UpdateNZ8(value);
        cycles += 5;
    } else {
        // 16-bit mode
        A = ReadWord(address);
        UpdateNZ16(A);
        cycles += 6;
    }
}

void CPU::LDA_LongX() {
    const uint32_t base_address = ReadByte(PC++) | (ReadByte(PC++) << 8) | (ReadByte(PC++) << 16);
    const uint32_t address = base_address + X;

    if (P & FLAG_M) {
        // 8-bit mode
        const uint8_t value = ReadByte(address);
        A = (A & 0xFF00) | value;
        UpdateNZ8(value);
        cycles += 5;
    } else {
        // 16-bit mode
        A = ReadWord(address);
        UpdateNZ16(A);
        cycles += 6;
    }
}

// Load X Register Instructions
void CPU::LDX_Immediate() {
    if (P & FLAG_X) {
        // 8-bit index mode
        X = ReadByte(PC++);
        UpdateNZ8(X & 0xFF);
        cycles += 2;
    } else {
        // 16-bit index mode
        X = ReadWord(PC);
        PC += 2;
        UpdateNZ16(X);
        cycles += 3;
    }
}

void CPU::LDX_Absolute() {
    const uint16_t address = ReadWord(PC);
    PC += 2;

    if (P & FLAG_X) {
        // 8-bit mode
        X = ReadByte(address);
        UpdateNZ8(X & 0xFF);
        cycles += 4;
    } else {
        // 16-bit mode
        X = ReadWord(address);
        UpdateNZ16(X);
        cycles += 5;
    }
}

void CPU::LDX_AbsoluteY() {
    const uint16_t base_address = ReadWord(PC);
    PC += 2;
    const uint32_t address = base_address + Y;

    // Page boundary crossing adds a cycle
    if ((base_address & 0xFF00) != (address & 0xFF00)) {
        cycles++;
    }

    if (P & FLAG_X) {
        // 8-bit mode
        X = ReadByte(address);
        UpdateNZ8(X & 0xFF);
        cycles += 4;
    } else {
        // 16-bit mode
        X = ReadWord(address);
        UpdateNZ16(X);
        cycles += 5;
    }
}

void CPU::LDX_DirectPage() {
    const uint8_t offset = ReadByte(PC++);
    const uint32_t address = D + offset;

    if (P & FLAG_X) {
        // 8-bit mode
        X = ReadByte(address);
        UpdateNZ8(X & 0xFF);
        cycles += 3;
    } else {
        // 16-bit mode
        X = ReadWord(address);
        UpdateNZ16(X);
        cycles += 4;
    }

    // Extra cycle if D register is not page-aligned
    if (D & 0xFF) cycles++;
}

void CPU::LDX_DirectPageY() {
    const uint8_t offset = ReadByte(PC++);
    const uint32_t address = D + offset + (P & FLAG_X ? (Y & 0xFF) : Y);

    if (P & FLAG_X) {
        // 8-bit mode
        X = ReadByte(address);
        UpdateNZ8(X & 0xFF);
        cycles += 4;
    } else {
        // 16-bit mode
        X = ReadWord(address);
        UpdateNZ16(X);
        cycles += 5;
    }

    // Extra cycle if D register is not page-aligned
    if (D & 0xFF) cycles++;
}

// Load Y Register Instructions
void CPU::LDY_Immediate() {
    if (P & FLAG_X) {
        // 8-bit index mode
        Y = ReadByte(PC++);
        UpdateNZ8(Y & 0xFF);
        cycles += 2;
    } else {
        // 16-bit index mode
        Y = ReadWord(PC);
        PC += 2;
        UpdateNZ16(Y);
        cycles += 3;
    }
}

void CPU::LDY_Absolute() {
    const uint16_t address = ReadWord(PC);
    PC += 2;

    if (P & FLAG_X) {
        // 8-bit mode
        Y = ReadByte(address);
        UpdateNZ8(Y & 0xFF);
        cycles += 4;
    } else {
        // 16-bit mode
        Y = ReadWord(address);
        UpdateNZ16(Y);
        cycles += 5;
    }
}

void CPU::LDY_AbsoluteX() {
    const uint16_t base_address = ReadWord(PC);
    PC += 2;
    const uint32_t address = base_address + X;

    // Page boundary crossing adds a cycle
    if ((base_address & 0xFF00) != (address & 0xFF00)) {
        cycles++;
    }

    if (P & FLAG_X) {
        // 8-bit mode
        Y = ReadByte(address);
        UpdateNZ8(Y & 0xFF);
        cycles += 4;
    } else {
        // 16-bit mode
        Y = ReadWord(address);
        UpdateNZ16(Y);
        cycles += 5;
    }
}

void CPU::LDY_DirectPage() {
    const uint8_t offset = ReadByte(PC++);
    const uint32_t address = D + offset;

    if (P & FLAG_X) {
        // 8-bit mode
        Y = ReadByte(address);
        UpdateNZ8(Y & 0xFF);
        cycles += 3;
    } else {
        // 16-bit mode
        Y = ReadWord(address);
        UpdateNZ16(Y);
        cycles += 4;
    }

    // Extra cycle if D register is not page-aligned
    if (D & 0xFF) cycles++;
}

void CPU::LDY_DirectPageX() {
    const uint8_t offset = ReadByte(PC++);
    const uint32_t address = D + offset + (P & FLAG_X ? (X & 0xFF) : X);

    if (P & FLAG_X) {
        // 8-bit mode
        Y = ReadByte(address);
        UpdateNZ8(Y & 0xFF);
        cycles += 4;
    } else {
        // 16-bit mode
        Y = ReadWord(address);
        UpdateNZ16(Y);
        cycles += 5;
    }

    // Extra cycle if D register is not page-aligned
    if (D & 0xFF) cycles++;
}

//Store operations implementation
void CPU::STA_Absolute() {
    const uint32_t address = ReadWord(PC + 1) | (DB << 16);
    PC += 3;

    if (P & FLAG_M) { // 8-bit mode
        WriteByte(address, A & 0xFF);
        cycles += 4;
    } else { // 16-bit mode
        WriteWord(address, A);
        cycles += 5;
    }
}

void CPU::STA_AbsoluteX() {
    const uint32_t base = ReadWord(PC + 1) | (DB << 16);
    const uint32_t address = base + X;
    PC += 3;

    if (P & FLAG_M) { // 8-bit mode
        WriteByte(address, A & 0xFF);
        cycles += 5;
    } else { // 16-bit mode
        WriteWord(address, A);
        cycles += 6;
    }
}

void CPU::STA_AbsoluteY() {
    const uint32_t base = ReadWord(PC + 1) | (DB << 16);
    const uint32_t address = base + Y;
    PC += 3;

    if (P & FLAG_M) { // 8-bit mode
        WriteByte(address, A & 0xFF);
        cycles += 5;
    } else { // 16-bit mode
        WriteWord(address, A);
        cycles += 6;
    }
}

void CPU::STA_DirectPage() {
    const uint8_t offset = ReadByte(PC + 1);
    const uint32_t address = (D + offset) & 0xFFFF;
    PC += 2;

    if (P & FLAG_M) { // 8-bit mode
        WriteByte(address, A & 0xFF);
        cycles += 3;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    } else { // 16-bit mode
        WriteWord(address, A);
        cycles += 4;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    }
}

void CPU::STA_DirectPageX() {
    const uint8_t offset = ReadByte(PC + 1);
    const uint32_t address = (D + offset + X) & 0xFFFF;
    PC += 2;

    if (P & FLAG_M) { // 8-bit mode
        WriteByte(address, A & 0xFF);
        cycles += 4;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    } else { // 16-bit mode
        WriteWord(address, A);
        cycles += 5;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    }
}

void CPU::STA_IndirectDirectPage() {
    const uint8_t offset = ReadByte(PC + 1);
    const uint32_t pointer = (D + offset) & 0xFFFF;
    const uint32_t address = ReadWord(pointer) | (DB << 16);
    PC += 2;

    if (P & FLAG_M) { // 8-bit mode
        WriteByte(address, A & 0xFF);
        cycles += 5;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    } else { // 16-bit mode
        WriteWord(address, A);
        cycles += 6;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    }
}

void CPU::STA_IndirectDirectPageY() {
    const uint8_t offset = ReadByte(PC + 1);
    const uint32_t pointer = (D + offset) & 0xFFFF;
    const uint32_t base = ReadWord(pointer) | (DB << 16);
    const uint32_t address = base + Y;
    PC += 2;

    if (P & FLAG_M) { // 8-bit mode
        WriteByte(address, A & 0xFF);
        cycles += 6;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    } else { // 16-bit mode
        WriteWord(address, A);
        cycles += 7;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    }
}

void CPU::STA_DirectPageIndirectX() {
    const uint8_t offset = ReadByte(PC + 1);
    const uint32_t pointer = (D + offset + X) & 0xFFFF;
    const uint32_t address = ReadWord(pointer) | (DB << 16);
    PC += 2;

    if (P & FLAG_M) { // 8-bit mode
        WriteByte(address, A & 0xFF);
        cycles += 6;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    } else { // 16-bit mode
        WriteWord(address, A);
        cycles += 7;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    }
}

void CPU::STA_Long() {
    const uint32_t address = ReadByte(PC + 1) | (ReadByte(PC + 2) << 8) | (ReadByte(PC + 3) << 16);
    PC += 4;

    if (P & FLAG_M) { // 8-bit mode
        WriteByte(address, A & 0xFF);
        cycles += 5;
    } else { // 16-bit mode
        WriteWord(address, A);
        cycles += 6;
    }
}

void CPU::STA_LongX() {
    const uint32_t base = ReadByte(PC + 1) | (ReadByte(PC + 2) << 8) | (ReadByte(PC + 3) << 16);
    const uint32_t address = base + X;
    PC += 4;

    if (P & FLAG_M) { // 8-bit mode
        WriteByte(address, A & 0xFF);
        cycles += 6;
    } else { // 16-bit mode
        WriteWord(address, A);
        cycles += 7;
    }
}

// STX - Store X Register
void CPU::STX_Absolute() {
    const uint32_t address = ReadWord(PC + 1) | (DB << 16);
    PC += 3;

    if (P & FLAG_X) { // 8-bit mode
        WriteByte(address, X & 0xFF);
        cycles += 4;
    } else { // 16-bit mode
        WriteWord(address, X);
        cycles += 5;
    }
}

void CPU::STX_DirectPage() {
    const uint8_t offset = ReadByte(PC + 1);
    const uint32_t address = (D + offset) & 0xFFFF;
    PC += 2;

    if (P & FLAG_X) { // 8-bit mode
        WriteByte(address, X & 0xFF);
        cycles += 3;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    } else { // 16-bit mode
        WriteWord(address, X);
        cycles += 4;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    }
}

void CPU::STX_DirectPageY() {
    uint8_t offset = ReadByte(PC + 1);
    uint32_t address = (D + offset + Y) & 0xFFFF;
    PC += 2;

    if (P & FLAG_X) { // 8-bit mode
        WriteByte(address, X & 0xFF);
        cycles += 4;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    } else { // 16-bit mode
        WriteWord(address, X);
        cycles += 5;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    }
}

// STY - Store Y Register
void CPU::STY_Absolute() {
    uint32_t address = ReadWord(PC + 1) | (DB << 16);
    PC += 3;

    if (P & FLAG_X) { // 8-bit mode
        WriteByte(address, Y & 0xFF);
        cycles += 4;
    } else { // 16-bit mode
        WriteWord(address, Y);
        cycles += 5;
    }
}

void CPU::STY_DirectPage() {
    const uint8_t offset = ReadByte(PC + 1);
    const uint32_t address = (D + offset) & 0xFFFF;
    PC += 2;

    if (P & FLAG_X) { // 8-bit mode
        WriteByte(address, Y & 0xFF);
        cycles += 3;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    } else { // 16-bit mode
        WriteWord(address, Y);
        cycles += 4;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    }
}

void CPU::STY_DirectPageX() {
    const uint8_t offset = ReadByte(PC + 1);
    const uint32_t address = (D + offset + X) & 0xFFFF;
    PC += 2;

    if (P & FLAG_X) { // 8-bit mode
        WriteByte(address, Y & 0xFF);
        cycles += 4;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    } else { // 16-bit mode
        WriteWord(address, Y);
        cycles += 5;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    }
}

// INC - Increment Memory
// Important to note: PC doesn't increment in accumulator mode I think
void CPU::INC_Accumulator() {
    if (P & FLAG_M) { // 8-bit mode
        A = (A & 0xFF00) | ((A + 1) & 0xFF);
        UpdateNZ8(A & 0xFF);
        cycles += 2;
    } else { // 16-bit mode
        A = (A + 1) & 0xFFFF;
        UpdateNZ16(A);
        cycles += 2;
    }
}

void CPU::INC_Absolute() {
    const uint32_t address = ReadWord(PC + 1) | (DB << 16);
    PC += 3;

    if (P & FLAG_M) { // 8-bit mode
        uint8_t value = ReadByte(address);
        value = (value + 1) & 0xFF;
        WriteByte(address, value);
        UpdateNZ8(value);
        cycles += 6;
    } else { // 16-bit mode
        uint16_t value = ReadWord(address);
        value = (value + 1) & 0xFFFF;
        WriteWord(address, value);
        UpdateNZ16(value);
        cycles += 8;
    }
}

void CPU::INC_AbsoluteX() {
    const uint32_t base = ReadWord(PC + 1) | (DB << 16);
    const uint32_t address = base + X;
    PC += 3;

    if (P & FLAG_M) { // 8-bit mode
        uint8_t value = ReadByte(address);
        value = (value + 1) & 0xFF;
        WriteByte(address, value);
        UpdateNZ8(value);
        cycles += 7;
    } else { // 16-bit mode
        uint16_t value = ReadWord(address);
        value = (value + 1) & 0xFFFF;
        WriteWord(address, value);
        UpdateNZ16(value);
        cycles += 9;
    }
}

void CPU::INC_DirectPage() {
    const uint8_t offset = ReadByte(PC + 1);
    const uint32_t address = (D + offset) & 0xFFFF;
    PC += 2;

    if (P & FLAG_M) { // 8-bit mode
        uint8_t value = ReadByte(address);
        value = (value + 1) & 0xFF;
        WriteByte(address, value);
        UpdateNZ8(value);
        cycles += 5;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    } else { // 16-bit mode
        uint16_t value = ReadWord(address);
        value = (value + 1) & 0xFFFF;
        WriteWord(address, value);
        UpdateNZ16(value);
        cycles += 7;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    }
}

void CPU::INC_DirectPageX() {
    const uint8_t offset = ReadByte(PC + 1);
    const uint32_t address = (D + offset + X) & 0xFFFF;
    PC += 2;

    if (P & FLAG_M) { // 8-bit mode
        uint8_t value = ReadByte(address);
        value = (value + 1) & 0xFF;
        WriteByte(address, value);
        UpdateNZ8(value);
        cycles += 6;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    } else { // 16-bit mode
        uint16_t value = ReadWord(address);
        value = (value + 1) & 0xFFFF;
        WriteWord(address, value);
        UpdateNZ16(value);
        cycles += 8;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    }
}

// DEC - Decrement Memory
void CPU::DEC_Accumulator() {
    if (P & FLAG_M) { // 8-bit mode
        A = (A & 0xFF00) | ((A - 1) & 0xFF);
        UpdateNZ8(A & 0xFF);
        cycles += 2;
    } else { // 16-bit mode
        A = (A - 1) & 0xFFFF;
        UpdateNZ16(A);
        cycles += 2;
    }
}

void CPU::DEC_Absolute() {
    const uint32_t address = ReadWord(PC + 1) | (DB << 16);
    PC += 3;

    if (P & FLAG_M) { // 8-bit mode
        uint8_t value = ReadByte(address);
        value = (value - 1) & 0xFF;
        WriteByte(address, value);
        UpdateNZ8(value);
        cycles += 6;
    } else { // 16-bit mode
        uint16_t value = ReadWord(address);
        value = (value - 1) & 0xFFFF;
        WriteWord(address, value);
        UpdateNZ16(value);
        cycles += 8;
    }
}

void CPU::DEC_AbsoluteX() {
    const uint32_t base = ReadWord(PC + 1) | (DB << 16);
    const uint32_t address = base + X;
    PC += 3;

    if (P & FLAG_M) { // 8-bit mode
        uint8_t value = ReadByte(address);
        value = (value - 1) & 0xFF;
        WriteByte(address, value);
        UpdateNZ8(value);
        cycles += 7;
    } else { // 16-bit mode
        uint16_t value = ReadWord(address);
        value = (value - 1) & 0xFFFF;
        WriteWord(address, value);
        UpdateNZ16(value);
        cycles += 9;
    }
}

void CPU::DEC_DirectPage() {
    const uint8_t offset = ReadByte(PC + 1);
    const uint32_t address = (D + offset) & 0xFFFF;
    PC += 2;

    if (P & FLAG_M) { // 8-bit mode
        uint8_t value = ReadByte(address);
        value = (value - 1) & 0xFF;
        WriteByte(address, value);
        UpdateNZ8(value);
        cycles += 5;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    } else { // 16-bit mode
        uint16_t value = ReadWord(address);
        value = (value - 1) & 0xFFFF;
        WriteWord(address, value);
        UpdateNZ16(value);
        cycles += 7;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    }
}

void CPU::DEC_DirectPageX() {
    const uint8_t offset = ReadByte(PC + 1);
    const uint32_t address = (D + offset + X) & 0xFFFF;
    PC += 2;

    if (P & FLAG_M) { // 8-bit mode
        uint8_t value = ReadByte(address);
        value = (value - 1) & 0xFF;
        WriteByte(address, value);
        UpdateNZ8(value);
        cycles += 6;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    } else { // 16-bit mode
        uint16_t value = ReadWord(address);
        value = (value - 1) & 0xFFFF;
        WriteWord(address, value);
        UpdateNZ16(value);
        cycles += 8;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    }
}

// INX - Increment X Register
void CPU::INX() {
    if (P & FLAG_X) { // 8-bit mode
        X = (X & 0xFF00) | ((X + 1) & 0xFF);
        UpdateNZ8(X & 0xFF);
    } else { // 16-bit mode
        X = (X + 1) & 0xFFFF;
        UpdateNZ16(X);
    }
    cycles += 2;
}

// INY - Increment Y Register
void CPU::INY() {
    if (P & FLAG_X) { // 8-bit mode
        Y = (Y & 0xFF00) | ((Y + 1) & 0xFF);
        UpdateNZ8(Y & 0xFF);
    } else { // 16-bit mode
        Y = (Y + 1) & 0xFFFF;
        UpdateNZ16(Y);
    }
    cycles += 2;
}

// DEX - Decrement X Register
void CPU::DEX() {
    if (P & FLAG_X) { // 8-bit mode
        X = (X & 0xFF00) | ((X - 1) & 0xFF);
        UpdateNZ8(X & 0xFF);
    } else { // 16-bit mode
        X = (X - 1) & 0xFFFF;
        UpdateNZ16(X);
    }
    cycles += 2;
}

// DEY - Decrement Y Register
void CPU::DEY() {
    if (P & FLAG_X) { // 8-bit mode
        Y = (Y & 0xFF00) | ((Y - 1) & 0xFF);
        UpdateNZ8(Y & 0xFF);
    } else { // 16-bit mode
        Y = (Y - 1) & 0xFFFF;
        UpdateNZ16(Y);
    }
    cycles += 2;
}

// CMP - Compare Accumulator
void CPU::CMP_Immediate() {
    if (P & FLAG_M) { // 8-bit mode
        const uint8_t operand = ReadByte(PC + 1);
        UpdateCompareFlags8(A & 0xFF, operand);
        PC += 2;
        cycles += 2;
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(PC + 1);
        UpdateCompareFlags16(A, operand);
        PC += 3;
        cycles += 3;
    }
}

void CPU::CMP_Absolute() {
    const uint32_t address = ReadWord(PC + 1) | (DB << 16);
    PC += 3;

    if (P & FLAG_M) { // 8-bit mode
        const uint8_t operand = ReadByte(address);
        UpdateCompareFlags8(A & 0xFF, operand);
        cycles += 4;
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(address);
        UpdateCompareFlags16(A, operand);
        cycles += 5;
    }
}

void CPU::CMP_AbsoluteX() {
    const uint32_t base = ReadWord(PC + 1) | (DB << 16);
    const uint32_t address = base + X;
    PC += 3;

    if (P & FLAG_M) { // 8-bit mode
        const uint8_t operand = ReadByte(address);
        UpdateCompareFlags8(A & 0xFF, operand);
        cycles += 4;
        // Add extra cycle if page boundary crossed
        if ((base & 0xFF00) != (address & 0xFF00)) cycles++;
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(address);
        UpdateCompareFlags16(A, operand);
        cycles += 5;
        // Add extra cycle if page boundary crossed
        if ((base & 0xFF00) != (address & 0xFF00)) cycles++;
    }
}

void CPU::CMP_AbsoluteY() {
    const uint32_t base = ReadWord(PC + 1) | (DB << 16);
    const uint32_t address = base + Y;
    PC += 3;

    if (P & FLAG_M) { // 8-bit mode
        const uint8_t operand = ReadByte(address);
        UpdateCompareFlags8(A & 0xFF, operand);
        cycles += 4;
        // Add extra cycle if page boundary crossed
        if ((base & 0xFF00) != (address & 0xFF00)) cycles++;
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(address);
        UpdateCompareFlags16(A, operand);
        cycles += 5;
        // Add extra cycle if page boundary crossed
        if ((base & 0xFF00) != (address & 0xFF00)) cycles++;
    }
}

void CPU::CMP_DirectPage() {
    const uint8_t offset = ReadByte(PC + 1);
    const uint32_t address = (D + offset) & 0xFFFF;
    PC += 2;

    if (P & FLAG_M) { // 8-bit mode
        const uint8_t operand = ReadByte(address);
        UpdateCompareFlags8(A & 0xFF, operand);
        cycles += 3;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(address);
        UpdateCompareFlags16(A, operand);
        cycles += 4;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    }
}

void CPU::CMP_DirectPageX() {
    const uint8_t offset = ReadByte(PC + 1);
    const uint32_t address = (D + offset + X) & 0xFFFF;
    PC += 2;

    if (P & FLAG_M) { // 8-bit mode
        const uint8_t operand = ReadByte(address);
        UpdateCompareFlags8(A & 0xFF, operand);
        cycles += 4;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(address);
        UpdateCompareFlags16(A, operand);
        cycles += 5;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    }
}

void CPU::CMP_IndirectDirectPage() {
    const uint8_t offset = ReadByte(PC + 1);
    const uint32_t pointer = (D + offset) & 0xFFFF;
    const uint32_t address = ReadWord(pointer) | (DB << 16);
    PC += 2;

    if (P & FLAG_M) { // 8-bit mode
        uint8_t operand = ReadByte(address);
        UpdateCompareFlags8(A & 0xFF, operand);
        cycles += 5;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(address);
        UpdateCompareFlags16(A, operand);
        cycles += 6;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    }
}

void CPU::CMP_IndirectDirectPageY() {
    const uint8_t offset = ReadByte(PC + 1);
    const uint32_t pointer = (D + offset) & 0xFFFF;
    const uint32_t base = ReadWord(pointer) | (DB << 16);
    const uint32_t address = base + Y;
    PC += 2;

    if (P & FLAG_M) { // 8-bit mode
        const uint8_t operand = ReadByte(address);
        UpdateCompareFlags8(A & 0xFF, operand);
        cycles += 5;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
        // Add extra cycle if page boundary crossed
        if ((base & 0xFF00) != (address & 0xFF00)) cycles++;
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(address);
        UpdateCompareFlags16(A, operand);
        cycles += 6;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
        // Add extra cycle if page boundary crossed
        if ((base & 0xFF00) != (address & 0xFF00)) cycles++;
    }
}

void CPU::CMP_DirectPageIndirectX() {
    const uint8_t offset = ReadByte(PC + 1);
    const uint32_t pointer = (D + offset + X) & 0xFFFF;
    const uint32_t address = ReadWord(pointer) | (DB << 16);
    PC += 2;

    if (P & FLAG_M) { // 8-bit mode
        const uint8_t operand = ReadByte(address);
        UpdateCompareFlags8(A & 0xFF, operand);
        cycles += 6;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(address);
        UpdateCompareFlags16(A, operand);
        cycles += 7;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    }
}

void CPU::CMP_Long() {
    const uint32_t address = ReadByte(PC + 1) | (ReadByte(PC + 2) << 8) | (ReadByte(PC + 3) << 16);
    PC += 4;

    if (P & FLAG_M) { // 8-bit mode
        const uint8_t operand = ReadByte(address);
        UpdateCompareFlags8(A & 0xFF, operand);
        cycles += 5;
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(address);
        UpdateCompareFlags16(A, operand);
        cycles += 6;
    }
}

void CPU::CMP_LongX() {
    const uint32_t base = ReadByte(PC + 1) | (ReadByte(PC + 2) << 8) | (ReadByte(PC + 3) << 16);
    const uint32_t address = base + X;
    PC += 4;

    if (P & FLAG_M) { // 8-bit mode
        const uint8_t operand = ReadByte(address);
        UpdateCompareFlags8(A & 0xFF, operand);
        cycles += 6;
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(address);
        UpdateCompareFlags16(A, operand);
        cycles += 7;
    }
}

// CPX - Compare X Register
void CPU::CPX_Immediate() {
    if (P & FLAG_X) { // 8-bit mode
        const uint8_t operand = ReadByte(PC + 1);
        UpdateCompareFlags8(X & 0xFF, operand);
        PC += 2;
        cycles += 2;
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(PC + 1);
        UpdateCompareFlags16(X, operand);
        PC += 3;
        cycles += 3;
    }
}

void CPU::CPX_Absolute() {
    const uint32_t address = ReadWord(PC + 1) | (DB << 16);
    PC += 3;

    if (P & FLAG_X) { // 8-bit mode
        const uint8_t operand = ReadByte(address);
        UpdateCompareFlags8(X & 0xFF, operand);
        cycles += 4;
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(address);
        UpdateCompareFlags16(X, operand);
        cycles += 5;
    }
}

void CPU::CPX_DirectPage() {
    const uint8_t offset = ReadByte(PC + 1);
    const int32_t address = (D + offset) & 0xFFFF;
    PC += 2;

    if (P & FLAG_X) { // 8-bit mode
        const uint8_t operand = ReadByte(address);
        UpdateCompareFlags8(X & 0xFF, operand);
        cycles += 3;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(address);
        UpdateCompareFlags16(X, operand);
        cycles += 4;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    }
}

// CPY - Compare Y Register
void CPU::CPY_Immediate() {
    if (P & FLAG_X) { // 8-bit mode
        const uint8_t operand = ReadByte(PC + 1);
        UpdateCompareFlags8(Y & 0xFF, operand);
        PC += 2;
        cycles += 2;
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(PC + 1);
        UpdateCompareFlags16(Y, operand);
        PC += 3;
        cycles += 3;
    }
}

void CPU::CPY_Absolute() {
    const uint32_t address = ReadWord(PC + 1) | (DB << 16);
    PC += 3;

    if (P & FLAG_X) { // 8-bit mode
        const uint8_t operand = ReadByte(address);
        UpdateCompareFlags8(Y & 0xFF, operand);
        cycles += 4;
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(address);
        UpdateCompareFlags16(Y, operand);
        cycles += 5;
    }
}

void CPU::CPY_DirectPage() {
    const uint8_t offset = ReadByte(PC + 1);
    const uint32_t address = (D + offset) & 0xFFFF;
    PC += 2;

    if (P & FLAG_X) { // 8-bit mode
        const uint8_t operand = ReadByte(address);
        UpdateCompareFlags8(Y & 0xFF, operand);
        cycles += 3;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(address);
        UpdateCompareFlags16(Y, operand);
        cycles += 4;
        if (D & 0xFF) cycles++; // Extra cycle if D register low byte != 0
    }
}

void CPU::JMP_Absolute() {
    const uint16_t address = ReadWord(PC);
    PC += 2;

    PC = (static_cast<uint32_t>(PB) << 16) | address;

    cycles += 3;
}

void CPU::JMP_AbsoluteIndirect() {
    const uint16_t indirect_addr = ReadWord(PC);
    PC += 2;

    const uint32_t full_indirect_addr = (static_cast<uint32_t>(DB) << 16) | indirect_addr;
    const uint16_t target_addr = ReadWord(full_indirect_addr);

    PC = (static_cast<uint32_t>(PB) << 16) | target_addr;

    cycles += 5;
}

void CPU::JMP_AbsoluteLong() {
    const uint16_t addr_low = ReadWord(PC);
    PC += 2;
    const uint8_t addr_high = ReadByte(PC);
    PC++;

    const uint32_t target_addr = (static_cast<uint32_t>(addr_high) << 16) | addr_low;

    PB = addr_high;
    PC = target_addr;

    cycles += 4;
}

void CPU::JMP_AbsoluteIndirectX() {
    const uint16_t base_addr = ReadWord(PC);
    PC += 2;

    const uint32_t indirect_addr = (static_cast<uint32_t>(PB) << 16) | (base_addr + X);

    const uint16_t target_addr = ReadWord(indirect_addr);

    PC = (static_cast<uint32_t>(PB) << 16) | target_addr;

    cycles += 6;
}

void CPU::BEQ_Relative() {
    DoBranch(P & FLAG_Z);
}

void CPU::BNE_Relative() {
    DoBranch(!(P & FLAG_Z));
}

void CPU::BCC_Relative() {
    DoBranch(!(P & FLAG_C));
}

void CPU::BCS_Relative() {
    DoBranch(P & FLAG_C);
}

void CPU::BMI_Relative() {
    DoBranch(P & FLAG_N);
}

void CPU::BPL_Relative() {
    DoBranch(!(P & FLAG_N));
}

void CPU::JSR_Absolute() {
    const uint16_t target_addr = ReadWord(PC);
    PC += 2;

    const uint16_t return_addr = (PC - 1) & 0xFFFF;
    PushWord(return_addr);

    PC = (static_cast<uint32_t>(PB) << 16) | target_addr;

    cycles += 6;
}

void CPU::JSR_AbsoluteLong() {
    const uint16_t addr_low = ReadWord(PC);
    PC += 2;
    const uint8_t addr_high = ReadByte(PC);
    PC++;

    PushByte(PB);

    const uint16_t return_addr = (PC - 1) & 0xFFFF;
    PushWord(return_addr);

    PB = addr_high;
    PC = (static_cast<uint32_t>(addr_high) << 16) | addr_low;

    cycles += 8;
}

void CPU::JSR_AbsoluteIndirectX() {
    const uint16_t base_addr = ReadWord(PC);
    PC += 2;

    const uint32_t indirect_addr = (static_cast<uint32_t>(PB) << 16) | ((base_addr + X) & 0xFFFF);

    const uint16_t target_addr = ReadWord(indirect_addr);

    const uint16_t return_addr = (PC - 1) & 0xFFFF;
    PushWord(return_addr);

    PC = (static_cast<uint32_t>(PB) << 16) | target_addr;

    cycles += 8;
}

void CPU::RTS() {
    const uint16_t return_addr = PopWord();

    PC = (static_cast<uint32_t>(PB) << 16) | ((return_addr + 1) & 0xFFFF);

    cycles += 6;
}

void CPU::RTL() {
    const uint16_t return_addr = PopWord();

    PB = PopByte();

    PC = (static_cast<uint32_t>(PB) << 16) | ((return_addr + 1) & 0xFFFF);

    cycles += 6;
}

void CPU::PHA() {
    if (P & FLAG_M) {
        // 8-bit mode: push low byte of accumulator
        PushByte(A & 0xFF);
        cycles += 3;
    } else {
        // 16-bit mode
        PushWord(A);
        cycles += 4;
    }
}

void CPU::PLA() {
    if (P & FLAG_M) {
        // 8-bit mode: pull into low byte, clear high byte
        A = PopByte();
        UpdateNZ8(A & 0xFF);
        cycles += 4;
    } else {
        // 16-bit mode
        A = PopWord();
        UpdateNZ16(A);
        cycles += 5;
    }
}

void CPU::PHX() {
    if (P & FLAG_X) {
        // 8-bit mode: push low byte of X
        PushByte(X & 0xFF);
        cycles += 3;
    } else {
        // 16-bit mode
        PushWord(X);
        cycles += 4;
    }
}

void CPU::PLX() {
    if (P & FLAG_X) {
        // 8-bit mode: pull into low byte, clear high byte
        X = PopByte();
        UpdateNZ8(X & 0xFF);
        cycles += 4;
    } else {
        // 16-bit mode
        X = PopWord();
        UpdateNZ16(X);
        cycles += 5;
    }
}

void CPU::PHY() {
    if (P & FLAG_X) {
        // 8-bit mode: push low byte of Y
        PushByte(Y & 0xFF);
        cycles += 3;
    } else {
        // 16-bit mode
        PushWord(Y);
        cycles += 4;
    }
}

void CPU::PLY() {
    if (P & FLAG_X) {
        // 8-bit mode: pull into low byte, clear high byte
        Y = PopByte();
        UpdateNZ8(Y & 0xFF);
        cycles += 4;
    } else {
        // 16-bit mode: pull full 16-bit value
        Y = PopWord();
        UpdateNZ16(Y);
        cycles += 5;
    }
}

void CPU::PHP() {
    PushByte(P);
    cycles += 3;
}

void CPU::PLP() {
    P = PopByte();
    cycles += 4;
}

void CPU::PHB() {
    PushByte(DB);
    cycles += 3;
}

void CPU::PLB() {
    DB = PopByte();
    UpdateNZ8(DB);
    cycles += 4;
}

void CPU::PHD() {
    PushWord(D);
    cycles += 4;
}

void CPU::PLD() {
    D = PopWord();
    UpdateNZ16(D);
    cycles += 5;
}

void CPU::PHK() {
    PushByte(PB);
    cycles += 3;
}

void CPU::ADC_Immediate() {
    if (P & FLAG_M) {
        // 8-bit immediate
        const uint8_t value = ReadByte(PC);
        PC++;
        DoADC(value);
        cycles += 2;
    } else {
        // 16-bit immediate
        const uint16_t value = ReadWord(PC);
        PC += 2;
        DoADC(value);
        cycles += 3;
    }
}

void CPU::ADC_Absolute() {
    const uint16_t address = ReadWord(PC);
    PC += 2;

    const uint32_t full_address = (static_cast<uint32_t>(DB) << 16) | address;

    if (P & FLAG_M) {
        const uint8_t value = ReadByte(full_address);
        DoADC(value);
        cycles += 4;
    } else {
        const uint16_t value = ReadWord(full_address);
        DoADC(value);
        cycles += 5;
    }
}

void CPU::ADC_AbsoluteX() {
    const uint16_t base_address = ReadWord(PC);
    PC += 2;

    const uint32_t full_address = (static_cast<uint32_t>(DB) << 16) | ((base_address + X) & 0xFFFF);

    if (P & FLAG_M) {
        const uint8_t value = ReadByte(full_address);
        DoADC(value);
        cycles += 4;
        if ((base_address & 0xFF00) != ((base_address + X) & 0xFF00)) {
            cycles++;
        }
    } else {
        const uint16_t value = ReadWord(full_address);
        DoADC(value);
        cycles += 5;
        if ((base_address & 0xFF00) != ((base_address + X) & 0xFF00)) {
            cycles++;
        }
    }
}

void CPU::ADC_AbsoluteY() {
    const uint16_t base_address = ReadWord(PC);
    PC += 2;

    const uint32_t full_address = (static_cast<uint32_t>(DB) << 16) | ((base_address + Y) & 0xFFFF);

    if (P & FLAG_M) {
        const uint8_t value = ReadByte(full_address);
        DoADC(value);
        cycles += 4;
        if ((base_address & 0xFF00) != ((base_address + Y) & 0xFF00)) {
            cycles++;
        }
    } else {
        const uint16_t value = ReadWord(full_address);
        DoADC(value);
        cycles += 5;
        if ((base_address & 0xFF00) != ((base_address + Y) & 0xFF00)) {
            cycles++;
        }
    }
}

void CPU::ADC_DirectPage() {
    const uint8_t offset = ReadByte(PC);
    PC++;

    const uint32_t address = D + offset;

    if (P & FLAG_M) {
        const uint8_t value = ReadByte(address);
        DoADC(value);
        cycles += 3;
        if (D & 0xFF) cycles++;
    } else {
        const uint16_t value = ReadWord(address);
        DoADC(value);
        cycles += 4;
        if (D & 0xFF) cycles++;
    }
}

void CPU::ADC_DirectPageX() {
    const uint8_t offset = ReadByte(PC);
    PC++;

    const uint32_t address = D + offset + X;

    if (P & FLAG_M) {
        const uint8_t value = ReadByte(address);
        DoADC(value);
        cycles += 4;
        if (D & 0xFF) cycles++;
    } else {
        const uint16_t value = ReadWord(address);
        DoADC(value);
        cycles += 5;
        if (D & 0xFF) cycles++;
    }
}

void CPU::ADC_IndirectDirectPage() {
    const uint8_t offset = ReadByte(PC);
    PC++;

    const uint32_t pointer_address = D + offset;
    const uint16_t target_address = ReadWord(pointer_address);
    const uint32_t full_address = (static_cast<uint32_t>(DB) << 16) | target_address;

    if (P & FLAG_M) {
        const uint8_t value = ReadByte(full_address);
        DoADC(value);
        cycles += 5;
        if (D & 0xFF) cycles++;
    } else {
        const uint16_t value = ReadWord(full_address);
        DoADC(value);
        cycles += 6;
        if (D & 0xFF) cycles++;
    }
}

void CPU::ADC_IndirectDirectPageY() {
    const uint8_t offset = ReadByte(PC);
    PC++;

    const uint32_t pointer_address = D + offset;
    const uint16_t base_address = ReadWord(pointer_address);
    const uint32_t full_address = (static_cast<uint32_t>(DB) << 16) | ((base_address + Y) & 0xFFFF);

    if (P & FLAG_M) {
        const uint8_t value = ReadByte(full_address);
        DoADC(value);
        cycles += 5;
        if (D & 0xFF) cycles++;
        if ((base_address & 0xFF00) != ((base_address + Y) & 0xFF00)) {
            cycles++;
        }
    } else {
        const uint16_t value = ReadWord(full_address);
        DoADC(value);
        cycles += 6;
        if (D & 0xFF) cycles++;
        if ((base_address & 0xFF00) != ((base_address + Y) & 0xFF00)) {
            cycles++;
        }
    }
}

void CPU::ADC_DirectPageIndirectX() {
    const uint8_t offset = ReadByte(PC);
    PC++;

    const uint32_t pointer_address = D + offset + X;
    const uint16_t target_address = ReadWord(pointer_address);
    const uint32_t full_address = (static_cast<uint32_t>(DB) << 16) | target_address;

    if (P & FLAG_M) {
        const uint8_t value = ReadByte(full_address);
        DoADC(value);
        cycles += 6;
        if (D & 0xFF) cycles++;
    } else {
        const uint16_t value = ReadWord(full_address);
        DoADC(value);
        cycles += 7;
        if (D & 0xFF) cycles++;
    }
}

void CPU::ADC_AbsoluteLong() {
    const uint16_t addr_low = ReadWord(PC);
    PC += 2;
    const uint8_t addr_high = ReadByte(PC);
    PC++;

    const uint32_t full_address = (static_cast<uint32_t>(addr_high) << 16) | addr_low;

    if (P & FLAG_M) {
        const uint8_t value = ReadByte(full_address);
        DoADC(value);
        cycles += 5;
    } else {
        const uint16_t value = ReadWord(full_address);
        DoADC(value);
        cycles += 6;
    }
}

void CPU::ADC_AbsoluteLongX() {
    const uint16_t addr_low = ReadWord(PC);
    PC += 2;
    const uint8_t addr_high = ReadByte(PC);
    PC++;

    const uint32_t base_address = (static_cast<uint32_t>(addr_high) << 16) | addr_low;
    const uint32_t full_address = base_address + X;

    if (P & FLAG_M) {
        const uint8_t value = ReadByte(full_address);
        DoADC(value);
        cycles += 5;
    } else {
        const uint16_t value = ReadWord(full_address);
        DoADC(value);
        cycles += 6;
    }
}

void CPU::ADC_DirectPageIndirectLong() {
    const uint8_t offset = ReadByte(PC);
    PC++;

    const uint32_t pointer_address = D + offset;
    const uint16_t addr_low = ReadWord(pointer_address);
    const uint8_t addr_high = ReadByte(pointer_address + 2);
    const uint32_t target_address = (static_cast<uint32_t>(addr_high) << 16) | addr_low;

    if (P & FLAG_M) {
        const uint8_t value = ReadByte(target_address);
        DoADC(value);
        cycles += 6;
        if (D & 0xFF) cycles++;
    } else {
        const uint16_t value = ReadWord(target_address);
        DoADC(value);
        cycles += 7;
        if (D & 0xFF) cycles++;
    }
}

void CPU::ADC_DirectPageIndirectLongY() {
    const uint8_t offset = ReadByte(PC);
    PC++;

    const uint32_t pointer_address = D + offset;
    const uint16_t addr_low = ReadWord(pointer_address);
    const uint8_t addr_high = ReadByte(pointer_address + 2);
    const uint32_t base_address = (static_cast<uint32_t>(addr_high) << 16) | addr_low;
    const uint32_t target_address = base_address + Y;

    if (P & FLAG_M) {
        const uint8_t value = ReadByte(target_address);
        DoADC(value);
        cycles += 6;
        if (D & 0xFF) cycles++;
    } else {
        const uint16_t value = ReadWord(target_address);
        DoADC(value);
        cycles += 7;
        if (D & 0xFF) cycles++;
    }
}

void CPU::ADC_StackRelative() {
    const uint8_t offset = ReadByte(PC);
    PC++;

    const uint32_t address = SP + offset;

    if (P & FLAG_M) {
        const uint8_t value = ReadByte(address);
        DoADC(value);
        cycles += 4;
    } else {
        const uint16_t value = ReadWord(address);
        DoADC(value);
        cycles += 5;
    }
}

void CPU::ADC_StackRelativeIndirectY() {
    const uint8_t offset = ReadByte(PC);
    PC++;

    const uint32_t pointer_address = SP + offset;
    const uint16_t base_address = ReadWord(pointer_address);
    const uint32_t target_address = (static_cast<uint32_t>(DB) << 16) | ((base_address + Y) & 0xFFFF);

    if (P & FLAG_M) {
        const uint8_t value = ReadByte(target_address);
        DoADC(value);
        cycles += 7;
    } else {
        const uint16_t value = ReadWord(target_address);
        DoADC(value);
        cycles += 8;
    }
}

void CPU::AND_Immediate() {
    if (P & FLAG_M) { // 8-bit mode
        const uint8_t operand = ReadByte(PC++);
        A = (A & 0xFF00) | ((A & 0xFF) & operand);
        UpdateNZ8(A & 0xFF);
        cycles += 2;
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(PC);
        PC += 2;
        A &= operand;
        UpdateNZ16(A);
        cycles += 3;
    }
}

void CPU::AND_Absolute() {
    const uint16_t address = ReadWord(PC);
    PC += 2;
    const uint32_t full_address = (DB << 16) | address;

    if (P & FLAG_M) { // 8-bit mode
        const uint8_t operand = ReadByte(full_address);
        A = (A & 0xFF00) | ((A & 0xFF) & operand);
        UpdateNZ8(A & 0xFF);
        cycles += 4;
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(full_address);
        A &= operand;
        UpdateNZ16(A);
        cycles += 5;
    }
}

void CPU::AND_AbsoluteX() {
    const uint16_t base_address = ReadWord(PC);
    PC += 2;
    const uint32_t full_address = (DB << 16) | (base_address + X);

    if (P & FLAG_M) { // 8-bit mode
        const uint8_t operand = ReadByte(full_address);
        A = (A & 0xFF00) | ((A & 0xFF) & operand);
        UpdateNZ8(A & 0xFF);
        cycles += 4;
        if ((base_address & 0xFF00) != ((base_address + X) & 0xFF00)) {
            cycles++;
        }
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(full_address);
        A &= operand;
        UpdateNZ16(A);
        cycles += 5;
        if ((base_address & 0xFF00) != ((base_address + X) & 0xFF00)) {
            cycles++;
        }
    }
}

void CPU::AND_AbsoluteY() {
    const uint16_t base_address = ReadWord(PC);
    PC += 2;
    const uint32_t full_address = (DB << 16) | (base_address + Y);

    if (P & FLAG_M) { // 8-bit mode
        const uint8_t operand = ReadByte(full_address);
        A = (A & 0xFF00) | ((A & 0xFF) & operand);
        UpdateNZ8(A & 0xFF);
        cycles += 4;
        if ((base_address & 0xFF00) != ((base_address + Y) & 0xFF00)) {
            cycles++;
        }
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(full_address);
        A &= operand;
        UpdateNZ16(A);
        cycles += 5;
        if ((base_address & 0xFF00) != ((base_address + Y) & 0xFF00)) {
            cycles++;
        }
    }
}

void CPU::AND_DirectPage() {
    const uint8_t offset = ReadByte(PC++);
    const uint32_t address = D + offset;

    if (P & FLAG_M) { // 8-bit mode
        const uint8_t operand = ReadByte(address);
        A = (A & 0xFF00) | ((A & 0xFF) & operand);
        UpdateNZ8(A & 0xFF);
        cycles += 3;
        if (D & 0xFF) cycles++;
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(address);
        A &= operand;
        UpdateNZ16(A);
        cycles += 4;
        if (D & 0xFF) cycles++;
    }
}

void CPU::AND_DirectPageX() {
    const uint8_t offset = ReadByte(PC++);
    const uint32_t address = D + offset + X;

    if (P & FLAG_M) { // 8-bit mode
        const uint8_t operand = ReadByte(address);
        A = (A & 0xFF00) | ((A & 0xFF) & operand);
        UpdateNZ8(A & 0xFF);
        cycles += 4;
        if (D & 0xFF) cycles++;
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(address);
        A &= operand;
        UpdateNZ16(A);
        cycles += 5;
        if (D & 0xFF) cycles++;
    }
}

void CPU::AND_IndirectDirectPage() {
    const uint8_t offset = ReadByte(PC++);
    const uint32_t pointer_address = D + offset;
    const uint16_t indirect_address = ReadWord(pointer_address);
    const uint32_t full_address = (DB << 16) | indirect_address;

    if (P & FLAG_M) { // 8-bit mode
        const uint8_t operand = ReadByte(full_address);
        A = (A & 0xFF00) | ((A & 0xFF) & operand);
        UpdateNZ8(A & 0xFF);
        cycles += 5;
        if (D & 0xFF) cycles++;
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(full_address);
        A &= operand;
        UpdateNZ16(A);
        cycles += 6;
        if (D & 0xFF) cycles++;
    }
}

void CPU::AND_IndirectDirectPageLong() {
    const uint8_t offset = ReadByte(PC++);
    const uint32_t pointer_address = D + offset;
    const uint32_t full_address = ReadByte(pointer_address) |
                           (ReadByte(pointer_address + 1) << 8) |
                           (ReadByte(pointer_address + 2) << 16);

    if (P & FLAG_M) { // 8-bit mode
        const uint8_t operand = ReadByte(full_address);
        A = (A & 0xFF00) | ((A & 0xFF) & operand);
        UpdateNZ8(A & 0xFF);
        cycles += 6;
        if (D & 0xFF) cycles++;
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(full_address);
        A &= operand;
        UpdateNZ16(A);
        cycles += 7;
        if (D & 0xFF) cycles++;
    }
}

void CPU::AND_IndexedIndirectDirectPageX() {
    const uint8_t offset = ReadByte(PC++);
    const uint32_t pointer_address = D + offset + X;
    const uint16_t indirect_address = ReadWord(pointer_address);
    const uint32_t full_address = (DB << 16) | indirect_address;

    if (P & FLAG_M) { // 8-bit mode
        const uint8_t operand = ReadByte(full_address);
        A = (A & 0xFF00) | ((A & 0xFF) & operand);
        UpdateNZ8(A & 0xFF);
        cycles += 6;
        if (D & 0xFF) cycles++;
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(full_address);
        A &= operand;
        UpdateNZ16(A);
        cycles += 7;
        if (D & 0xFF) cycles++;
    }
}

void CPU::AND_IndirectDirectPageY() {
    const uint8_t offset = ReadByte(PC++);
    const uint32_t pointer_address = D + offset;
    const uint16_t base_address = ReadWord(pointer_address);
    const uint32_t full_address = (DB << 16) | (base_address + Y);

    if (P & FLAG_M) { // 8-bit mode
        const uint8_t operand = ReadByte(full_address);
        A = (A & 0xFF00) | ((A & 0xFF) & operand);
        UpdateNZ8(A & 0xFF);
        cycles += 5;
        if (D & 0xFF) cycles++;
        if ((base_address & 0xFF00) != ((base_address + Y) & 0xFF00)) {
            cycles++;
        }
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(full_address);
        A &= operand;
        UpdateNZ16(A);
        cycles += 6;
        if (D & 0xFF) cycles++;
        if ((base_address & 0xFF00) != ((base_address + Y) & 0xFF00)) {
            cycles++;
        }
    }
}

void CPU::AND_IndirectDirectPageLongY() {
    const uint8_t offset = ReadByte(PC++);
    const uint32_t pointer_address = D + offset;
    const uint32_t base_address = ReadByte(pointer_address) |
                           (ReadByte(pointer_address + 1) << 8) |
                           (ReadByte(pointer_address + 2) << 16);
    const uint32_t full_address = base_address + Y;

    if (P & FLAG_M) { // 8-bit mode
        uint8_t operand = ReadByte(full_address);
        A = (A & 0xFF00) | ((A & 0xFF) & operand);
        UpdateNZ8(A & 0xFF);
        cycles += 6;
        if (D & 0xFF) cycles++;
    } else { // 16-bit mode
        uint16_t operand = ReadWord(full_address);
        A &= operand;
        UpdateNZ16(A);
        cycles += 7;
        if (D & 0xFF) cycles++;
    }
}

void CPU::AND_AbsoluteLong() {
    const uint32_t address = ReadByte(PC) | (ReadByte(PC + 1) << 8) | (ReadByte(PC + 2) << 16);
    PC += 3;

    if (P & FLAG_M) { // 8-bit mode
        const uint8_t operand = ReadByte(address);
        A = (A & 0xFF00) | ((A & 0xFF) & operand);
        UpdateNZ8(A & 0xFF);
        cycles += 5;
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(address);
        A &= operand;
        UpdateNZ16(A);
        cycles += 6;
    }
}

void CPU::AND_AbsoluteLongX() {
    const uint32_t base_address = ReadByte(PC) | (ReadByte(PC + 1) << 8) | (ReadByte(PC + 2) << 16);
    PC += 3;
    const uint32_t full_address = base_address + X;

    if (P & FLAG_M) { // 8-bit mode
        const uint8_t operand = ReadByte(full_address);
        A = (A & 0xFF00) | ((A & 0xFF) & operand);
        UpdateNZ8(A & 0xFF);
        cycles += 5;
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(full_address);
        A &= operand;
        UpdateNZ16(A);
        cycles += 6;
    }
}

void CPU::AND_StackRelative() {
    const uint8_t offset = ReadByte(PC++);
    const uint32_t address = SP + offset;

    if (P & FLAG_M) { // 8-bit mode
        const uint8_t operand = ReadByte(address);
        A = (A & 0xFF00) | ((A & 0xFF) & operand);
        UpdateNZ8(A & 0xFF);
        cycles += 4;
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(address);
        A &= operand;
        UpdateNZ16(A);
        cycles += 5;
    }
}

void CPU::AND_StackRelativeIndirectY() {
    const uint8_t offset = ReadByte(PC++);
    const uint32_t pointer_address = SP + offset;
    const uint16_t base_address = ReadWord(pointer_address);
    const uint32_t full_address = (DB << 16) | (base_address + Y);

    if (P & FLAG_M) { // 8-bit mode
        const uint8_t operand = ReadByte(full_address);
        A = (A & 0xFF00) | ((A & 0xFF) & operand);
        UpdateNZ8(A & 0xFF);
        cycles += 7;
    } else { // 16-bit mode
        const uint16_t operand = ReadWord(full_address);
        A &= operand;
        UpdateNZ16(A);
        cycles += 8;
    }
}