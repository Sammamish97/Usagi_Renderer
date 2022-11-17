#pragma once
#include <DirectXMath.h>
#include <d3dx12.h>
#include <vector>
#include <wrl.h>

#include "CommandList.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"

using namespace Microsoft::WRL;
using namespace DirectX;

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT2 UV;
	XMFLOAT3 tangent;
	XMFLOAT3 biTangent;
};

class Mesh
{
public:
	Mesh(std::vector<Vertex> input_vertices, std::vector<UINT> input_indices, CommandList& commandList);
	void Draw(CommandList& commandList);

private:
	VertexBuffer mVertexBuffer;
	IndexBuffer mIndexBuffer;

	std::vector<Vertex> mVertices;
	std::vector<UINT> mIndices;

	UINT mIndexCount;
	UINT mBoneCount;

private:
	void Init(CommandList& commandList);
};

