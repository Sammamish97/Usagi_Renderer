#include "LinePass.h"
#include "DxEngine.h"
#include "BufferFormat.h"
#include "DxUtil.h"
#include <DirectXMath.h>
LinePass::LinePass(ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader)
	:IPass(vertShader, pixelShader)
{
	InitRootSignature();
	InitPSO();
}
void LinePass::InitRootSignature()
{
    //Forward Pass use these uniform values
        //Object World
        //Common CB
    auto device = DxEngine::Get().GetDevice();
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_ROOT_PARAMETER1 rootParameters[2];
    rootParameters[0].InitAsConstants(sizeof(DirectX::XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsConstantBufferView(1);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;

    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, featureData.HighestVersion, &rootSignatureBlob, &errorBlob))
        ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(mRootSig.GetAddressOf())))
}
void LinePass::InitPSO()
{
    //ForwardPass use these frame buffers
        //RTV, DSV
    auto device = DxEngine::Get().GetDevice();
    auto msaaState = DxEngine::Get().GetMultisampleQualityLevels(BufferFormat::GetBufferFormat(BufferType::SWAPCHAIN), D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC defaultPSODesc;
    ZeroMemory(&defaultPSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    defaultPSODesc.pRootSignature = mRootSig.Get();
    defaultPSODesc.VS =
    {
        reinterpret_cast<BYTE*>(mVertShader->GetBufferPointer()),
        mVertShader->GetBufferSize()
    };
    defaultPSODesc.PS =
    {
        reinterpret_cast<BYTE*>(mPixelShader->GetBufferPointer()),
        mPixelShader->GetBufferSize()
    };
    defaultPSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    defaultPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    defaultPSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    defaultPSODesc.SampleMask = UINT_MAX;
    defaultPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    defaultPSODesc.NumRenderTargets = 1;
    defaultPSODesc.RTVFormats[0] = BufferFormat::GetBufferFormat(BufferType::SWAPCHAIN);
    defaultPSODesc.DSVFormat = BufferFormat::GetBufferFormat(BufferType::DEPTH_STENCIL_DSV);
    /* defaultPSODesc.SampleDesc.Count = msaaState.Count;
     defaultPSODesc.SampleDesc.Quality = msaaState.Quality;*/
    defaultPSODesc.SampleDesc.Count = 1;
    defaultPSODesc.SampleDesc.Quality = 0;

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    defaultPSODesc.InputLayout = { inputLayout, _countof(inputLayout) };

    ThrowIfFailed(device->CreateGraphicsPipelineState(&defaultPSODesc, IID_PPV_ARGS(mPSO.GetAddressOf())))
}