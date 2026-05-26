//
//  GlyphUtils.swift
//  SwiftSDF Demo
//
//  Created by ZeroOneZeroR on 5/26/26.
//

import CoreText
import CoreGraphics
import UIKit

struct GlyphUtils {
    static func createPath(for character: Character, font: UIFont) -> CGPath? {
        let string = String(character)
        let attrString = NSAttributedString(string: string, attributes: [.font: font])
        let line = CTLineCreateWithAttributedString(attrString)
        
        guard let runs = CTLineGetGlyphRuns(line) as? [CTRun],
              let run = runs.first else { return nil }
        
        let glyphCount = CTRunGetGlyphCount(run)
        guard glyphCount > 0 else { return nil }
        
        var glyph = CGGlyph()
        CTRunGetGlyphs(run, CFRangeMake(0, 1), &glyph)
        
        let ctFont = font as CTFont
        return CTFontCreatePathForGlyph(ctFont, glyph, nil)
    }
}
