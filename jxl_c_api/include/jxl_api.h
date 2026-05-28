#ifndef JXL_API_H
#define JXL_API_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JXL_SIG_INVALID 0
#define JXL_SIG_VALID 1
#define JXL_SIG_NOT_ENOUGH_BYTES 2

int32_t JxlCheckSignature(const uint8_t* buf, size_t len);

typedef struct OpaqueDecoder OpaqueDecoder;

typedef struct JxlCBasicInfo {
    size_t width;
    size_t height;
    uint32_t bits_per_sample;
    int32_t is_float;
    int32_t uses_original_profile;
} JxlCBasicInfo;

OpaqueDecoder* JxlDecoderCreate(void);
void JxlDecoderDestroy(OpaqueDecoder* dec);

void JxlDecoderFeedInput(OpaqueDecoder* dec, const uint8_t* data, size_t len);

int32_t JxlDecoderProcessInput(OpaqueDecoder* dec);

int32_t JxlDecoderRequestRGBA8(OpaqueDecoder* dec);

int32_t JxlDecoderProcessFrame(
    OpaqueDecoder* dec, 
    uint8_t* out_pixels, 
    size_t out_len
);

/**
 * Flushes partially decoded pixels into the output buffer.
 * Must be called in the WithFrameInfo state (when JxlDecoderProcessFrame returns 2).
 * Returns 1 if new pixels were drawn, 0 if nothing new, -1 on error.
 */
int32_t JxlDecoderFlushPixels(
    OpaqueDecoder* dec, 
    uint8_t* out_pixels, 
    size_t out_len
);

int32_t JxlDecoderGetBasicInfo(const OpaqueDecoder* dec, JxlCBasicInfo* info);

#ifdef __cplusplus
}
#endif

#endif // JXL_API_H
