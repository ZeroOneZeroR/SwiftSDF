//
//  SDFGeneratorCore.h
//  SwiftSDF
//
//  Created by ZeroOneZeroR on 5/26/26.
//

#ifndef SDFGeneratorCore_h
#define SDFGeneratorCore_h

#include <CoreGraphics/CoreGraphics.h>
#include <cstddef>
#include <vector>

namespace SDFGen {

// Generation mode
enum class GenerationMode {
    SDF,    // Single channel
    MSDF,   // Actually MTSDF, Four channels (RGBA)
    Auto    // Auto-select based on shape analysis
};

// Configuration for generation
struct GenerationConfig {
    int width = 0;
    int height = 0;
    double padding = 0.0;          // Should be >= pixelRange
    double pixelRange = 4.0;       // Distance field range in pixels
    bool flipY = false;            // Should flip the shape vertically
    double angleThreshold = 3.0;   // Edge coloring threshold (radians, msdfgen default)
    bool simplifyPath = false;     // Run Skia path simplification before generation
};

// Error codes
enum class ErrorCode {
    Success = 0,
    NullPath,
    NullOutput,
    InvalidSize,
    InvalidPadding,
    EmptyShape,
    GenerationFailed,
    UnsupportedMode
};

/**
 * Generate SDF or MSDF from a CGPath.
 *
 * @param path          CGPath to generate from.
 * @param config        Generation parameters.
 * @param mode          SDF, MSDF, or Auto.
 * @param output        Output buffer (will be resized automatically).
 *                      For SDF: width*height floats.
 *                      For MSDF: width*height*3 floats.
 * @param outActualMode Returns the mode actually used (for Auto).
 * @return ErrorCode::Success on success.
 */
ErrorCode generateFromPath(
                           CGPathRef path,
                           const GenerationConfig& config,
                           GenerationMode mode,
                           std::vector<float>& output,
                           GenerationMode* outActualMode = nullptr
                           );

} // namespace SDFGen

#endif /* SDFGeneratorCore_h */
