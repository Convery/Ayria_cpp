/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-07-22
    License: MIT
*/

#include "Graphics.hpp"

// Windows wants to use BGR for bitmaps, probably some Win16 reasoning.
void Texture2D::RGBA_SwapRB(size_t Size, void *Data)
{
    // TODO(tcn): SSE optimization here.
    for (size_t i = 0; i < Size; i += 4)
    {
        std::swap(((uint8_t *)Data)[i], ((uint8_t *)Data)[i + 2]);
    }
}
void Texture2D::RGB_SwapRB(size_t Size, void *Data)
{
    // TODO(tcn): SSE optimization here.
    for (size_t i = 0; i < Size; i += 3)
    {
        std::swap(((uint8_t *)Data)[i], ((uint8_t *)Data)[i + 2]);
    }
}

// Load texture from disk.
Texture2D::Texture2D(std::string_view Filepath)
{
    // TODO(tcn): STBi?
    assert(false);
}
