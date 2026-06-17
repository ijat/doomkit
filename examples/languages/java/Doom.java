// =============================================================================
//  examples/languages/java/Doom.java
// -----------------------------------------------------------------------------
//  Drive the doomkit shared library from PURE Java using the Foreign
//  Function & Memory API (Project Panama). No JNI, no C glue. Java 22+.
//  GPLv2. See LICENSE.
// =============================================================================
//
//  THE FFM CONCEPTS USED
//    * Linker + SymbolLookup        -> find and call native functions (downcalls)
//    * upcallStub                   -> turn a Java method into a C function ptr
//    * Arena / MemorySegment        -> native memory (the callback struct, argv,
//                                      and reading the framebuffer)
//
//  BUILD & RUN (after building libdoomgeneric -- see bindings/README.md):
//      java --enable-native-access=ALL-UNNAMED \
//           -Djava.library.path=/path/to/lib \
//           Doom.java -iwad doom1.wad
//  (Single-file source launch; needs JDK 22 or newer.)
// =============================================================================

import java.io.FileOutputStream;
import java.lang.foreign.*;
import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;

import static java.lang.foreign.ValueLayout.*;

public final class Doom {

    // One arena for the whole run; it owns the upcall stubs and native buffers,
    // so it must stay open as long as the engine might call back (i.e. forever).
    static final Arena ARENA = Arena.ofShared();
    static final Linker LINKER = Linker.nativeLinker();
    static final SymbolLookup LIB = SymbolLookup.libraryLookup(
            System.mapLibraryName("doomgeneric"), ARENA);

    // ---- downcall handles for the library functions we call ----------------
    static final MethodHandle DG_SET_CALLBACKS = down("dg_set_callbacks", FunctionDescriptor.ofVoid(ADDRESS));
    static final MethodHandle DG_CREATE        = down("dg_create",        FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
    static final MethodHandle DG_TICK          = down("dg_tick",          FunctionDescriptor.ofVoid());
    static final MethodHandle DG_SCREEN_BUFFER = down("dg_screen_buffer", FunctionDescriptor.of(ADDRESS));
    static final MethodHandle DG_RESX          = down("dg_resx",          FunctionDescriptor.of(JAVA_INT));
    static final MethodHandle DG_RESY          = down("dg_resy",          FunctionDescriptor.of(JAVA_INT));

    static MethodHandle down(String name, FunctionDescriptor fd) {
        return LINKER.downcallHandle(LIB.find(name).orElseThrow(), fd);
    }

    static final long startMs = System.currentTimeMillis();
    static int frame = 0;

    // ---- the six callbacks, as plain static methods ------------------------

    static void onInit() {
        System.out.printf("[java] init: framebuffer %dx%d%n", resx(), resy());
    }

    static void onDrawFrame() {
        if (++frame != 100) return;
        try {
            int w = resx(), h = resy();
            // dg_screen_buffer returns a zero-length segment; give it a size.
            MemorySegment fb = ((MemorySegment) DG_SCREEN_BUFFER.invoke())
                    .reinterpret((long) w * h * 4);
            byte[] rgb = new byte[w * h * 3];
            for (int i = 0; i < w * h; i++) {
                int p = fb.getAtIndex(JAVA_INT, i);   // 0x00RRGGBB
                rgb[i * 3]     = (byte) (p >> 16);
                rgb[i * 3 + 1] = (byte) (p >> 8);
                rgb[i * 3 + 2] = (byte) p;
            }
            try (var out = new FileOutputStream("frame.ppm")) {
                out.write(("P6\n" + w + " " + h + "\n255\n").getBytes("US-ASCII"));
                out.write(rgb);
            }
            System.out.printf("[java] wrote frame.ppm at frame %d%n", frame);
        } catch (Throwable t) { throw new RuntimeException(t); }
    }

    static void onSleepMs(int ms) {
        try { Thread.sleep(ms & 0xFFFFFFFFL); } catch (InterruptedException ignored) {}
    }
    static int onGetTicksMs() { return (int) (System.currentTimeMillis() - startMs); }
    static int onGetKey(MemorySegment pressed, MemorySegment key) { return 0; } // no input
    static void onSetTitle(MemorySegment title) {
        System.out.println("[java] title: " + title.reinterpret(Long.MAX_VALUE).getString(0));
    }

    // Build a C function pointer from a static method of this class.
    static MemorySegment upcall(String method, FunctionDescriptor fd, MethodType mt) {
        try {
            MethodHandle mh = MethodHandles.lookup().findStatic(Doom.class, method, mt);
            return LINKER.upcallStub(mh, fd, ARENA);
        } catch (ReflectiveOperationException e) { throw new RuntimeException(e); }
    }

    static int resx() { try { return (int) DG_RESX.invoke(); } catch (Throwable t) { throw new RuntimeException(t); } }
    static int resy() { try { return (int) DG_RESY.invoke(); } catch (Throwable t) { throw new RuntimeException(t); } }

    public static void main(String[] args) throws Throwable {
        // Assemble the dg_callbacks struct: 6 consecutive C function pointers.
        MemorySegment cb = ARENA.allocate(ADDRESS.byteSize() * 6);
        cb.setAtIndex(ADDRESS, 0, upcall("onInit",       FunctionDescriptor.ofVoid(),                    MethodType.methodType(void.class)));
        cb.setAtIndex(ADDRESS, 1, upcall("onDrawFrame",  FunctionDescriptor.ofVoid(),                    MethodType.methodType(void.class)));
        cb.setAtIndex(ADDRESS, 2, upcall("onSleepMs",    FunctionDescriptor.ofVoid(JAVA_INT),            MethodType.methodType(void.class, int.class)));
        cb.setAtIndex(ADDRESS, 3, upcall("onGetTicksMs", FunctionDescriptor.of(JAVA_INT),                MethodType.methodType(int.class)));
        cb.setAtIndex(ADDRESS, 4, upcall("onGetKey",     FunctionDescriptor.of(JAVA_INT, ADDRESS, ADDRESS), MethodType.methodType(int.class, MemorySegment.class, MemorySegment.class)));
        cb.setAtIndex(ADDRESS, 5, upcall("onSetTitle",   FunctionDescriptor.ofVoid(ADDRESS),             MethodType.methodType(void.class, MemorySegment.class)));
        DG_SET_CALLBACKS.invoke(cb);

        // Build argv: program name + the user's args, as a C char**.
        MemorySegment argv = ARENA.allocate(ADDRESS.byteSize() * (args.length + 1));
        argv.setAtIndex(ADDRESS, 0, ARENA.allocateFrom("doomdemo"));
        for (int i = 0; i < args.length; i++) {
            argv.setAtIndex(ADDRESS, i + 1, ARENA.allocateFrom(args[i]));
        }
        DG_CREATE.invoke(args.length + 1, argv);

        for (int i = 0; i < 200; i++) DG_TICK.invoke();
        System.out.println("[java] done.");
    }
}
