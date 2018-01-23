#ifndef _APP_H
#define _APP_H

#include <vector>

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
#include "framebuffer.h"


#define FRAMESX 2
#define FRAMESY 2
#define FRAMEST 50
#define FRAMESTOTAL (FRAMESX*FRAMESY*FRAMEST)

float interpolate(float x, float y, float a) {
    return a*x + (1-a)*y;
}

class App {
    int _dimx, _dimy, _max_rate;

    SDL_Window *_window;
    SDL_GLContext _glcontext;
    SDL_Renderer *_renderer;
    FrameBuffer *_fb;

    Display *_xdisplay;

    Tablet *_tablet;

    void set(TabletEvent res) {
        try {
            auto pxl = _fb->getPixel(res.x, res.y);
            pxl[0] = pxl[1] = pxl[2] = std::min(255, pxl[0] + res.pressure/5);
            pxl[3] = 255;
        } catch (std::range_error&) {
            printf("oob pixel at %d %d\n", res.x, res.y);
        }
    }

public:
    App() {
        SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER);

        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
        SDL_DisplayMode display_mode;
        SDL_GetCurrentDisplayMode(0, &display_mode);
        _dimx = display_mode.w;
        _dimy = display_mode.h;
        _max_rate = std::max(60, display_mode.refresh_rate);

        _window = SDL_CreateWindow("XFlipbook",
                            SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED,
                            _dimx, _dimy,
                            SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_BORDERLESS | SDL_WINDOW_OPENGL);

        _glcontext = SDL_GL_CreateContext(_window);

        ImGui_ImplSdlGL2_Init(_window);

        glewExperimental = true;
        glewInit();

        ImGui::StyleColorsClassic();
        //ImGui::StyleColorsDark();

        _renderer = SDL_CreateRenderer(_window, -1,  SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

        _fb = new FrameBuffer(_renderer, FRAMEST, _dimx, _dimy, FRAMESX, FRAMESY);

        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(_window, &wmInfo);
        _xdisplay = wmInfo.info.x11.display;
        Window xwindow = wmInfo.info.x11.window;

        auto xscreen = DefaultScreen(_xdisplay);
        _tablet = new Tablet(_xdisplay);
        _tablet->open(xscreen, xwindow);
    }

    ~App() {
        delete _fb;
        ImGui_ImplSdlGL2_Shutdown();
        SDL_DestroyRenderer(_renderer);
        SDL_GL_DeleteContext(_glcontext);
        delete _tablet;
        SDL_DestroyWindow(_window);

        SDL_Quit();
    }

private:
    TabletEvent _last{0, 0, 0};

public:
    bool done = false;
    bool playing = false;
    int frame_rate = 12;
    int frame_cnt = 12;

    bool onion_prev = false;
    bool onion_next = false;

public:
    void processEvents() {
        std::vector<XEvent> events;

        ImGuiIO& io = ImGui::GetIO();
        while (XPending(_xdisplay)) {
            XEvent event;
            XNextEvent(_xdisplay, &event);
            if (_tablet->eventOf(&event) && !io.WantCaptureMouse) {
                auto res = _tablet->parse(&event);
                /* printf ("%d data is %d,%d,%d\n", event_cursor, res.x, res.y, res.pressure); */
                res.x = res.x * (_dimx / 16777216.);
                res.y = res.y * (_dimy / 16777216.);

                float norm = sqrtf(powf(_last.x-res.x, 2)+powf(_last.y-res.y, 2));
                int STEPS = ceilf(norm*5);
                if (_last.pressure > 0 && res.pressure > 0) {
                    for (int step = 0; step <= STEPS; ++step) {
                        TabletEvent in{
                            (int)interpolate(_last.x, res.x, step*1./STEPS),
                            (int)interpolate(_last.y, res.y, step*1./STEPS),
                            int(interpolate(_last.pressure, res.pressure, step*1./STEPS)*norm/STEPS),
                        };
                        set(in);
                    }
                }
                _last = res;
            } else {
                events.push_back(event);
            }
        }

        for (auto &event : events) {
            XPutBackEvent(_xdisplay, &event);
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
                    case ',': _fb->prevFrame(); break;
                    case '.': _fb->nextFrame(); break;
                    case 'q': done = true; break;
                    case '[': onion_prev = !onion_prev; break;
                    case ']': onion_next = !onion_next; break;
                }
            }
        }

        events.clear();
    }

    void render() {
        // Rendering
        SDL_Rect vp;
        vp.x = vp.y = 0;
        vp.w = (int) ImGui::GetIO().DisplaySize.x;
        vp.h = (int) ImGui::GetIO().DisplaySize.y;
        SDL_RenderSetViewport(_renderer, &vp);
        SDL_RenderClear(_renderer);

        _fb->updateActive();

        for (int i = 0; i < onion_prev*2; ++i) {
            _fb->prevFrame();
        }
        for (int i = 0; i < onion_prev*2+1+onion_next*2; ++i) {
            _fb->renderActive();
            _fb->nextFrame();
        }
        for (int i = 0; i < onion_next*2+1; ++i) {
            _fb->prevFrame();
        }

        renderGUI();
        SDL_RenderPresent(_renderer);
    }

    void renderGUI() {
        glUseProgram (0);
        ImGui_ImplSdlGL2_NewFrame(_window);
        if (ImGui::Button("Quit")) {
            done = true;
        }
        ImGui::SliderInt("frame_rate", &frame_rate, 1, _max_rate);
        ImGui::SliderInt("frame_cnt", &frame_cnt, 1, FRAMESTOTAL - 1);
        ImGui::SliderInt("frame", &_fb->getCurrentFrame(), 0, frame_cnt - 1);
        if (ImGui::Button("Play/Stop")) {
            playing = !playing;
        }
        ImGui::SameLine();
        if (ImGui::Button("<")) {
            _fb->prevFrame();
        }
        ImGui::SameLine();
        if (ImGui::Button(">")) {
            _fb->nextFrame();
        }
        ImGui::SameLine();
        ImGui::Checkbox("onion_prev", &onion_prev);
        ImGui::SameLine();
        ImGui::Checkbox("onion_next", &onion_next);
        ImGui::Render();
    }

    void run() {
        while (!done) {
            processEvents();
            render();

            if (playing) {
                _fb->nextFrame();
                SDL_Delay(1000/frame_rate);
            } else {
                /* SDL_Delay(16); */
            }
        }
    }
};
#endif
