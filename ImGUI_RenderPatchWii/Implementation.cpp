#ifdef NINTENDOWII
#include "WiiIR/IR.hpp"
#include "imgui_impl_sdlrenderer2.h"
#include "imgui_impl_sdl2.h"

static bool Platform_OpenInShellFn_WiiImpl(ImGuiContext* ctx, const char* path)
{
    (void)ctx; // unused

    if (!path)
        return false;

    // Launch the Internet Channel with the URL/path
    // Confirm weather or not the user actually wants to launch the URL?
    bool done = false;
    bool wantsOpen = false;
    float frame;
    while(!done) {

        // --- Event Handling ---
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT) {
                done = true;
                break;
            }
        }

        // --- Begin ImGui Frame ---
        ImGui_ImplSDL2_NewFrame();
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui::NewFrame();

        // --- Build UI ---
        ImGui::SetNextWindowSize(ImVec2(300, 150), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2( (640-300)/2, (480-150)/2 ), ImGuiCond_Always);

        ImGui::Begin("Are you sure?", nullptr,
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoCollapse);

        // Render in ImGui
        ImGui::TextWrapped("You will be redirected to \"%s\" in the internet channel.", path);
        ImGui::Spacing();

        if (ImGui::Button("Ok", ImVec2(120, 40))) {
            done = true;
            wantsOpen = true;
        }

        ImGui::End();

        // --- Rendering ---
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        frame += 1.0f;
        SDL_Texture* perlinTexture = GeneratePS3Background(renderer, 64, 128, frame, 0);
        SDL_RenderCopy(renderer, perlinTexture, nullptr, nullptr);
        FreeTexture(perlinTexture);

        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    // Define return variable
    int ret = -1; // Default response is not-executed
    if(wantsOpen) {
        ret = WII_LaunchTitleWithArgs(
            0x0001000148414445ULL,  // Internet Channel title ID
            0,                       // launch flags
            path,                    // pass the URL string
            NULL                     // reserved
        );
    }

    // Return true if the launch succeeded (non-negative return usually means success)
    return ret >= 0;
}

void SetupWiiImplementation() {
    ImGui::GetPlatformIO().Platform_OpenInShellFn = Platform_OpenInShellFn_WiiImpl;
}

#endif