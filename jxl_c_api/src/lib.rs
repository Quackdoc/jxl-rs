use std::slice;
use jxl::api::{check_signature, ProcessingResult, JxlDecoder, JxlDecoderOptions, JxlBitDepth, JxlBasicInfo, JxlPixelFormat, JxlOutputBuffer, JxlProgressiveMode};
use jxl::api::states::{Initialized, WithImageInfo, WithFrameInfo};

/// Return values for `JxlCheckSignature`.
pub const JXL_SIG_INVALID: i32 = 0;
pub const JXL_SIG_VALID: i32 = 1;
pub const JXL_SIG_NOT_ENOUGH_BYTES: i32 = 2;

#[no_mangle]
pub extern "C" fn JxlCheckSignature(buf: *const u8, len: usize) -> i32 {
    if buf.is_null() && len > 0 {
        return JXL_SIG_INVALID;
    }
    let slice = unsafe { slice::from_raw_parts(buf, len) };
    match check_signature(slice) {
        ProcessingResult::Complete { result: Some(_) } => JXL_SIG_VALID,
        ProcessingResult::Complete { result: None } => JXL_SIG_INVALID,
        ProcessingResult::NeedsMoreInput { .. } => JXL_SIG_NOT_ENOUGH_BYTES,
    }
}

pub enum DecoderState {
    Initialized(JxlDecoder<Initialized>),
    WithImageInfo(JxlDecoder<WithImageInfo>),
    WithFrameInfo(JxlDecoder<WithFrameInfo>, JxlBasicInfo),
    Consumed,
}

pub struct OpaqueDecoder {
    state: DecoderState,
    buffer: Vec<u8>,
}

#[no_mangle]
pub extern "C" fn JxlDecoderCreate() -> *mut OpaqueDecoder {
    let mut options = JxlDecoderOptions::default();
    options.progressive_mode = JxlProgressiveMode::Eager; // Good for streaming progress
    let decoder = JxlDecoder::new(options);
    let opaque = Box::new(OpaqueDecoder {
        state: DecoderState::Initialized(decoder),
        buffer: Vec::new(),
    });
    Box::into_raw(opaque)
}

#[no_mangle]
pub extern "C" fn JxlDecoderDestroy(dec: *mut OpaqueDecoder) {
    if !dec.is_null() {
        unsafe { drop(Box::from_raw(dec)); }
    }
}

#[no_mangle]
pub extern "C" fn JxlDecoderFeedInput(dec: *mut OpaqueDecoder, data: *const u8, len: usize) {
    if dec.is_null() || data.is_null() || len == 0 { return; }
    let opaque = unsafe { &mut *dec };
    let input_slice = unsafe { slice::from_raw_parts(data, len) };
    opaque.buffer.extend_from_slice(input_slice);
}

#[no_mangle]
pub extern "C" fn JxlDecoderRequestRGBA8(dec: *mut OpaqueDecoder) -> i32 {
    if dec.is_null() { return 0; }
    let opaque = unsafe { &mut *dec };
    if let DecoderState::WithImageInfo(ref mut decoder) = opaque.state {
        let num_extra = decoder.basic_info().extra_channels.len();
        let fmt = JxlPixelFormat::rgba8(num_extra);
        decoder.set_pixel_format(fmt);
        return 1;
    }
    0 // Invalid state
}

#[no_mangle]
pub extern "C" fn JxlDecoderProcessInput(dec: *mut OpaqueDecoder) -> i32 {
    if dec.is_null() { return -1; }
    let opaque = unsafe { &mut *dec };
    
    let current = std::mem::replace(&mut opaque.state, DecoderState::Consumed);
    let res;
    
    let mut input_slice = opaque.buffer.as_slice();
    let initial_len = input_slice.len();
    
    match current {
        DecoderState::Initialized(decoder) => {
            match decoder.process(&mut input_slice) {
                Ok(ProcessingResult::Complete { result }) => {
                    opaque.state = DecoderState::WithImageInfo(result);
                    res = 1;
                }
                Ok(ProcessingResult::NeedsMoreInput { fallback, .. }) => {
                    opaque.state = DecoderState::Initialized(fallback);
                    res = 2;
                }
                Err(_) => { res = -1; }
            }
        }
        DecoderState::WithImageInfo(decoder) => {
            let basic_info = decoder.basic_info().clone();
            match decoder.process(&mut input_slice) {
                Ok(ProcessingResult::Complete { result }) => {
                    opaque.state = DecoderState::WithFrameInfo(result, basic_info);
                    res = 1;
                }
                Ok(ProcessingResult::NeedsMoreInput { fallback, .. }) => {
                    opaque.state = DecoderState::WithImageInfo(fallback);
                    res = 2;
                }
                Err(_) => { res = -1; }
            }
        }
        DecoderState::WithFrameInfo(decoder, info) => {
            opaque.state = DecoderState::WithFrameInfo(decoder, info);
            res = 1; 
        }
        DecoderState::Consumed => { res = -1; }
    }
    
    let consumed = initial_len - input_slice.len();
    opaque.buffer.drain(..consumed);
    res
}

#[no_mangle]
pub extern "C" fn JxlDecoderProcessFrame(
    dec: *mut OpaqueDecoder, 
    out_pixels: *mut u8, 
    out_len: usize
) -> i32 {
    if dec.is_null() || out_pixels.is_null() { return -1; }
    let opaque = unsafe { &mut *dec };
    
    let current = std::mem::replace(&mut opaque.state, DecoderState::Consumed);
    let res;
    
    let mut input_slice = opaque.buffer.as_slice();
    let initial_len = input_slice.len();
    
    if let DecoderState::WithFrameInfo(decoder, info) = current {
        let expected_len = info.size.0 * info.size.1 * 4;
        if out_len < expected_len {
            opaque.state = DecoderState::WithFrameInfo(decoder, info);
            res = -1;
        } else {
            let out_slice = unsafe { slice::from_raw_parts_mut(out_pixels, expected_len) };
            let buffer = JxlOutputBuffer::new(out_slice, info.size.1, info.size.0 * 4);
            let mut buffers = [buffer];
            
            match decoder.process(&mut input_slice, &mut buffers) {
                Ok(ProcessingResult::Complete { result }) => {
                    opaque.state = DecoderState::WithImageInfo(result);
                    res = 1;
                }
                Ok(ProcessingResult::NeedsMoreInput { fallback, .. }) => {
                    opaque.state = DecoderState::WithFrameInfo(fallback, info);
                    res = 2;
                }
                Err(_) => { res = -1; }
            }
        }
    } else {
        opaque.state = current;
        res = -1;
    }

    let consumed = initial_len - input_slice.len();
    opaque.buffer.drain(..consumed);
    res
}

#[no_mangle]
pub extern "C" fn JxlDecoderFlushPixels(
    dec: *mut OpaqueDecoder, 
    out_pixels: *mut u8, 
    out_len: usize
) -> i32 {
    if dec.is_null() || out_pixels.is_null() { return -1; }
    let opaque = unsafe { &mut *dec };
    
    if let DecoderState::WithFrameInfo(ref mut decoder, ref info) = opaque.state {
        let expected_len = info.size.0 * info.size.1 * 4;
        if out_len < expected_len {
            return -1;
        }
        
        let out_slice = unsafe { slice::from_raw_parts_mut(out_pixels, expected_len) };
        let buffer = JxlOutputBuffer::new(out_slice, info.size.1, info.size.0 * 4);
        let mut buffers = [buffer];
        
        match decoder.flush_pixels(&mut buffers) {
            Ok(true) => return 1,  // New pixels written
            Ok(false) => return 0, // No new pixels
            Err(_) => return -1,
        }
    }
    
    0
}

#[repr(C)]
pub struct JxlCBasicInfo {
    pub width: usize,
    pub height: usize,
    pub bits_per_sample: u32,
    pub is_float: i32,
    pub uses_original_profile: i32,
}

#[no_mangle]
pub extern "C" fn JxlDecoderGetBasicInfo(dec: *const OpaqueDecoder, info: *mut JxlCBasicInfo) -> i32 {
    if dec.is_null() || info.is_null() { return 0; }
    let opaque = unsafe { &*dec };
    
    let basic_info = match &opaque.state {
        DecoderState::WithImageInfo(decoder) => decoder.basic_info(),
        DecoderState::WithFrameInfo(_, info) => info,
        _ => return 0,
    };
    
    let (is_float, bps) = match &basic_info.bit_depth {
        JxlBitDepth::Int { bits_per_sample } => (0, *bits_per_sample),
        JxlBitDepth::Float { bits_per_sample, .. } => (1, *bits_per_sample),
    };

    let c_info = JxlCBasicInfo {
        width: basic_info.size.0,
        height: basic_info.size.1,
        bits_per_sample: bps,
        is_float,
        uses_original_profile: if basic_info.uses_original_profile { 1 } else { 0 },
    };
    
    unsafe { *info = c_info };
    1
}
