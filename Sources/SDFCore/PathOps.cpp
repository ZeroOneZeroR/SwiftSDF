//
//  PathOps.cpp
//  SwiftSDF
//
//  Created by ZeroOneZeroR on 5/26/26.
//

#include "PathOps.h"
#include "PathConversion.h"     // SDFGen::CGPathToSkPath, SDFGen::SkPathToCGPath

// Bring in conversion utilities without polluting the global namespace.
using SDFGen::CGPathToSkPath;
using SDFGen::SkPathToCGPath;

namespace PathOps {

// ─────────────────────────────────────────────────────────────────────────────
// MARK: - Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

/// PathOps always operates in winding space; convert before every op.
static SkPath toSkPath(CGPathRef path) {
    return CGPathToSkPath(path, SkPathFillType::kWinding);
}

/// Thin alias — makes call sites below symmetric and self-documenting.
static CF_RETURNS_RETAINED CGMutablePathRef toCGPath(const SkPath& path) {
    return SkPathToCGPath(path);
}

static CF_RETURNS_RETAINED CGMutablePathRef cgOp(CGPathRef a, CGPathRef b, SkPathOp op) {
    if (!a || !b) return nullptr;
    SkPath result;
    if (!::Op(toSkPath(a), toSkPath(b), op, &result)) return nullptr;
    return toCGPath(result);
}

static SkPath skOp(const SkPath& a, const SkPath& b, SkPathOp op) {
    SkPath result;
    return ::Op(a, b, op, &result) ? result : SkPath();
}

// ─────────────────────────────────────────────────────────────────────────────
// MARK: - Single-path operations (CGPath)
// ─────────────────────────────────────────────────────────────────────────────

CF_RETURNS_RETAINED CGMutablePathRef Simplify(CGPathRef path) {
    if (!path) return nullptr;
    SkPath result;
    if (!::Simplify(toSkPath(path), &result)) return nullptr;
    return toCGPath(result);
}

CF_RETURNS_RETAINED CGMutablePathRef AsWinding(CGPathRef path) {
    if (!path) return nullptr;
    SkPath result;
    if (!::AsWinding(toSkPath(path), &result)) return nullptr;
    return toCGPath(result);
}

// ─────────────────────────────────────────────────────────────────────────────
// MARK: - Two-path boolean operations (CGPath)
// ─────────────────────────────────────────────────────────────────────────────

CF_RETURNS_RETAINED CGMutablePathRef Op(CGPathRef pathA, CGPathRef pathB, SkPathOp op) {
    return cgOp(pathA, pathB, op);
}
CF_RETURNS_RETAINED CGMutablePathRef Difference(CGPathRef pathA, CGPathRef pathB) {
    return cgOp(pathA, pathB, kDifference_SkPathOp);
}
CF_RETURNS_RETAINED CGMutablePathRef Intersect(CGPathRef pathA, CGPathRef pathB) {
    return cgOp(pathA, pathB, kIntersect_SkPathOp);
}
CF_RETURNS_RETAINED CGMutablePathRef Unite(CGPathRef pathA, CGPathRef pathB) {
    return cgOp(pathA, pathB, kUnion_SkPathOp);
}
CF_RETURNS_RETAINED CGMutablePathRef ExclusiveOr(CGPathRef pathA, CGPathRef pathB) {
    return cgOp(pathA, pathB, kXOR_SkPathOp);
}
CF_RETURNS_RETAINED CGMutablePathRef ReverseDifference(CGPathRef pathA, CGPathRef pathB) {
    return cgOp(pathA, pathB, kReverseDifference_SkPathOp);
}

// ─────────────────────────────────────────────────────────────────────────────
// MARK: - Single-path operations (SkPath)
//
// ::Simplify / ::AsWinding refer to the global Skia free functions, not the
// overloads defined below — the :: prefix avoids calling ourselves.
// ─────────────────────────────────────────────────────────────────────────────

SkPath Simplify(const SkPath& path) {
    SkPath result;
    return ::Simplify(path, &result) ? result : path;
}

SkPath AsWinding(const SkPath& path) {
    SkPath result;
    return ::AsWinding(path, &result) ? result : path;
}

// ─────────────────────────────────────────────────────────────────────────────
// MARK: - Two-path boolean operations (SkPath)
//
// ::Op refers to the global Skia free function.
// ─────────────────────────────────────────────────────────────────────────────

SkPath Op(const SkPath& pathA, const SkPath& pathB, SkPathOp op) {
    return skOp(pathA, pathB, op);
}
SkPath Difference(const SkPath& pathA, const SkPath& pathB) {
    return skOp(pathA, pathB, kDifference_SkPathOp);
}
SkPath Intersect(const SkPath& pathA, const SkPath& pathB) {
    return skOp(pathA, pathB, kIntersect_SkPathOp);
}
SkPath Unite(const SkPath& pathA, const SkPath& pathB) {
    return skOp(pathA, pathB, kUnion_SkPathOp);
}
SkPath ExclusiveOr(const SkPath& pathA, const SkPath& pathB) {
    return skOp(pathA, pathB, kXOR_SkPathOp);
}
SkPath ReverseDifference(const SkPath& pathA, const SkPath& pathB) {
    return skOp(pathA, pathB, kReverseDifference_SkPathOp);
}

} // namespace PathOps
