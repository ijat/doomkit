// build.rs -- tell cargo where to find and how to link libdoomgeneric.
//
// Adjust the search path to wherever you put the shared library. By default we
// look in a `lib/` folder next to this file.
use std::path::PathBuf;

fn main() {
    let lib_dir: PathBuf = [env!("CARGO_MANIFEST_DIR"), "lib"].iter().collect();
    println!("cargo:rustc-link-search=native={}", lib_dir.display());
    println!("cargo:rustc-link-lib=dylib=doomgeneric");
    // Embed an rpath so the dynamic loader finds the library at runtime.
    println!("cargo:rustc-link-arg=-Wl,-rpath,{}", lib_dir.display());
}
