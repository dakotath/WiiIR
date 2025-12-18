// Basic Includes.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <ctype.h>
#include <time.h>
#include <fstream>
#include "WiiIR/IR.hpp"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "implot.h"
#include "implot_internal.h"
#include "stb/stb_image.h"
#include "stb/stb_perlin.h"

// BGMusic
#include "music_ogg.h"

// Test JPG
#include "wiiir_logo_png.h"

#include <SDL.h>
#ifdef _WIN32
#include <windows.h>        // SetProcessDPIAware()
#endif

float main_scale;
SDL_Window* window;
SDL_Renderer* renderer;

SDL_Texture* GeneratePS3Background(SDL_Renderer* renderer, int width, int height, float frame, int mode)
{
    SDL_Texture* texture = SDL_CreateTexture(renderer,
                                             SDL_PIXELFORMAT_RGBA32,
                                             SDL_TEXTUREACCESS_STATIC,
                                             width,
                                             height);
    if (!texture)
        return nullptr;

    unsigned char* pixels = new unsigned char[width * height * 4];

    // Base Perlin scales
    float scaleX = 0.01f;
    float scaleY = 0.02f;

    // Animation speed
    float t = frame * 0.01f;     // smooth scrolling
    float ripple = sinf(frame * 0.02f); // PS3 wave effect

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            // Animated Perlin sampling coordinates (PS3-style scroll)
            float nx = (x * scaleX) + t;
            float ny = (y * scaleY) + t * 0.5f;

            // Add a ripple wave distortion like PS3 background
            float wave = sinf((y * 0.03f) + t * 2.0f) * 0.3f;
            nx += wave;

            // Layered Perlin noise for richer waves
            float n1 = stb_perlin_noise3(nx, ny, t, 0,0,0);
            float n2 = stb_perlin_noise3(nx * 0.6f, ny * 0.6f, t * 0.5f, 0,0,0);
            float n = n1 * 0.7f + n2 * 0.3f; // combine layers

            // Normalize to [0,1]
            n = (n + 1.0f) * 0.5f;

            // PS3-style color gradient (blue → violet)
            float brightness = n * 0.9f + 0.1f;

            // Red Mode
            unsigned char r, g, b;
            if(mode == 0) {         // Red
                r = (unsigned char)(brightness * 255);
                g = (unsigned char)(brightness * 120);
                b = (unsigned char)(brightness * 90);
            } else if(mode == 1) {  // Green
                r = (unsigned char)(brightness * 90);
                g = (unsigned char)(brightness * 255);
                b = (unsigned char)(brightness * 120);
            } else if(mode == 2) {  // Blue
                r = (unsigned char)(brightness * 90);
                g = (unsigned char)(brightness * 120);
                b = (unsigned char)(brightness * 255);
            }

            // Alpha always 255
            unsigned char a = 255;

            int idx = (y * width + x) * 4;
            pixels[idx + 0] = r;
            pixels[idx + 1] = g;
            pixels[idx + 2] = b;
            pixels[idx + 3] = a;
        }
    }

    SDL_UpdateTexture(texture, nullptr, pixels, width * 4);
    delete[] pixels;

    return texture;
}

// Load texture from memory (JPEG/PNG/etc.) array
SDL_Texture* LoadTextureFromArray(SDL_Renderer* renderer, const unsigned char* data, size_t size, int* outWidth = nullptr, int* outHeight = nullptr)
{
    if (!data || size == 0)
        return nullptr;

    int width, height, channels;
    // Decode image from memory, force RGBA
    unsigned char* pixels = stbi_load_from_memory(data, (int)size, &width, &height, &channels, 4);
    if (!pixels)
    {
        SDL_Log("Failed to load image from memory");
        return nullptr;
    }

    #ifdef NINTENDOWII
    // Flip the image vertically
    int rowSize = width * 4; // 4 bytes per pixel (RGBA)
    unsigned char* rowBuffer = new unsigned char[rowSize];
    for (int y = 0; y < height / 2; y++)
    {
        unsigned char* rowTop = pixels + y * rowSize;
        unsigned char* rowBottom = pixels + (height - 1 - y) * rowSize;
        memcpy(rowBuffer, rowTop, rowSize);
        memcpy(rowTop, rowBottom, rowSize);
        memcpy(rowBottom, rowBuffer, rowSize);
    }
    delete[] rowBuffer;
    #endif

    SDL_Texture* texture = SDL_CreateTexture(renderer,
                                             SDL_PIXELFORMAT_RGBA32,
                                             SDL_TEXTUREACCESS_STATIC,
                                             width,
                                             height);
    if (!texture)
    {
        SDL_Log("Failed to create SDL texture: %s", SDL_GetError());
        stbi_image_free(pixels);
        return nullptr;
    }

    SDL_UpdateTexture(texture, nullptr, pixels, width * 4);
    stbi_image_free(pixels);

    if (outWidth) *outWidth = width;
    if (outHeight) *outHeight = height;

    return texture;
}

// Load an image from a file path and return an SDL_Texture* ready to render
SDL_Texture* LoadTextureFromFile(SDL_Renderer* renderer, const char* filePath, int* outWidth, int* outHeight)
{
    int width, height, channels;
    unsigned char* pixels = stbi_load(filePath, &width, &height, &channels, 4); // Force 4 channels RGBA
    if (!pixels)
    {
        SDL_Log("Failed to load image %s", filePath);
        return nullptr;
    }

    #ifdef NINTENDOWII
    // Flip the image vertically
    int rowSize = width * 4; // 4 bytes per pixel (RGBA)
    unsigned char* rowBuffer = new unsigned char[rowSize];
    for (int y = 0; y < height / 2; y++)
    {
        unsigned char* rowTop = pixels + y * rowSize;
        unsigned char* rowBottom = pixels + (height - 1 - y) * rowSize;
        memcpy(rowBuffer, rowTop, rowSize);
        memcpy(rowTop, rowBottom, rowSize);
        memcpy(rowBottom, rowBuffer, rowSize);
    }
    delete[] rowBuffer;
    #endif

    SDL_Texture* texture = SDL_CreateTexture(renderer,
                                             SDL_PIXELFORMAT_RGBA32,
                                             SDL_TEXTUREACCESS_STATIC,
                                             width,
                                             height);
    if (!texture)
    {
        SDL_Log("Failed to create SDL texture: %s", SDL_GetError());
        stbi_image_free(pixels);
        return nullptr;
    }

    SDL_UpdateTexture(texture, nullptr, pixels, width * 4);
    stbi_image_free(pixels);

    if (outWidth) *outWidth = width;
    if (outHeight) *outHeight = height;

    return texture;
}

void FreeTexture(SDL_Texture* tex)
{
    if (tex)
        SDL_DestroyTexture(tex);
}

void ShutdownUI()
{
    // 1. ImGui renderer backend
    ImGui_ImplSDLRenderer2_Shutdown();

    // 2. ImGui SDL input backend
    ImGui_ImplSDL2_Shutdown();

    // 3. Destroy ImGui context
    ImPlot::DestroyContext();
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

bool doStorageSelection() {
    bool running = true;
    bool result = false;   // false = SD, true = USB
    bool demoPlot = true;

    // BG Background
    int texW, texH;
    float frame;

    // WiiIR Logo
    SDL_Texture* wiiIRLogo = LoadTextureFromArray(renderer, wiiir_logo_png, wiiir_logo_png_size, &texW, &texH);

    if (!wiiIRLogo)
        SDL_Log("Failed to load WiiIR Logo embedded texture!");

    while (running) {
        // --- Event Handling ---
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT) {
                running = false;
                break;
            }
        }

        // --- Begin ImGui Frame ---
        ImGui_ImplSDL2_NewFrame();
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui::NewFrame();

        ImPlot::ShowDemoWindow(&demoPlot);

        // --- Build UI ---
        ImGui::SetNextWindowSize(ImVec2(300, 150), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2( (640-300)/2, (480-150)/2 ), ImGuiCond_Always);

        ImGui::Begin("Select Storage", nullptr,
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoCollapse);

        // Render in ImGui
        ImGui::Image((ImTextureID)wiiIRLogo, ImVec2((float)texW, (float)texH));

        ImGui::Text("Choose your storage device:");
        ImGui::Spacing();

        if (ImGui::Button("Use SD Card", ImVec2(120, 40))) {
            result = false;
            running = false;
        }

        ImGui::SameLine();

        if (ImGui::Button("Use USB", ImVec2(120, 40))) {
            result = true;
            running = false;
        }

        ImGui::End();

        // --- Rendering ---
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Increment BG frame
        frame += 1.0f;

        // Generate frame, then render it.
        SDL_Texture* perlinTexture = GeneratePS3Background(renderer, 64, 128, frame, 1);
        SDL_RenderCopy(renderer, perlinTexture, nullptr, nullptr);
        FreeTexture(perlinTexture);

        // Render ImUGI
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    return result;
}

void StartUI() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return;
    }

    // Initialize SDL_mixer with OGG support
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer could not initialize! %s\n", Mix_GetError());
        return;
    }

    // Create an SDL_RWops from memory
    SDL_RWops* rw = SDL_RWFromMem((void*)music_ogg, music_ogg_size);
    if (!rw) {
        printf("SDL_RWFromMem Error: %s\n", SDL_GetError());
        Mix_CloseAudio();
        SDL_Quit();
        return;
    }

    // Load music from RWops
    Mix_Music* music = Mix_LoadMUS_RW(rw, 1); // 1 = SDL_mixer will free the RWops
    if (!music) {
        printf("Mix_LoadMUS_RW Error: %s\n", Mix_GetError());
        Mix_CloseAudio();
        SDL_Quit();
        return;
    }

    // Play music
    if (Mix_PlayMusic(music, 1) < 0) {
        printf("Mix_PlayMusic Error: %s\n", Mix_GetError());
        Mix_FreeMusic(music);
        Mix_CloseAudio();
        SDL_Quit();
        return;
    }

    // Redirect std::cout and std::cerr
    /*
    freopen("output_printf.txt", "w", stdout);
    freopen("output_error.txt", "w", stderr);
    std::ofstream ofs("output.txt", std::ios::app);
    std::cout.rdbuf(ofs.rdbuf());
    std::cerr.rdbuf(ofs.rdbuf());
    std::cout << "Hello from cout!\n";
    */

    // Get main scale
    main_scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);
    window = SDL_CreateWindow("WiiIR", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int)(640 * main_scale), (int)(480 * main_scale), 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    // Attempt to load theme file
    LoadImGuiThemeFromJSON("imgui_theme.json");

    // Initialize SDL2 renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // Setup Wii Impl
    #ifdef NINTENDOWII
    SetupWiiImplementation();
    #endif

    // Select storage medium
    doStorageSelection();
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

void DrawMSPaintEasterEgg(bool* p_open)
{
    if (!*p_open)
        return;

    // ----------------------------
    // Persistent static state
    // ----------------------------
    static bool init = false;
    static ImVec2 canvas_size = ImVec2(128, 64);
    static ImVec4 current_color = ImVec4(0, 0, 0, 1);
    static std::vector<ImVec4> pixel_buffer;

    if (!init)
    {
        pixel_buffer.resize((size_t)canvas_size.x * (size_t)canvas_size.y,
                            ImVec4(1, 1, 1, 1));  // white canvas
        init = true;
    }

    //ImGui::SetNextWindowSize(ImVec2(640, 480), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("MS Paint (Easter Egg)", p_open, ImGuiWindowFlags_NoScrollbar))
    {
        // --------------------------------------
        // 1. Fake Menu Bar
        // --------------------------------------
        if (ImGui::BeginMenuBar())
        {
            ImGui::Text("File   Edit   View   Image   Colors   Help");
            ImGui::EndMenuBar();
        }

        ImGui::Separator();

        // --------------------------------------
        // 2. Fake Toolbar
        // --------------------------------------
        ImGui::Text("Tools:");

        ImGui::SameLine();
        if (ImGui::Button("Pencil")) {}
        ImGui::SameLine();
        if (ImGui::Button("Brush")) {}
        ImGui::SameLine();
        if (ImGui::Button("Eraser")) {}
        ImGui::SameLine();
        if (ImGui::Button("Fill")) {}

        ImGui::Separator();

        // --------------------------------------
        // 3. Color Palette
        // --------------------------------------
        ImGui::Text("Colors:");
        static ImVec4 palette[] = {
            {0,0,0,1}, {1,1,1,1}, {1,0,0,1}, {0,1,0,1}, {0,0,1,1},
            {1,1,0,1}, {1,0,1,1}, {0,1,1,1}, {0.5f,0.5f,0.5f,1},
            {1,0.5f,0,1}
        };

        for (int i = 0; i < IM_ARRAYSIZE(palette); i++)
        {
            ImGui::PushID(i);
            ImGui::ColorButton("col", palette[i], 0, ImVec2(22, 22));
            if (ImGui::IsItemClicked())
                current_color = palette[i];
            ImGui::PopID();

            ImGui::SameLine();
        }

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Text("Canvas:");
        ImGui::BeginChild("CanvasRegion", canvas_size, true);

        ImVec2 origin = ImGui::GetCursorScreenPos();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Draw current canvas from buffer
        for (int y = 0; y < (int)canvas_size.y; y++)
        {
            for (int x = 0; x < (int)canvas_size.x; x++)
            {
                ImVec4 c = pixel_buffer[y * (int)canvas_size.x + x];
                ImU32 col = ImColor(c);

                draw_list->AddRectFilled(
                    ImVec2(origin.x + x, origin.y + y),
                    ImVec2(origin.x + x + 1, origin.y + y + 1),
                    col
                );
            }
        }

        // Handle drawing
        ImGuiIO& io = ImGui::GetIO();
        bool drawing =
            ImGui::IsMouseDown(0) &&
            ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);

        if (drawing)
        {
            ImVec2 mouse = io.MousePos;
            int x = (int)(mouse.x - origin.x);
            int y = (int)(mouse.y - origin.y);

            if (x >= 0 && y >= 0 && x < (int)canvas_size.x && y < (int)canvas_size.y)
            {
                pixel_buffer[y * (int)canvas_size.x + x] = current_color;
            }
        }

        ImGui::EndChild(); // canvas
    }

    ImGui::End();
}