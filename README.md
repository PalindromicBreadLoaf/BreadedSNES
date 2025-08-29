# BreadedSNES

**BreadedSNES** is an up-and-coming cross-platform Super Nintendo (SNES) emulator written in modern C++23. SDL2 is used for audio, video, and input support.

Note, this does not function currently. Maybe it'll boot something in the future. Now is certainly not that time.
## Build Instructions

### Prerequisites

You will need:

- CMake ≥ 3.16
- C++23-compatible compiler (GCC, Clang, or MSVC)
- SDL2 development libraries

---

### Windows

1. Clone the repository and vcpkg:

    ```bash
    git clone https://github.com/microsoft/vcpkg.git
    cd vcpkg
    ./bootstrap-vcpkg.bat
    ```

2. Install SDL2:

    ```bash
    ./vcpkg install sdl2
    ```

3. Build the project:

    ```bash
    cd path/to/breadedSNES
    cmake -B build -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
    ```

---

###  macOS

1. Install dependencies:

    ```bash
    brew install cmake sdl2
    ```

2. Build the project:

    ```bash
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
    ```

---

### 🐧 Linux

1. Install dependencies:

   >apt:
   > ```bash
    >sudo apt install cmake build-essential libsdl2-dev pkg-config
    >```

   > pacman:
   > ```bash
    > sudo pacman -S cmake sdl2 pkgconf
    > ```

2. Build the project:

    ```bash
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
    ```

---

### Packaging

To create distributable packages (e.g. `.zip`, `.dmg`, `.tgz`), run:

```bash
cd build
cpack
