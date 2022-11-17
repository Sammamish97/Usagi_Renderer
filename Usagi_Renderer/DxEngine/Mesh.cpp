#include "pch.h"
#include "Mesh.h"
Mesh::Mesh(std::vector<Vertex> input_vertices, std::vector<UINT> input_indices, CommandList& commandList)
	:mVertices(std::move(input_vertices)), mIndices(std::move(input_indices)), mIndexCount(0), mBoneCount(0)
{
	Init(commandList);
}

void Mesh::Init(CommandList& commandList)
{
	if (mVertices.size() >= UINT_MAX)
	{
		throw std::exception("Too many vertices for 16-bit index buffer");
	}

	commandList.CopyVertexBuffer(mVertexBuffer, mVertices);
	mVertexBuffer.CreateVertexBufferView(mVertices.size(), sizeof(mVertices[0]));
	commandList.CopyIndexBuffer(mIndexBuffer, mIndices);
	mIndexBuffer.CreateIndexBufferView(mIndices.size(), sizeof(mIndices[0]));

	mIndexCount = static_cast<UINT>(mIndices.size());
}

void Mesh::Draw(CommandList& commandList)
{
	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList.SetVertexBuffer(0, mVertexBuffer);
	commandList.SetIndexBuffer(mIndexBuffer);
	commandList.DrawIndexed(mIndexCount);
}