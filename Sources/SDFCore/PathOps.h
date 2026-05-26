//
//  PathOps.h
//  SwiftSDF
//
//  Created by ZeroOneZeroR on 5/26/26.
//
//  Wraps every Skia PathOps operation (Simplify, AsWinding, and all five
//  boolean ops) for both CGPath and SkPath.
//
//  Ownership rule: every function marked CF_RETURNS_RETAINED returns a +1
//  retained CGPath.  Caller must release with CGPathRelease() (or bridge to ARC).

#ifndef PathOps_h
#define PathOps_h

#include <CoreGraphics/CoreGraphics.h>
#include "include/core/SkPath.h"
#include "include/pathops/SkPathOps.h"

/// Boolean and topological path operations built on Skia PathOps.
/// CGPath variants: caller owns result — release with CGPathRelease().
/// SkPath variants: value-type, no ownership concerns.
/// On failure: returns nullptr (CGPath) or empty/original SkPath.
namespace PathOps {

// ─────────────────────────────────────────────────────────────────────────────
// MARK: - Single-path operations
// ─────────────────────────────────────────────────────────────────────────────

/// Resolves self-intersections and overlaps into non-overlapping contours.
/// Result always uses the winding fill rule — required for correct MSDF generation.
CF_RETURNS_RETAINED CGMutablePathRef Simplify(CGPathRef path);

/// Converts an even-odd fill-rule path into an equivalent winding path.
CF_RETURNS_RETAINED CGMutablePathRef AsWinding(CGPathRef path);

// ─────────────────────────────────────────────────────────────────────────────
// MARK: - Two-path boolean operations (CGPath)
//
//   Difference        A − B   regions in A but not in B
//   Intersect         A ∩ B   regions common to both
//   Unite             A ∪ B   regions in A, B, or both
//   ExclusiveOr       A ⊕ B   regions in exactly one of A or B
//   ReverseDifference B − A   regions in B but not in A
// ─────────────────────────────────────────────────────────────────────────────

/// Generic operation — prefer the named variants below for readability.
CF_RETURNS_RETAINED CGMutablePathRef Op(CGPathRef pathA, CGPathRef pathB, SkPathOp op);

CF_RETURNS_RETAINED CGMutablePathRef Difference       (CGPathRef pathA, CGPathRef pathB);
CF_RETURNS_RETAINED CGMutablePathRef Intersect        (CGPathRef pathA, CGPathRef pathB);
CF_RETURNS_RETAINED CGMutablePathRef Unite            (CGPathRef pathA, CGPathRef pathB);
CF_RETURNS_RETAINED CGMutablePathRef ExclusiveOr      (CGPathRef pathA, CGPathRef pathB);
CF_RETURNS_RETAINED CGMutablePathRef ReverseDifference(CGPathRef pathA, CGPathRef pathB);

// ─────────────────────────────────────────────────────────────────────────────
// MARK: - Single-path operations (SkPath)
// ─────────────────────────────────────────────────────────────────────────────

/// Returns simplified path, or the original if Skia reports failure.
SkPath Simplify (const SkPath& path);

/// Returns winding-rule equivalent, or the original if Skia reports failure.
SkPath AsWinding(const SkPath& path);

// ─────────────────────────────────────────────────────────────────────────────
// MARK: - Two-path boolean operations (SkPath)
//
// Returns an empty SkPath on failure — callers should check path.isEmpty().
// ─────────────────────────────────────────────────────────────────────────────

SkPath Op              (const SkPath& pathA, const SkPath& pathB, SkPathOp op);
SkPath Difference      (const SkPath& pathA, const SkPath& pathB);
SkPath Intersect       (const SkPath& pathA, const SkPath& pathB);
SkPath Unite           (const SkPath& pathA, const SkPath& pathB);
SkPath ExclusiveOr     (const SkPath& pathA, const SkPath& pathB);
SkPath ReverseDifference(const SkPath& pathA, const SkPath& pathB);

} // namespace PathOps

#endif /* PathOps_h */
