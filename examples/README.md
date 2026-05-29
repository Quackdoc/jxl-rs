# Examples

This directory contains examples demonstrating how to use the `jxl-rs` library.

## C API Examples

The C examples are located in the `c/` directory and demonstrate how to use the `jxl_c_api` bindings.

### Prerequisites

Before compiling the examples, you must compile the `jxl_c_api` library in release mode:

```sh
# From the root of the workspace
cargo build --release -p jxl_c_api
```

### Compiling and Running C Examples

You can use a C compiler such as `gcc` or `clang`. You must specify the include path for the C API headers and the library path for the compiled Rust library.

#### `decode` Example

To compile the `decode` example (using `gcc` on Linux as an example):

```sh
cd c/decode
gcc -O3 -o decode decode.c -I../../../jxl_c_api/include -L../../../target/release -ljxl_c_api -lm -lpthread -ldl
```

or simply run `make`

To run the example, make sure the shared library can be found by the dynamic linker:

```sh
LD_LIBRARY_PATH=../../../target/release/ ./decode ../../../jxl/resources/test/conformance_test_images/spot.jxl output.png
```

#### `decode_stream` Example

To compile the `decode_stream` example:

```sh
cd c/decode_stream
gcc -O3 -o decode_stream decode_stream.c -I../../../jxl_c_api/include -L../../../target/release -ljxl_c_api -lm -lpthread -ldl
```

to compile with musl:
```sh
cd c/decode_stream
clang --target=x86_64-linux-musl -O3 -static -o decode_stream_static decode_stream.c \
  -I../../../jxl_c_api/include \
  -L../../../target/x86_64-unknown-linux-musl/release \
  -ljxl_c_api -lm -lpthread
```

or simply run `make`

To run it:

```sh
LD_LIBRARY_PATH=../../../target/release ./decode_stream output.png < ../../../jxl/resources/test/conformance_test_images/spot.jxl
```
