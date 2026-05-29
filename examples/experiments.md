# Binary Size Reduction Experiments

To aggressively reduce the size of the statically compiled `decode_stream` executable without using a binary packer like `upx`, you can apply the following optimizations. These steps reduced the uncompressed static binary size from **2.62 MB** down to **1.92 MB**.

### 1. Disable Pre-compiled SIMD Kernels (Saves ~700KB)

By default, the `jxl-rs` crate compiles multiple separate versions of its image processing kernels for different CPU extensions (AVX2, AVX512, SSE4.2, etc.) so it can switch between them dynamically at runtime. Removing these drastically reduces the binary size.

Edit `jxl_c_api/Cargo.toml` to disable the default features of the `jxl` dependency:

```toml
# In jxl_c_api/Cargo.toml
[dependencies]
# Change this line to disable default features:
jxl = { path = "../jxl", default-features = false }
```

### 2. Aggressive C Compiler Flags (Saves ~100KB)

Switching your optimization flag from `-Os` to `-Oz` tells Clang to optimize *purely* for size, ignoring performance penalties. Enabling function and data sections, combined with linker garbage collection, ensures the linker drops any unreferenced code.

First, build the Rust library with the size-optimized profile:
```bash
cargo build --profile minsize -p jxl_c_api --target x86_64-unknown-linux-musl
```

Then, compile the C executable with extreme size optimizations:
```bash
cd examples/c/decode_stream
clang --target=x86_64-linux-musl -Oz -flto -ffunction-sections -fdata-sections -Wl,--gc-sections -static -s -o decode_stream_micro decode_stream.c \
  -I../../../jxl_c_api/include \
  -L../../../target/x86_64-unknown-linux-musl/minsize \
  -ljxl_c_api -lm -lpthread
```

### 3. Recompile the Rust Standard Library (Saves ~40KB)

Rust usually statically links a pre-compiled standard library which includes unneeded features like stack-unwinding mechanisms. If you use a Nightly Rust toolchain, you can compile the `std` library from scratch on the fly specifically tuned for your minimal profile.

First, prepare the nightly toolchain:
```bash
rustup toolchain install nightly
rustup component add rust-src --toolchain nightly
```

Next, build the Rust library using `build-std`:
```bash
cargo +nightly build --profile minsize -p jxl_c_api --target x86_64-unknown-linux-musl -Z build-std=std,panic_abort,core,alloc
```

*(Note: After doing this, re-run the `clang` command from step 2 to link the new, smaller standard library code into your final binary)*
