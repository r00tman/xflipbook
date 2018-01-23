#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

#include <vector>

#include <SDL2/SDL.h>

#include "buffer.h"


class FrameBuffer {
    SDL_Renderer *_renderer;
    int _frame;
    int _dimx, _dimy;
    int _framesx, _framesy;
    std::vector<Buffer*> _buffers;

    int _getBufferIdx(int frame) {
        return (frame / _framesy) / _framesx;
    }
    int _getOffsetX(int frame) {
        return (frame / _framesy) % _framesx;
    }
    int _getOffsetY(int frame) {
        return frame % _framesy;
    }

public:
    FrameBuffer(SDL_Renderer *renderer, int total_frames, int dimx, int dimy, int framesx, int framesy)
        : _renderer(renderer), _frame(0), _dimx(dimx), _dimy(dimy), _framesx(framesx), _framesy(framesy)
    {
        int n_buffers = (total_frames + framesx * framesy - 1) / (framesx * framesy);
        _buffers.reserve(n_buffers);
        for (int i = 0; i < n_buffers; ++i) {
            addNewFrame();
        }
    }

    ~FrameBuffer() {
        for (Buffer *buff : _buffers) {
            delete buff;
        }
    }

    void addNewFrame() {
        _buffers.push_back(new Buffer(_renderer, _dimx * _framesx, _dimy * _framesy));
    }

    int &getCurrentFrame() {
        return _frame;
    }

    Uint8 *getPixel(int x, int y) {
        auto frame = getCurrentFrame();
        x += _getOffsetX(frame) * _dimx;
        y += _getOffsetY(frame) * _dimy;
        auto pxl = _buffers[_getBufferIdx(frame)]->getPixel(x, y);

        return pxl;
    }

    void updateActive() {
        auto frame = getCurrentFrame();
        _buffers[_getBufferIdx(frame)]->update();
    }

    void renderActive() {
        auto frame = getCurrentFrame();
        SDL_Rect what;
        what.x = _getOffsetX(frame) * _dimx;
        what.y = _getOffsetY(frame) * _dimy;
        what.w = _dimx;
        what.h = _dimy;

        _buffers[_getBufferIdx(frame)]->render(&what, nullptr);
    }

    void prevFrame() {
        if (_buffers.size()) {
            _frame = (_frame + _buffers.size() - 1) % _buffers.size();
        }
    }

    void nextFrame() {
        if (_buffers.size()) {
            _frame = (_frame + 1) % _buffers.size();
        }
    }
};

#endif
