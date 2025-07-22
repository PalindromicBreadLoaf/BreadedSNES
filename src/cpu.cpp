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

void CPU::ExecuteInstruction() {
    uint8_t opcode = bus->Read(PC++);

    // TODO: Actual Opcode decoding
    switch (opcode) {
        case 0xEA: NOP(); break;
        case 0xA9: LDA(); break;

        default:
            std::cout << "Unknown opcode: 0x" << std::hex << (int)opcode << std::endl;
            break;
    }

    cycles++;
}

void CPU::NOP() {
    // No operation
}

void CPU::LDA() {
    // Load accumulator - immediate mode
    if (P & FLAG_M) {
        // 8-bit mode
        A = (A & 0xFF00) | bus->Read(PC++);
    } else {
        // 16-bit mode
        A = bus->Read16(PC);
        PC += 2;
    }
}