# jxl_c_api

This library provides C API bindings for the `jxl-rs` project.

## Compilation

To compile the library, you must have the Rust toolchain installed (including `cargo`).

From the root of the `jxl-rs` workspace or the `jxl_c_api` directory, run:

```sh
cargo build --release
```

This will produce the compiled library in the `target/release/` directory of the workspace.
The library is generated as both a shared library (e.g., `libjxl_c_api.so` on Linux, `libjxl_c_api.dylib` on macOS, `jxl_c_api.dll` on Windows) and a static library (e.g., `libjxl_c_api.a`).

To use it in your C/C++ projects, include the header files located in the `include/` directory and link against the produced library.
