// Basic Includes.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <ctype.h>
#include <time.h>
#include "WiiIR/IR.hpp"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include <SDL.h>
#ifdef _WIN32
#include <windows.h>        // SetProcessDPIAware()
#endif

float main_scale;
SDL_Window* window;
SDL_Renderer* renderer;

void ShutdownUI()
{
    // 1. ImGui renderer backend
    ImGui_ImplSDLRenderer2_Shutdown();

    // 2. ImGui SDL input backend
    ImGui_ImplSDL2_Shutdown();

    // 3. Destroy ImGui context
    ImGui::DestroyContext();     

    // 4. Destroy SDL renderer
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }

    // 5. Destroy SDL window
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    // 6. Shutdown SDL subsystems
    SDL_Quit();   // fully tears down Wii SDL (video, event loop, etc.)

    // 7. Shutdown GX manually
    // VERY IMPORTANT on Wii or FIFO/interrupts remain active.
    #ifdef NINTENDOWII
    VIDEO_SetBlack(TRUE);
    VIDEO_Flush();
    VIDEO_WaitVSync();

    // This resets FIFO, interrupts, and VI scheduling:
    GX_AbortFrame();
    GX_Flush();
    GX_DrawDone();
    #endif
}

void StartUI() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return;
    }

    main_scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);

    window = SDL_CreateWindow("Dear ImGui SDL2+SDL_Renderer example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int)(640 * main_scale), (int)(480 * main_scale), 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);
}

// Initializer.
void Init() {
	// Initialise the video system
    #ifdef NINTENDOWII
	VIDEO_Init();

	// This function initialises the attached controllers
	WPAD_Init();

	rmode = VIDEO_GetPreferredMode(NULL);
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	console_init(xfb,20,20,rmode->fbWidth-20,rmode->xfbHeight-20,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	//SYS_STDIO_Report(true);
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_ClearFrameBuffer(rmode, xfb, COLOR_BLACK);
	VIDEO_SetBlack(false);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

    // SD Card
    fatInitDefault();
    #endif

    // Welcome
    SetColors(BLACK, WHITE);   // Default color mode.
    ClearScreen();             // Clear Screen.
    SetCursorApp(0,2);            // Reset Cursor Position.
}

// Deinitializer.
void Deinit() {
    // --- SD / FAT cleanup ---
    //fatUnmount("sd:/");   // Unmount SD if mounted
    //fatUnmount("usb:/");  // In case USB was used

    // --- Controllers ---
    // Aparently SDL2 forgets how to restart it so forget this.
    //WPAD_Shutdown();      // Stop Wii Remote system

    // This resets FIFO, interrupts, and VI scheduling:
    #ifdef NINTENDOWII
    GX_AbortFrame();
    GX_Flush();
    GX_DrawDone();

    // 7. Shutdown GX manually
    // VERY IMPORTANT on Wii or FIFO/interrupts remain active.
    VIDEO_SetBlack(TRUE);
    VIDEO_SetNextFramebuffer(NULL);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    
    // Free the XFB allocation
    if (xfb) {
        free(MEM_K1_TO_K0(xfb));
        xfb = NULL;
    }
    #endif
}

// Function to set the terminal cursor position at X,Y cordinates.
void SetCursorApp(int x, int y) {
    printf("\x1b[%d;%dH", y,x);
}

// Function to set background and foreground color on console.
void SetColors(Color background, Color foreground) {
    printf("\x1b[%dm", 40 + background); // BG
    printf("\x1b[%dm", 30 + foreground); // FG
}

// Function to clear the screen.
void ClearScreen() {
    printf("\x1b[2J");
}

// Critical Error Display.
void StopCritical(const char* msg, const char* running_func, const char* occuring_file, int lcall, uint32_t code) {
    // Clear the display with blue.
    SetColors(BLUE, WHITE);
    ClearScreen();
    SetCursorApp(0,2);

    // Error
    printf("DEMO ERROR 0x%04x:\n\n", code);
    SetColors(BLACK, YELLOW);
    printf(msg);
    SetColors(BLUE, WHITE);
    printf("\n*DONT PANIC* If you see this, its nothing\nharmful, this in-fact is a safety measure taken to\nprevent damage to your console/emulator.\n");
    printf("\nPress HOME or RESET to exit.\n\n");

    // Print Extra Info (Useful for debugging).
    printf("This error occured in '%s' (Called from L%d by '%s()')\nThis demo was built '%s %s', refer to source for debugging.\nBuilt by 'unknown'\n",
        occuring_file,
        lcall,
        running_func,
        __DATE__, __TIME__
    );

    // Critial Loop.
    #ifdef NINTENDOWII
    while(true) {
        // Scan WPAD
        WPAD_ScanPads();
        s32 pressed = WPAD_ButtonsDown(WPAD_CHAN_0); // State on every remote.
        
        // Check for home on each remote, and check reset button.
        if(pressed & WPAD_BUTTON_HOME || SYS_ResetButtonDown()) break;
        
        // Wait VSync.
        VIDEO_WaitVSync();
    }
    #endif

    // Notify that the system is exiting.
    printf("\nEXITING...\n");
    exit(code);
}