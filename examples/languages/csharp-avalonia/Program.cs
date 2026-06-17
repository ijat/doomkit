// =============================================================================
//  examples/languages/csharp-avalonia/Program.cs
// -----------------------------------------------------------------------------
//  Draw DOOM's framebuffer in a desktop window using Avalonia + P/Invoke.
//  GPLv2. See LICENSE.
// =============================================================================
//
//  WHAT THIS SHOWS
//    A small *graphical* port: take the same FFI binding as the plain C# example
//    (../csharp) and, instead of writing a PPM file, copy each frame into an
//    Avalonia WriteableBitmap shown in an <Image>. A few keys are wired so you
//    can actually play:  arrows = move/turn,  E = use,  Space = shoot,
//    Enter = confirm menus.
//
//  THREE THINGS THAT MATTER
//    1. Keep the Callbacks struct rooted (the _cb field). The engine stores the
//       function pointers and calls them later; if the GC collects the delegates
//       the engine calls freed memory and crashes.
//    2. DOOM owns its own loop (dg_tick blocks via SleepMs), so it runs on a
//       background thread, leaving the UI thread free for Avalonia.
//    3. DOOM pixels are 0x00RRGGBB; in memory that is [B,G,R,A] -- already the
//       Bgra8888 byte order. We only OR in 0xFF alpha (DOOM leaves it 0).
//    4. Key events arrive on the UI thread (KeyDown/KeyUp) but GetKey is called
//       on the DOOM thread, so they cross over via a thread-safe queue.
//
//  BUILD & RUN (after `make lib` -- see ../../../bindings/README.md):
//      cp ../../../build/lib/libdoomgeneric.dylib .      # or .so / .dll
//      dotnet run -- -iwad /path/to/doom1.wad
// =============================================================================
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Threading;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Input;
using Avalonia.Media;
using Avalonia.Media.Imaging;
using Avalonia.Platform;
using Avalonia.Threading;

// ---- FFI binding: identical to examples/languages/csharp -------------------
internal static class Doom
{
    private const string Lib = "doomgeneric";

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate void InitFn();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate void DrawFrameFn();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate void SleepMsFn(uint ms);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate uint GetTicksMsFn();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate int  GetKeyFn(ref int pressed, ref byte doomKey);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate void SetTitleFn([MarshalAs(UnmanagedType.LPStr)] string title);

    [StructLayout(LayoutKind.Sequential)]
    public struct Callbacks
    {
        [MarshalAs(UnmanagedType.FunctionPtr)] public InitFn       Init;
        [MarshalAs(UnmanagedType.FunctionPtr)] public DrawFrameFn  DrawFrame;
        [MarshalAs(UnmanagedType.FunctionPtr)] public SleepMsFn    SleepMs;
        [MarshalAs(UnmanagedType.FunctionPtr)] public GetTicksMsFn GetTicksMs;
        [MarshalAs(UnmanagedType.FunctionPtr)] public GetKeyFn     GetKey;
        [MarshalAs(UnmanagedType.FunctionPtr)] public SetTitleFn   SetWindowTitle;
    }

    [DllImport(Lib)] public static extern void   dg_set_callbacks(ref Callbacks cb);
    [DllImport(Lib)] public static extern void   dg_create(int argc,
        [In, MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPStr)] string[] argv);
    [DllImport(Lib)] public static extern void   dg_tick();
    [DllImport(Lib)] public static extern IntPtr dg_screen_buffer();
    [DllImport(Lib)] public static extern int    dg_resx();
    [DllImport(Lib)] public static extern int    dg_resy();
}

internal class MainWindow : Window
{
    private readonly int _w = Doom.dg_resx(), _h = Doom.dg_resy(); // 640x400, safe to call early
    private readonly WriteableBitmap _bitmap;
    private readonly Image _image;
    private readonly int[] _pixels;
    private readonly Stopwatch _clock = Stopwatch.StartNew();
    private readonly string[] _args;
    private Doom.Callbacks _cb;                                    // roots the delegates (GC safety!)

    // DOOM key codes from include/doomkit/dg_keys.h.
    private const byte KEY_USE        = 0xa2;   // open doors / flip switches
    private const byte KEY_FIRE       = 0xa3;   // shoot
    private const byte KEY_UPARROW    = 0xad;   // move forward
    private const byte KEY_DOWNARROW  = 0xaf;   // move back
    private const byte KEY_LEFTARROW  = 0xac;   // turn left
    private const byte KEY_RIGHTARROW = 0xae;   // turn right
    private const byte KEY_ENTER      = 13;     // confirm menu selections

    // UI thread enqueues (pressed, doomKey); the DOOM thread dequeues in GetKey.
    private readonly ConcurrentQueue<(int Pressed, byte Key)> _events = new();
    private readonly HashSet<Key> _heldKeys = new();              // to drop OS auto-repeat

    public MainWindow(string[] args)
    {
        _args = args;
        _pixels = new int[_w * _h];
        _bitmap = new WriteableBitmap(new PixelSize(_w, _h), new Vector(96, 96),
            PixelFormat.Bgra8888, AlphaFormat.Opaque);
        _image = new Image { Source = _bitmap, Stretch = Stretch.Uniform };

        Title = "DOOM (Avalonia)";
        Width = _w;
        Height = _h;
        Content = _image;

        KeyDown += OnKeyDown;                                     // arrows, E, Space, Enter
        KeyUp   += OnKeyUp;

        new Thread(RunDoom) { IsBackground = true }.Start();      // DOOM loop off the UI thread
    }

    // Map the two host keys we care about to DOOM key codes.
    private static bool TryMap(Key key, out byte doomKey)
    {
        switch (key)
        {
            case Key.E:      doomKey = KEY_USE;        return true;
            case Key.Space:  doomKey = KEY_FIRE;       return true;
            case Key.Up:     doomKey = KEY_UPARROW;    return true;
            case Key.Down:   doomKey = KEY_DOWNARROW;  return true;
            case Key.Left:   doomKey = KEY_LEFTARROW;  return true;
            case Key.Right:  doomKey = KEY_RIGHTARROW; return true;
            case Key.Enter:  doomKey = KEY_ENTER;      return true;
            default:         doomKey = 0;              return false;
        }
    }

    private void OnKeyDown(object sender, KeyEventArgs e)         // UI thread
    {
        if (TryMap(e.Key, out var dk) && _heldKeys.Add(e.Key))   // Add() is false on auto-repeat
            _events.Enqueue((1, dk));
    }

    private void OnKeyUp(object sender, KeyEventArgs e)           // UI thread
    {
        if (TryMap(e.Key, out var dk) && _heldKeys.Remove(e.Key))
            _events.Enqueue((0, dk));
    }

    private void RunDoom()
    {
        _cb = new Doom.Callbacks
        {
            Init           = () => { },
            DrawFrame      = OnDrawFrame,
            SleepMs        = ms => Thread.Sleep((int)ms),
            GetTicksMs     = () => (uint)_clock.ElapsedMilliseconds,
            GetKey         = OnGetKey,                            // drain one queued event
            SetWindowTitle = t => { },
        };
        Doom.dg_set_callbacks(ref _cb);

        var argv = new string[_args.Length + 1];
        argv[0] = "doomavalonia";                                 // C programs expect argv[0]
        Array.Copy(_args, 0, argv, 1, _args.Length);
        Doom.dg_create(argv.Length, argv);

        while (true) Doom.dg_tick();
    }

    private int OnGetKey(ref int pressed, ref byte doomKey)       // called on the DOOM thread
    {
        if (!_events.TryDequeue(out var ev)) return 0;            // 0 = no more events this frame
        pressed = ev.Pressed;
        doomKey = ev.Key;
        return 1;
    }

    private void OnDrawFrame()                                    // called on the DOOM thread
    {
        Marshal.Copy(Doom.dg_screen_buffer(), _pixels, 0, _pixels.Length);
        for (int i = 0; i < _pixels.Length; i++)
            _pixels[i] |= unchecked((int)0xFF000000);             // force opaque alpha

        // Invoke (not Post) blocks DOOM until the frame is on screen: no buffer
        // race, and it naturally paces the engine to the display.
        Dispatcher.UIThread.Invoke(() =>
        {
            using (var fb = _bitmap.Lock())
                Marshal.Copy(_pixels, 0, fb.Address, _pixels.Length);
            _image.InvalidateVisual();
        });
    }
}

internal class App : Application
{
    public override void OnFrameworkInitializationCompleted()
    {
        if (ApplicationLifetime is IClassicDesktopStyleApplicationLifetime d)
            d.MainWindow = new MainWindow(d.Args ?? Array.Empty<string>());
        base.OnFrameworkInitializationCompleted();
    }
}

internal static class Program
{
    [STAThread]
    public static void Main(string[] args) =>
        AppBuilder.Configure<App>().UsePlatformDetect().StartWithClassicDesktopLifetime(args);
}
