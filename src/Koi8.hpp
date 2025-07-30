#ifndef KOI8_HPP
#define KOI8_HPP

#include <iostream>
#include <cstdint>
#include <cassert>
#include <array>
#include <stack>
#include <chrono>
#include <fstream>
#include <string>

sf::RectangleShape onRect, offRect;
constexpr uint32_t window_width = 640;
constexpr uint32_t window_height = 320;
constexpr uint32_t tile_size = window_height / 32;
constexpr uint32_t tiles_x = window_width / tile_size;
constexpr uint32_t tiles_y = window_height / tile_size;

// sf::RenderWindow window;

void Draw(sf::RenderWindow *window, const uint32_t *graphics_buffer) {
    for (int x = 0; x < tiles_x; x++) {
        for (int y = 0; y < tiles_y; y++) {
            if (graphics_buffer[x + y * 64] == 0) {
                offRect.setPosition({static_cast<float>(x * tile_size), static_cast<float>(y * tile_size)});
                window->draw(offRect);
            } else {
                onRect.setPosition({static_cast<float>(x * tile_size), static_cast<float>(y * tile_size)});
                window->draw(onRect);
            }
        }
    }
}

enum class Reg {
    V0 = 0, V1, V2, V3,
    V4, V5, V6, V7,
    V8, V9, VA, VB,
    VC, VD, VE, VF
};

enum class Opcodes : uint16_t {
    ClearDisplay = 0x00E0,
};

class Koi8 {
public:
    Koi8() : window(sf::RenderWindow(sf::VideoMode({window_width, window_height}), "CMake SFML Project")), pc(0x200),
             should_close(false) {
        window.setFramerateLimit(60);

        onRect.setSize({tile_size, tile_size});
        onRect.setFillColor(sf::Color(0xbc89ffff));
        // onRect.setFillColor(sf::Color(188, 137, 255));

        offRect.setSize({tile_size, tile_size});
        offRect.setFillColor(sf::Color(0x5e4580ff));
        // offRect.setFillColor(sf::Color(94, 69, 128));

        Initialize();
        std::cout << "Initialized" << std::endl;
    }

    ~Koi8() = default;

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

    void Update() {
        // Decrement timers every second
        // if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - last_dec_time) >= std::chrono::milliseconds(1000)) {
        //     last_dec_time = std::chrono::high_resolution_clock::now();
        //     sound_timer--;
        //     delay_timer--;
        //     std::cout << "dec" << std::endl;
        // }

        // Fetch
        instruction = memory[pc] << 8 | memory[pc + 1];
        if (instruction == 0) return;
        opcode = instruction >> 12 & 0xF;

        if (instruction != 0) {
            std::cout << "Instruction: " << std::hex << instruction << std::endl;
            pc += 2;
        }

        // Decode
        switch (opcode) {
            case 0x0: {
                switch (instruction) {
                    case 0x00E0: // Clear screen
                        Op_ClearScreen();
                        break;
                    case 0x00EE: // Return subroutine
                        Op_ReturnSub();
                        break;
                }
                break;
            }

            case 0x1:
                Op_Jump();
                break;

            case 0x2:
                Op_GoSubroutine();
                break;

            case 0x3:
                Op_SkipEqI();
                break;

            case 0x4:
                Op_SkipNeqI();
                break;

            case 0x5:
                Op_SkipEqR();
                break;

            case 0x6:
                Op_SetRegI();
                break;

            case 0x7:
                Op_AddToRegI();
                break;

            case 0x8: {
                switch (uint8_t last_nibble = instruction & 0xF) {
                    case 0x0:
                        Op_SetRegR();
                        break;

                    case 0x1:
                        Op_BitOr();
                        break;

                    case 0x2:
                        Op_BitAnd();
                        break;

                    case 0x3:
                        Op_BitXor();
                        break;

                    case 0x4:
                        Op_AddToRegR();
                        break;

                    case 0x5:
                        Op_SubRegs(0);
                        break;

                    case 0x6:
                        Op_ShiftRight();
                        break;

                    case 0x7:
                        Op_SubRegs(1);
                        break;

                    case 0xE:
                        Op_ShiftLeft();
                        break;
                }
                break;
            }


            case 0xA: {
                idx_reg = instruction & 0x0FFF;
                break;
            }

            // Display
            case 0xD:
                Op_Draw();
                break;

            default:
                break;
        }
    }

    void Run() {
        while (window.isOpen()) {
            while (const std::optional event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window.close();
                }
            }

            Update();

            window.clear();
            Draw(&window, graphics_buffer.data());
            window.display();
        }
    }

    const uint32_t *GetGraphicsBuffer() const { return graphics_buffer.data(); }

private:
    sf::RenderWindow window;
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
    bool should_close;
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


    void Initialize() {
        // Copy font into memory starting at 0x050
        std::ranges::copy(font_bytes, memory.begin() + 0x050);
        assert(memory[0x050] == font_bytes.front() && memory[0x050 + font_bytes.size()-1] == font_bytes.back());

        // for (int i = 0; i < graphics_buffer.size(); i++) {
        //     graphics_buffer[i] = i % 2 == 0 ? 1 : 0;
        // }
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

    // dir = 0: Vx - Vy
    // dir = 1: Vy - Vx
    void Op_SubRegs(uint8_t dir) {
        uint8_t *Vx = &reg_v[instruction >> 8 & 0xF];
        uint8_t *Vy = &reg_v[instruction >> 4 & 0xF];
        uint8_t diff = dir ? *Vy - *Vx : *Vx - *Vy;
        reg_v[0xF] = dir ? *Vy > *Vx : *Vx > *Vy;
        *Vx = diff;
    }

    void Op_ShiftLeft() {
        reg_v[0xF] = reg_v[instruction >> 8 & 0xF] >> 7; // Grab the most significant bit and store it in VF
        reg_v[instruction >> 8 & 0xF] <<= 1;
    }

    void Op_ShiftRight() {
        reg_v[0xF] = reg_v[instruction >> 8 & 0xF] & 0x1; // Grab the least significant bit and store it in VF
        reg_v[instruction >> 8 & 0x0F] >>= 1;
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
};


#endif //KOI8_HPP
