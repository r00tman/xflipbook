#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/extensions/XInput.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include "tablet.h"
	
#define DIMX 2560
#define DIMY 1440
	
Uint8 pixels[DIMX*DIMY*4];

void set(TabletEvent res) {
    auto pxl = pixels + res.y * DIMX * 4 + res.x * 4;
    pxl[0] = pxl[1] = pxl[2] = std::min(255, pxl[0] + res.pressure/5);
    pxl[3] = 255;
}
float interpolate(float x, float y, float a) {
    return a*x + (1-a)*y;
}

int main() {
    SDL_Init(0);

    SDL_Window* window = SDL_CreateWindow("My Game Window",
                          SDL_WINDOWPOS_CENTERED,
                          SDL_WINDOWPOS_CENTERED,
                          DIMX, DIMY,
                          SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_BORDERLESS | SDL_WINDOW_OPENGL);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

    auto texture = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               DIMX, DIMY);

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);
    Display *xdisplay = wmInfo.info.x11.display;
    Window xwindow = wmInfo.info.x11.window;

    XEvent event;
    XEvent *pevent = &event;

    auto xscreen = DefaultScreen(xdisplay);
    Tablet tablet(xdisplay);
    tablet.open(xscreen, xwindow);
	   
    TabletEvent last{0, 0, -1};

    while (1) {
        while (XPending(xdisplay)) {
            XNextEvent(xdisplay, pevent);
            if (tablet.eventOf(pevent)) {
                auto res = tablet.parse(pevent);
                /* printf ("data is %d,%d,%d\n", res.x, res.y, res.pressure); */
                res.x = res.x * 2560 / 30950;
                res.y = res.y * 1440 / 17410;

                float norm = sqrtf(powf(last.x-res.x, 2)+powf(last.y-res.y, 2));
                /* printf("%f\n", norm); */
                int STEPS = ceilf(norm*10);
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
            }
        }
	   	

        SDL_UpdateTexture(texture, NULL, pixels, DIMX * sizeof (Uint32));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        /* SDL_Delay(16); */
    }
	   
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return(0);
}
