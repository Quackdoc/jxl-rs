#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "../../../jxl_c_api/include/jxl_api.h"

#define CHUNK_SIZE 1024

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <output.png> < input.jxl\n", argv[0]);
        return 1;
    }

    const char* output_path = argv[1];

    OpaqueDecoder* dec = JxlDecoderCreate();
    if (!dec) {
        fprintf(stderr, "Failed to create decoder\n");
        return 1;
    }

    uint8_t buffer[CHUNK_SIZE];
    int reached_image_info = 0;
    int res = 2; // Needs more input

    // 1. Process until we get Image Info (streaming)
    while (res == 2) {
        size_t bytes_read = fread(buffer, 1, CHUNK_SIZE, stdin);
        if (bytes_read > 0) {
            JxlDecoderFeedInput(dec, buffer, bytes_read);
        } else if (feof(stdin)) {
            fprintf(stderr, "Unexpected end of stream while looking for ImageInfo\n");
            break;
        } else {
            fprintf(stderr, "Stream read error\n");
            break;
        }

        res = JxlDecoderProcessInput(dec);
        if (res == 1) {
            reached_image_info = 1;
            break;
        } else if (res == -1) {
            fprintf(stderr, "Decoding error\n");
            break;
        }
    }

    if (!reached_image_info) {
        JxlDecoderDestroy(dec);
        return 1;
    }

    JxlCBasicInfo info;
    if (!JxlDecoderGetBasicInfo(dec, &info)) {
        fprintf(stderr, "Failed to get basic info\n");
        JxlDecoderDestroy(dec);
        return 1;
    }

    printf("Image size: %zu x %zu\n", info.width, info.height);

    // 2. Request RGBA8
    if (!JxlDecoderRequestRGBA8(dec)) {
        fprintf(stderr, "Failed to request RGBA8 format\n");
        JxlDecoderDestroy(dec);
        return 1;
    }

    // 3. Process until Frame Info
    int reached_frame_info = 0;
    res = JxlDecoderProcessInput(dec); // Attempt with currently buffered data
    
    while (res == 2) {
        size_t bytes_read = fread(buffer, 1, CHUNK_SIZE, stdin);
        if (bytes_read > 0) {
            JxlDecoderFeedInput(dec, buffer, bytes_read);
        } else if (feof(stdin)) {
            fprintf(stderr, "Unexpected end of stream while looking for FrameInfo\n");
            break;
        }
        res = JxlDecoderProcessInput(dec);
    }
    
    if (res == 1) {
        reached_frame_info = 1;
    }

    if (!reached_frame_info) {
        fprintf(stderr, "Failed to reach frame info\n");
        JxlDecoderDestroy(dec);
        return 1;
    }

    // 4. Extract frame pixels and progressively render
    size_t pixels_size = info.width * info.height * 4;
    uint8_t* pixels = (uint8_t*)malloc(pixels_size);
    if (!pixels) {
        fprintf(stderr, "Failed to allocate pixels buffer\n");
        JxlDecoderDestroy(dec);
        return 1;
    }

    // Initialize pixels to transparent or black so intermediate PNGs look clean
    memset(pixels, 0, pixels_size);

    int update_count = 0;

    res = JxlDecoderProcessFrame(dec, pixels, pixels_size);
    while (res == 2) {
        // Attempt to flush partially decoded pixels
        if (JxlDecoderFlushPixels(dec, pixels, pixels_size) == 1) {
            update_count++;
            printf("Progressive update %d. Writing PNG to %s...\n", update_count, output_path);
            stbi_write_png(output_path, info.width, info.height, 4, pixels, info.width * 4);
        }

        size_t bytes_read = fread(buffer, 1, CHUNK_SIZE, stdin);
        if (bytes_read > 0) {
            JxlDecoderFeedInput(dec, buffer, bytes_read);
        } else if (feof(stdin)) {
            break; // Try one last time or break
        }
        res = JxlDecoderProcessFrame(dec, pixels, pixels_size);
    }

    if (res != 1) {
        fprintf(stderr, "Failed to process full frame pixels (res=%d)\n", res);
        free(pixels);
        JxlDecoderDestroy(dec);
        return 1;
    }

    printf("Final frame complete. Writing PNG to %s...\n", output_path);
    if (!stbi_write_png(output_path, info.width, info.height, 4, pixels, info.width * 4)) {
        fprintf(stderr, "Failed to write final PNG\n");
        free(pixels);
        JxlDecoderDestroy(dec);
        return 1;
    }

    printf("Done streaming decode.\n");

    free(pixels);
    JxlDecoderDestroy(dec);
    return 0;
}
