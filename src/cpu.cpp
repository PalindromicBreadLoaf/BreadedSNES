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
    uint8_t low = ReadByte(address);
    uint8_t high = ReadByte(address + 1);
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

void CPU::ExecuteInstruction() {
    // TODO: Actual Opcode decoding
    switch (const uint8_t opcode = bus->Read(PC++)) {
        // DEC - Decrement Memory
        case 0x3A: DEC_Accumulator(); break;       // DEC A
        case 0xCE: DEC_Absolute(); break;          // DEC $nnnn
        case 0xDE: DEC_AbsoluteX(); break;         // DEC $nnnn,X
        case 0xC6: DEC_DirectPage(); break;        // DEC $nn
        case 0xD6: DEC_DirectPageX(); break;       // DEC $nn,X

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
        case 0xA9: LDA_Immediate(); break;          // LDA #$nn or #$nnnn
        case 0xAD: LDA_Absolute(); break;           // LDA $nnnn
        case 0xBD: LDA_AbsoluteX(); break;          // LDA $nnnn,X
        case 0xB9: LDA_AbsoluteY(); break;          // LDA $nnnn,Y
        case 0xA5: LDA_DirectPage(); break;         // LDA $nn
        case 0xB5: LDA_DirectPageX(); break;        // LDA $nn,X
        case 0xB2: LDA_IndirectDirectPage(); break; // LDA ($nn)
        case 0xB1: LDA_IndirectDirectPageY(); break;// LDA ($nn),Y
        case 0xA1: LDA_DirectPageIndirectX(); break;// LDA ($nn,X)
        case 0xAF: LDA_Long(); break;               // LDA $nnnnnn
        case 0xBF: LDA_LongX(); break;              // LDA $nnnnnn,X

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

        //SDA - Store Accumulator
        case 0x8D: STA_Absolute(); break;
        case 0x9D: STA_AbsoluteX(); break;
        case 0x99: STA_AbsoluteY(); break;
        case 0x85: STA_DirectPage(); break;
        case 0x95: STA_DirectPageX(); break;
        case 0x92: STA_IndirectDirectPage(); break;
        case 0x91: STA_IndirectDirectPageY(); break;
        case 0x81: STA_DirectPageIndirectX(); break;
        case 0x8F: STA_Long(); break;
        case 0x9F: STA_LongX(); break;

        //SDX - Store X Register
        case 0x8E: STX_Absolute(); break;
        case 0x86: STX_DirectPage(); break;
        case 0x96: STX_DirectPageY(); break;

        // SDY - Store Y Register
        case 0x8C: STY_Absolute();  break;
        case 0x84: STY_DirectPage();  break;
        case 0x94: STY_DirectPageX(); break;

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