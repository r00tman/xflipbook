#ifndef _BUFFER_H
#define _BUFFER_H

#include <stdexcept>

#include <SDL2/SDL.h>


class Buffer {
    SDL_Renderer *_renderer;
    int _dimx, _dimy;
    Uint8 *_pixels;
    SDL_Texture *_texture;

public:
    Buffer(SDL_Renderer *renderer, int dimx, int dimy) :
        _renderer(renderer), _dimx(dimx), _dimy(dimy)
    {
        _pixels = new Uint8[_dimx*_dimy*4];
        _texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            _dimx, _dimy);
        SDL_SetTextureBlendMode(_texture, SDL_BLENDMODE_ADD);
    }

    ~Buffer() {
        SDL_DestroyTexture(_texture);
        delete[] _pixels;
    }

    Uint8 *getPixel(int x, int y) {
        if (y < 0 || y >= _dimy || x < 0 || x >= _dimx) {
            throw std::range_error("pixel is oob");
        }
        return &_pixels[4 * (x + _dimx * y)];
    }

    void update() {
        SDL_UpdateTexture(_texture, NULL, _pixels, 4 * _dimx * sizeof (Uint8));
    }

    void render(SDL_Rect *src, SDL_Rect *dest) {
        SDL_RenderCopy(_renderer, _texture, src, dest);
    }
};

#endif
