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

#include "stdafx.h"
#include "INTC_Atomics_64bit_Max.h"

INTC_Atomics_64bit_Max::INTC_Atomics_64bit_Max(UINT width, UINT height, std::wstring name) :
    DXSample(width, height, name),
    m_frameIndex(0),
    m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
    m_rtvDescriptorSize(0)
{
}

void INTC_Atomics_64bit_Max::OnInit()
{
    LoadPipeline();
    LoadAssets();
}


bool INTC_Atomics_64bit_Max::InitINTCExtensions()
{
#ifdef INTC_EXTENSIONS
    const INTCExtensionVersion requiredVersion = { 3,4,1 }; 
    INTCExtensionVersion* pSupportedExtVersions = nullptr;
    uint32_t supportedExtVersionCount = 0;
    INTCExtensionInfo intcExtensionInfo = {};

    //default is to use the driver's installed DLL, setting this input value to true will start search with current process directory
    if (SUCCEEDED(INTC_LoadExtensionsLibrary(true)))
    {
        printf("SUCCESS: INTC_LoadExtensionsLibrary succeeded.\n");
    }
    else
    {
        printf("ERROR: INTC_LoadExtensionsLibrary failed\n");
        return false;
    }

    //first, fill in the supportedExtVersionCount. This does not populate pSupportedExtVersions
    if (SUCCEEDED(INTC_D3D12_GetSupportedVersions(m_device.Get(), nullptr, &supportedExtVersionCount)))
    {
        printf("SUCCESS: GetSupportedVersions 1 of 2 calls successful\n");
    }
    else
    {
        printf("ERROR: GetSupportedVersions 1 of 2 calls failed\n");
        return false;
    }

    //Next, use returned value for supportedExtVersionCount to allocate space for the supported extensions
    pSupportedExtVersions = new INTCExtensionVersion[supportedExtVersionCount]{};

    if (SUCCEEDED(INTC_D3D12_GetSupportedVersions(m_device.Get(), pSupportedExtVersions, &supportedExtVersionCount)))
    {
        printf("SUCCESS: GetSupportedVersions 2 of 2 calls successful\n");

        printf("Supported Extension Versions in this driver:\n");

        //showing how to print out all the version numbers
        for (uint32_t i = 0; i < supportedExtVersionCount; i++)
        {
            printf("[%u of %u] Version %u.%u.%u\n", 
                i + 1, 
                supportedExtVersionCount, 
                pSupportedExtVersions[i].HWFeatureLevel, 
                pSupportedExtVersions[i].APIVersion, 
                pSupportedExtVersions[i].Revision);
        }

        printf("Locating requested extension version: %u.%u.%u...\n", 
            requiredVersion.HWFeatureLevel, 
            requiredVersion.APIVersion, 
            requiredVersion.Revision);

        for (uint32_t i = 0; i < supportedExtVersionCount; i++)
        {
            if ((pSupportedExtVersions[i].HWFeatureLevel >= requiredVersion.HWFeatureLevel) &&
                (pSupportedExtVersions[i].APIVersion >= requiredVersion.APIVersion) &&
                (pSupportedExtVersions[i].Revision >= requiredVersion.Revision))
            {
                printf("SUCCESS: located requested version %u.%u.%u\n\n",
                    pSupportedExtVersions[i].HWFeatureLevel,
                    pSupportedExtVersions[i].APIVersion,
                    pSupportedExtVersions[i].Revision);

                intcExtensionInfo.RequestedExtensionVersion = pSupportedExtVersions[i];

                break;
            }
            else
            {
                printf("%u.%u.%u doesn't match required version: %u.%u.%u, let's try the next one\n", 
                    pSupportedExtVersions[i].HWFeatureLevel, 
                    pSupportedExtVersions[i].APIVersion, 
                    pSupportedExtVersions[i].Revision,
                    requiredVersion.HWFeatureLevel, 
                    requiredVersion.APIVersion, 
                    requiredVersion.Revision);
            }
        }

    }
    else
    {
        printf("ERROR: GetSupportedVersions 2 of 2 calls failed\n");
        return false;
    }


    if (pSupportedExtVersions != nullptr)
    {
        delete[] pSupportedExtVersions;
    }

    if (SUCCEEDED(INTC_D3D12_CreateDeviceExtensionContext(m_device.Get(), &m_pINTCExtensionContext, &intcExtensionInfo, nullptr)))
    {
        printf("Let me tell you a little bit about this GPU:\n\tGPUMaxFrequency: %u Mhz\n\tGTGeneration:%u\n\tEUCount:%u\n\tPackageTDP:%u Watts\n\tMaxFillRate:%u pixels/clock@32bpp\n",
            intcExtensionInfo.IntelDeviceInfo.GPUMaxFreq,
            intcExtensionInfo.IntelDeviceInfo.GTGeneration,
            intcExtensionInfo.IntelDeviceInfo.EUCount,
            intcExtensionInfo.IntelDeviceInfo.PackageTDP,
            intcExtensionInfo.IntelDeviceInfo.MaxFillRate);
        printf("Done reporting intcExtensionInfo\n\n");
    }
    else
    {
        return false;
    }

    return true;
#else
    return false;
#endif
}

// Load the rendering pipeline dependencies.
void INTC_Atomics_64bit_Max::LoadPipeline()
{
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    if (m_useWarpDevice)
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        ThrowIfFailed(D3D12CreateDevice(
            warpAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)
            ));
    }
    else
    {
        ComPtr<IDXGIAdapter1> hardwareAdapter;
        GetHardwareAdapter(factory.Get(), &hardwareAdapter);

        ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)
            ));
    }

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        Win32Application::GetHwnd(),
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
        ));

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain.As(&m_swapChain));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Create descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = FrameCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

        // Describe and create a shader resource view (SRV) heap for the texture.
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = 1;
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap)));

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        
    }

    // Create frame resources.
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create a RTV for each frame.
        for (UINT n = 0; n < FrameCount; n++)
        {
            ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
            m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);
        }
    }

    ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));

#ifdef INTC_EXTENSIONS
    // Create UAV descriptor heap for compute pipeline.
    {
        // Describe and create a UAV descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC uavHeapDesc = {};
        uavHeapDesc.NumDescriptors = 1;
        uavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        uavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&uavHeapDesc, IID_PPV_ARGS(&m_computeUAVHeap)));
        NAME_D3D12_OBJECT(m_computeUAVHeap);
    }

    bool bInitIntelExtensions = false;
    bInitIntelExtensions = InitINTCExtensions();

    if (bInitIntelExtensions == false)
    {
        printf("Unable To Load Intel Graphics Extensions.\n");
        
        MessageBox(Win32Application::GetHwnd(), L"Unable To Load Intel Graphics Extensions", L"Extension Error", MB_OK);

        exit(1);
    }
    else
    {
        printf("Intel Extensions Succesfully Loaded.\n");
    }
#endif

   
}

// Load the sample assets.
void INTC_Atomics_64bit_Max::LoadAssets()
{
    // Create the root signature.
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

        // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);

        CD3DX12_ROOT_PARAMETER1 rootParameters[1];
        rootParameters[0].InitAsDescriptorTable(_countof(ranges), ranges, D3D12_SHADER_VISIBILITY_ALL);

        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.MipLODBias = 0;
        sampler.MaxAnisotropy = 0;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
        ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
    }

    // Create the pipeline state, which includes compiling and loading shaders.
    {
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif

        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_1", compileFlags, 0, &vertexShader, nullptr));
        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_1", compileFlags, 0, &pixelShader, nullptr));

        // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;
        ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
    }

    // Create the command list.
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));

    // Create the vertex buffer.
    {
        // Define the geometry for a triangle.
        Vertex triangleVertices[] =
        {
            { { 0.0f, 0.25f * m_aspectRatio, 0.0f }, { 0.5f, 0.0f } },
            { { 0.25f, -0.25f * m_aspectRatio, 0.0f }, { 1.0f, 1.0f } },
            { { -0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 1.0f } }
        };

        const UINT vertexBufferSize = sizeof(triangleVertices);

        // Note: using upload heaps to transfer static data like vert buffers is not 
        // recommended. Every time the GPU needs it, the upload heap will be marshalled 
        // over. Please read up on Default Heap usage. An upload heap is used here for 
        // code simplicity and because there are very few verts to actually transfer.
        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_vertexBuffer)));

        // Copy the triangle data to the vertex buffer.
        UINT8* pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
        m_vertexBuffer->Unmap(0, nullptr);

        // Initialize the vertex buffer view.
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        m_vertexBufferView.SizeInBytes = vertexBufferSize;
    }

    // Note: ComPtr's are CPU objects but this resource needs to stay in scope until
    // the command list that references it has finished executing on the GPU.
    // We will flush the GPU at the end of this method to ensure the resource is not
    // prematurely destroyed.
    ComPtr<ID3D12Resource> textureUploadHeap;

    // Create the texture.
    {
        // Describe and create a Texture2D.
        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.MipLevels = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.Width = TextureWidth;
        textureDesc.Height = TextureHeight;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_texture)));

        const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture.Get(), 0, 1);

        // Create the GPU upload buffer.
        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&textureUploadHeap)));

        // Copy data to the intermediate upload heap and then schedule a copy 
        // from the upload heap to the Texture2D.
        std::vector<UINT8> texture = GenerateTextureData();

        D3D12_SUBRESOURCE_DATA textureData = {};
        textureData.pData = &texture[0];
        textureData.RowPitch = TextureWidth * TexturePixelSize;
        textureData.SlicePitch = textureData.RowPitch * TextureHeight;

        UpdateSubresources(m_commandList.Get(), m_texture.Get(), textureUploadHeap.Get(), 0, 0, 1, &textureData);
        m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        // Describe and create a SRV for the texture.
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        m_device->CreateShaderResourceView(m_texture.Get(), &srvDesc, m_srvHeap->GetCPUDescriptorHandleForHeapStart());
    }
    
#ifdef INTC_EXTENSIONS

    // Create compute root signature
    {
        CD3DX12_DESCRIPTOR_RANGE1 descRange[2] = {};
        descRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);
        descRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 7, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);

        CD3DX12_ROOT_PARAMETER1 rootParameters[1] = {};
        rootParameters[0].InitAsDescriptorTable(_countof(descRange), descRange, D3D12_SHADER_VISIBILITY_ALL);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;

        const HRESULT hr = D3DX12SerializeVersionedRootSignature(
            &rootSignatureDesc,
            D3D_ROOT_SIGNATURE_VERSION_1,
            &signature,
            &error);
        if (FAILED(hr) && error != nullptr)
        {
            OutputDebugStringA((char*)error->GetBufferPointer());
        }
        ThrowIfFailed(hr);

        ThrowIfFailed(m_device->CreateRootSignature(
            0,
            signature->GetBufferPointer(),
            signature->GetBufferSize(),
            IID_PPV_ARGS(&m_computeRootSignature)));

        NAME_D3D12_OBJECT(m_computeRootSignature);
    }

    // Create the pipeline state, which includes compiling and loading shaders.
    {
#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        //UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
        UINT compileFlags = D3DCOMPILE_DEBUG;
#else
        UINT compileFlags = 0;
#endif
        ComPtr<ID3DBlob> computeShader;
        ComPtr<ID3DBlob> error;

        UINT8* pComputeShaderData = nullptr;
        UINT computeShaderDataLength = 0;
        wchar_t shaderPath[] = L"INTC_Atomics_64bit_Max.cso";
        printf("Using as path to shaders: %ls\n", shaderPath);

        ThrowIfFailed(ReadDataFromFile(shaderPath, &pComputeShaderData, &computeShaderDataLength));

        // Describe and create the compute pipeline state object (PSO).
        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_computeRootSignature.Get();

        //compiled with MSVC->Tools->Commnad Line->Developer Command Prompt: Dxc -E CSMain -T cs_5_1 shaders_atomics_RWTexture2D.hlsl -Fo CSMain.cso /Od
        psoDesc.CS = CD3DX12_SHADER_BYTECODE(pComputeShaderData, computeShaderDataLength);

        {
            ThrowIfFailed(m_device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_computePipelineState)));
        }

        NAME_D3D12_OBJECT(m_computePipelineState);
    }

    // Create Compute Texture
    {
        D3D12_RESOURCE_DESC texture2D = {};

        texture2D.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texture2D.Alignment = 0;
        texture2D.Width = TEX_WIDTH;  //pretty sure this is 128 * sizeof(surfaceformat) 
        texture2D.Height = TEX_HEIGHT;
        texture2D.DepthOrArraySize = 1;
        texture2D.MipLevels = 1;
        texture2D.Format = DXGI_FORMAT_R32G32_UINT;
        texture2D.SampleDesc.Count = 1;
        texture2D.SampleDesc.Quality = 0;
        texture2D.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        texture2D.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        D3D12_HEAP_PROPERTIES heap_props{};
        heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;
        heap_props.CreationNodeMask = 1;
        heap_props.VisibleNodeMask = 1;
        heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        INTC_D3D12_RESOURCE_DESC_0001 dsResDesc1;
        texture2D.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        dsResDesc1.pD3D12Desc = &texture2D;
        dsResDesc1.Texture2DArrayMipPack = FALSE;
        dsResDesc1.EmulatedTyped64bitAtomics = TRUE;

        INTC_D3D12_CreateCommittedResource(
            m_pINTCExtensionContext,
            &heap_props,
            D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS,
            &dsResDesc1,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr,
            IID_PPV_ARGS(&m_computeBuffer)
        );
    }

    // Create a UAV resource to write to.
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle(m_computeUAVHeap->GetCPUDescriptorHandleForHeapStart());

        //texture2D
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc2 = {};
        uavDesc2.Format = DXGI_FORMAT_R32G32_UINT;
        uavDesc2.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc2.Texture2D.MipSlice = 0;
        uavDesc2.Texture2D.PlaneSlice = 0;

        m_device->CreateUnorderedAccessView(m_computeBuffer.Get(), nullptr, &uavDesc2, uavHandle);
    }

    // Create the readback buffer.
    {
        // Create a readback buffer large enough to hold all texel data
        D3D12_HEAP_PROPERTIES HeapProps = {};
        HeapProps.Type = D3D12_HEAP_TYPE_READBACK;
        HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        HeapProps.CreationNodeMask = 1;
        HeapProps.VisibleNodeMask = 1;

        // Readback buffers must be 1-dimensional, i.e. "buffer" not "texture2d"
        D3D12_RESOURCE_DESC ResourceDesc = {};
        ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        ResourceDesc.Width = TEX_WIDTH * TEX_HEIGHT * 4 * 2;  //note that i'd expect to call this an R32_UINT but instead have to do this, cause WTF DX , must be bytes
        ResourceDesc.Height = 1;
        ResourceDesc.DepthOrArraySize = 1;
        ResourceDesc.MipLevels = 1;
        ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        ResourceDesc.SampleDesc.Count = 1;
        ResourceDesc.SampleDesc.Quality = 0;
        ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        ThrowIfFailed(m_device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_readbackBuffer)));

     }
#endif

    // Close the command list and execute it to begin the initial GPU setup.
    ThrowIfFailed(m_commandList->Close());
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceValue = 1;

        // Create an event handle to use for frame synchronization.
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        // Wait for the command list to execute; we are reusing the same command 
        // list in our main loop but for now, we just want to wait for setup to 
        // complete before continuing.
        WaitForPreviousFrame();
    }
}

// Generate a simple black and white checkerboard texture.
std::vector<UINT8> INTC_Atomics_64bit_Max::GenerateTextureData()
{
    const UINT rowPitch = TextureWidth * TexturePixelSize;
    const UINT cellPitch = rowPitch >> 3;        // The width of a cell in the checkboard texture.
    const UINT cellHeight = TextureWidth >> 3;    // The height of a cell in the checkerboard texture.
    const UINT textureSize = rowPitch * TextureHeight;

    std::vector<UINT8> data(textureSize);
    UINT8* pData = &data[0];

    for (UINT n = 0; n < textureSize; n += TexturePixelSize)
    {
        UINT x = n % rowPitch;
        UINT y = n / rowPitch;
        UINT i = x / cellPitch;
        UINT j = y / cellHeight;

        if (i % 2 == j % 2)
        {
            pData[n] = 0x00;        // R
            pData[n + 1] = 0x00;    // G
            pData[n + 2] = 0x00;    // B
            pData[n + 3] = 0xff;    // A
        }
        else
        {
            pData[n] = 0xff;        // R
            pData[n + 1] = 0xff;    // G
            pData[n + 2] = 0xff;    // B
            pData[n + 3] = 0xff;    // A
        }
    }

    return data;
}

// Update frame-based values.
void INTC_Atomics_64bit_Max::OnUpdate()
{

}

// Render the scene.
void INTC_Atomics_64bit_Max::OnRender()
{
    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    ThrowIfFailed(m_commandAllocator->Reset());

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

    // Set necessary state.
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeap.Get() };
    m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    m_commandList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGPUDescriptorHandleForHeapStart());

    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    // Indicate that the back buffer will be used as a render target.
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    m_commandList->DrawInstanced(3, 1, 0, 0);

#ifdef INTC_EXTENSIONS
    ID3D12DescriptorHeap* ppHeapsCompute[] = { m_computeUAVHeap.Get() };
    m_commandList->SetDescriptorHeaps(_countof(ppHeapsCompute), ppHeapsCompute);
    m_commandList->SetPipelineState(m_computePipelineState.Get());
    m_commandList->SetComputeRootSignature(m_computeRootSignature.Get());
    m_commandList->SetComputeRootDescriptorTable(0, m_computeUAVHeap->GetGPUDescriptorHandleForHeapStart());
    m_commandList->Dispatch(TEX_WIDTH / 32, TEX_HEIGHT / 32, 1);

    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_computeBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedFootprint;
    D3D12_RESOURCE_DESC texture2D;
    texture2D.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texture2D.Alignment = 0;
    texture2D.Width = TEX_WIDTH;
    texture2D.Height = TEX_HEIGHT;
    texture2D.DepthOrArraySize = 1;
    texture2D.MipLevels = 1;
    texture2D.Format = DXGI_FORMAT_R32G32_UINT;
    texture2D.SampleDesc.Count = 1;
    texture2D.SampleDesc.Count = 1;
    texture2D.SampleDesc.Quality = 0;
    texture2D.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texture2D.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    //later, can use this to extract other useful info about the placedfootprint
    m_device->GetCopyableFootprints(&texture2D, 0, 1, 0, &placedFootprint, nullptr, nullptr, nullptr);

    D3D12_TEXTURE_COPY_LOCATION src;
    src.pResource = m_computeBuffer.Get();
    src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    src.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION dst;
    dst.pResource = m_readbackBuffer.Get();
    dst.PlacedFootprint = placedFootprint;
    dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

    m_commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_computeBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
#endif

    // Indicate that the back buffer will now be used to present.
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    ThrowIfFailed(m_commandList->Close());

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present the frame.
    ThrowIfFailed(m_swapChain->Present(1, 0));

    WaitForPreviousFrame();

#ifdef INTC_EXTENSIONS
    uint64_t* data = nullptr;
    ThrowIfFailed(m_readbackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&data)));

    printf("the first few values are: %llu, %llu, %llu, %llu\n",
        data[0],
        data[1],
        data[2],
        data[3]);

    m_readbackBuffer->Unmap(0, nullptr);
#endif
}

void INTC_Atomics_64bit_Max::OnDestroy()
{
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    WaitForPreviousFrame();

#ifdef INTC_EXTENSIONS
    if (m_pINTCExtensionContext != nullptr)
    {
        HRESULT hr = INTC_DestroyDeviceExtensionContext(&m_pINTCExtensionContext);
        if (FAILED(hr))
        {
            printf("ERROR: INTC_DestroyDeviceExtensionContext\n");
        }
    }

    INTC_UnloadExtensionsLibrary();
#endif

    CloseHandle(m_fenceEvent);
}

void INTC_Atomics_64bit_Max::WaitForPreviousFrame()
{
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
    // sample illustrates how to use fences for efficient resource usage and to
    // maximize GPU utilization.

    // Signal and increment the fence value.
    const UINT64 fence = m_fenceValue;
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
    m_fenceValue++;

    // Wait until the previous frame is finished.
    if (m_fence->GetCompletedValue() < fence)
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}
