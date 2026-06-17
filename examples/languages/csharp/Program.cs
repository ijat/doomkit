// =============================================================================
//  examples/languages/csharp/Program.cs
// -----------------------------------------------------------------------------
//  Drive the doomkit shared library from C# using P/Invoke (the built-in
//  .NET interop). GPLv2. See LICENSE.
// =============================================================================
//
//  KEY POINTS FOR .NET INTEROP
//    * [DllImport("doomgeneric")] binds to libdoomgeneric.{so,dylib}/doomgeneric.dll.
//    * Each callback is a delegate marked Cdecl; .NET marshals a delegate to a
//      C function pointer automatically inside the struct.
//    * You MUST keep the delegate instances alive (store them in static fields),
//      or the GC will collect them and the engine will call freed memory.
//
//  BUILD & RUN (after building libdoomgeneric -- see bindings/README.md):
//      dotnet build -c Release
//      # ensure the native lib is loadable (copy it next to the .dll, or set
//      # DYLD_LIBRARY_PATH / LD_LIBRARY_PATH), then:
//      dotnet run -- -iwad doom1.wad
// =============================================================================

using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;

internal static class Doom
{
    private const string Lib = "doomgeneric";

    // ---- delegate types matching the C callback signatures -----------------
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate void InitFn();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate void DrawFrameFn();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate void SleepMsFn(uint ms);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate uint GetTicksMsFn();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate int  GetKeyFn(ref int pressed, ref byte doomKey);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate void SetTitleFn([MarshalAs(UnmanagedType.LPStr)] string title);

    // ---- the struct handed to dg_set_callbacks (6 function pointers) -------
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

    // ---- the exported library functions ------------------------------------
    [DllImport(Lib)] public static extern void   dg_set_callbacks(ref Callbacks cb);
    [DllImport(Lib)] public static extern void   dg_create(int argc,
        [In, MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPStr)] string[] argv);
    [DllImport(Lib)] public static extern void   dg_tick();
    [DllImport(Lib)] public static extern IntPtr dg_screen_buffer();
    [DllImport(Lib)] public static extern int    dg_resx();
    [DllImport(Lib)] public static extern int    dg_resy();
}

internal static class Program
{
    // Roots so the GC never collects the delegates the engine holds.
    private static Doom.Callbacks _cb;
    private static readonly Stopwatch _clock = Stopwatch.StartNew();
    private static int _frame;

    private static void OnInit() =>
        Console.WriteLine($"[c#] init: framebuffer {Doom.dg_resx()}x{Doom.dg_resy()}");

    private static void OnDrawFrame()
    {
        if (++_frame != 100) return;

        int w = Doom.dg_resx(), h = Doom.dg_resy();
        var pixels = new int[w * h];
        Marshal.Copy(Doom.dg_screen_buffer(), pixels, 0, pixels.Length);

        using var f = new FileStream("frame.ppm", FileMode.Create);
        var header = System.Text.Encoding.ASCII.GetBytes($"P6\n{w} {h}\n255\n");
        f.Write(header, 0, header.Length);
        var rgb = new byte[w * h * 3];
        for (int i = 0; i < pixels.Length; i++)
        {
            uint p = (uint)pixels[i];              // 0x00RRGGBB
            rgb[i * 3 + 0] = (byte)(p >> 16);
            rgb[i * 3 + 1] = (byte)(p >> 8);
            rgb[i * 3 + 2] = (byte)p;
        }
        f.Write(rgb, 0, rgb.Length);
        Console.WriteLine($"[c#] wrote frame.ppm at frame {_frame}");
    }

    private static void OnSleepMs(uint ms) => System.Threading.Thread.Sleep((int)ms);
    private static uint OnGetTicksMs() => (uint)_clock.ElapsedMilliseconds;
    private static int  OnGetKey(ref int pressed, ref byte doomKey) => 0; // no input
    private static void OnSetTitle(string title) => Console.WriteLine($"[c#] title: {title}");

    private static void Main(string[] args)
    {
        _cb = new Doom.Callbacks
        {
            Init           = OnInit,
            DrawFrame      = OnDrawFrame,
            SleepMs        = OnSleepMs,
            GetTicksMs     = OnGetTicksMs,
            GetKey         = OnGetKey,
            SetWindowTitle = OnSetTitle,
        };
        Doom.dg_set_callbacks(ref _cb);

        // dg_create reads argv[0..]; prepend a program name like a real argv.
        var argv = new string[args.Length + 1];
        argv[0] = "doomdemo";
        Array.Copy(args, 0, argv, 1, args.Length);
        Doom.dg_create(argv.Length, argv);

        for (int i = 0; i < 200; i++) Doom.dg_tick();
        Console.WriteLine("[c#] done.");
    }
}
