#include <SFML/Graphics.hpp>
#include "Koi8.hpp"

int main() {
    Koi8 koi8;
    // koi8.LoadROM("../test-roms/IBM Logo.ch8");
    koi8.LoadROM("../test-roms/test_opcode.ch8");
    koi8.Run();
}

