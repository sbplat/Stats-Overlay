/*
MIT License

Copyright (c) 2022 sbplat

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_TTF.h>

#include <memory>

// RAII style
namespace SDL2 {

    struct SDL_Deleter {
        void operator()(SDL_Surface *ptr) {
            if (ptr) {
                SDL_FreeSurface(ptr);
            }
        }
        void operator()(SDL_Texture *ptr) {
            if (ptr) {
                SDL_DestroyTexture(ptr);
            }
        }
        void operator()(SDL_Renderer *ptr) {
            if (ptr) {
                SDL_DestroyRenderer(ptr);
            }
        }
        void operator()(SDL_Window *ptr) {
            if (ptr) {
                SDL_DestroyWindow(ptr);
            }
        }
        void operator()(SDL_RWops *ptr) {
            if (ptr) {
                SDL_FreeRW(ptr);
            }
        }
        void operator()(TTF_Font *ptr) {
            if (ptr) {
                TTF_CloseFont(ptr);
            }
        }
    };

    using Surface = std::unique_ptr<SDL_Surface, SDL_Deleter>;
    using Texture = std::unique_ptr<SDL_Texture, SDL_Deleter>;
    using Renderer = std::unique_ptr<SDL_Renderer, SDL_Deleter>;
    using Window = std::unique_ptr<SDL_Window, SDL_Deleter>;
    using RWops = std::unique_ptr<SDL_RWops, SDL_Deleter>;
    using TTF_Font = std::unique_ptr<TTF_Font, SDL_Deleter>;

}  // namespace SDL2
