// swift-tools-version: 5.9

import PackageDescription

let package = Package(
    name: "SwiftSDF",
    platforms: [
        .iOS(.v12),
        .macOS(.v11),
        .tvOS(.v12),
        .watchOS(.v4),
        .visionOS(.v1)
    ],
    products: [
        // Products define the executables and libraries a package produces, making them visible to other packages.
        .library(
            name: "SwiftSDF",
            targets: ["SwiftSDF"]
        ),
    ],
    targets: [
        // 1. Core C/C++ Engine Target
        .target(
            name: "SDFCore",
            path: "Sources/SDFCore",
            publicHeadersPath: "include",
            cxxSettings: [
                // Skia Pathops headers
                .headerSearchPath("skia-pathops"),
                
                // msdfgen headers
                .headerSearchPath("msdfgen"),
                
                // Global fallback for the target root
                .headerSearchPath("."),
                
                // Mute precision conversion warnings inside this target locally
                .unsafeFlags(["-Wno-shorten-64-to-32", "-Wno-conversion"])
            ]
        ),
        
        // 2. Objective-C++ Bridging Layer
        .target(
            name: "SDFFoundation",
            dependencies: ["SDFCore"],
            path: "Sources/SDFFoundation",
            publicHeadersPath: "include",
            cxxSettings: [
                .headerSearchPath(".")
            ]
        ),
        
        // 3. Public Swift API & Extensions Layer
        .target(
            name: "SwiftSDF",
            dependencies: ["SDFFoundation"],
            path: "Sources/SwiftSDF"
        )
    ],
    cxxLanguageStandard: .cxx17
)

