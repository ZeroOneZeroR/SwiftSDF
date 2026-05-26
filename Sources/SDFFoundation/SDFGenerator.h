//
//  SDFGenerator.h
//  SwiftSDF
//
//  Created by ZeroOneZeroR on 5/26/26.
//

#ifndef SDFGenerator_h
#define SDFGenerator_h

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>

NS_ASSUME_NONNULL_BEGIN

// Error Domain and Codes matching the validation states
extern NSString * const SDFGeneratorErrorDomain;

typedef NS_ENUM(NSInteger, SDFGeneratorError) {
    SDFGeneratorErrorInvalidSize = 1001,
    SDFGeneratorErrorInvalidPadding = 1002,
    SDFGeneratorErrorInvalidChannelFormatForMSDF = 1003,
    SDFGeneratorErrorInternalFailure = 1004
};

typedef NS_CLOSED_ENUM(NSInteger, SDFRequestMode) {
    SDFRequestModeSdf,
    SDFRequestModeMsdf,
    SDFRequestModeAuto
} NS_SWIFT_NAME(SDFRequestMode);

typedef NS_CLOSED_ENUM(NSInteger, SDFMode) {
    SDFModeSdf      NS_SWIFT_NAME(sdf),
    SDFModeMsdf     NS_SWIFT_NAME(msdf)
} NS_SWIFT_NAME(SDFMode);

typedef NS_CLOSED_ENUM(NSInteger, SDFChannelFormat) {
    SDFChannelFormatR       NS_SWIFT_NAME(r),
    SDFChannelFormatRgba    NS_SWIFT_NAME(rgba)
} NS_SWIFT_NAME(SDFChannelFormat);

typedef NS_CLOSED_ENUM(NSInteger, SDFPrecision) {
    SDFPrecisionUnorm8      NS_SWIFT_NAME(unorm8),
    SDFPrecisionFloat16     NS_SWIFT_NAME(float16)
} NS_SWIFT_NAME(SDFPrecision);

NS_INLINE NSInteger SDFGetBytesPerChannel(SDFPrecision precision) NS_SWIFT_NAME(getter:SDFPrecision.bytesPerChannel(self:)) {
    switch (precision) {
        case SDFPrecisionUnorm8:  return 1;
        case SDFPrecisionFloat16: return 2;
    }
}

NS_INLINE NSInteger SDFGetChannelCount(SDFChannelFormat format) NS_SWIFT_NAME(getter:SDFChannelFormat.channelCount(self:)) {
    switch (format) {
        case SDFChannelFormatR:    return 1;
        case SDFChannelFormatRgba: return 4;
    }
}

NS_INLINE NSInteger SDFGetBytesPerPixel(SDFChannelFormat format, SDFPrecision precision) NS_SWIFT_NAME(SDFChannelFormat.bytesPerPixel(self:precision:)) {
    return SDFGetChannelCount(format) * SDFGetBytesPerChannel(precision);
}


NS_SWIFT_NAME(SDFConfiguration)
@interface SDFConfiguration : NSObject

@property (nonatomic, assign) NSInteger outputWidth;
@property (nonatomic, assign) NSInteger outputHeight;
@property (nonatomic, assign) CGFloat padding;
@property (nonatomic, assign) CGFloat range;
@property (nonatomic, assign) SDFPrecision precision;
@property (nonatomic, assign) BOOL flipY;

// CPU-Backend specific properties (Defaults: angleThreshold = 3.0, simplifyPath = NO)
@property (nonatomic, assign) CGFloat angleThreshold;
@property (nonatomic, assign) BOOL simplifyPath;

/// Designated Initializer - Exposes every configuration option
- (instancetype)initWithOutputWidth:(NSInteger)outputWidth
                       outputHeight:(NSInteger)outputHeight
                            padding:(CGFloat)padding
                              range:(CGFloat)range
                          precision:(SDFPrecision)precision
                              flipY:(BOOL)flipY
                     angleThreshold:(CGFloat)angleThreshold
                       simplifyPath:(BOOL)simplifyPath;

/// Convenience Initializer - Automatically applies defaults for angleThreshold (3.0)
- (instancetype)initWithOutputWidth:(NSInteger)outputWidth
                       outputHeight:(NSInteger)outputHeight
                            padding:(CGFloat)padding
                              range:(CGFloat)range
                          precision:(SDFPrecision)precision
                              flipY:(BOOL)flipY
                       simplifyPath:(BOOL)simplifyPath;

/// Convenience Initializer - Automatically applies defaults for angleThreshold (3.0) and simplifyPath (YES)
- (instancetype)initWithOutputWidth:(NSInteger)outputWidth
                       outputHeight:(NSInteger)outputHeight
                            padding:(CGFloat)padding
                              range:(CGFloat)range
                          precision:(SDFPrecision)precision
                              flipY:(BOOL)flipY;
@end

NS_SWIFT_NAME(SDFResult)
@interface SDFResult : NSObject

@property (nonatomic, strong, readonly) NSData *data;
@property (nonatomic, assign, readonly) SDFMode sdfMode;
@property (nonatomic, assign, readonly) SDFChannelFormat channelFormat;
@property (nonatomic, assign, readonly) SDFPrecision precision;

- (instancetype)initWithData:(NSData *)data
                     sdfMode:(SDFMode)mode
               channelFormat:(SDFChannelFormat)channelFormat
                   precision:(SDFPrecision)precision NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;
@end

NS_SWIFT_NAME(SDFGenerator)
@interface SDFGenerator : NSObject

/// Generates a Distance Field from a CGPath. Completely validates inputs and handles format resolution natively.
+ (nullable SDFResult *)generateFromPath:(CGPathRef _Nonnull)path
                              requestMode:(SDFRequestMode)requestMode
                                  config:(SDFConfiguration *)config
                                   error:(NSError **)error;

@end

NS_ASSUME_NONNULL_END

#endif /* SDFGenerator_h */
