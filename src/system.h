//
// Created by Palindromic Bread Loaf on 7/21/25.
//

#ifndef SYSTEM_H
#define SYSTEM_H

#include <memory>
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "bus.h"


// Main SNES System class
class System {
private:
    std::unique_ptr<CPU> cpu;
    std::unique_ptr<PPU> ppu;
    std::unique_ptr<APU> apu;
    std::unique_ptr<Bus> bus;

    std::vector<uint8_t> cartridge_data;
    bool running;

public:
    System();
    ~System();

    bool LoadROM(const std::string& filename);
    void Reset();
    void Run();
    void Step();
    void Shutdown();
};

#endif //SYSTEM_H
