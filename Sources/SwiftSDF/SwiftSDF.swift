//
//  SwiftSDF.swift
//  SwiftSDF
//
//  Created by ZeroOneZeroR on 5/26/26.
//

// MARK: - Re-Export Foundation Layer

/// The `@_exported` attribute turns `SwiftSDF` into an "umbrella" module.
///
/// By re-exporting `SDFFoundation` here, any client application that calls `import SwiftSDF`
/// will automatically and transparently gain access to all public things.
///
/// This delivers a clean architecture:
/// 1. The client app only interacts with a single, unified interface (`import SwiftSDF`).
/// 2. Internal layers (`SDFFoundation` and `SDFCore`) remain hidden implementation details.
/// 3. Other Swift files within this target don't have to import `SDFFoundation`.
///

#if !COCOAPODS
#if canImport(SDFFoundation)
@_exported import SDFFoundation
#endif
#endif

// MARK: - SDFConfiguration extensions

import Metal

public extension SDFConfiguration {
    func metalPixelFormat(channelFormat: SDFChannelFormat)-> MTLPixelFormat {
        switch (channelFormat, precision) {
        case (.r, .unorm8):
            return .r8Unorm
            
        case (.r, .float16):
            return .r16Float
            
        case (.rgba, .unorm8):
            return .rgba8Unorm
            
        case (.rgba, .float16):
            return .rgba16Float
        }
    }
}
