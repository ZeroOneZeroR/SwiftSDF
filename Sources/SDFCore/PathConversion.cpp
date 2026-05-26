//
//  PathConversion.cpp
//  SwiftSDF
//
//  Created by ZeroOneZeroR on 5/26/26.
//

#include "PathConversion.h"

using namespace msdfgen;

namespace SDFGen {

// ---------- CGPath <-> msdfgen::Shape ----------
struct CGPathToShapeContext {
    Shape* shape;
    Contour* currentContour;
    Point2 currentPoint;
    Point2 contourStartPoint;
};

static void CGPathToShapeApplier(void* info, const CGPathElement* element) {
    auto& ctx = *static_cast<CGPathToShapeContext*>(info);
    
    switch (element->type) {
        case kCGPathElementMoveToPoint: {
            // Each moveTo starts a fresh contour
            ctx.shape->contours.push_back(Contour());
            ctx.currentContour    = &ctx.shape->contours.back();
            const Point2 p(element->points[0].x, element->points[0].y);
            ctx.currentPoint      = p;
            ctx.contourStartPoint = p;
            break;
        }
        case kCGPathElementAddLineToPoint: {
            if (!ctx.currentContour) break;
            const Point2 p1(element->points[0].x, element->points[0].y);
            ctx.currentContour->addEdge(EdgeHolder(ctx.currentPoint, p1));
            ctx.currentPoint = p1;
            break;
        }
        case kCGPathElementAddQuadCurveToPoint: {
            if (!ctx.currentContour) break;
            const Point2 c (element->points[0].x, element->points[0].y);
            const Point2 p1(element->points[1].x, element->points[1].y);
            ctx.currentContour->addEdge(EdgeHolder(ctx.currentPoint, c, p1));
            ctx.currentPoint = p1;
            break;
        }
        case kCGPathElementAddCurveToPoint: {
            if (!ctx.currentContour) break;
            const Point2 c1(element->points[0].x, element->points[0].y);
            const Point2 c2(element->points[1].x, element->points[1].y);
            const Point2 p1(element->points[2].x, element->points[2].y);
            ctx.currentContour->addEdge(EdgeHolder(ctx.currentPoint, c1, c2, p1));
            ctx.currentPoint = p1;
            break;
        }
        case kCGPathElementCloseSubpath: {
            if (ctx.currentContour) {
                // Close gap only if the pen has moved away from the start
                if (ctx.currentPoint.x != ctx.contourStartPoint.x ||
                    ctx.currentPoint.y != ctx.contourStartPoint.y) {
                    ctx.currentContour->addEdge(EdgeHolder(ctx.currentPoint, ctx.contourStartPoint));
                }
            }
            // Null out so stray segments after close don't corrupt the contour
            ctx.currentContour = nullptr;
            break;
        }
    }
}

Shape CGPathToShape(CGPathRef cgPath) {
    Shape shape;
    if (!cgPath) return shape;
    
    // Zero-initialise every field — avoids UB on the first non-moveTo element
    CGPathToShapeContext ctx{};
    ctx.shape = &shape;
    
    CGPathApply(cgPath, &ctx, CGPathToShapeApplier);
    return shape;
}

CF_RETURNS_RETAINED CGMutablePathRef ShapeToCGPath(const Shape &shape) {
    CGMutablePathRef cgPath = CGPathCreateMutable();
    
    for (const Contour &contour : shape.contours) {
        if (contour.edges.empty())
            continue;
        
        const EdgeSegment *first = contour.edges.front();
        const Point2 *p = first->controlPoints();
        
        CGPathMoveToPoint(cgPath, nullptr,
                          (CGFloat)p[0].x,
                          (CGFloat)p[0].y);
        
        for (const auto &edge : contour.edges) {
            const Point2 *cp = edge->controlPoints();
            
            switch (edge->type()) {
                case LinearSegment::EDGE_TYPE:
                    CGPathAddLineToPoint(cgPath, nullptr,
                                         (CGFloat)cp[1].x,
                                         (CGFloat)cp[1].y);
                    break;
                    
                case QuadraticSegment::EDGE_TYPE:
                    CGPathAddQuadCurveToPoint(cgPath, nullptr,
                                              (CGFloat)cp[1].x,
                                              (CGFloat)cp[1].y,
                                              (CGFloat)cp[2].x,
                                              (CGFloat)cp[2].y);
                    break;
                    
                case CubicSegment::EDGE_TYPE:
                    CGPathAddCurveToPoint(cgPath, nullptr,
                                          (CGFloat)cp[1].x,
                                          (CGFloat)cp[1].y,
                                          (CGFloat)cp[2].x,
                                          (CGFloat)cp[2].y,
                                          (CGFloat)cp[3].x,
                                          (CGFloat)cp[3].y);
                    break;
            }
        }
        
        CGPathCloseSubpath(cgPath);
    }
    
    return cgPath;
}




// ---------- CGPath <-> SkPath ----------
static void CGPathToSkPathApplier(void* info, const CGPathElement* element) {
    SkPathBuilder* builder = static_cast<SkPathBuilder*>(info);
    switch (element->type) {
        case kCGPathElementMoveToPoint:
            builder->moveTo(static_cast<SkScalar>(element->points[0].x), static_cast<SkScalar>(element->points[0].y));
            break;
        case kCGPathElementAddLineToPoint:
            builder->lineTo(static_cast<SkScalar>(element->points[0].x), static_cast<SkScalar>(element->points[0].y));
            break;
        case kCGPathElementAddQuadCurveToPoint:
            builder->quadTo(static_cast<SkScalar>(element->points[0].x), static_cast<SkScalar>(element->points[0].y),
                            static_cast<SkScalar>(element->points[1].x), static_cast<SkScalar>(element->points[1].y));
            break;
        case kCGPathElementAddCurveToPoint:
            builder->cubicTo(static_cast<SkScalar>(element->points[0].x), static_cast<SkScalar>(element->points[0].y),
                             static_cast<SkScalar>(element->points[1].x), static_cast<SkScalar>(element->points[1].y),
                             static_cast<SkScalar>(element->points[2].x), static_cast<SkScalar>(element->points[2].y));
            break;
        case kCGPathElementCloseSubpath:
            builder->close();
            break;
    }
}

SkPath CGPathToSkPath(CGPathRef cgPath, SkPathFillType skPathFillType) {
    if (!cgPath) return SkPath();
    SkPathBuilder builder;
    builder.setFillType(skPathFillType);
    CGPathApply(cgPath, &builder, CGPathToSkPathApplier);
    return builder.detach();
}

CF_RETURNS_RETAINED CGMutablePathRef SkPathToCGPath(const SkPath& skPath) {
    CGMutablePathRef cgPath = CGPathCreateMutable();
    SkPath::RawIter iter(skPath);
    SkPoint pts[4];
    SkPath::Verb verb;
    while ((verb = iter.next(pts)) != SkPath::kDone_Verb) {
        switch (verb) {
            case SkPath::kMove_Verb:
                CGPathMoveToPoint(cgPath, nullptr, static_cast<CGFloat>(pts[0].fX), static_cast<CGFloat>(pts[0].fY));
                break;
            case SkPath::kLine_Verb:
                CGPathAddLineToPoint(cgPath, nullptr, static_cast<CGFloat>(pts[1].fX), static_cast<CGFloat>(pts[1].fY));
                break;
            case SkPath::kQuad_Verb:
                CGPathAddQuadCurveToPoint(cgPath, nullptr, static_cast<CGFloat>(pts[1].fX), static_cast<CGFloat>(pts[1].fY),
                                          static_cast<CGFloat>(pts[2].fX), static_cast<CGFloat>(pts[2].fY));
                break;
            case SkPath::kConic_Verb: {
                const SkScalar weight = iter.conicWeight();
                const int pow2 = 2;
                const int quadCount = 1 << pow2;
                SkPoint quads[1 + 2 * (1 << 2)];
                SkPath::ConvertConicToQuads(pts[0], pts[1], pts[2], weight, quads, pow2);
                for (int i = 0; i < quadCount; ++i) {
                    CGPathAddQuadCurveToPoint(cgPath, nullptr, static_cast<CGFloat>(quads[2 * i + 1].fX), static_cast<CGFloat>(quads[2 * i + 1].fY),
                                              static_cast<CGFloat>(quads[2 * i + 2].fX), static_cast<CGFloat>(quads[2 * i + 2].fY));
                }
                break;
            }
            case SkPath::kCubic_Verb:
                CGPathAddCurveToPoint(cgPath, nullptr, static_cast<CGFloat>(pts[1].fX), static_cast<CGFloat>(pts[1].fY),
                                      static_cast<CGFloat>(pts[2].fX), static_cast<CGFloat>(pts[2].fY),
                                      static_cast<CGFloat>(pts[3].fX), static_cast<CGFloat>(pts[3].fY));
                break;
            case SkPath::kClose_Verb:
                CGPathCloseSubpath(cgPath);
                break;
            default: break;
        }
    }
    return cgPath;
}


// ---------- msdfgen::Shape <-> SkPath ----------

// double ↔ SkScalar conversion helpers (precision-explicit)
static inline SkPoint pointToSkiaPoint(const Point2 &p) {
    return SkPoint::Make(static_cast<SkScalar>(p.x), static_cast<SkScalar>(p.y));
}

static inline Point2 pointFromSkiaPoint(const SkPoint &p) {
    return Point2(static_cast<double>(p.fX), static_cast<double>(p.fY));
}

SkPath ShapeToSkPath(const Shape& shape, SkPathFillType skPathFillType) {
    SkPathBuilder builder;
    builder.setFillType(skPathFillType);
    
    for (const auto &contour : shape.contours) {
        if (contour.edges.empty()) continue;
        
        // Start the contour at the beginning of the first edge
        const Point2 *firstEdgePoints = contour.edges.front()->controlPoints();
        builder.moveTo(pointToSkiaPoint(firstEdgePoints[0]));
        
        for (const auto &edge : contour.edges) {
            const Point2 *p = edge->controlPoints();
            switch (edge->type()) {
                case LinearSegment::EDGE_TYPE:
                    builder.lineTo(pointToSkiaPoint(p[1]));
                    break;
                case QuadraticSegment::EDGE_TYPE:
                    builder.quadTo(pointToSkiaPoint(p[1]), pointToSkiaPoint(p[2]));
                    break;
                case CubicSegment::EDGE_TYPE:
                    builder.cubicTo(pointToSkiaPoint(p[1]), pointToSkiaPoint(p[2]), pointToSkiaPoint(p[3]));
                    break;
            }
        }
        // msdfgen contours are implicitly closed, but we call close() to be explicit in Skia
        builder.close();
    }
    return builder.detach();
}

Shape SkPathToShape(const SkPath &skPath) {
    Shape shape;
    
    // Use RawIter to get the exact path data without Skia's "Iter" auto-closing logic
    SkPath::RawIter iter(skPath);
    SkPoint pts[4];
    SkPath::Verb verb;
    Contour *currentContour = nullptr;
    
    SkPoint contourStart = {0, 0};
    
    while ((verb = iter.next(pts)) != SkPath::kDone_Verb) {
        switch (verb) {
            case SkPath::kMove_Verb:
                // Start a new contour
                shape.contours.push_back(Contour());
                currentContour = &shape.contours.back();
                contourStart = pts[0];
                break;
                
            case SkPath::kLine_Verb:
                if (currentContour) {
                    currentContour->addEdge(EdgeHolder(pointFromSkiaPoint(pts[0]), pointFromSkiaPoint(pts[1])));
                }
                break;
                
            case SkPath::kQuad_Verb:
                if (currentContour) {
                    currentContour->addEdge(EdgeHolder(pointFromSkiaPoint(pts[0]), pointFromSkiaPoint(pts[1]), pointFromSkiaPoint(pts[2])));
                }
                break;
                
            case SkPath::kConic_Verb:
                if (currentContour) {
                    const SkScalar weight = iter.conicWeight();
                    const int pow2 = 2; // 4 quads - better precision for MSDF
                    const int quadCount = 1 << pow2;
                    SkPoint quads[1 + 2 * (1 << 2)];
                    SkPath::ConvertConicToQuads(pts[0], pts[1], pts[2], weight, quads, pow2);
                    for (int i = 0; i < quadCount; ++i) {
                        currentContour->addEdge(EdgeHolder(pointFromSkiaPoint(quads[2 * i]),
                                                           pointFromSkiaPoint(quads[2 * i + 1]),
                                                           pointFromSkiaPoint(quads[2 * i + 2])));
                    }
                }
                break;
                
            case SkPath::kCubic_Verb:
                if (currentContour) {
                    currentContour->addEdge(EdgeHolder(pointFromSkiaPoint(pts[0]),
                                                       pointFromSkiaPoint(pts[1]),
                                                       pointFromSkiaPoint(pts[2]),
                                                       pointFromSkiaPoint(pts[3])));
                }
                break;
                
            case SkPath::kClose_Verb:
                if (currentContour) {
                    // pts[0] is the current pen position in RawIter's kClose_Verb
                    if (pts[0] != contourStart) {
                        currentContour->addEdge(EdgeHolder(
                                                           pointFromSkiaPoint(pts[0]),
                                                           pointFromSkiaPoint(contourStart)));
                    }
                }
                break;
                
            default:
                break;
        }
    }
    
    // Cleanup: Remove any contours that ended up with no edges
    shape.contours.erase(std::remove_if(shape.contours.begin(), shape.contours.end(),
                                        [](const Contour& c){ return c.edges.empty(); }),
                         shape.contours.end());
    return shape;
}

}
