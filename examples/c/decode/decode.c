#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "../../../jxl_c_api/include/jxl_api.h"

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input.jxl> <output.png>\n", argv[0]);
        return 1;
    }

    const char* input_path = argv[1];
    const char* output_path = argv[2];

    FILE* f = fopen(input_path, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open %s\n", input_path);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t* file_data = (uint8_t*)malloc(file_size);
    if (!file_data) {
        fprintf(stderr, "Failed to allocate memory\n");
        fclose(f);
        return 1;
    }
    fread(file_data, 1, file_size, f);
    fclose(f);

    OpaqueDecoder* dec = JxlDecoderCreate();
    if (!dec) {
        fprintf(stderr, "Failed to create decoder\n");
        free(file_data);
        return 1;
    }

    // 1. Process until we get Image Info
    JxlDecoderFeedInput(dec, file_data, file_size);
    
    int res = JxlDecoderProcessInput(dec);
    if (res != 1) {
        fprintf(stderr, "Failed to process image info (res=%d)\n", res);
        JxlDecoderDestroy(dec);
        free(file_data);
        return 1;
    }

    JxlCBasicInfo info;
    if (!JxlDecoderGetBasicInfo(dec, &info)) {
        fprintf(stderr, "Failed to get basic info\n");
        JxlDecoderDestroy(dec);
        free(file_data);
        return 1;
    }

    printf("Image size: %zu x %zu, bits_per_sample: %u, is_float: %d\n", 
            info.width, info.height, info.bits_per_sample, info.is_float);

    // 2. Request RGBA8
    if (!JxlDecoderRequestRGBA8(dec)) {
        fprintf(stderr, "Failed to request RGBA8 format\n");
        JxlDecoderDestroy(dec);
        free(file_data);
        return 1;
    }

    // 3. Process until Frame Info
    res = JxlDecoderProcessInput(dec);
    if (res != 1) {
        fprintf(stderr, "Failed to reach frame info (res=%d)\n", res);
        JxlDecoderDestroy(dec);
        free(file_data);
        return 1;
    }

    // 4. Extract frame pixels
    size_t pixels_size = info.width * info.height * 4;
    uint8_t* pixels = (uint8_t*)malloc(pixels_size);
    if (!pixels) {
        fprintf(stderr, "Failed to allocate pixels buffer\n");
        JxlDecoderDestroy(dec);
        free(file_data);
        return 1;
    }

    res = JxlDecoderProcessFrame(dec, pixels, pixels_size);
    if (res != 1 && res != 2) {
        fprintf(stderr, "Failed to process frame pixels (res=%d)\n", res);
        free(pixels);
        JxlDecoderDestroy(dec);
        free(file_data);
        return 1;
    }

    printf("Decoded frame. Writing PNG to %s...\n", output_path);

    if (!stbi_write_png(output_path, info.width, info.height, 4, pixels, info.width * 4)) {
        fprintf(stderr, "Failed to write PNG\n");
        free(pixels);
        JxlDecoderDestroy(dec);
        free(file_data);
        return 1;
    }

    printf("Done.\n");

    free(pixels);
    JxlDecoderDestroy(dec);
    free(file_data);
    return 0;
}
