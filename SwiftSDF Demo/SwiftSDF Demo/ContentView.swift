//
//  ContentView.swift
//  SwiftSDF Demo
//
//  Created by ZeroOneZeroR on 5/26/26.
//

import SwiftUI

struct ContentView: View {
    @State private var cgPath: CGPath?
    
    var body: some View {
        VStack(spacing: 20) {
            Text("Text rendered with MSDF on metal:")
                .font(.headline)
            
            if let path = cgPath {
                // Render the MSDF texture via Metal
                SDFMetalView(cgPath: path)
                    .frame(width: 300, height: 300)
                    .clipShape(RoundedRectangle(cornerRadius: 16))
                    .shadow(radius: 10)
            } else {
                Text("Generating Path...")
                    .frame(width: 300, height: 300)
                    .background(Color.gray.opacity(0.2))
                    .clipShape(RoundedRectangle(cornerRadius: 16))
            }
        }
        .padding()
        .onAppear {
            let testCharacter: Character = "H"
            let largeFont = UIFont.systemFont(ofSize: 20, weight: .bold)
            
            DispatchQueue.global(qos: .userInitiated).async {
                let path = GlyphUtils.createPath(for: testCharacter, font: largeFont)
                DispatchQueue.main.async {
                    self.cgPath = path
                }
            }
        }
    }
}
