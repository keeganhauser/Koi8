#ifndef KOI8_HPP
#define KOI8_HPP

#include <iostream>
#include <cstdint>
#include <cassert>
#include <array>
#include <stack>
#include <chrono>
#include <fstream>
#include <unordered_map>

enum class Reg {
    V0 = 0, V1, V2, V3,
    V4, V5, V6, V7,
    V8, V9, VA, VB,
    VC, VD, VE, VF
};

class Koi8 {
    typedef void(Koi8::*instruction_func)(void);

public:
    Koi8() { Initialize(); }
    ~Koi8() = default;

    // Load a .ch8 file into Koi8's memory
    void LoadROM(const std::filesystem::path &rom_path) {
        std::ifstream rom_file(rom_path, std::ios::binary | std::ios::ate);

        if (rom_file.is_open()) {
            std::streampos file_size = rom_file.tellg();
            char *buffer = new char[file_size];

            rom_file.seekg(0, std::ios::beg);
            rom_file.read(buffer, file_size);
            rom_file.close();

            std::memcpy(&memory[0x200], buffer, file_size);

            delete [] buffer;
        } else {
            throw std::runtime_error("Failed to load ROM");
        }
    }

    // Update() will fetch and execute the next instruction being pointed at by the program counter.
    void Update() {
        // Decrement timers every second
        if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - last_dec_time) >= std::chrono::milliseconds(1000)) {
            last_dec_time = std::chrono::high_resolution_clock::now();
            if (sound_timer > 0) sound_timer--;
            if (delay_timer > 0) delay_timer--;
        }

        // Fetch
        instruction = memory[pc] << 8 | memory[pc + 1];
        if (instruction == 0) return;
        opcode = instruction >> 12 & 0xF;

        pc += 2;

        // Decode & execute
        try {
            (this->*(func_table.at(opcode)))();
        } catch (std::out_of_range &e) {
            std::cout << "Encountered error trying to execute instruction `" << std::hex << instruction << "`: " << e.what() << std::endl;
        }
    }

    // Retrieves the pointer to the start of the graphics buffer.
    [[nodiscard]] const uint32_t *GetGraphicsBuffer() const { return graphics_buffer.data(); }

private:
    std::array<uint8_t, 4096> memory{};
    std::array<uint32_t, 2048> graphics_buffer{};
    uint16_t pc;
    uint16_t idx_reg{};
    uint8_t delay_timer{};
    uint8_t sound_timer{};
    uint16_t instruction{};
    uint8_t opcode{};
    std::array<uint8_t, 16> reg_v{};
    std::stack<uint16_t> stack{};
    std::chrono::time_point<std::chrono::high_resolution_clock> last_dec_time;
    const std::array<uint8_t, 80> font_bytes{
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

    const std::unordered_map<uint8_t, instruction_func> func_table = {
        { 0x0, &Koi8::decode_0_opcode },
        { 0x1, &Koi8::Op_Jump },
        { 0x2, &Koi8::Op_GoSubroutine },
        { 0x3, &Koi8::Op_SkipEqI },
        { 0x4, &Koi8::Op_SkipNeqI },
        { 0x5, &Koi8::Op_SkipEqR },
        { 0x6, &Koi8::Op_SetRegI },
        { 0x7, &Koi8::Op_AddToRegI },
        { 0x8, &Koi8::decode_8_opcode },
        { 0x9, &Koi8::Op_SkipNeqR },
        { 0xA, &Koi8::Op_SetIdx },
        { 0xD, &Koi8::Op_Draw },
        { 0xF, &Koi8::decode_F_opcode },
    };

    const std::unordered_map<uint8_t, instruction_func> func_0_table = {
        { 0x0, &Koi8::Op_ClearScreen },
        { 0xE, &Koi8::Op_ReturnSub }
    };

    const std::unordered_map<uint8_t, instruction_func> func_8_table = {
        { 0x0, &Koi8::Op_SetRegR },
        { 0x1, &Koi8::Op_BitOr },
        { 0x2, &Koi8::Op_BitAnd },
        { 0x3, &Koi8::Op_BitXor },
        { 0x4, &Koi8::Op_AddToRegR },
        { 0x5, &Koi8::Op_SubXY },
        { 0x6, &Koi8::Op_ShiftRight },
        { 0x7, &Koi8::Op_SubYX },
        { 0xE, &Koi8::Op_ShiftLeft },
    };

    const std::unordered_map<uint8_t, instruction_func> func_F_table = {
        { 0x15, &Koi8::Op_SetDelayTimer },
        { 0x18, &Koi8::Op_SetSoundTimer },
        { 0x33, &Koi8::Op_BCD },
        { 0x55, &Koi8::Op_RegDump },
        { 0x65, &Koi8::Op_RegLoad },
    };

    void decode_0_opcode() {
        // Instructions start with 00E, then last nibble decides functionality
        uint8_t last_nibble = instruction & 0xF;
        (this->*(func_0_table.at(last_nibble)))();
    }

    void decode_8_opcode() {
        // Function changes based off instruction's last nibble
        uint8_t last_nibble = instruction & 0xF;
        (this->*(func_8_table.at(last_nibble)))();
    }

    void decode_F_opcode() {
        // Function changes based off instruction's last byte
        uint8_t last_byte = instruction & 0xFF;
        (this->*(func_F_table.at(last_byte)))();
    }

    void Initialize() {
        // Copy font into memory starting at 0x050
        std::ranges::copy(font_bytes, memory.begin() + 0x050);
        assert(memory[0x050] == font_bytes.front() && memory[0x050 + font_bytes.size()-1] == font_bytes.back());

        // Start program counter at 0x200
        pc = 0x200;
    }

    void Op_ClearScreen() {
        graphics_buffer.fill(0);
    }

    void Op_ReturnSub() {
        pc = stack.top();
        stack.pop();
    }

    void Op_Jump() {
        const uint16_t address = instruction & 0x0FFF;
        pc = address;
    }

    void Op_GoSubroutine() {
        const uint16_t address = instruction & 0x0FFF;
        stack.push(pc);
        pc = address;
    }

    void Op_SkipEqI() {
        uint8_t lhs = reg_v[instruction >> 8 & 0x0F];
        uint8_t rhs = instruction & 0x00FF;
        if (lhs == rhs) {
            pc += 2;
        }
    }

    void Op_SkipNeqI() {
        uint8_t lhs = reg_v[instruction >> 8 & 0x0F];
        uint8_t rhs = instruction & 0x00FF;
        if (lhs != rhs) {
            pc += 2;
        }
    }

    void Op_SkipEqR() {
        uint8_t lhs = reg_v[instruction >> 8 & 0x0F];
        uint8_t rhs = reg_v[instruction >> 4 & 0x0F];
        if (lhs == rhs) {
            pc += 2;
        }
    }

    void Op_SkipNeqR() {
        uint8_t lhs = reg_v[instruction >> 8 & 0x0F];
        uint8_t rhs = reg_v[instruction >> 4 & 0x0F];
        if (lhs != rhs) {
            pc += 2;
        }
    }

    void Op_SetRegI() {
        reg_v[instruction >> 8 & 0xF] = instruction & 0x00FF;
    }

    void Op_AddToRegI() {
        reg_v[instruction >> 8 & 0xF] += instruction & 0x00FF;
    }

    void Op_SetRegR() {
        reg_v[instruction >> 8 & 0xF] = reg_v[instruction >> 4 & 0xF];
    }

    void Op_BitOr() {
        reg_v[instruction >> 8 & 0xF] |= reg_v[instruction >> 4 & 0xF];
    }

    void Op_BitAnd() {
        reg_v[instruction >> 8 & 0xF] &= reg_v[instruction >> 4 & 0xF];
    }

    void Op_BitXor() {
        reg_v[instruction >> 8 & 0xF] ^= reg_v[instruction >> 4 & 0xF];
    }

    void Op_AddToRegR() {
        uint8_t *Vx = &reg_v[instruction >> 8 & 0xF];
        uint8_t Vy = reg_v[instruction >> 4 & 0xF];
        uint16_t sum = *Vx + Vy;
        *Vx = sum & 0xFF;

        if (sum > 0xFF) {
            reg_v[0xF] = 1;
        } else {
            reg_v[0xF] = 0;
        }
    }

    void Op_SubXY() {
        reg_v[instruction >> 8 & 0xF] -= reg_v[instruction >> 4 & 0xF];
    }

    void Op_SubYX() {
        reg_v[instruction >> 8 & 0xF] = reg_v[instruction >> 4 & 0xF] - reg_v[instruction >> 8 & 0xF];
    }

    void Op_ShiftLeft() {
        reg_v[0xF] = reg_v[instruction >> 8 & 0xF] >> 7; // Grab the most significant bit and store it in VF
        reg_v[instruction >> 8 & 0xF] <<= 1;
    }

    void Op_ShiftRight() {
        reg_v[0xF] = reg_v[instruction >> 8 & 0xF] & 0x1; // Grab the least significant bit and store it in VF
        reg_v[instruction >> 8 & 0x0F] >>= 1;
    }

    void Op_SetIdx() {
        idx_reg = instruction & 0x0FFF;
    }

    void Op_Draw() {
        uint8_t x = reg_v[instruction >> 8 & 0xF];
        uint8_t y = reg_v[instruction >> 4 & 0xF];
        const uint8_t n = instruction & 0xF;

        x %= 64;
        y %= 32;
        reg_v[0xF] = 0;

        for (int row = 0; row < n; row++) {
            uint8_t sprite_data = memory[idx_reg + row];

            for (int col = 0; col < 8; col++) {
                if (!(sprite_data & (0x80 >> col))) continue;

                uint32_t *pixel = &graphics_buffer[(y + row) * 64 + (x + col)];
                if (*pixel != 0) {
                    reg_v[0xF] = 1;
                }
                *pixel ^= 0xFFFFFFFF;
            }
        }

    }

    void Op_SetDelayTimer() {
        delay_timer = reg_v[instruction >> 8 & 0xF];
    }

    void Op_SetSoundTimer() {
        sound_timer = reg_v[instruction >> 8 & 0xF];
    }

    void Op_BCD() {
        uint8_t val = reg_v[instruction >> 8 & 0xF];
        for (int i = 2; i >= 0; i--, val /= 10) {
            memory[idx_reg + i] = val % 10;
        }
    }

    void Op_RegDump() {
        uint8_t last_reg = instruction >> 8 & 0xF;
        for (uint8_t i = 0; i <= last_reg; i++) {
            memory[idx_reg + i] = reg_v[i];
        }
    }

    void Op_RegLoad() {
        uint8_t last_reg = instruction >> 8 & 0xF;
        for (uint8_t i = 0; i <= last_reg; i++) {
            reg_v[i] = memory[idx_reg + i];
        }
    }
};


#endif //KOI8_HPP
