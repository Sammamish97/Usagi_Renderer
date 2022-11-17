#pragma once

#include <DirectXMath.h> // For XMFLOAT2
#include <cstdint>
#include <vector>

#include "Texture.h"

enum AttachmentPoint
{
    Color0,
    Color1,
    Color2,
    Color3,
    Color4,
    Color5,
    Color6,
    Color7,
    DepthStencil,
    NumAttachmentPoints
};

class RenderTarget
{
public:
    RenderTarget();

    RenderTarget(const RenderTarget& copy) = default;
    RenderTarget(RenderTarget&& copy) = default;

    RenderTarget& operator=(const RenderTarget& other) = default;
    RenderTarget& operator=(RenderTarget&& other) = default;

    void AttachTexture(AttachmentPoint attachmentPoint, const Texture& texture);
    const Texture& GetTexture(AttachmentPoint attachmentPoint) const;

    DirectX::XMUINT2 GetSize() const;
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;

    D3D12_VIEWPORT GetViewport(DirectX::XMFLOAT2 scale = { 1.0f, 1.0f }, DirectX::XMFLOAT2 bias = { 0.0f, 0.0f }, float minDepth = 0.0f, float maxDepth = 1.0f) const;

    // Get a list of the textures attached to the render target.
    // This method is primarily used by the CommandList when binding the
    // render target to the output merger stage of the rendering pipeline.
    const std::vector<Texture>& GetTextures() const;

    // Get the render target formats of the textures currently 
    // attached to this render target object.
    // This is needed to configure the Pipeline state object.
    D3D12_RT_FORMAT_ARRAY GetRenderTargetFormats() const;

    // Get the format of the attached depth/stencil buffer.
    DXGI_FORMAT GetDepthStencilFormat() const;

private:
    std::vector<Texture> m_Textures;
    DirectX::XMUINT2 m_Size;
};

