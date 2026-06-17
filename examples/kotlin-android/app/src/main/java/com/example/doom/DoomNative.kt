// =============================================================================
//  DoomNative.kt  --  Kotlin <-> native (JNI) declarations.
// -----------------------------------------------------------------------------
//  GPLv2. See LICENSE.
// =============================================================================
//
//  Each `external fun` here maps to a Java_com_example_doom_DoomNative_native*
//  function in native-doom.cpp. The JNI symbol name is derived from the package
//  + class + method name, so the package/class MUST stay com.example.doom /
//  DoomNative to match the C++ side.
// =============================================================================

package com.example.doom

import android.view.Surface

object DoomNative {
    init {
        System.loadLibrary("doom")   // loads libdoom.so built by CMakeLists.txt
    }

    /** Boot the engine and load the given WAD. Call once. */
    external fun nativeCreate(iwadPath: String)

    /** Give the engine a Surface to draw into (or null when it goes away). */
    external fun nativeSetSurface(surface: Surface?)

    /** Advance one frame. Call ~35 times per second from a render thread. */
    external fun nativeTick()

    /** Push a key event. doomKey is a DOOM key code (see dg_keys.h). */
    external fun nativeKey(doomKey: Int, down: Boolean)
}
