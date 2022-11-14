#include "pch.h"
#include "ConstantBuffer.h"
#include "d3dx12.h"

ConstantBuffer::ConstantBuffer(const std::wstring& name)
	: Buffer(name)
	, m_SizeInBytes(0)
{
}

ConstantBuffer::~ConstantBuffer()
{
}
