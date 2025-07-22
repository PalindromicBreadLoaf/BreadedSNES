//
// Created by Palindromic Bread Loaf on 7/21/25.
//

#ifndef PPU_H
#define PPU_H
#include <cstdint>

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
    [[nodiscard]] bool IsFrameComplete() const { return frame_complete; }
    void SetFrameComplete(bool complete) { frame_complete = complete; }

    std::uint8_t ReadVRAM(uint16_t address);
    void WriteVRAM(uint16_t address, uint8_t value);

    // TODO: Implement Rendering
    void RenderScanline();
    void UpdateScreen();
};

#endif //PPU_H
