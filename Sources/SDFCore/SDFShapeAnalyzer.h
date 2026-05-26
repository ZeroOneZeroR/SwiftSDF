//
//  SDFShapeAnalyzer.h
//  SwiftSDF
//
//  Created by ZeroOneZeroR on 5/26/26.
//

#ifndef SDFShapeAnalyzer_h
#define SDFShapeAnalyzer_h

#include "msdfgen.h"

namespace SDFGen {

/**
 * Analyzes an msdfgen Shape to decide whether MSDF is beneficial.
 *
 * Returns true if:
 *   - Shape has at least one geometrically sharp corner (tangent change > 45°)
 *   - AND shape is not overly complex (segment count <= 48, contour count <= 5)
 */
bool shouldPreferMultiChannel(const msdfgen::Shape& shape);

} // namespace SDFGen

#endif /* SDFShapeAnalyzer_h */
