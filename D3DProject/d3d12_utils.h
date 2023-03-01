#pragma once

#include <d3d12.h>

struct CD3DX12_DEFAULT {};
extern const DECLSPEC_SELECTANY CD3DX12_DEFAULT D3D12_DEFAULT;

struct CD3DX12_RESOURCE_BARRIER : public D3D12_RESOURCE_BARRIER
{
	CD3DX12_RESOURCE_BARRIER() = default;
	explicit CD3DX12_RESOURCE_BARRIER(const D3D12_RESOURCE_BARRIER& o) noexcept :
		D3D12_RESOURCE_BARRIER(o)
	{}
	static inline CD3DX12_RESOURCE_BARRIER Transition(
		_In_ ID3D12Resource* pResource,
		D3D12_RESOURCE_STATES stateBefore,
		D3D12_RESOURCE_STATES stateAfter,
		UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
		D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE) noexcept
	{
		CD3DX12_RESOURCE_BARRIER result = {};
		D3D12_RESOURCE_BARRIER& barrier = result;
		result.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		result.Flags = flags;
		barrier.Transition.pResource = pResource;
		barrier.Transition.StateBefore = stateBefore;
		barrier.Transition.StateAfter = stateAfter;
		barrier.Transition.Subresource = subresource;
		return result;
	}
	static inline CD3DX12_RESOURCE_BARRIER Aliasing(
		_In_ ID3D12Resource* pResourceBefore,
		_In_ ID3D12Resource* pResourceAfter) noexcept
	{
		CD3DX12_RESOURCE_BARRIER result = {};
		D3D12_RESOURCE_BARRIER& barrier = result;
		result.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
		barrier.Aliasing.pResourceBefore = pResourceBefore;
		barrier.Aliasing.pResourceAfter = pResourceAfter;
		return result;
	}
	static inline CD3DX12_RESOURCE_BARRIER UAV(
		_In_ ID3D12Resource* pResource) noexcept
	{
		CD3DX12_RESOURCE_BARRIER result = {};
		D3D12_RESOURCE_BARRIER& barrier = result;
		result.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.UAV.pResource = pResource;
		return result;
	}
};

struct CD3DX12_CPU_DESCRIPTOR_HANDLE : public D3D12_CPU_DESCRIPTOR_HANDLE
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE() = default;
	explicit CD3DX12_CPU_DESCRIPTOR_HANDLE(const D3D12_CPU_DESCRIPTOR_HANDLE& o) noexcept :
		D3D12_CPU_DESCRIPTOR_HANDLE(o)
	{}
	CD3DX12_CPU_DESCRIPTOR_HANDLE(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE& other, INT offsetScaledByIncrementSize) noexcept
	{
		InitOffsetted(other, offsetScaledByIncrementSize);
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE& other, INT offsetInDescriptors, UINT descriptorIncrementSize) noexcept
	{
		InitOffsetted(other, offsetInDescriptors, descriptorIncrementSize);
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE& Offset(INT offsetInDescriptors, UINT descriptorIncrementSize) noexcept
	{
		ptr = SIZE_T(INT64(ptr) + INT64(offsetInDescriptors) * INT64(descriptorIncrementSize));
		return *this;
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE& Offset(INT offsetScaledByIncrementSize) noexcept
	{
		ptr = SIZE_T(INT64(ptr) + INT64(offsetScaledByIncrementSize));
		return *this;
	}
	bool operator==(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE& other) const noexcept
	{
		return (ptr == other.ptr);
	}
	bool operator!=(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE& other) const noexcept
	{
		return (ptr != other.ptr);
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE& operator=(const D3D12_CPU_DESCRIPTOR_HANDLE& other) noexcept
	{
		ptr = other.ptr;
		return *this;
	}

	inline void InitOffsetted(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE& base, INT offsetScaledByIncrementSize) noexcept
	{
		InitOffsetted(*this, base, offsetScaledByIncrementSize);
	}

	inline void InitOffsetted(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE& base, INT offsetInDescriptors, UINT descriptorIncrementSize) noexcept
	{
		InitOffsetted(*this, base, offsetInDescriptors, descriptorIncrementSize);
	}

	static inline void InitOffsetted(_Out_ D3D12_CPU_DESCRIPTOR_HANDLE& handle, _In_ const D3D12_CPU_DESCRIPTOR_HANDLE& base, INT offsetScaledByIncrementSize) noexcept
	{
		handle.ptr = SIZE_T(INT64(base.ptr) + INT64(offsetScaledByIncrementSize));
	}

	static inline void InitOffsetted(_Out_ D3D12_CPU_DESCRIPTOR_HANDLE& handle, _In_ const D3D12_CPU_DESCRIPTOR_HANDLE& base, INT offsetInDescriptors, UINT descriptorIncrementSize) noexcept
	{
		handle.ptr = SIZE_T(INT64(base.ptr) + INT64(offsetInDescriptors) * INT64(descriptorIncrementSize));
	}
};

struct CD3DX12_ROOT_SIGNATURE_DESC : public D3D12_ROOT_SIGNATURE_DESC
{
	CD3DX12_ROOT_SIGNATURE_DESC() = default;
	explicit CD3DX12_ROOT_SIGNATURE_DESC(const D3D12_ROOT_SIGNATURE_DESC& o) noexcept :
		D3D12_ROOT_SIGNATURE_DESC(o)
	{}
	CD3DX12_ROOT_SIGNATURE_DESC(
		UINT numParameters,
		_In_reads_opt_(numParameters) const D3D12_ROOT_PARAMETER* _pParameters,
		UINT numStaticSamplers = 0,
		_In_reads_opt_(numStaticSamplers) const D3D12_STATIC_SAMPLER_DESC* _pStaticSamplers = nullptr,
		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE) noexcept
	{
		Init(numParameters, _pParameters, numStaticSamplers, _pStaticSamplers, flags);
	}
	CD3DX12_ROOT_SIGNATURE_DESC(CD3DX12_DEFAULT) noexcept
	{
		Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);
	}

	inline void Init(
		UINT numParameters,
		_In_reads_opt_(numParameters) const D3D12_ROOT_PARAMETER* _pParameters,
		UINT numStaticSamplers = 0,
		_In_reads_opt_(numStaticSamplers) const D3D12_STATIC_SAMPLER_DESC* _pStaticSamplers = nullptr,
		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE) noexcept
	{
		Init(*this, numParameters, _pParameters, numStaticSamplers, _pStaticSamplers, flags);
	}

	static inline void Init(
		_Out_ D3D12_ROOT_SIGNATURE_DESC& desc,
		UINT numParameters,
		_In_reads_opt_(numParameters) const D3D12_ROOT_PARAMETER* _pParameters,
		UINT numStaticSamplers = 0,
		_In_reads_opt_(numStaticSamplers) const D3D12_STATIC_SAMPLER_DESC* _pStaticSamplers = nullptr,
		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE) noexcept
	{
		desc.NumParameters = numParameters;
		desc.pParameters = _pParameters;
		desc.NumStaticSamplers = numStaticSamplers;
		desc.pStaticSamplers = _pStaticSamplers;
		desc.Flags = flags;
	}
};

struct CD3DX12_VIEWPORT : public D3D12_VIEWPORT
{
	CD3DX12_VIEWPORT() = default;
	explicit CD3DX12_VIEWPORT(const D3D12_VIEWPORT& o) noexcept :
		D3D12_VIEWPORT(o)
	{}
	explicit CD3DX12_VIEWPORT(
		FLOAT topLeftX,
		FLOAT topLeftY,
		FLOAT width,
		FLOAT height,
		FLOAT minDepth = D3D12_MIN_DEPTH,
		FLOAT maxDepth = D3D12_MAX_DEPTH) noexcept
	{
		TopLeftX = topLeftX;
		TopLeftY = topLeftY;
		Width = width;
		Height = height;
		MinDepth = minDepth;
		MaxDepth = maxDepth;
	}
	explicit CD3DX12_VIEWPORT(
		_In_ ID3D12Resource* pResource,
		UINT mipSlice = 0,
		FLOAT topLeftX = 0.0f,
		FLOAT topLeftY = 0.0f,
		FLOAT minDepth = D3D12_MIN_DEPTH,
		FLOAT maxDepth = D3D12_MAX_DEPTH) noexcept
	{
		auto Desc = pResource->GetDesc();
		const UINT64 SubresourceWidth = Desc.Width >> mipSlice;
		const UINT64 SubresourceHeight = Desc.Height >> mipSlice;
		switch (Desc.Dimension)
		{
		case D3D12_RESOURCE_DIMENSION_BUFFER:
			TopLeftX = topLeftX;
			TopLeftY = 0.0f;
			Width = float(Desc.Width) - topLeftX;
			Height = 1.0f;
			break;
		case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
			TopLeftX = topLeftX;
			TopLeftY = 0.0f;
			Width = (SubresourceWidth ? float(SubresourceWidth) : 1.0f) - topLeftX;
			Height = 1.0f;
			break;
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
			TopLeftX = topLeftX;
			TopLeftY = topLeftY;
			Width = (SubresourceWidth ? float(SubresourceWidth) : 1.0f) - topLeftX;
			Height = (SubresourceHeight ? float(SubresourceHeight) : 1.0f) - topLeftY;
			break;
		default: break;
		}

		MinDepth = minDepth;
		MaxDepth = maxDepth;
	}
};

struct CD3DX12_RECT : public D3D12_RECT
{
	CD3DX12_RECT() = default;
	explicit CD3DX12_RECT(const D3D12_RECT& o) noexcept :
		D3D12_RECT(o)
	{}
	explicit CD3DX12_RECT(
		LONG Left,
		LONG Top,
		LONG Right,
		LONG Bottom) noexcept
	{
		left = Left;
		top = Top;
		right = Right;
		bottom = Bottom;
	}
};
