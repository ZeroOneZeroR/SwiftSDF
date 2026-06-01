//
//  SDFMetalView.swift
//  SwiftSDF Demo
//
//  Created by ZeroOneZeroR on 5/26/26.
//

import SwiftUI
import MetalKit
import SwiftSDF

struct SDFMetalView: UIViewRepresentable {
    let cgPath: CGPath
    
    func makeCoordinator() -> Coordinator {
        Coordinator(self)
    }
    
    func makeUIView(context: Context) -> MTKView {
        let mtkView = MTKView()
        mtkView.device = MTLCreateSystemDefaultDevice()
        mtkView.delegate = context.coordinator
        mtkView.clearColor = MTLClearColor(red: 0.1, green: 0.1, blue: 0.12, alpha: 1.0)
        mtkView.colorPixelFormat = .bgra8Unorm
        
        context.coordinator.setupMetal(mtkView: mtkView, path: cgPath)
        return mtkView
    }
    
    func updateUIView(_ uiView: MTKView, context: Context) {
        // Handle dynamic updates if needed
    }
    
    class Coordinator: NSObject, MTKViewDelegate {
        var parent: SDFMetalView
        var device: MTLDevice?
        var commandQueue: MTLCommandQueue?
        var pipelineState: MTLRenderPipelineState?
        var texture: MTLTexture?
        
        init(_ parent: SDFMetalView) {
            self.parent = parent
        }
        
        func setupMetal(mtkView: MTKView, path: CGPath) {
            guard let device = mtkView.device else { return }
            self.device = device
            self.commandQueue = device.makeCommandQueue()
            
            // Generate MSDF Texture using SwiftSDF
            generateTexture(device: device, path: path)
            
            // Setup Render Pipeline
            let library = device.makeDefaultLibrary()
            let vertexFunction = library?.makeFunction(name: "vertexMain")
            let fragmentFunction = library?.makeFunction(name: "fragmentMain")
            
            let pipelineDescriptor = MTLRenderPipelineDescriptor()
            pipelineDescriptor.vertexFunction = vertexFunction
            pipelineDescriptor.fragmentFunction = fragmentFunction
            pipelineDescriptor.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat
            
            // Enable alpha blending for the text
            pipelineDescriptor.colorAttachments[0].isBlendingEnabled = true
            pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
            pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha
            
            do {
                pipelineState = try device.makeRenderPipelineState(descriptor: pipelineDescriptor)
            } catch {
                print("Failed to create pipeline state: \(error)")
            }
        }
        
        private func generateTexture(device: MTLDevice, path: CGPath) {
            let width = 100
            let height = 100
            
            // Configure SwiftSDF
            let config = SDFConfiguration(
                outputWidth: width,
                outputHeight: height,
                padding: 16.0,
                range: 32.0,
                precision: .float16,
                flipY: true // Metal's texture coordinate system expects this
            )
            
            do {
                // Generate SDF/MSDF
                let result = try SDFGenerator.generate(from: path, requestMode: .msdf, config: config)
                
                // Map configuration to Metal Pixel Format
                let pixelFormat: MTLPixelFormat = config.metalPixelFormat(channelFormat: result.channelFormat)
                
                let textureDescriptor = MTLTextureDescriptor.texture2DDescriptor(
                    pixelFormat: pixelFormat,
                    width: width,
                    height: height,
                    mipmapped: false
                )
                textureDescriptor.usage = .shaderRead
                
                guard let newTexture = device.makeTexture(descriptor: textureDescriptor) else { return }
                
                // Calculate bytes per row
                let channelCount = result.channelFormat.channelCount
                let bytesPerChannel = result.precision.bytesPerChannel
                let bytesPerRow = width * channelCount * bytesPerChannel
                
                // Upload generated NSData to MTLRegion
                result.data.withUnsafeBytes { rawBufferPointer in
                    if let baseAddress = rawBufferPointer.baseAddress {
                        newTexture.replace(region: MTLRegionMake2D(0, 0, width, height),
                                           mipmapLevel: 0,
                                           withBytes: baseAddress,
                                           bytesPerRow: bytesPerRow)
                    }
                }
                self.texture = newTexture
                
            } catch {
                print("SwiftSDF Generation Failed: \(error)")
            }
        }
        
        func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {}
        
        func draw(in view: MTKView) {
            guard let drawable = view.currentDrawable,
                  let passDescriptor = view.currentRenderPassDescriptor,
                  let pipelineState = pipelineState,
                  let texture = texture,
                  let commandBuffer = commandQueue?.makeCommandBuffer(),
                  let renderEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: passDescriptor) else {
                return
            }
            
            renderEncoder.setRenderPipelineState(pipelineState)
            // Bind the MSDF texture generated by SwiftSDF
            renderEncoder.setFragmentTexture(texture, index: 0)
            renderEncoder.drawPrimitives(type: .triangleStrip, vertexStart: 0, vertexCount: 4)
            renderEncoder.endEncoding()
            
            commandBuffer.present(drawable)
            commandBuffer.commit()
        }
    }
}
