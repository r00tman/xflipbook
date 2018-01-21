#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/extensions/XInput.h>
#include <GL/glew.h>

#include "imgui/imgui.h"
#include "imgui_impl_sdl_gl2.h"

#include "tablet.h"
	
int DIMX, DIMY, MAX_RATE;
#define FRAMESX 2
#define FRAMESY 2
#define FRAMEST 50
#define FRAMESTOTAL (FRAMESX*FRAMESY*FRAMEST)
#define MAXEVENTS (1000000)
	
Uint8 *pixels[FRAMEST];
XEvent event[MAXEVENTS];
int frame_cnt = 12;
int frame;

int offsett() {
    return (frame/FRAMESY)/FRAMESX;
}

int offsetx() {
    return (frame/FRAMESY)%FRAMESX;
}

int offsety() {
    return frame%FRAMESY;
}

void set(TabletEvent res) {
    if (res.x < 0)     res.x = 0;
    if (res.y < 0)     res.y = 0;
    if (res.x >= DIMX) res.x = DIMX-1;
    if (res.y >= DIMY) res.y = DIMY-1;
    res.x += offsetx() * DIMX;
    res.y += offsety() * DIMY;
    auto pxl = pixels[offsett()] + 4 * (res.x + DIMX * FRAMESX * res.y);
    pxl[0] = pxl[1] = pxl[2] = std::min(255, pxl[0] + res.pressure/5);
    pxl[3] = 255;
}

float interpolate(float x, float y, float a) {
    return a*x + (1-a)*y;
}

int main() {
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_DisplayMode display_mode;
    SDL_GetCurrentDisplayMode(0, &display_mode);
    DIMX = display_mode.w;
    DIMY = display_mode.h;
    MAX_RATE = std::max(60, display_mode.refresh_rate);

    SDL_Window *window = SDL_CreateWindow("My Game Window",
                          SDL_WINDOWPOS_CENTERED,
                          SDL_WINDOWPOS_CENTERED,
                          DIMX, DIMY,
                          SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_BORDERLESS | SDL_WINDOW_OPENGL);

    SDL_GLContext glcontext = SDL_GL_CreateContext(window);

    ImGui_ImplSdlGL2_Init(window);

    glewExperimental = true;
    glewInit();

    ImGui::StyleColorsClassic();
    //ImGui::StyleColorsDark();

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,  SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    SDL_Texture *texture[FRAMEST];
    for (int i = 0; i < FRAMEST; ++i) {
        pixels[i] = new Uint8[FRAMESX*FRAMESY*DIMY*DIMX*4];
        texture[i] = SDL_CreateTexture(renderer,
                                       SDL_PIXELFORMAT_ARGB8888,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       DIMX*FRAMESX, DIMY*FRAMESY);
        SDL_SetTextureBlendMode(texture[i], SDL_BLENDMODE_ADD);
    }

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);
    Display *xdisplay = wmInfo.info.x11.display;
    Window xwindow = wmInfo.info.x11.window;

    int event_cursor = 0;

    auto xscreen = DefaultScreen(xdisplay);
    Tablet tablet(xdisplay);
    tablet.open(xscreen, xwindow);
	   
    TabletEvent last{0, 0, -1};

    bool done = false;
    bool playing = false;
    int frame_rate = 12;

    bool onion_prev = false;
    bool onion_next = false;
    while (!done) {
        ImGuiIO& io = ImGui::GetIO();
        while (XPending(xdisplay)) {
            XNextEvent(xdisplay, &event[event_cursor]);
            if (tablet.eventOf(&event[event_cursor]) && !io.WantCaptureMouse) {
                auto res = tablet.parse(&event[event_cursor]);
                /* printf ("%d data is %d,%d,%d\n", event_cursor, res.x, res.y, res.pressure); */
                res.x = res.x * (DIMX / 16777216.);
                res.y = res.y * (DIMY / 16777216.);

                float norm = sqrtf(powf(last.x-res.x, 2)+powf(last.y-res.y, 2));
                int STEPS = ceilf(norm*5);
                if (last.pressure != -1 && res.pressure > 0) {
                    for (int step = 0; step <= STEPS; ++step) {
                        TabletEvent in{
                            (int)interpolate(last.x, res.x, step*1./STEPS),
                            (int)interpolate(last.y, res.y, step*1./STEPS),
                            int(interpolate(last.pressure, res.pressure, step*1./STEPS)*norm/STEPS),
                        };
                        set(in); 
                    }
                }
                last = res;
            } else if (event_cursor < MAXEVENTS) {
                event_cursor++;
            }
        }
        while (event_cursor) {
            XPutBackEvent(xdisplay, &event[--event_cursor]);
        }
        SDL_Event sdl_event;
        while (SDL_PollEvent(&sdl_event))
        {
            ImGui_ImplSdlGL2_ProcessEvent(&sdl_event);
            if (sdl_event.type == SDL_QUIT)
                done = true;
            if (sdl_event.type == SDL_KEYDOWN) {
                switch (sdl_event.key.keysym.sym) {
                    case SDLK_SPACE: playing = !playing; break;
                    case ',': frame = (frame + frame_cnt - 1) % frame_cnt; break;
                    case '.': frame = (frame + 1) % frame_cnt; break;
                    case 'q': done = true; break;
                    case '[': onion_prev = !onion_prev; break;
                    case ']': onion_next = !onion_next; break;
                }
            }
        }

        // Rendering
        SDL_Rect vp;
        vp.x = vp.y = 0;
        vp.w = (int) ImGui::GetIO ().DisplaySize.x;
        vp.h = (int) ImGui::GetIO ().DisplaySize.y;
        SDL_RenderSetViewport (renderer, &vp);
        SDL_RenderClear (renderer);

        SDL_UpdateTexture(texture[offsett()], NULL, pixels[offsett()], FRAMESX * DIMX * sizeof (Uint32));

        for (int i = 0; i < onion_prev*2; ++i) {
            frame = (frame + frame_cnt - 1) % frame_cnt;
        }
        for (int i = 0; i < onion_prev*2+1+onion_next*2; ++i) {
            SDL_Rect what;
            what.x = offsetx()*DIMX;
            what.y = offsety()*DIMY;
            what.w = DIMX;
            what.h = DIMY;

            SDL_RenderCopy(renderer, texture[offsett()], &what, NULL);
            frame = (frame + 1) % frame_cnt;
        }
        for (int i = 0; i < onion_next*2+1; ++i) {
            frame = (frame + frame_cnt - 1) % frame_cnt;
        }

        // GUI
        glUseProgram (0);
        ImGui_ImplSdlGL2_NewFrame(window);
        ImGui::Text("drawing on %d, %d, %d", offsett(), offsety(), offsetx());
        if (ImGui::Button("Quit")) {
            done = true;
        }
        ImGui::SliderInt("frame_rate", &frame_rate, 1, MAX_RATE);
        ImGui::SliderInt("frame_cnt", &frame_cnt, 1, FRAMESTOTAL - 1);
        ImGui::SliderInt("frame", &frame, 0, frame_cnt - 1);
        if (ImGui::Button("Play/Stop")) {
            playing = !playing;
        }
        ImGui::SameLine();
        if (ImGui::Button("<")) {
            frame = (frame + frame_cnt - 1) % frame_cnt;
        }
        ImGui::SameLine();
        if (ImGui::Button(">")) {
            frame = (frame + 1) % frame_cnt;
        }
        ImGui::SameLine();
        ImGui::Checkbox("onion_prev", &onion_prev);
        ImGui::SameLine();
        ImGui::Checkbox("onion_next", &onion_next);
        ImGui::Render();
        SDL_RenderPresent(renderer);

        if (playing) {
            frame = (frame + 1) % frame_cnt;
            SDL_Delay(1000/frame_rate);
        } else {
            /* SDL_Delay(16); */
        }
    }
	   
    ImGui_ImplSdlGL2_Shutdown();
    for (int i = 0; i < FRAMEST; ++i) {
        delete[] pixels[i];
        SDL_DestroyTexture(texture[i]);
    }
    SDL_DestroyRenderer(renderer);
    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return(0);
}
