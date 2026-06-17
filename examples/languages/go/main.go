// =============================================================================
//  examples/languages/go/main.go
// -----------------------------------------------------------------------------
//  Drive the doomkit shared library from Go using cgo (Go's built-in FFI).
//  GPLv2. See LICENSE.
// =============================================================================
//
//  HOW CALLBACKS WORK IN CGO
//  -------------------------
//  C must be able to call back into Go. cgo allows that via //export: each
//  //export'd Go function becomes a C-callable symbol. There is one rule: a
//  file that uses //export may only DECLARE things in its C preamble, not
//  define them. So the small bit of C that bundles our exported functions into
//  a dg_callbacks struct lives in a separate file, bridge.c, and we just declare
//  registerGoCallbacks() here.
//
//  BUILD (after building libdoomgeneric -- see bindings/README.md):
//      put libdoomgeneric.{so,dylib} where the LDFLAGS below point (./lib), then:
//      go build -o doomdemo .
//  RUN:
//      ./doomdemo -iwad doom1.wad
// =============================================================================

package main

/*
#cgo CFLAGS:  -I${SRCDIR}/../../../bindings
#cgo LDFLAGS: -L${SRCDIR}/lib -ldoomgeneric -Wl,-rpath,${SRCDIR}/lib
#include <stdlib.h>
#include "doomgeneric_capi.h"

// Defined in bridge.c -- wires the exported Go funcs into the library.
void registerGoCallbacks(void);
*/
import "C"

import (
	"fmt"
	"os"
	"time"
	"unsafe"
)

var (
	startTime = time.Now()
	frame     = 0
)

//export goDGInit
func goDGInit() {
	fmt.Printf("[go] init: framebuffer %dx%d\n", int(C.dg_resx()), int(C.dg_resy()))
}

//export goDGDrawFrame
func goDGDrawFrame() {
	frame++
	if frame != 100 {
		return
	}
	w, h := int(C.dg_resx()), int(C.dg_resy())
	// View the C framebuffer as a Go slice without copying.
	pixels := unsafe.Slice((*uint32)(unsafe.Pointer(C.dg_screen_buffer())), w*h)

	f, err := os.Create("frame.ppm")
	if err != nil {
		return
	}
	defer f.Close()
	fmt.Fprintf(f, "P6\n%d %d\n255\n", w, h)
	row := make([]byte, 0, w*3)
	for _, p := range pixels { // p is 0x00RRGGBB
		row = append(row, byte(p>>16), byte(p>>8), byte(p))
		if len(row) == cap(row) {
			f.Write(row)
			row = row[:0]
		}
	}
	fmt.Printf("[go] wrote frame.ppm at frame %d\n", frame)
}

//export goDGSleepMs
func goDGSleepMs(ms C.uint32_t) {
	time.Sleep(time.Duration(ms) * time.Millisecond)
}

//export goDGGetTicksMs
func goDGGetTicksMs() C.uint32_t {
	return C.uint32_t(time.Since(startTime).Milliseconds())
}

//export goDGGetKey
func goDGGetKey(pressed *C.int, key *C.uchar) C.int {
	return 0 // no input in this headless demo
}

//export goDGSetWindowTitle
func goDGSetWindowTitle(title *C.char) {
	fmt.Printf("[go] title: %s\n", C.GoString(title))
}

// cArgv converts os.Args into a C argv the engine can read. The caller frees it.
func cArgv() (C.int, **C.char, func()) {
	argc := C.int(len(os.Args))
	ptrSize := unsafe.Sizeof((*C.char)(nil))
	argv := (**C.char)(C.malloc(C.size_t(uintptr(len(os.Args)) * ptrSize)))
	slice := unsafe.Slice(argv, len(os.Args))
	for i, a := range os.Args {
		slice[i] = C.CString(a)
	}
	free := func() {
		for i := range slice {
			C.free(unsafe.Pointer(slice[i]))
		}
		C.free(unsafe.Pointer(argv))
	}
	return argc, argv, free
}

func main() {
	C.registerGoCallbacks() // register BEFORE create

	argc, argv, free := cArgv()
	defer free()
	C.dg_create(argc, argv)

	for i := 0; i < 200; i++ {
		C.dg_tick()
	}
	fmt.Println("[go] done.")
}
