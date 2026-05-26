//
//  PathConversion.h
//  SwiftSDF
//
//  Created by ZeroOneZeroR on 5/26/26.
//

#ifndef PathConversion_h
#define PathConversion_h

#include <CoreGraphics/CoreGraphics.h>
#include "include/core/SkPath.h"
#include "include/core/SkPathBuilder.h"
#include "msdfgen.h"
#include <vector>

namespace SDFGen {

// CGPath <-> msdfgen::Shape
msdfgen::Shape      CGPathToShape(CGPathRef cgPath);
// Ownership: functions marked CF_RETURNS_RETAINED return a +1 retained CGPath.
// Caller must release via CGPathRelease() or assign to a __strong/__bridge_transfer ref.
CF_RETURNS_RETAINED CGMutablePathRef ShapeToCGPath(const msdfgen::Shape& shape);

// CGPath <-> SkPath
SkPath              CGPathToSkPath(CGPathRef cgPath, SkPathFillType fillType = SkPathFillType::kWinding);
CF_RETURNS_RETAINED CGMutablePathRef SkPathToCGPath(const SkPath& skPath);

// msdfgen::Shape <-> SkPath
SkPath              ShapeToSkPath(const msdfgen::Shape& shape, SkPathFillType fillType = SkPathFillType::kWinding);
msdfgen::Shape      SkPathToShape(const SkPath& skPath);

} // namespace SDFGen


#endif /* PathConversion_h */
