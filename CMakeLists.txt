# Project ControlMii (WiiIR)
# (C)2025 Dakota Thorpe, and Larsen Vallecillo.
# This software includes build scripts for these environments:
#   * devkitPro (wii-dev)
#   * mingw64 (x86_64-w64-mingw32)
# The reason that the software supports windows builds is to debug
# any GUI bugs that would take longer to debug on Wii. The other
# reason is because we can directly see the timing values, then
# demodulate the signal and identify if the signal is correct.
# This lets us know if it's a limitation of the Wii, or an invalid
# implementation of a code protocol.
#
# To build on platforms that aren't devkitpro, you still need the devkitpro
# toolchain for Wii installed, there are tools used by this cmake script
# that require use of the toolchain

# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name
project(WiiIR C CXX)
add_compile_definitions(GEKKO __wii__)
add_compile_definitions(DEBUG)

set(DEVKITPRO "C:/devkitPro")
set(DEVKITPPC "${DEVKITPRO}/devkitPPC")
set(LIBOGC "${DEVKITPRO}/libogc")

# Set C and C++ standards
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source directories
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/source")
set(CJSON_DIR "${CMAKE_SOURCE_DIR}/libCJSON")
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/ImGUI")
set(IMGUI_EXT_DIR "${CMAKE_SOURCE_DIR}/ImGUI_extensions")

# ImGUI Extras
set(IMGUI_FREETYPE_DIR "${IMGUI_DIR}/misc/freetype")
set(IMGUI_PATCH_DIR "${CMAKE_SOURCE_DIR}/ImGUI_RenderPatchWii")
set(IMGUI_IMPLOT_DIR "${IMGUI_EXT_DIR}/implot")
set(IMGUI_IMFDLG_DIR "${IMGUI_EXT_DIR}/ImGuiFileDialog")

# XML Parser
set(TINYXML2_DIR "${CMAKE_SOURCE_DIR}/TinyXML2")

# Main include directory
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")

# Collect sources
file(GLOB_RECURSE SOURCES
    "${SOURCE_DIR}/*.c"
    "${SOURCE_DIR}/*.cpp"
)

# Submodule sources
set(CJSON_SOURCES
    ${CJSON_DIR}/cJSON.c
    ${CJSON_DIR}/cJSON_Utils.c
)

set(TINYXML2_SOURCES
    ${TINYXML2_DIR}/tinyxml2.cpp
)

set(IMGUI_ENABLE_FREETYPE true)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_FREETYPE_DIR}/imgui_freetype.cpp

    # Renderer
    ${IMGUI_PATCH_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_PATCH_DIR}/backends/imgui_impl_sdlrenderer2.cpp

    # Extra Wii ImGUI wrappings
    ${IMGUI_PATCH_DIR}/Implementation.cpp

    # Extension: ImPlot
    # ${IMGUI_IMPLOT_DIR}/implot.cpp
    # ${IMGUI_IMPLOT_DIR}/implot_items.cpp
    # ${IMGUI_IMPLOT_DIR}/implot_demo.cpp

    # Extension: ImGuiFileDialog
    # ${IMGUI_IMFDLG_DIR}/ImGuiFileDialog.cpp
)

# Combine all sources
set(ALL_SOURCES
    ${SOURCES}
    ${CJSON_SOURCES}
    ${TINYXML2_SOURCES}
    ${IMGUI_SOURCES}
)

# Include directories
include_directories(
    ${INCLUDE_DIR}
    $ENV{DEVKITPRO}/libogc/include
    $ENV{DEVKITPRO}/portlibs/ppc/include
    $ENV{DEVKITPRO}/portlibs/ppc/include/freetype2
    $ENV{DEVKITPRO}/portlibs/wii/include
    ${CMAKE_BINARY_DIR}/data_objs

    # ImGUI
    ${IMGUI_DIR}
    ${IMGUI_FREETYPE_DIR}
    ${IMGUI_PATCH_DIR}/backends

    # ImGUI Extensions
    # ${IMGUI_IMPLOT_DIR}
    # ${IMGUI_IMFDLG_DIR}

    # cJSON
    ${CJSON_DIR}

    # TinyXML2
    ${TINYXML2_DIR}
)

# Built-in data generation
set(DATA_DIR ${CMAKE_SOURCE_DIR}/data)
set(GEN_OBJ_DIR ${CMAKE_BINARY_DIR}/data_objs)
file(MAKE_DIRECTORY ${GEN_OBJ_DIR})

# Valid data files
file(GLOB BINFILES
    "${DATA_DIR}/*.*"
)

# Embed the files into the software build
set(GENERATED_OBJECTS)
foreach(DATA_FILE ${BINFILES})
    get_filename_component(FILENAME ${DATA_FILE} NAME)
    string(REPLACE "." "_" SAFE ${FILENAME})
    string(REPLACE " " "_" SAFE ${SAFE})

    set(C_FILE   ${GEN_OBJ_DIR}/${SAFE}.c)
    set(H_FILE   ${GEN_OBJ_DIR}/${SAFE}.h)
    set(OBJ_FILE ${GEN_OBJ_DIR}/${SAFE}.o)

    add_custom_command(
        OUTPUT ${OBJ_FILE}
        COMMAND ${CMAKE_COMMAND} -E echo "Embedding binary: ${FILENAME}"
        COMMAND py ${CMAKE_SOURCE_DIR}/tools/bin2c.py
            ${DATA_FILE}
            ${C_FILE}
            ${H_FILE}
        COMMAND ${CMAKE_C_COMPILER} -c ${C_FILE} -o ${OBJ_FILE}
        DEPENDS ${DATA_FILE} ${CMAKE_SOURCE_DIR}/tools/bin2c.py
        BYPRODUCTS ${OBJ_FILE} ${C_FILE} ${H_FILE}
        VERBATIM
    )

    list(APPEND GENERATED_OBJECTS ${OBJ_FILE})
endforeach()

# Embed all data files before building app
add_custom_target(embed_all_data ALL DEPENDS ${GENERATED_OBJECTS})

# Define the executable
add_executable(${PROJECT_NAME} ${ALL_SOURCES} ${GENERATED_OBJECTS})

# Output directory (bin)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

# Libraries (Wii)
if("${CMAKE_SYSTEM_NAME}" STREQUAL "NintendoWii")
    add_compile_definitions(HW_RVL)
    include_directories(
        ${DEVKITPPC}/powerpc-eabi/include
        ${LIBOGC}/include
    )
    target_link_libraries(${PROJECT_NAME}
        z
        m
        ogc
        fat
    )
endif()

# SDL2 & Freetype
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(CMAKE_LINK_SEARCH_START_STATIC TRUE)
set(CMAKE_LINK_SEARCH_END_STATIC TRUE)

# Find the necessary FFmpeg components
# TODO: Fix FFMPeg Build Wii
# find_package(PkgConfig REQUIRED)
# pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET libavcodec libavformat libavutil libswresample libswscale)

# Find the main required libraries.
find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(Freetype REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}
    SDL2::SDL2main
    SDL2::SDL2-static
    SDL2_mixer::SDL2_mixer-static
    Freetype::Freetype
    # PkgConfig::LIBAV
)

# Libraries (Win32)
if(WIN32)
    target_link_libraries(${PROJECT_NAME}
        # SDL2_mixer dependencies (Audio)
        ogg
        vorbis
        vorbisfile
        flac
        mpg123
        opus
        opusfile

        # Freetype & graphics deps
        brotlidec
        brotlicommon
        harfbuzz
        png
        bz2
        graphite2
        z

        # Standard libs
        rpcrt4
        dwrite
        m
    )

    # When building for Windows, build everything as one app.
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
endif()

# Build info
string(TIMESTAMP BUILD_DATE "%Y-%m-%d %H:%M:%S")

# Getting the hostname is different on windows.
if(WIN32)
    execute_process(COMMAND hostname OUTPUT_VARIABLE BUILD_HOST_RAW OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    execute_process(COMMAND uname -n OUTPUT_VARIABLE BUILD_HOST_RAW OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

set(BUILD_HOST "${BUILD_HOST_RAW}")
set(BUILD_TARG "Unknown")

if("${CMAKE_SYSTEM_NAME}" STREQUAL "NintendoWii")
    set(BUILD_TARG "Wii")
    add_compile_definitions(NINTENDOWII)
elseif(WIN32)
    set(BUILD_TARG "Windows32")
endif()

# Build information
add_compile_definitions(
    BUILD_DATE="${BUILD_DATE}"
    BUILD_HOST="${BUILD_HOST}"
    BUILD_TARG="${BUILD_TARG}"
)

# Info messages
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Compiler Information:")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  System: ${CMAKE_SYSTEM_NAME} / ${CMAKE_SYSTEM_PROCESSOR}")
