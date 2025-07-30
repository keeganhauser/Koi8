cmake_minimum_required(VERSION 3.31)
project(koi8)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)
FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.0.1
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
FetchContent_MakeAvailable(SFML)

add_executable(koi8)
add_subdirectory(src)
target_link_libraries(koi8 PRIVATE SFML::Graphics)
