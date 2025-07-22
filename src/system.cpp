//
// Created by Palindromic Bread Loaf on 7/21/25.
//

#include <fstream>
#include <iostream>

#include "bus.h"
#include "system.h"

// SNES System Implementation
System::System() : running(false) {
    bus = std::make_unique<Bus>(&cartridge_data);
    cpu = std::make_unique<CPU>(bus.get());
    ppu = std::make_unique<PPU>();
    apu = std::make_unique<APU>();
}

System::~System() {
    Shutdown();
}

bool System::LoadROM(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cout << "Failed to open ROM file: " << filename << std::endl;
        return false;
    }

    file.seekg(0, std::ios::end);
    const size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    cartridge_data.resize(size);
    file.read(reinterpret_cast<char*>(cartridge_data.data()), size);

    std::cout << "Loaded ROM: " << filename << " (" << size << " bytes)" << std::endl;
    return true;
}

void System::Reset() {
    cpu->Reset();
    ppu->Reset();
    apu->Reset();
}

void System::Step() {
    cpu->Step();
    ppu->Step();
    apu->Step();
}

void System::Run() {
    running = true;
    while (running) {
        Step();

        if (ppu->IsFrameComplete()) {
            ppu->SetFrameComplete(false);
            // TODO: Handle frame rendering and input
        }
    }
}

void System::Shutdown() {
    running = false;
}