//
//  SDFShapeAnalyzer.cpp
//  SwiftSDF
//
//  Created by ZeroOneZeroR on 5/26/26.
//

#include "SDFShapeAnalyzer.h"
#include <cmath>

namespace SDFGen {

/// Minimum tangent-direction change (degrees) for a junction to be
/// classified as a "sharp" corner for the purpose of the SDF/MSDF decision.
///
/// Rationale for 45°:
///   • At angles below ≈ 45° the SDF rounding radius at typical atlas
///     resolutions (32–64 px/em) is sub-pixel and perceptually invisible.
///   • At 45° and above the rounding becomes visible as a blunted corner,
///     which MSDF corrects cleanly.
///   • This threshold is independent of msdfgen's coloring threshold (≈ 8°),
///     which is deliberately over-sensitive for rendering quality.
///
/// Increase toward 60–70° to reduce MSDF usage further; decrease toward
/// 30° to catch subtler corners (larger atlas cells, better fidelity).
constexpr double kCornerAngleThresholdDegrees = 45.0;

/// Maximum total segment count across all contours for a glyph to be
/// eligible for MSDF.
///
/// Typical segment counts by font class:
///   Simple glyphs ('I', 'l', '-')        :  4 – 10
///   Standard Latin ('H', 'O', 'g', 'B')  :  8 – 30
///   Complex / CJK                         : 20 – 48
///   Artistic / ornamental / script        : 50 – 300+
///
/// 48 sits at the upper bound of ordinary typographic complexity.
constexpr uint32_t kComplexShapeSegmentThreshold = 48;

/// Maximum contour count for MSDF eligibility.
///
/// Most Latin/CJK glyphs have 1–4 contours (outer shell + counter-forms,
/// e.g. the two holes in 'B', the bowl and leg of 'p').  Artistic glyphs
/// (swashes, ornaments, inline-stroked display fonts) often have 6–20.
/// 5 contours is the boundary: it accommodates the most complex standard
/// glyphs ('æ', '&', '€', '¥') while excluding decorative outliers.
constexpr uint32_t kComplexShapeContourThreshold = 5;

inline float toRadians(double degrees) {
    return static_cast<float>(degrees * M_PI / 180.0);
}

/**
 * Returns true when the shape is too geometrically complex for MSDF to
 * produce reliable results at typical atlas resolutions.
 *
 * Checks two independent metrics; either one triggers the "complex" verdict:
 *
 *   (a) Contour count > kComplexShapeContourThreshold
 *       Many sub-paths indicate an artistic or layered glyph design that
 *       is structurally outside the scope of ordinary typographic MSDF.
 *
 *   (b) Total segment count > kComplexShapeSegmentThreshold
 *       High segment density means edges are closer together than one atlas
 *       texel, at which point msdfgen's channel-assignment breaks down and
 *       produces cross-channel interference (fringe / halo artefacts).
 *
 * Both checks use early-out so the function is O(1) in the common case
 * where the contour count already exceeds the threshold.
 */
bool isShapeTooComplex(const msdfgen::Shape& shape) {
    uint32_t contourCount = static_cast<uint32_t>(shape.contours.size());
    
    // Fast path: contour count alone can settle the question.
    if (contourCount > kComplexShapeContourThreshold) return true;
    
    uint32_t totalSegments = 0;
    for (const auto& contour : shape.contours) {
        totalSegments += static_cast<uint32_t>(contour.edges.size());
        if (totalSegments > kComplexShapeSegmentThreshold) return true;
    }
    return false;
}

/**
 * Returns true when the shape contains at least one corner junction whose
 * tangent-direction change exceeds kCornerAngleThresholdDegrees.
 *
 * Two-stage gate per junction:
 *
 *   Stage 1 — Topological (O(1), branch-free):
 *     msdfgen assigned different colours to the two edges meeting at this
 *     junction.  If colours are equal the junction is smooth by msdfgen's
 *     own definition and we skip it immediately — no vector math needed.
 *     Contours with a single edge (assigned WHITE throughout) never have
 *     a colour transition and are skipped at the loop level.
 *
 *   Stage 2 — Geometric (sqrt + dot product):
 *     Normalise the outgoing tangent of edge i and the incoming tangent
 *     of edge i+1.  Their dot product equals cos(θ) where θ is the
 *     tangent-change angle.
 *
 *       cosAngle < cos(threshold)  ⟺  θ > threshold
 *
 *     Using the cosine comparison avoids atan2 and keeps the hot path
 *     to a single branch.
 *
 * Returns true as soon as the first qualifying corner is found; the
 * function does not need to count all corners.
 */
bool hasSignificantSharpCorners(const msdfgen::Shape& shape) {
    // Pre-compute once; std::cos is not constexpr for all compilers.
    const float cosThreshold = std::cos(toRadians(kCornerAngleThresholdDegrees));
    
    for (const auto& contour : shape.contours) {
        const size_t n = contour.edges.size();
        if (n < 2) continue;  // Single-edge contour → no junction possible.
        
        for (size_t i = 0; i < n; ++i) {
            const auto& e1 = contour.edges[i];
            const auto& e2 = contour.edges[(i + 1) % n];
            
            // ── Stage 1: topological gate ─────────────────────────────────
            // WHITE == WHITE is the most common case (smooth fonts); skip
            // immediately to avoid the more expensive geometric check.
            if (e1->color == e2->color) continue;
            
            // ── Stage 2: geometric gate ───────────────────────────────────
            // direction(1) = arriving tangent  at the end   of edge i   (path enters junction)
            // direction(0) = departing tangent at the start of edge i+1 (path leaves junction)
            // Both are normalised to unit length for a clean dot product.
            const msdfgen::Vector2 outDir = e1->direction(1).normalize();
            const msdfgen::Vector2 inDir  = e2->direction(0).normalize();
            
            // Guard against degenerate (zero-length) edges.
            const double outLen2 = outDir.x * outDir.x + outDir.y * outDir.y;
            const double inLen2  = inDir.x  * inDir.x  + inDir.y  * inDir.y;
            if (outLen2 < 1e-10 || inLen2 < 1e-10) continue;  // degenerate edge, skip safely
            
            const float cosAngle = static_cast<float>(
                                                      outDir.x * inDir.x + outDir.y * inDir.y
                                                      );
            
            // cosAngle < cosThreshold  ⟺  angle > kCornerAngleThresholdDegrees
            if (cosAngle < cosThreshold) return true;
        }
    }
    return false;
}

/**
 * Master predicate: returns true when the glyph should use MSDF.
 *
 * The decision is a logical AND of two independent criteria:
 *
 *   (a) NOT isShapeTooComplexForMSDF(shape)
 *       Artistic / ornamental fonts are excluded from MSDF regardless of
 *       corner content, because their segment density makes msdfgen's
 *       channel assignment unreliable (see isShapeTooComplexForMSDF).
 *
 *   (b) hasSignificantSharpCorners(shape)
 *       Only glyphs with at least one geometrically sharp corner benefit
 *       from multi-channel rendering.  Smooth-curve glyphs (circles,
 *       script letters, etc.) are handled perfectly by SDF.
 *
 * Evaluation order: complexity check first (has early-out on contour count,
 * O(1) in the common complex case) before the more expensive corner walk.
 */
bool shouldPreferMultiChannel(const msdfgen::Shape& shape) {
    // Note: Edge coloring must have been applied before calling this!
    if (isShapeTooComplex(shape))      return false;
    if (!hasSignificantSharpCorners(shape))    return false;
    return true;
}

} // namespace SDFGen
