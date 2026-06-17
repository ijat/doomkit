// =============================================================================
//  native-doom.cpp  --  JNI bridge that runs doomkit on Android.
// -----------------------------------------------------------------------------
//  GPLv2. See LICENSE.
// =============================================================================
//
//  On Android the built-in "use a native library" path is the NDK + JNI. Unlike
//  the desktop examples (which load a prebuilt libdoomgeneric and register
//  callbacks), here we compile the engine straight into our app's .so and
//  implement the six DG_* symbols directly in C++ -- that is the most natural
//  NDK approach. The DG_* functions render into the Surface that Kotlin gives
//  us and read keys from a queue Kotlin fills.
//
//  Kotlin <-> native methods (see MainActivity.kt):
//    nativeCreate(iwadPath)   -> doomgeneric_Create(...)
//    nativeSetSurface(surface)-> grab an ANativeWindow to draw into
//    nativeTick()             -> doomgeneric_Tick() (call ~35x/sec)
//    nativeKey(doomKey, down) -> push a key event
// =============================================================================

#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>

#include <cstring>
#include <ctime>
#include <unistd.h>

extern "C" {
#include "doomgeneric.h"                 // engine: DG_ScreenBuffer, RESX/Y, Create/Tick
#include "doomkit/dg_keyqueue.h"      // our reusable input ring buffer
}

#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "doom", __VA_ARGS__)

static ANativeWindow* g_window = nullptr;
static dg_keyqueue_t  g_keys;
static uint32_t       g_start_ms = 0;

static uint32_t now_ms() {
    timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}

// ---- the six doomkit callbacks, implemented for Android ----------------
extern "C" {

void DG_Init(void) {
    dg_keyqueue_init(&g_keys);
    g_start_ms = now_ms();
    LOG("DG_Init: %dx%d", DOOMGENERIC_RESX, DOOMGENERIC_RESY);
}

void DG_DrawFrame(void) {
    if (!g_window) return;
    ANativeWindow_Buffer buf;
    if (ANativeWindow_lock(g_window, &buf, nullptr) != 0) return;

    const uint32_t* src = (const uint32_t*)DG_ScreenBuffer;   // 0x00RRGGBB
    int rows = buf.height < DOOMGENERIC_RESY ? buf.height : DOOMGENERIC_RESY;
    int cols = buf.width  < DOOMGENERIC_RESX ? buf.width  : DOOMGENERIC_RESX;
    for (int y = 0; y < rows; y++) {
        uint32_t* dst = (uint32_t*)((uint8_t*)buf.bits + (size_t)y * buf.stride * 4);
        const uint32_t* s = src + (size_t)y * DOOMGENERIC_RESX;
        for (int x = 0; x < cols; x++) {
            uint32_t p = s[x];                                // 0x00RRGGBB
            // Android RGBA_8888 stores bytes R,G,B,A. Repack so colours match.
            uint8_t r = (p >> 16) & 0xFF, g = (p >> 8) & 0xFF, b = p & 0xFF;
            dst[x] = (0xFFu << 24) | (b << 16) | (g << 8) | r; // little-endian RGBA
        }
    }
    ANativeWindow_unlockAndPost(g_window);
}

void DG_SleepMs(uint32_t ms)      { usleep(ms * 1000); }
uint32_t DG_GetTicksMs(void)      { return now_ms() - g_start_ms; }
int  DG_GetKey(int* pressed, unsigned char* key) { return dg_keyqueue_pop(&g_keys, pressed, key); }
void DG_SetWindowTitle(const char* t) { LOG("title: %s", t ? t : ""); }

} // extern "C" (callbacks)

// ---- JNI entry points called from Kotlin -----------------------------------
extern "C" {

JNIEXPORT void JNICALL
Java_com_example_doom_DoomNative_nativeCreate(JNIEnv* env, jobject, jstring iwadPath) {
    const char* path = env->GetStringUTFChars(iwadPath, nullptr);
    char a0[] = "doomgeneric", a1[] = "-iwad";
    char* argv[] = { a0, a1, (char*)path, nullptr };
    doomgeneric_Create(3, argv);          // calls DG_Init, loads the WAD
    env->ReleaseStringUTFChars(iwadPath, path);
}

JNIEXPORT void JNICALL
Java_com_example_doom_DoomNative_nativeSetSurface(JNIEnv* env, jobject, jobject surface) {
    if (g_window) { ANativeWindow_release(g_window); g_window = nullptr; }
    if (surface) {
        g_window = ANativeWindow_fromSurface(env, surface);
        ANativeWindow_setBuffersGeometry(g_window, DOOMGENERIC_RESX, DOOMGENERIC_RESY,
                                         WINDOW_FORMAT_RGBA_8888);
    }
}

JNIEXPORT void JNICALL
Java_com_example_doom_DoomNative_nativeTick(JNIEnv*, jobject) {
    doomgeneric_Tick();
}

JNIEXPORT void JNICALL
Java_com_example_doom_DoomNative_nativeKey(JNIEnv*, jobject, jint doomKey, jboolean down) {
    dg_keyqueue_push(&g_keys, down ? 1 : 0, (unsigned char)doomKey);
}

} // extern "C" (JNI)
