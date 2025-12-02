# Building WiiIR

## Requirements

Install the following packages using **(dkp-)pacman**:

```bash
sudo dkp-pacman -S wii-dev wii-cmake wii-sdl2 wii-sdl2_gfx wii-sdl2_image wii-sdl2_mixer wii-sdl2_ttf
```

### Cloning the Repository

To clone the whole project, you need to do more than just a git clone, this project uses 
submodules, so you gotta grab them too.

```sh
git clone https://github.com/dakotath/WiiIR
cd WiiIR
git submodule update --init --recursive
```

### Building the source
```bash
mkdir build && cd build
/opt/devkitpro/portlibs/wii/bin/powerpc-eabi-cmake -S .. -B .
make -j6
```

***Note:*** After running cmake, your output should have been similar to this, if they were not, then you may expirence errors while trying to build it.
```cmake
-- ====================================
-- Compiler Information:
--   C Compiler: /opt/devkitpro/devkitPPC/bin/powerpc-eabi-gcc.exe
--   C++ Compiler: /opt/devkitpro/devkitPPC/bin/powerpc-eabi-g++.exe
--   C Compiler ID: GNU
--   C Compiler Version: 15.1.0
--   C++ Compiler ID: GNU
--   C++ Compiler Version: 15.1.0
--   System Processor: ppc
--   System Name: NintendoWii
-- ====================================
-- Configuring done (1.8s)
-- Generating done (0.8s)
```