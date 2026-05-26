//
//  SDFGeneratorCore.cpp
//  SwiftSDF
//
//  Created by ZeroOneZeroR on 5/26/26.
//

#include "SDFGeneratorCore.h"
#include "PathConversion.h"
#include "SDFShapeAnalyzer.h"
#include "PathOps.h"
#include "msdfgen.h"
#include "msdfgen-ext.h"
#include <algorithm>
#include <cmath>
#include <cstring>

using namespace msdfgen;

namespace SDFGen {

// Compute projection to fit shape within bitmap with padding
Projection computeProjection(const Shape& shape, int bitmapWidth, int bitmapHeight, double padding, double& outScale) {
    Shape::Bounds bounds = shape.getBounds();
    double l = bounds.l, b = bounds.b, r = bounds.r, t = bounds.t;
    
    double activeWidth  = std::max(static_cast<double>(bitmapWidth)  - 2.0 * padding, 1.0);
    double activeHeight = std::max(static_cast<double>(bitmapHeight) - 2.0 * padding, 1.0);
    double shapeWidth   = r - l;
    double shapeHeight  = t - b;
    
    if (shapeWidth <= 0 || shapeHeight <= 0) {
        return Projection(1.0, Vector2(0, 0));
    }
    
    double scale = std::min(activeWidth / shapeWidth, activeHeight / shapeHeight);
    
    outScale = scale;
    
    double tx = -l + (bitmapWidth  / 2.0) / scale - shapeWidth  / 2.0;
    double ty = -b + (bitmapHeight / 2.0) / scale - shapeHeight / 2.0;
    
    return Projection(scale, Vector2(tx, ty));
}

// Core generation for MSDF (Actually MTSDF, 4 channels)
bool generateMSDFFromShape(Shape& shape, const GenerationConfig& config, bool edgeColoringNeeded, std::vector<float>& output) {
    // Edge coloring for MSDF
    if (edgeColoringNeeded) edgeColoringInkTrap(shape, config.angleThreshold);
    
    double projectionScale = 1.0;
    Projection projection = computeProjection(shape, config.width, config.height, config.padding, projectionScale);
    Bitmap<float, 4> bitmap(config.width, config.height);
    MSDFGeneratorConfig msdfConfig(!config.simplifyPath);
    
    // Convert pixel range to shape space range
    double shapeRange = config.pixelRange / projectionScale;
    
    generateMTSDF(bitmap, shape, projection, Range(shapeRange), msdfConfig);
    
    // Copy to output vector
    size_t pixelCount = config.width * config.height;
    output.resize(pixelCount * 4);
    std::memcpy(output.data(), bitmap, pixelCount * 4 * sizeof(float));
    return true;
}

// Core generation for SDF (1 channel)
bool generateSDFFromShape(const Shape& shape, const GenerationConfig& config, std::vector<float>& output) {
    // No edge coloring needed for SDF
    
    double projectionScale = 1.0;
    Projection projection = computeProjection(shape, config.width, config.height, config.padding, projectionScale);
    Bitmap<float, 1> bitmap(config.width, config.height);
    MSDFGeneratorConfig msdfConfig(!config.simplifyPath);
    
    // Convert pixel range to shape space range
    double shapeRange = config.pixelRange / projectionScale;
    
    generateSDF(bitmap, shape, projection, Range(shapeRange), msdfConfig);
    
    output.resize(config.width * config.height);
    std::memcpy(output.data(), bitmap, config.width * config.height * sizeof(float));
    return true;
}

ErrorCode generateFromPath(CGPathRef path,
                           const GenerationConfig& config,
                           GenerationMode mode,
                           std::vector<float>& output,
                           GenerationMode* outActualMode)
{
    // Validate inputs
    if (!path) return ErrorCode::NullPath;
    if (config.width <= 0 || config.height <= 0) return ErrorCode::InvalidSize;
    if (config.padding < 0.0 || config.padding >= std::min(config.width, config.height) / 2.0) {
        return ErrorCode::InvalidPadding;
    }
    
    // Convert CGPath to Shape (with optional simplification)
    CGPathRef sourcePath = path;
    CGMutablePathRef simplifiedPath = nullptr;
    if (config.simplifyPath) {
        simplifiedPath = PathOps::Simplify(path);
        if (simplifiedPath) sourcePath = simplifiedPath;
    }
    
    Shape shape = SDFGen::CGPathToShape(sourcePath);
    if (simplifiedPath) {
        CGPathRelease(simplifiedPath);
    }
    
    if (shape.contours.empty()) {
        return ErrorCode::EmptyShape;
    }
    
    // Shape ops
    shape.inverseYAxis = config.flipY;
    shape.validate();
    shape.normalize();
    shape.orientContours();
    
    // For Auto mode, we need edge coloring first (to detect corners)
    bool edgeColoringDone = false;
    GenerationMode actualMode = mode;
    if (mode == GenerationMode::Auto) {
        // Apply edge coloring for analysis
        edgeColoringInkTrap(shape, config.angleThreshold);
        edgeColoringDone = true;
        if (shouldPreferMultiChannel(shape)) {
            actualMode = GenerationMode::MSDF;
        } else {
            actualMode = GenerationMode::SDF;
        }
    }
    
    if (outActualMode) *outActualMode = actualMode;
    
    bool success = false;
    switch (actualMode) {
        case GenerationMode::SDF:
            success = generateSDFFromShape(shape, config, output);
            break;
        case GenerationMode::MSDF:
            success = generateMSDFFromShape(shape, config, !edgeColoringDone, output);
            break;
        default:
            return ErrorCode::UnsupportedMode;
    }
    
    return success ? ErrorCode::Success : ErrorCode::GenerationFailed;
}

} // namespace SDFGen
