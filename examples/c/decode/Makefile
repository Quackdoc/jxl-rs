CC = gcc
CFLAGS = -I../../../jxl_c_api/include -O2 -Wall
LDFLAGS = -L../../../target/release -ljxl_c_api -lpthread -ldl -lm

# Depending on your platform and Rust version, you might need extra linked libraries for static compilation.
# For a cdylib/dynamic loading, using the shared object is easier.
# We will use the shared library or static depending on what cargo output.
# Rust compiles to libjxl_c_api.a and libjxl_c_api.so on Linux.

TARGET = decode

all: $(TARGET)

$(TARGET): decode.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGET)
