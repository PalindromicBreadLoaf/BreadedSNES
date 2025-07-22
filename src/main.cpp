//
// Created by Palindromic Bread Loaf on 7/21/25.
//

#include <SDL2/SDL.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>

class CPU;
class PPU;
class APU;
class Bus;

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

// Memory Bus - handles memory mapping
class Bus {
private:
    uint8_t wram[0x20000];      // 128KB Work RAM
    uint8_t sram[0x8000];       // 32KB Save RAM
    std::vector<uint8_t>* cartridge; // Cartridge Data

public:
    Bus(std::vector<uint8_t>* cart) : cartridge(cart) {
        std::fill(wram, wram + sizeof(wram), 0);
        std::fill(sram, sram + sizeof(sram), 0);
    }

    uint8_t Read(uint32_t address);
    void Write(uint32_t address, uint8_t value);
    uint16_t Read16(uint32_t address);
    void Write16(uint32_t address, uint16_t value);
};

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

// PPU (Picture Processing Unit)
class PPU {
private:
    uint8_t vram[0x10000];      // 64KB Video RAM
    uint8_t oam[0x220];         // Object Attribute Memory
    uint8_t cgram[0x200];       // Color Generator RAM

    uint16_t scanline;
    uint16_t dot;
    bool frame_complete;

    // PPU registers
    uint8_t brightness;
    uint8_t bg_mode;
    // TODO: Add remaining registers

public:
    PPU() {
        Reset();
    }

    void Reset();
    void Step();
    bool IsFrameComplete() const { return frame_complete; }
    void SetFrameComplete(bool complete) { frame_complete = complete; }

    uint8_t ReadVRAM(uint16_t address);
    void WriteVRAM(uint16_t address, uint8_t value);

    // TODO: Implement Rendering
    void RenderScanline();
    void UpdateScreen();
};

// SPC700 APU
class APU {
private:
    uint8_t spc_ram[0x10000];   // 64KB SPC700 RAM

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

// Bus class
uint8_t Bus::Read(uint32_t address) {
    // TODO: Map properly based on SNES memory map
    if (address < 0x2000) {
        return wram[address];
    } else if (address >= 0x7E0000 && address < 0x800000) {
        return wram[address - 0x7E0000];
    } else if (address >= 0x800000 && cartridge) {
        // ROM access
        uint32_t rom_addr = address & 0x7FFFFF;
        if (rom_addr < cartridge->size()) {
            return (*cartridge)[rom_addr];
        }
    }
    return 0x00; // Open bus
}

void Bus::Write(uint32_t address, uint8_t value) {
    if (address < 0x2000) {
        wram[address] = value;
    } else if (address >= 0x7E0000 && address < 0x800000) {
        wram[address - 0x7E0000] = value;
    }
    // TODO: Add PPU/APU register writes here
}

uint16_t Bus::Read16(uint32_t address) {
    return Read(address) | (Read(address + 1) << 8);
}

void Bus::Write16(uint32_t address, uint16_t value) {
    Write(address, value & 0xFF);
    Write(address + 1, (value >> 8) & 0xFF);
}

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

// PPU Implementation
void PPU::Reset() {
    scanline = 0;
    dot = 0;
    frame_complete = false;
    brightness = 0x0F;
    bg_mode = 0;

    std::fill(vram, vram + sizeof(vram), 0);
    std::fill(oam, oam + sizeof(oam), 0);
    std::fill(cgram, cgram + sizeof(cgram), 0);
}

void PPU::Step() {
    dot++;
    if (dot >= 341) {
        dot = 0;
        scanline++;

        if (scanline >= 262) {
            scanline = 0;
            frame_complete = true;
        }
    }

    // TODO: Implement PPU renderer
}

uint8_t PPU::ReadVRAM(uint16_t address) {
    return vram[address & 0xFFFF];
}

void PPU::WriteVRAM(uint16_t address, uint8_t value) {
    vram[address & 0xFFFF] = value;
}

// APU Implementation
void APU::Reset() {
    A = X = Y = 0;
    SP = 0xFF;
    PC = 0x0000;
    PSW = 0x02;

    std::fill(spc_ram, spc_ram + sizeof(spc_ram), 0);
}

void APU::Step() {
    // TODO: Implement SPC700 instruction execution
}

uint8_t APU::ReadSPC(uint16_t address) {
    return spc_ram[address];
}

void APU::WriteSPC(uint16_t address, uint8_t value) {
    spc_ram[address] = value;
}

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
    size_t size = file.tellg();
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

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "BreadedSNES",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        256, 224,  // SNES output resolution
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cout << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    System snes;

    if (argc > 1) {
        if (!snes.LoadROM(argv[1])) {
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return -1;
        }
    }

    snes.Reset();

    bool quit = false;
    SDL_Event e;

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        // Run emulation step
        snes.Step();

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // TODO: Render frame

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}