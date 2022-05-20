# 64-bit-Typed-Atomics-Extension

## Introduction

Intel offers a subset of the HLSL Shader Model 6.6 64-bit Typed Atomic support through the Intel Extension Framework on our latest Alchemist A-Series GPUs. This paper will detail how to use the Intel Extension Framework for both DirectX 12 API Extensions as well as how to use the extension framework to use a shader (HLSL) extension. Initializing the Extension Framework is an easy task.  

## Limitations

It is important to understand that there are limitations to the 64-bit typed atomic support on DG2/Alchemist. Because of these limitations we are exposing what we can support via an extension until we are able to more fully support Shader Model 6.6. 

- D3D12_RESOURCE_DIMENSION_TEXTURE2D only
- No mip-map surfaces, no 3D surfaces or 2D arrays
- We are not supporting subregions of a surface

## The Sample

We wrote a basic sample to demonstrate the instantiation and use of the extension in an easy to follow project. First import and link the extension framework igdex64.lib along with its dependencies: shlwapi.lib, setupapi.lib, and cfgmgr32.lib to your project. Next, add the igdext.h and IntelExtensions12.hlsl header files to the project. Once this is completed the framework can be initialized during application startup.

Please refer to the "Intel Extension Framework 64-bit Atomics.docx" file for additional implementation details.
