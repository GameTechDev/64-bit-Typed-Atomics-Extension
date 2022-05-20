//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

#include "DXSample.h"

using namespace DirectX;

#define TEX_WIDTH 640
#define TEX_HEIGHT 480
#define INTC_EXTENSIONS

#ifdef INTC_EXTENSIONS
#define INTC_IGDEXT_D3D12
//#define INTC_IGDEXT_D3D11

#include "igdext.h"
#endif

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

class INTC_Atomics_64bit_Max : public DXSample
{
public:
    INTC_Atomics_64bit_Max(UINT width, UINT height, std::wstring name);

    virtual void OnInit();
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();

private:
    static const UINT FrameCount = 2;
    static const UINT TextureWidth = 256;
    static const UINT TextureHeight = 256;
    static const UINT TexturePixelSize = 4;    // The number of bytes used to represent a pixel in the texture.

    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT2 uv;
    };

    // Pipeline objects.
    CD3DX12_VIEWPORT m_viewport;
    CD3DX12_RECT m_scissorRect;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_srvHeap;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    ComPtr<ID3D12Resource> m_readbackBuffer;
    UINT m_rtvDescriptorSize;

    // App resources.
    ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    ComPtr<ID3D12Resource> m_texture;

    // Synchronization objects.
    UINT m_frameIndex;
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue;


    
    void LoadPipeline();
    void LoadAssets();
    std::vector<UINT8> GenerateTextureData();
    void WaitForPreviousFrame();
    bool InitINTCExtensions();

#ifdef INTC_EXTENSIONS

    //HANDLE m_computeFenceEvent;
    //ComPtr<ID3D12Fence> m_computeFence;
    //UINT64 m_computeFenceValue;

    ComPtr<ID3D12Resource> m_computeBuffer;

    ComPtr<ID3D12DescriptorHeap> m_computeUAVHeap;
    ComPtr<ID3D12RootSignature> m_computeRootSignature;
    ComPtr<ID3D12PipelineState> m_computePipelineState;

    //ComPtr<ID3D12CommandQueue> m_computeCommandQueue;
    //ComPtr<ID3D12CommandAllocator> m_computeCommandAllocator;
    //ComPtr<ID3D12GraphicsCommandList> m_computeCommandList;

    INTCExtensionContext* m_pINTCExtensionContext = NULL;
#endif
};
