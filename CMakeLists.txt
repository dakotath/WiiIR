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

set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/source")
set(CJSON_DIR "${CMAKE_SOURCE_DIR}/libCJSON")
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/ImGUI")
set(IMGUI_FREETYPE_DIR "${CMAKE_SOURCE_DIR}/ImGUI/misc/freetype")
set(IMGUI_PATCH_DIR "${CMAKE_SOURCE_DIR}/ImGUI_RenderPatchWii")
set(TINYXML2_DIR "${CMAKE_SOURCE_DIR}/TinyXML2")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")

# Only recurse your own source
file(GLOB_RECURSE SOURCES
    "${SOURCE_DIR}/*.c"
    "${SOURCE_DIR}/*.cpp"
)

# Submodule sources — manually listed
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

    # Freetype2
    ${IMGUI_FREETYPE_DIR}/imgui_freetype.cpp

    # Custom render patch:
    ${IMGUI_PATCH_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_PATCH_DIR}/backends/imgui_impl_sdlrenderer2.cpp
)

# Combine everything
set(ALL_SOURCES
    ${SOURCES}
    ${CJSON_SOURCES}
    ${TINYXML2_SOURCES}
    ${IMGUI_SOURCES}
)

# Add include directories
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

    # cJSON Includes
    ${CJSON_DIR}

    # TinyXML2
    ${TINYXML2_DIR}
)

#---------------------------------------------------------------------------------
# Locate devkitPro + toolchain tools
#---------------------------------------------------------------------------------
if(NOT DEFINED ENV{DEVKITPRO})
    message(FATAL_ERROR "DEVKITPRO environment variable not set. Please run 'source /opt/devkitpro/devkit-env.sh'")
endif()

set(DEVKITPRO $ENV{DEVKITPRO})

# Find the Wii toolchain tools
find_program(BIN2S_EXECUTABLE bin2s HINTS "$ENV{DEVKITPRO}/tools/bin")
find_program(PPC_CC powerpc-eabi-gcc HINTS "$ENV{DEVKITPPC}/bin")

if(NOT BIN2S_EXECUTABLE)
    message(FATAL_ERROR "bin2s not found in $ENV{DEVKITPRO}/tools/bin — install with 'dkp-pacman -S devkitpro-pkgbuild-helpers'")
endif()

if(NOT PPC_CC)
    message(FATAL_ERROR "powerpc-eabi-gcc not found — make sure devkitPPC is installed and in PATH")
endif()

#---------------------------------------------------------------------------------
# Convert binary data to object files (.o)
#---------------------------------------------------------------------------------
# Path to data directory
set(DATA_DIR ${CMAKE_SOURCE_DIR}/data)
set(GEN_OBJ_DIR ${CMAKE_BINARY_DIR}/data_objs)
file(MAKE_DIRECTORY ${GEN_OBJ_DIR})

# List all files in data/ (can be filtered by extensions)
file(GLOB BINFILES
    "${DATA_DIR}/*.ogg"
    "${DATA_DIR}/*.txt"
    "${DATA_DIR}/*.tar"
    "${DATA_DIR}/*.ttf"
    "${DATA_DIR}/*.lang"
)

set(GENERATED_OBJECTS)

foreach(DATA_FILE ${BINFILES})
    get_filename_component(FILENAME ${DATA_FILE} NAME)
    string(REPLACE "." "_" SYMBOL ${FILENAME})
    set(ASM_FILE ${GEN_OBJ_DIR}/${FILENAME}.s)
    set(HEADER_FILE ${GEN_OBJ_DIR}/${SYMBOL}.h)
    set(OBJ_FILE ${GEN_OBJ_DIR}/${FILENAME}.o)

    add_custom_command(
        OUTPUT ${OBJ_FILE}
        COMMAND ${CMAKE_COMMAND} -E echo "Embedding binary data: ${FILENAME}"
        COMMAND ${BIN2S_EXECUTABLE} -a 32 -H ${HEADER_FILE} ${DATA_FILE} > ${ASM_FILE}
        COMMAND ${PPC_CC} -x assembler-with-cpp -c ${ASM_FILE} -o ${OBJ_FILE}
        # Make the command ALWAYS run
        BYPRODUCTS ${OBJ_FILE}
        COMMENT "Generating object for ${FILENAME}"
        VERBATIM
    )

    list(APPEND GENERATED_OBJECTS ${OBJ_FILE})
endforeach()

# Wrap it in a phony target that always runs
add_custom_target(embed_all_data ALL DEPENDS ${GENERATED_OBJECTS})

# Define the executable output
add_executable(${PROJECT_NAME} ${ALL_SOURCES} ${GENERATED_OBJECTS})

# (Optional) If you want to place binaries in a specific folder
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

# Explicitly add libogc's Wii library folder
link_directories(
    $ENV{DEVKITPRO}/libogc/lib/wii
    $ENV{DEVKITPRO}/portlibs/wii/lib
    $ENV{DEVKITPRO}/portlibs/ppc/lib
)

# Find SDL2 and Freetype
find_package(SDL2 REQUIRED)
find_package(Freetype REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${FREETYPE_INCLUDE_DIRS})

# Link SDL2
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES} Freetype::Freetype)

# libcURL
target_link_libraries(${PROJECT_NAME}
    # System
    ogc     # core Wii/LibOGC functions
    fat     # SD/USB File handlers
    m       # Math
    z       # Compression
)

# Get build date
string(TIMESTAMP BUILD_DATE "%Y-%m-%d %H:%M:%S")

# Get hostname
# Works on Windows, macOS, Linux
if (WIN32)
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

# Export to compiler definitions
add_compile_definitions(
    BUILD_DATE="${BUILD_DATE}"
    BUILD_HOST="${BUILD_HOST}"
    BUILD_TARG="${BUILD_TARG}"
)

# Print some info to confirm configuration
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Source files: ${SOURCES}")
message(STATUS "Include path: ${INCLUDE_DIR}")
message(STATUS "====================================")
message(STATUS "Compiler Information:")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  C Compiler ID: ${CMAKE_C_COMPILER_ID}")
message(STATUS "  C Compiler Version: ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  C++ Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  C++ Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  System Processor: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  System Name: ${CMAKE_SYSTEM_NAME}")
message(STATUS "====================================")