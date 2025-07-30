#include <SFML/Graphics.hpp>
#include "Koi8.hpp"

sf::RectangleShape onRect, offRect;
constexpr uint32_t window_width = 640;
constexpr uint32_t window_height = 320;
constexpr uint32_t tile_size = window_height / 32;
constexpr uint32_t tiles_x = window_width / tile_size;
constexpr uint32_t tiles_y = window_height / tile_size;

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

int main() {
    Koi8 koi8;
    koi8.LoadROM("../test-roms/IBM Logo.ch8");
    // koi8.LoadROM("../test-roms/test_opcode.ch8");

    sf::RenderWindow window(sf::VideoMode({window_width, window_height}), "Koi8");
    window.setFramerateLimit(60);

    onRect.setSize({tile_size, tile_size});
    onRect.setFillColor(sf::Color(0xbc89ffff));

    offRect.setSize({tile_size, tile_size});
    offRect.setFillColor(sf::Color(0x5e4580ff));

    while (window.isOpen()) {

        // ---------- SFML Handling ----------
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        // ---------- Koi8 emulation update ----------
        koi8.Update();

        // ---------- Draw to screen ----------
        window.clear();
        Draw(&window, koi8.GetGraphicsBuffer());
        window.display();
    }
}

