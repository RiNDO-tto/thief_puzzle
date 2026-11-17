cmake_minimum_required(VERSION 3.16)
project(ThiefPuzzle LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(SFML 3 REQUIRED COMPONENTS Graphics)
add_executable(thief_puzzle
    src/main.cpp
    src/Game.cpp
    src/Renderer.cpp
    src/Menu.cpp
    src/Solver.cpp
)
target_link_libraries(thief_puzzle PRIVATE SFML::Graphics)

# Optional developer executable. It uses exactly the same crate-aware Game/Solver.
option(THIEF_PUZZLE_BUILD_DEV "Build developer-mode GUI" ON)
if(THIEF_PUZZLE_BUILD_DEV)
    add_executable(thief_puzzle_dev
        src/main.cpp src/Game.cpp src/Renderer.cpp src/Menu.cpp src/Solver.cpp
    )
    target_compile_definitions(thief_puzzle_dev PRIVATE THIEF_PUZZLE_DEV)
    target_link_libraries(thief_puzzle_dev PRIVATE SFML::Graphics)
endif()
