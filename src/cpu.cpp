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
        case 0xEA: NOP(); break;

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
    uint32_t address = ReadWord(PC + 1) | (DB << 16);
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
    uint32_t base = ReadWord(PC + 1) | (DB << 16);
    uint32_t address = base + X;
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
    uint32_t base = ReadWord(PC + 1) | (DB << 16);
    uint32_t address = base + Y;
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
    uint8_t offset = ReadByte(PC + 1);
    uint32_t address = (D + offset) & 0xFFFF;
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
    uint8_t offset = ReadByte(PC + 1);
    uint32_t address = (D + offset + X) & 0xFFFF;
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
    uint8_t offset = ReadByte(PC + 1);
    uint32_t pointer = (D + offset) & 0xFFFF;
    uint32_t address = ReadWord(pointer) | (DB << 16);
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
    uint8_t offset = ReadByte(PC + 1);
    uint32_t pointer = (D + offset) & 0xFFFF;
    uint32_t base = ReadWord(pointer) | (DB << 16);
    uint32_t address = base + Y;
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
    uint8_t offset = ReadByte(PC + 1);
    uint32_t pointer = (D + offset + X) & 0xFFFF;
    uint32_t address = ReadWord(pointer) | (DB << 16);
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
    uint32_t address = ReadByte(PC + 1) | (ReadByte(PC + 2) << 8) | (ReadByte(PC + 3) << 16);
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
    uint32_t base = ReadByte(PC + 1) | (ReadByte(PC + 2) << 8) | (ReadByte(PC + 3) << 16);
    uint32_t address = base + X;
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
    uint32_t address = ReadWord(PC + 1) | (DB << 16);
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
    uint8_t offset = ReadByte(PC + 1);
    uint32_t address = (D + offset) & 0xFFFF;
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
    uint8_t offset = ReadByte(PC + 1);
    uint32_t address = (D + offset) & 0xFFFF;
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
    uint8_t offset = ReadByte(PC + 1);
    uint32_t address = (D + offset + X) & 0xFFFF;
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