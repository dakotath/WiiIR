# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name
project(WiiIR C CXX)
add_compile_definitions(GEKKO __wii__)
add_compile_definitions(DEBUG)

# Set C and C++ standards
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source directories
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/source")
set(CJSON_DIR "${CMAKE_SOURCE_DIR}/libCJSON")
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/ImGUI")
set(IMGUI_FREETYPE_DIR "${IMGUI_DIR}/misc/freetype")
set(IMGUI_PATCH_DIR "${CMAKE_SOURCE_DIR}/ImGUI_RenderPatchWii")
set(TINYXML2_DIR "${CMAKE_SOURCE_DIR}/TinyXML2")
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
    ${IMGUI_FREETYPE_DIR}/imgui_freetype.cpp
    ${IMGUI_PATCH_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_PATCH_DIR}/backends/imgui_impl_sdlrenderer2.cpp
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

    # cJSON
    ${CJSON_DIR}

    # TinyXML2
    ${TINYXML2_DIR}
)

#---------------------------------------------------------------------------------
# DevkitPro + toolchain detection (for Wii builds)
#---------------------------------------------------------------------------------
if(NOT DEFINED ENV{DEVKITPRO})
    message(FATAL_ERROR "DEVKITPRO environment variable not set. Please run 'source /opt/devkitpro/devkit-env.sh'")
endif()

set(DEVKITPRO $ENV{DEVKITPRO})

# Find Wii toolchain tools
find_program(BIN2S_EXECUTABLE bin2s HINTS "$ENV{DEVKITPRO}/tools/bin")
find_program(PPC_CC powerpc-eabi-gcc HINTS "$ENV{DEVKITPPC}/bin")

if(NOT BIN2S_EXECUTABLE)
    message(FATAL_ERROR "bin2s not found — install with 'dkp-pacman -S devkitpro-pkgbuild-helpers'")
endif()

if(NOT PPC_CC)
    message(FATAL_ERROR "powerpc-eabi-gcc not found — make sure devkitPPC is installed and in PATH")
endif()

set(DATA_DIR ${CMAKE_SOURCE_DIR}/data)
set(GEN_OBJ_DIR ${CMAKE_BINARY_DIR}/data_objs)
file(MAKE_DIRECTORY ${GEN_OBJ_DIR})

file(GLOB BINFILES
    "${DATA_DIR}/*.*"
)

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
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/bin2c.py
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

add_custom_target(embed_all_data ALL DEPENDS ${GENERATED_OBJECTS})

# Define the executable
add_executable(${PROJECT_NAME} ${ALL_SOURCES} ${GENERATED_OBJECTS})

# Output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

# Libraries (Wii)
if("${CMAKE_SYSTEM_NAME}" STREQUAL "NintendoWii")
    target_link_libraries(${PROJECT_NAME}
        ogc
        fat
        m
        z
    )
endif()

# SDL2 & Freetype
find_package(SDL2 REQUIRED)
find_package(Freetype REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES} Freetype::Freetype)

# Build info
string(TIMESTAMP BUILD_DATE "%Y-%m-%d %H:%M:%S")

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
endif()

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
