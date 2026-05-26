//
//  SDFGenerator.m
//  SwiftSDF
//
//  Created by ZeroOneZeroR on 5/26/26.
//

#import "SDFGenerator.h"
#import <SDFCore.h>
#import <algorithm>
#import <vector>

NSString * const SDFGeneratorErrorDomain = @"com.sdf.SDFGeneratorErrorDomain";

// Secure compilation for __fp16 or _Float16 across Apple Silicon and Intel architectures
#if defined(__arm64__) || defined(__aarch64__)
typedef __fp16 float16_t;
#else
typedef _Float16 float16_t;
#endif

@implementation SDFConfiguration

// The Designated Initializer
- (instancetype)initWithOutputWidth:(NSInteger)outputWidth
                       outputHeight:(NSInteger)outputHeight
                            padding:(CGFloat)padding
                              range:(CGFloat)range
                          precision:(SDFPrecision)precision
                              flipY:(BOOL)flipY
                     angleThreshold:(CGFloat)angleThreshold
                       simplifyPath:(BOOL)simplifyPath {
    
    self = [super init];
    if (self) {
        _outputWidth = outputWidth;
        _outputHeight = outputHeight;
        _padding = padding;
        _range = range;
        _precision = precision;
        _flipY = flipY;
        _angleThreshold = angleThreshold;
        _simplifyPath = simplifyPath;
    }
    return self;
}

// The Convenience Initializer (hides angleThreshold)
- (instancetype)initWithOutputWidth:(NSInteger)outputWidth
                       outputHeight:(NSInteger)outputHeight
                            padding:(CGFloat)padding
                              range:(CGFloat)range
                          precision:(SDFPrecision)precision
                              flipY:(BOOL)flipY
                       simplifyPath:(BOOL)simplifyPath {
    
    return [self initWithOutputWidth:outputWidth
                        outputHeight:outputHeight
                             padding:padding
                               range:range
                           precision:precision
                               flipY:(BOOL)flipY
                      angleThreshold:3.0
                        simplifyPath:simplifyPath];
}

// The Convenience Initializer (hides advanced settings, applying defaults)
- (instancetype)initWithOutputWidth:(NSInteger)outputWidth
                       outputHeight:(NSInteger)outputHeight
                            padding:(CGFloat)padding
                              range:(CGFloat)range
                          precision:(SDFPrecision)precision
                              flipY:(BOOL)flipY {
    
    return [self initWithOutputWidth:outputWidth
                        outputHeight:outputHeight
                             padding:padding
                               range:range
                           precision:precision
                               flipY:(BOOL)flipY
                      angleThreshold:3.0
                        simplifyPath:YES];
}
@end

@implementation SDFResult

- (instancetype)initWithData:(NSData *)data
                     sdfMode:(SDFMode)mode
               channelFormat:(SDFChannelFormat)channelFormat
                   precision:(SDFPrecision)precision {
    self = [super init];
    if (self) {
        _data = data;
        _sdfMode = mode;
        _channelFormat = channelFormat;
        _precision = precision;
    }
    return self;
}
@end

@implementation SDFGenerator

+ (nullable SDFResult *)generateFromPath:(CGPathRef)path
                             requestMode:(SDFRequestMode)requestMode
                                  config:(SDFConfiguration *)config
                                   error:(NSError **)error {
    // Validation Logic
    if (config.outputWidth <= 0 || config.outputHeight <= 0) {
        if (error) {
            *error = [NSError errorWithDomain:SDFGeneratorErrorDomain
                                         code:SDFGeneratorErrorInvalidSize
                                     userInfo:@{NSLocalizedDescriptionKey: @"outputWidth and outputHeight must be greater than 0."}];
        }
        return nil;
    }
    
    CGFloat maxAllowedPadding = (CGFloat)std::min(config.outputWidth, config.outputHeight) / 2.0;
    if (config.padding < 0.0 || config.padding >= maxAllowedPadding) {
        if (error) {
            *error = [NSError errorWithDomain:SDFGeneratorErrorDomain
                                         code:SDFGeneratorErrorInvalidPadding
                                     userInfo:@{NSLocalizedDescriptionKey: @"Padding must be >= 0 and less than half of the minimum output dimension."}];
        }
        return nil;
    }
    
    // Map Configuration parameters to the underlying C++ Core
    SDFGen::GenerationConfig cppConfig;
    cppConfig.width = (int)config.outputWidth;
    cppConfig.height = (int)config.outputHeight;
    cppConfig.padding = (double)config.padding;
    cppConfig.pixelRange = (double)config.range;
    cppConfig.flipY = (bool)config.flipY;
    cppConfig.angleThreshold = (double)config.angleThreshold;
    cppConfig.simplifyPath = (bool)config.simplifyPath;
    
    SDFGen::GenerationMode cppMode;
    switch (requestMode) {
        case SDFRequestModeSdf:  cppMode = SDFGen::GenerationMode::SDF;  break;
        case SDFRequestModeMsdf: cppMode = SDFGen::GenerationMode::MSDF; break;
        case SDFRequestModeAuto: cppMode = SDFGen::GenerationMode::Auto; break;
    }
    
    std::vector<float> cppOutput;
    SDFGen::GenerationMode actualCppMode;
    
    // Fire C++ Core Generator
    SDFGen::ErrorCode coreErr = SDFGen::generateFromPath(path, cppConfig, cppMode, cppOutput, &actualCppMode);
    
    if (coreErr != SDFGen::ErrorCode::Success) {
        if (error) {
            *error = [NSError errorWithDomain:SDFGeneratorErrorDomain
                                         code:SDFGeneratorErrorInternalFailure
                                     userInfo:@{NSLocalizedDescriptionKey: [NSString stringWithFormat:@"Internal C++ generation failure. Core Error Code: %d", (int)coreErr]}];
        }
        return nil;
    }
    
    SDFMode finalSdfMode = (actualCppMode == SDFGen::GenerationMode::SDF) ? SDFModeSdf : SDFModeMsdf;
    SDFChannelFormat channelFormat = (finalSdfMode == SDFModeSdf) ? SDFChannelFormatR : SDFChannelFormatRgba;
    
    // Blazing-fast In-Memory Memory Reshaping Loop
    size_t pixelCount = (size_t)cppConfig.width * (size_t)cppConfig.height;
    //size_t targetChannels = SDFGetChannelCount(channelFormat);
    //size_t bytesPerChannel = SDFGetBytesPerChannel(config.precision);
    size_t totalBytes = pixelCount * SDFGetBytesPerPixel(channelFormat, config.precision);
    
    NSMutableData *packedData = [NSMutableData dataWithLength:totalBytes];
    const float *srcBuffer = cppOutput.data();
    
    if (config.precision == SDFPrecisionUnorm8) {
        uint8_t *dstBuffer = (uint8_t *)packedData.mutableBytes;
        
        if (finalSdfMode == SDFModeSdf) {
            for (size_t i = 0; i < pixelCount; ++i) {
                dstBuffer[i] = (uint8_t)(std::clamp(srcBuffer[i], 0.0f, 1.0f) * 255.0f + 0.5f);
            }
        } else {
            for (size_t i = 0; i < pixelCount; ++i) {
                size_t srcOffset = i * 4;
                size_t dstOffset = i * 4;
                dstBuffer[dstOffset + 0] = (uint8_t)(std::clamp(srcBuffer[srcOffset + 0], 0.0f, 1.0f) * 255.0f + 0.5f);
                dstBuffer[dstOffset + 1] = (uint8_t)(std::clamp(srcBuffer[srcOffset + 1], 0.0f, 1.0f) * 255.0f + 0.5f);
                dstBuffer[dstOffset + 2] = (uint8_t)(std::clamp(srcBuffer[srcOffset + 2], 0.0f, 1.0f) * 255.0f + 0.5f);
                dstBuffer[dstOffset + 3] = (uint8_t)(std::clamp(srcBuffer[srcOffset + 3], 0.0f, 1.0f) * 255.0f + 0.5f);
            }
        }
        
    } else if (config.precision == SDFPrecisionFloat16) {
        float16_t *dstBuffer = (float16_t *)packedData.mutableBytes;
        
        if (finalSdfMode == SDFModeSdf) {
            for (size_t i = 0; i < pixelCount; ++i) {
                dstBuffer[i] = (float16_t)srcBuffer[i];
            }
        } else {
            for (size_t i = 0; i < pixelCount; ++i) {
                size_t srcOffset = i * 4;
                size_t dstOffset = i * 4;
                dstBuffer[dstOffset + 0] = (float16_t)srcBuffer[srcOffset + 0];
                dstBuffer[dstOffset + 1] = (float16_t)srcBuffer[srcOffset + 1];
                dstBuffer[dstOffset + 2] = (float16_t)srcBuffer[srcOffset + 2];
                dstBuffer[dstOffset + 3] = (float16_t)srcBuffer[srcOffset + 3];
            }
        }
    }
    
    return [[SDFResult alloc] initWithData:packedData
                                   sdfMode:finalSdfMode
                             channelFormat:channelFormat
                                 precision:config.precision];
}

@end
