// =============================================================================
//  examples/languages/cpp/main.cpp
// -----------------------------------------------------------------------------
//  Drive the genericdoom shared library from C++. GPLv2. See LICENSE.
// =============================================================================
//
//  The C API is already extern "C", so C++ can call it directly. The only C++
//  specific things to know:
//    * Callbacks handed to C must be plain functions or *non-capturing* lambdas
//      (a capturing lambda is not convertible to a C function pointer).
//    * Wrap the framebuffer in a small class for idiomatic, range-checked access.
//
//  BUILD (after building libdoomgeneric -- see bindings/README.md):
//      c++ -std=c++17 -I../../../bindings main.cpp -L<dir> -ldoomgeneric -o doomdemo
//  RUN:
//      ./doomdemo -iwad doom1.wad
// =============================================================================

#include "doomgeneric_capi.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>

namespace {

// A thin, safe view over the engine's framebuffer.
class Framebuffer {
public:
    Framebuffer() : w_(dg_resx()), h_(dg_resy()), pixels_(dg_screen_buffer()) {}
    int width()  const { return w_; }
    int height() const { return h_; }
    std::uint32_t at(int x, int y) const { return pixels_[y * w_ + x]; }

    void save_ppm(const char* path) const {
        std::FILE* f = std::fopen(path, "wb");
        if (!f) return;
        std::fprintf(f, "P6\n%d %d\n255\n", w_, h_);
        for (int i = 0; i < w_ * h_; ++i) {
            std::uint32_t p = pixels_[i];                 // 0x00RRGGBB
            unsigned char rgb[3] = {
                static_cast<unsigned char>((p >> 16) & 0xFF),
                static_cast<unsigned char>((p >> 8) & 0xFF),
                static_cast<unsigned char>(p & 0xFF),
            };
            std::fwrite(rgb, 1, 3, f);
        }
        std::fclose(f);
    }

private:
    int w_, h_;
    std::uint32_t* pixels_;
};

auto g_start = std::chrono::steady_clock::now();
int  g_frame = 0;

// --- callbacks: free functions / non-capturing lambdas only ----------------

void on_init() {
    std::printf("[c++] init: framebuffer %dx%d\n", dg_resx(), dg_resy());
}

void on_draw_frame() {
    if (++g_frame == 100) {
        Framebuffer{}.save_ppm("frame.ppm");
        std::printf("[c++] wrote frame.ppm at frame %d\n", g_frame);
    }
}

std::uint32_t on_get_ticks_ms() {
    using namespace std::chrono;
    return static_cast<std::uint32_t>(
        duration_cast<milliseconds>(steady_clock::now() - g_start).count());
}

void on_sleep_ms(std::uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

int on_get_key(int* /*pressed*/, unsigned char* /*key*/) { return 0; }  // no input
void on_set_title(const char* t) { std::printf("[c++] title: %s\n", t); }

} // namespace

int main(int argc, char** argv) {
    dg_callbacks cb{};
    cb.init             = on_init;
    cb.draw_frame       = on_draw_frame;
    cb.sleep_ms         = on_sleep_ms;
    cb.get_ticks_ms     = on_get_ticks_ms;
    cb.get_key          = on_get_key;
    cb.set_window_title = on_set_title;
    dg_set_callbacks(&cb);

    dg_create(argc, argv);
    for (int i = 0; i < 200; ++i) {
        dg_tick();
    }
    std::printf("[c++] done.\n");
    return 0;
}
