/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-05-29
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>

namespace Assets
{
    // PNG
    class Staticimage_t
    {
        const uint8_t *Imagedata{};
        const size_t Imagesize{};

        public:
        const uint16_t Height{};
        const uint16_t Width{};

        // Bitblt the image with transperency.
        bool Bitblt(HDC Devicecontext, vec4i Destinationrect, vec2i Sourceoffset = {})
        {
            const auto Outputheight = std::min(Height - Sourceoffset.y, Destinationrect.w - Destinationrect.y);
            const auto Outputwidth = std::min(Width - Sourceoffset.x, Destinationrect.z - Destinationrect.x);

            int W{}, H{}, Channels{};
            const auto Image = stbi_load_from_memory(Imagedata, (int)Imagesize, &W, &H, &Channels, STBI_rgb_alpha);
            if (!Image)
            {
                Errorprint("Could not load the PNG from memory.");
                return false;
            }

            // Most properties could be infrered from the imagedata..
            auto BMI = (BITMAPINFO *)alloca(sizeof(BITMAPINFO) + sizeof(DWORD) * 4);
            std::memset(BMI, 0, sizeof(BITMAPINFO) + sizeof(DWORD) * 4);
            BMI->bmiHeader.biCompression = BI_RGB | BI_BITFIELDS;
            BMI->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            BMI->bmiHeader.biBitCount = 32;
            BMI->bmiHeader.biHeight = (-1) * H;
            BMI->bmiHeader.biWidth = Width;
            BMI->bmiHeader.biPlanes = 1;

            // Windows thinks in BGR so we need masks..
            auto Masks = (DWORD *)BMI->bmiColors;
            Masks[0] = 0x000000FF;
            Masks[1] = 0x0000FF00;
            Masks[2] = 0x00FF0000;
            Masks[3] = 0x00000000;

            // Copy into device memory.
            if (GDI_ERROR != SetDIBitsToDevice(Devicecontext,
                Destinationrect.x, Destinationrect.y, Outputwidth, Outputheight,
                Sourceoffset.x, Sourceoffset.y, 0, Outputheight, Image, BMI, DIB_RGB_COLORS))
            {
                stbi_image_free(Image);
                return true;
            }

            stbi_image_free(Image);
            return false;
        }

        // Helper for initialization.
        constexpr Staticimage_t(uint16_t W, uint16_t H, size_t N, const uint8_t *Data) :
            Width(W), Height(H), Imagesize(N), Imagedata(Data)
        {
        }
};

    // Bitmap animation.
    template <size_t Palettecount> class Animatedimage_t
    {
        std::array<PALETTEENTRY, Palettecount> Palettentries{};
        const uint8_t *Compresseddata{};
        const size_t Decompressedsize{};
        const size_t Compressedsize{};
        uint32_t Lastrotate{};
        HDC Devicecontext{};
        HPALETTE Palette{};

        public:
        const uint16_t Width{}, Height{};
        const uint16_t RotationrateMS{};
        const int8_t Rotationoffset{};
        const uint8_t BPP{};

        // Animate whichever direction.
        void Animatepalette()
        {
            // NOTE(tcn): If the user is running the app when their computer passes
            // 49.7 days of uptime the animations will stop working. Not going to waste
            // 4 bytes per image just to prevent that. But if there's any complaints..
            const auto Currenttime = GetTickCount();

            // Incase this is called from multiple threads, we track the rate.
            if ((Currenttime - Lastrotate) < RotationrateMS) [[likely]] return;

            if (Rotationoffset) std::rotate(Palettentries.rbegin(), Palettentries.rbegin() + Rotationoffset, Palettentries.rend());
            else std::rotate(Palettentries.begin(), Palettentries.begin() + std::abs(Rotationoffset), Palettentries.end());

            AnimatePalette(Palette, 0, Palettentries.size(), Palettentries.data());
            Lastrotate = Currenttime;
        }

        // Lazy creation of the Devicecontext.
        HDC getDC()
        {
            // Only need to setup once.
            if (Devicecontext) [[likely]] return Devicecontext;

            // The standard LZ4 function handles LZ4HC as well.
            auto Decompresseddata = std::make_unique<uint8_t[]>(Decompressedsize);
            if (0 > LZ4_decompress_safe((const char *)Compresseddata, (char *)Decompresseddata.get(), Compressedsize, Decompressedsize))
            {
                assert(false);
                return {};
            }

            // Windows wants the height to be upsidedown.
            const auto BMI = (BITMAPINFO *)calloc(sizeof(BITMAPINFO) + sizeof(RGBQUAD) * (1 << BPP), 1);
            BMI->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            BMI->bmiHeader.biHeight = (-1) * Height;
            BMI->bmiHeader.biBitCount = BPP;
            BMI->bmiHeader.biWidth = Width;
            BMI->bmiHeader.biPlanes = 1;

            // Get some system memory for us.
            void *Pixelbuffer{};
            const auto BMP = CreateDIBSection(Devicecontext, BMI, DIB_PAL_COLORS, &Pixelbuffer, NULL, 0);
            DeleteObject(SelectObject(Devicecontext, BMP));

            // Placeholder palette.
            const LOGPALETTE Logicalpal{ 0x300, 1, {} };
            Palette = CreatePalette(&Logicalpal);

            // Put the colors into device memory.
            SetPaletteEntries(Palette, 0, Palettentries.size(), Palettentries.data());
            SelectPalette(Devicecontext, Palette, TRUE);
            RealizePalette(Devicecontext);

            // Extra syncing to be safe.
            GdiFlush(); std::memmove(Pixelbuffer, Decompresseddata.get(), Decompressedsize); GdiFlush();

            // Cleanup.
            free(BMI);
            DeleteObject(BMP);
            return Devicecontext;
        }

        // Helper for initialization.
        template <size_t N, size_t P> constexpr Animatedimage_t(uint16_t W, uint16_t H, uint16_t RR, int8_t RO, uint8_t Bits,
            size_t Decompsize, const uint8_t(&Data)[N], const uint8_t(&Palettes)[P]) :
            Width(W), Height(H), RotationrateMS(RR), Rotationoffset(RO), BPP(Bits),
            Decompressedsize(Decompsize), Compressedsize(N), Compresseddata(Data)
        {
            assert(Data);
            assert(Bits <= 8);
            assert(P == Palettecount * 3);

            for (size_t i = 0; i < Palettecount; i += 3)
            {
                Palettentries[i] =
                {
                    Palettes[i * 3 + 0],
                    Palettes[i * 3 + 1],
                    Palettes[i * 3 + 2],
                    PC_RESERVED
                };
            }
        }
    };

    // Forward declaration of the embedded assets.
    //extern Staticimage_t *Loginbackground;
    //extern Staticimage_t *LoginBG;
    extern Staticimage_t *JNZBG;
}