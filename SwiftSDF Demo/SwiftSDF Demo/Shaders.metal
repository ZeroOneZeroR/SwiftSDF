//
//  Shaders.metal
//  SwiftSDF Demo
//
//  Created by ZeroOneZeroR on 5/26/26.
//

#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float2 uv;
};

// Simple full-quad vertex shader
vertex VertexOut vertexMain(uint vertexID [[vertex_id]]) {
    float2 positions[] = {
        float2(-1, -1), float2(1, -1),
        float2(-1,  1), float2(1,  1)
    };
    float2 uvs[] = {
        float2(0, 1), float2(1, 1),
        float2(0, 0), float2(1, 0)
    };
    
    VertexOut out;
    out.position = float4(positions[vertexID], 0.0, 1.0);
    out.uv = uvs[vertexID];
    return out;
}

// Median function required for MSDF (Multi-channel Signed Distance Field)
float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

fragment float4 fragmentMain(VertexOut in [[stage_in]],
                             texture2d<float> sdfTexture [[texture(0)]]) {
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    
    // Sample the MSDF texture
    float4 sample = sdfTexture.sample(textureSampler, in.uv);
    
    // Calculate the signed distance
    float sigDist = median(sample.r, sample.g, sample.b) - 0.5;
    
    // Compute anti-aliased alpha
    // fwidth gives us the rate of change of the distance field to ensure uniform crispness
    float alpha = clamp(sigDist / fwidth(sigDist) + 0.5, 0.0, 1.0);
    
    // Output a white glyph with the computed alpha
    return float4(1.0, 1.0, 1.0, alpha);
}


