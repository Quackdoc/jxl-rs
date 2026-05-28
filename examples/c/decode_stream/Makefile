CC = gcc
CFLAGS = -I../../../jxl_c_api/include -O2 -Wall
LDFLAGS = -L../../../target/release -ljxl_c_api -lpthread -ldl -lm

TARGET = decode_stream

all: $(TARGET)

$(TARGET): decode_stream.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGET)
