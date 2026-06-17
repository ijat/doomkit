// =============================================================================
//  MainActivity.kt  --  Minimal Android host for genericdoom.
// -----------------------------------------------------------------------------
//  GPLv2. See LICENSE.
// =============================================================================
//
//  A SurfaceView gives us a Surface to render into; a background thread runs the
//  create/tick loop (never block the UI thread). On-screen buttons or hardware
//  keys translate to DOOM key codes and are pushed down via DoomNative.nativeKey.
//
//  This is a teaching skeleton: copy it into a fresh Android Studio project
//  (Kotlin, with C++/NDK support) alongside the cpp/ folder and DoomNative.kt.
// =============================================================================

package com.example.doom

import android.app.Activity
import android.os.Bundle
import android.view.KeyEvent
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import java.io.File

// DOOM key codes we forward (mirror of include/genericdoom/dg_keys.h).
private const val KEY_FIRE = 0xa3
private const val KEY_USE = 0xa2
private const val KEY_UP = 0xad
private const val KEY_DOWN = 0xaf
private const val KEY_LEFT = 0xac
private const val KEY_RIGHT = 0xae
private const val KEY_ENTER = 13
private const val KEY_ESCAPE = 27

class MainActivity : Activity(), SurfaceHolder.Callback {

    private lateinit var view: SurfaceView
    private var loop: Thread? = null
    @Volatile private var running = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        view = SurfaceView(this).also { it.holder.addCallback(this) }
        setContentView(view)
    }

    // --- Surface lifecycle: start/stop the engine loop with the Surface ------

    override fun surfaceCreated(holder: SurfaceHolder) {
        // Copy doom1.wad into app storage (e.g. from assets) and point the
        // engine at it. Here we assume it already sits in filesDir.
        val wad = File(filesDir, "doom1.wad").absolutePath

        DoomNative.nativeSetSurface(holder.surface)
        running = true
        loop = Thread {
            DoomNative.nativeCreate(wad)
            while (running) {
                DoomNative.nativeTick()
                try { Thread.sleep(28) } catch (_: InterruptedException) {}  // ~35 fps
            }
        }.also { it.start() }
    }

    override fun surfaceChanged(h: SurfaceHolder, fmt: Int, w: Int, height: Int) {}

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        running = false
        loop?.join()
        loop = null
        DoomNative.nativeSetSurface(null)
    }

    // --- Input: hardware keys -> DOOM key codes ------------------------------

    override fun onKeyDown(keyCode: Int, event: KeyEvent?): Boolean =
        forward(keyCode, true) || super.onKeyDown(keyCode, event)

    override fun onKeyUp(keyCode: Int, event: KeyEvent?): Boolean =
        forward(keyCode, false) || super.onKeyUp(keyCode, event)

    private fun forward(keyCode: Int, down: Boolean): Boolean {
        val doomKey = when (keyCode) {
            KeyEvent.KEYCODE_DPAD_UP -> KEY_UP
            KeyEvent.KEYCODE_DPAD_DOWN -> KEY_DOWN
            KeyEvent.KEYCODE_DPAD_LEFT -> KEY_LEFT
            KeyEvent.KEYCODE_DPAD_RIGHT -> KEY_RIGHT
            KeyEvent.KEYCODE_DPAD_CENTER, KeyEvent.KEYCODE_BUTTON_A -> KEY_FIRE
            KeyEvent.KEYCODE_SPACE, KeyEvent.KEYCODE_BUTTON_B -> KEY_USE
            KeyEvent.KEYCODE_ENTER -> KEY_ENTER
            KeyEvent.KEYCODE_BACK, KeyEvent.KEYCODE_ESCAPE -> KEY_ESCAPE
            else -> return false
        }
        DoomNative.nativeKey(doomKey, down)
        return true
    }
}
