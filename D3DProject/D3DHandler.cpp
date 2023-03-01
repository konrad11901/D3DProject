#include "pch.h"
#include "D3DHandler.h"
#include "Win32Application.h"
#include "vertex_shader.h"
#include "pixel_shader.h"
#include "BitmapDefinition.h"
#include "SceneData.h"

D3DHandler::D3DHandler(UINT width, UINT height)
	: frame_index(0), rtv_descriptor_size(0), viewport(0.0f, 0.0f, static_cast<FLOAT>(width),
		static_cast<FLOAT>(height)), scissor_rect(0, 0, width, height), width(width), height(height) {
	SceneData scene_data(SCENE_PATH);
	triangle_data = scene_data.GetTriangleData();
}

void D3DHandler::OnInit() {
	LoadPipeline();
	LoadAssets();
}

void D3DHandler::OnRender() {
	PopulateCommandList();

	ID3D12CommandList* command_lists[] = { command_list.get() };
	command_queue->ExecuteCommandLists(_countof(command_lists), command_lists);

	winrt::check_hresult(swap_chain->Present(1, 0));

	WaitForPreviousFrame();
}

void D3DHandler::OnDestroy() {
	WaitForPreviousFrame();

	CloseHandle(fence_event);
}

void D3DHandler::LoadPipeline() {
	winrt::check_hresult(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));

	UINT dxgi_factory_flags = 0;

#if defined(_DEBUG)
	{
		winrt::com_ptr<ID3D12Debug> debug_controller;
		winrt::check_hresult(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)));
		debug_controller->EnableDebugLayer();

		dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	winrt::com_ptr<IDXGIFactory7> factory;
	winrt::check_hresult(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(factory.put())));

	CreateDevice();

	CreateCommandQueue();

	CreateSwapChain(factory.get());

	winrt::check_hresult(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

	CreateDescriptorHeaps();

	CreateFrameResources();

	CreateCommandAllocator();
}

void D3DHandler::LoadAssets() {
	CreateRootSignature();

	CreatePipelineState();

	CreateCommandList();

	CreateVertexBuffer();

	CreateConstantBuffer();

	CreateDepthBuffer();

	CreateTextureAndSynchronizationResources();
}

void D3DHandler::PopulateCommandList() {
	winrt::check_hresult(command_allocator->Reset());
	winrt::check_hresult(command_list->Reset(command_allocator.get(), pipeline_state.get()));

	command_list->SetGraphicsRootSignature(root_signature.get());

	ID3D12DescriptorHeap* heaps[] = { cbv_heap.get() };
	command_list->SetDescriptorHeaps(_countof(heaps), heaps);
	D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle = cbv_heap->GetGPUDescriptorHandleForHeapStart();
	command_list->SetGraphicsRootDescriptorTable(0, gpu_desc_handle);
	gpu_desc_handle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	command_list->SetGraphicsRootDescriptorTable(1, gpu_desc_handle);

	command_list->RSSetViewports(1, &viewport);
	command_list->RSSetScissorRects(1, &scissor_rect);

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(render_targets[frame_index].get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	command_list->ResourceBarrier(1, &barrier);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(rtv_heap->GetCPUDescriptorHandleForHeapStart(),
		frame_index, rtv_descriptor_size);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsv_handle(dsv_heap->GetCPUDescriptorHandleForHeapStart());
	command_list->OMSetRenderTargets(1, &rtv_handle, FALSE, &dsv_handle);

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	command_list->ClearRenderTargetView(rtv_handle, clearColor, 0, nullptr);
	command_list->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	command_list->IASetVertexBuffers(0, 1, &vertex_buffer_view);
	command_list->DrawInstanced(static_cast<UINT>(triangle_data.size()), 1, 0, 0);

	barrier = CD3DX12_RESOURCE_BARRIER::Transition(render_targets[frame_index].get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	command_list->ResourceBarrier(1, &barrier);

	winrt::check_hresult(command_list->Close());
}

void D3DHandler::WaitForPreviousFrame() {
	const UINT64 fence_tmp = fence_value;
	winrt::check_hresult(command_queue->Signal(fence.get(), fence_tmp));
	fence_value++;

	if (fence->GetCompletedValue() < fence_tmp) {
		winrt::check_hresult(fence->SetEventOnCompletion(fence_tmp, fence_event));
		WaitForSingleObject(fence_event, INFINITE);
	}

	frame_index = swap_chain->GetCurrentBackBufferIndex();
}

void D3DHandler::CreateDevice() {
	winrt::check_hresult(D3D12CreateDevice(
		nullptr,
		D3D_FEATURE_LEVEL_12_0,
		IID_PPV_ARGS(device.put())
	));
}

void D3DHandler::CreateCommandQueue() {
	D3D12_COMMAND_QUEUE_DESC queue_desc = {};
	queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	winrt::check_hresult(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(command_queue.put())));
}

void D3DHandler::CreateSwapChain(IDXGIFactory7* factory) {
	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
	swap_chain_desc.BufferCount = FRAME_COUNT;
	swap_chain_desc.Width = 0;
	swap_chain_desc.Height = 0;
	swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swap_chain_desc.SampleDesc.Count = 1;

	winrt::com_ptr<IDXGISwapChain1> tmp_swap_chain;
	winrt::check_hresult(factory->CreateSwapChainForHwnd(
		command_queue.get(),
		Win32Application::GetHwnd(),
		&swap_chain_desc,
		nullptr,
		nullptr,
		tmp_swap_chain.put()
	));

	tmp_swap_chain.as(swap_chain);

	frame_index = swap_chain->GetCurrentBackBufferIndex();
}

void D3DHandler::CreateDescriptorHeaps() {
	D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
	rtv_heap_desc.NumDescriptors = FRAME_COUNT;
	rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	winrt::check_hresult(device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(rtv_heap.put())));

	rtv_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = 2,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		.NodeMask = 0
	};
	winrt::check_hresult(device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(cbv_heap.put())));

	D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask = 0
	};
	winrt::check_hresult(device->CreateDescriptorHeap(&dsv_heap_desc, IID_PPV_ARGS(dsv_heap.put())));
}

void D3DHandler::CreateFrameResources() {
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtv_heap->GetCPUDescriptorHandleForHeapStart();

	for (UINT n = 0; n < FRAME_COUNT; n++) {
		winrt::check_hresult(swap_chain->GetBuffer(n, IID_PPV_ARGS(render_targets[n].put())));
		device->CreateRenderTargetView(render_targets[n].get(), nullptr, rtvHandle);
		rtvHandle.ptr += rtv_descriptor_size;
	}
}

void D3DHandler::CreateCommandAllocator() {
	winrt::check_hresult(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(command_allocator.put())));
}

void D3DHandler::CreateRootSignature() {
	D3D12_DESCRIPTOR_RANGE descriptor_ranges[] = {
	{
		.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		.NumDescriptors = 1,
		.BaseShaderRegister = 0,
		.RegisterSpace = 0,
		.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	},
	{
		.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		.NumDescriptors = 1,
		.BaseShaderRegister = 0,
		.RegisterSpace = 0,
		.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	}
	};
	D3D12_ROOT_PARAMETER root_parameters[] = {
		{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
			.DescriptorTable = { 1, descriptor_ranges },
			.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX
		},
		{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
			.DescriptorTable = { 1, descriptor_ranges + 1 },
			.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
		}
	};
	D3D12_STATIC_SAMPLER_DESC tex_sampler_desc = {
		.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		.MipLODBias = 0,
		.MaxAnisotropy = 0,
		.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
		.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
		.MinLOD = 0.0f,
		.MaxLOD = D3D12_FLOAT32_MAX,
		.ShaderRegister = 0,
		.RegisterSpace = 0,
		.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
	};
	CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc{};
	root_signature_desc.Init(_countof(root_parameters), root_parameters, 1, &tex_sampler_desc,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
	);

	winrt::com_ptr<ID3DBlob> signature, error;
	winrt::check_hresult(D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1,
		signature.put(), error.put()));
	winrt::check_hresult(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
		IID_PPV_ARGS(root_signature.put())));
}

void D3DHandler::CreatePipelineState() {
	D3D12_INPUT_ELEMENT_DESC input_element_descs[] = {
		{
			.SemanticName = "POSITION",
			.SemanticIndex = 0,
			.Format = DXGI_FORMAT_R32G32B32_FLOAT,
			.InputSlot = 0,
			.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
			.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			.InstanceDataStepRate = 0
		},
		{
			.SemanticName = "COLOR",
			.SemanticIndex = 0,
			.Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
			.InputSlot = 0,
			.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
			.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			.InstanceDataStepRate = 0
		},
		{
			.SemanticName = "TEXCOORD",
			.SemanticIndex = 0,
			.Format = DXGI_FORMAT_R32G32_FLOAT,
			.InputSlot = 0,
			.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
			.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			.InstanceDataStepRate = 0
		},
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
	pso_desc.InputLayout = { input_element_descs, _countof(input_element_descs) };
	pso_desc.pRootSignature = root_signature.get();
	pso_desc.VS = { vs_main, sizeof(vs_main) };
	pso_desc.PS = { ps_main, sizeof(ps_main) };
	pso_desc.RasterizerState = {
		.FillMode = D3D12_FILL_MODE_SOLID,
		.CullMode = D3D12_CULL_MODE_BACK,
		.FrontCounterClockwise = FALSE,
		.DepthBias = D3D12_DEFAULT_DEPTH_BIAS,
		.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
		.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
		.DepthClipEnable = TRUE,
		.MultisampleEnable = FALSE,
		.AntialiasedLineEnable = FALSE,
		.ForcedSampleCount = 0,
		.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
	};
	pso_desc.BlendState = {
		.AlphaToCoverageEnable = FALSE,
		.IndependentBlendEnable = FALSE,
		.RenderTarget = {
			{
				.BlendEnable = FALSE,
				.LogicOpEnable = FALSE,
				.SrcBlend = D3D12_BLEND_ONE,
				.DestBlend = D3D12_BLEND_ZERO,
				.BlendOp = D3D12_BLEND_OP_ADD,
				.SrcBlendAlpha = D3D12_BLEND_ONE,
				.DestBlendAlpha = D3D12_BLEND_ZERO,
				.BlendOpAlpha = D3D12_BLEND_OP_ADD,
				.LogicOp = D3D12_LOGIC_OP_NOOP,
				.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL
			}
		}
	};
	pso_desc.DepthStencilState = {
		.DepthEnable = TRUE,
		.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
		.DepthFunc = D3D12_COMPARISON_FUNC_LESS,
		.StencilEnable = FALSE,
		.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
		.StencilWriteMask = D3D12_DEFAULT_STENCIL_READ_MASK,
		.FrontFace = {
			.StencilFailOp = D3D12_STENCIL_OP_KEEP,
			.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
			.StencilPassOp = D3D12_STENCIL_OP_KEEP,
			.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
		},
		.BackFace = {
			.StencilFailOp = D3D12_STENCIL_OP_KEEP,
			.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
			.StencilPassOp = D3D12_STENCIL_OP_KEEP,
			.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
		}
	};
	pso_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pso_desc.SampleMask = UINT_MAX;
	pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pso_desc.NumRenderTargets = 1;
	pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pso_desc.SampleDesc.Count = 1;

	device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(pipeline_state.put()));
}

void D3DHandler::CreateCommandList() {
	winrt::check_hresult(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator.get(),
		pipeline_state.get(), IID_PPV_ARGS(command_list.put())));
}

void D3DHandler::CreateVertexBuffer() {
	D3D12_HEAP_PROPERTIES heap_properties = {
		.Type = D3D12_HEAP_TYPE_UPLOAD,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		.CreationNodeMask = 1,
		.VisibleNodeMask = 1
	};
	D3D12_RESOURCE_DESC resource_desc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment = 0,
		.Width = triangle_data.size() * sizeof(vertex_t),
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_UNKNOWN,
		.SampleDesc = {.Count = 1, .Quality = 0 },
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags = D3D12_RESOURCE_FLAG_NONE
	};

	device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(vertex_buffer.put()));

	UINT8* vertex_data_begin = nullptr;
	D3D12_RANGE read_range = { 0, 0 };
	winrt::check_hresult(vertex_buffer->Map(0, &read_range, reinterpret_cast<void**>(&vertex_data_begin)));
	memcpy(vertex_data_begin, triangle_data.data(), triangle_data.size() * sizeof(vertex_t));
	vertex_buffer->Unmap(0, nullptr);

	vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
	vertex_buffer_view.StrideInBytes = sizeof(vertex_t);
	vertex_buffer_view.SizeInBytes = static_cast<UINT>(triangle_data.size()) * sizeof(vertex_t);
}

void D3DHandler::CreateConstantBuffer() {
	D3D12_HEAP_PROPERTIES heap_properties = {
		.Type = D3D12_HEAP_TYPE_UPLOAD,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		.CreationNodeMask = 1,
		.VisibleNodeMask = 1
	};
	D3D12_RESOURCE_DESC resource_desc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment = 0,
		.Width = CONST_BUFFER_SIZE,
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_UNKNOWN,
		.SampleDesc = {.Count = 1, .Quality = 0 },
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags = D3D12_RESOURCE_FLAG_NONE
	};
	device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(constant_buffer.put()));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {
		.BufferLocation = constant_buffer->GetGPUVirtualAddress(),
		.SizeInBytes = CONST_BUFFER_SIZE
	};
	device->CreateConstantBufferView(&cbv_desc, cbv_heap->GetCPUDescriptorHandleForHeapStart());

	XMStoreFloat4x4(&const_buffer_data.matWorldViewProj, XMMatrixIdentity());
	D3D12_RANGE read_range = { 0, 0 };
	winrt::check_hresult(constant_buffer->Map(0, &read_range, reinterpret_cast<void**>(&cbv_data_begin)));
	memcpy(cbv_data_begin, &const_buffer_data, sizeof(const_buffer_data));
}

void D3DHandler::CreateDepthBuffer() {
	D3D12_HEAP_PROPERTIES heap_properties = {
		.Type = D3D12_HEAP_TYPE_DEFAULT,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		.CreationNodeMask = 1,
		.VisibleNodeMask = 1
	};
	D3D12_RESOURCE_DESC resource_desc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		.Alignment = 0,
		.Width = width,
		.Height = height,
		.DepthOrArraySize = 1,
		.MipLevels = 0,
		.Format = DXGI_FORMAT_D32_FLOAT,
		.SampleDesc = {.Count = 1, .Quality = 0 },
		.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	};
	D3D12_CLEAR_VALUE clear_value = {
		.Format = DXGI_FORMAT_D32_FLOAT,
		.DepthStencil = {.Depth = 1.0f, .Stencil = 0 }
	};
	device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value, IID_PPV_ARGS(depth_buffer.put()));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {
		.Format = DXGI_FORMAT_D32_FLOAT,
		.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
		.Flags = D3D12_DSV_FLAG_NONE,
		.Texture2D = {}
	};
	device->CreateDepthStencilView(depth_buffer.get(), &dsv_desc, dsv_heap->GetCPUDescriptorHandleForHeapStart());
}

void D3DHandler::CreateTextureAndSynchronizationResources() {
	winrt::com_ptr<IWICImagingFactory2> imaging_factory;
	winrt::check_hresult(CoCreateInstance(
		CLSID_WICImagingFactory2,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(imaging_factory.put())
	));
	BitmapDefinition texture_bitmap(TEXTURE_PATH);
	UINT bmp_width, bmp_height;
	texture_bitmap.CreateDeviceIndependentResources(imaging_factory.get());
	auto bmp_bits = texture_bitmap.GetBitmapAsBytes(&bmp_width, &bmp_height);

	D3D12_HEAP_PROPERTIES tex_heap_prop = {
		.Type = D3D12_HEAP_TYPE_DEFAULT,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		.CreationNodeMask = 1,
		.VisibleNodeMask = 1
	};
	D3D12_RESOURCE_DESC tex_resource_desc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		.Alignment = 0,
		.Width = bmp_width,
		.Height = bmp_height,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.SampleDesc = {.Count = 1, .Quality = 0 },
		.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		.Flags = D3D12_RESOURCE_FLAG_NONE
	};
	device->CreateCommittedResource(
		&tex_heap_prop, D3D12_HEAP_FLAG_NONE,
		&tex_resource_desc, D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr, IID_PPV_ARGS(&texture_resource)
	);

	winrt::com_ptr<ID3D12Resource> texture_upload_buffer = nullptr;
	UINT64 required_size = 0;
	auto desc = texture_resource->GetDesc();
	ID3D12Device* helper_device = nullptr;
	texture_resource->GetDevice(__uuidof(*helper_device), reinterpret_cast<void**>(&helper_device));
	helper_device->GetCopyableFootprints(&desc, 0, 1, 0, nullptr, nullptr, nullptr, &required_size);
	helper_device->Release();

	D3D12_HEAP_PROPERTIES tex_upload_heap_prop = {
		.Type = D3D12_HEAP_TYPE_UPLOAD,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		.CreationNodeMask = 1,
		.VisibleNodeMask = 1
	};
	D3D12_RESOURCE_DESC tex_upload_resource_desc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment = 0,
		.Width = required_size,
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_UNKNOWN,
		.SampleDesc = {.Count = 1, .Quality = 0 },
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags = D3D12_RESOURCE_FLAG_NONE
	};
	device->CreateCommittedResource(
		&tex_upload_heap_prop, D3D12_HEAP_FLAG_NONE,
		&tex_upload_resource_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&texture_upload_buffer)
	);

	D3D12_SUBRESOURCE_DATA texture_data = {
		.pData = bmp_bits,
		.RowPitch = bmp_width * BMP_PX_SIZE,
		.SlicePitch = bmp_width * bmp_height * BMP_PX_SIZE
	};
	UINT const MAX_SUBRESOURCES = 1;
	required_size = 0;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts[MAX_SUBRESOURCES];
	UINT num_rows[MAX_SUBRESOURCES];
	UINT64 row_sizes_in_bytes[MAX_SUBRESOURCES];
	desc = texture_resource->GetDesc();
	helper_device = nullptr;
	texture_resource->GetDevice(__uuidof(*helper_device), reinterpret_cast<void**>(&helper_device));
	helper_device->GetCopyableFootprints(&desc, 0, 1, 0, layouts, num_rows, row_sizes_in_bytes, &required_size);
	helper_device->Release();

	BYTE* map_tex_data = nullptr;
	texture_upload_buffer->Map(0, nullptr, reinterpret_cast<void**>(&map_tex_data));
	D3D12_MEMCPY_DEST dest_data = {
		.pData = map_tex_data + layouts[0].Offset,
		.RowPitch = layouts[0].Footprint.RowPitch,
		.SlicePitch = SIZE_T(layouts[0].Footprint.RowPitch) * SIZE_T(num_rows[0])
	};
	for (UINT z = 0; z < layouts[0].Footprint.Depth; ++z) {
		auto dest_slice = static_cast<UINT8*>(dest_data.pData) + dest_data.SlicePitch * z;
		auto src_slice = static_cast<const UINT8*>(texture_data.pData) + texture_data.SlicePitch * LONG_PTR(z);
		for (UINT y = 0; y < num_rows[0]; ++y) {
			memcpy(
				dest_slice + dest_data.RowPitch * y,
				src_slice + texture_data.RowPitch * LONG_PTR(y),
				static_cast<SIZE_T>(row_sizes_in_bytes[0])
			);
		}
	}
	texture_upload_buffer->Unmap(0, nullptr);

	D3D12_TEXTURE_COPY_LOCATION dst = {
		.pResource = texture_resource.get(),
		.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
		.SubresourceIndex = 0
	};
	D3D12_TEXTURE_COPY_LOCATION src = {
		.pResource = texture_upload_buffer.get(),
		.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
		.PlacedFootprint = layouts[0]
	};
	command_list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
	D3D12_RESOURCE_BARRIER tex_upload_resource_barrier = {
		.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
		.Transition = {
			.pResource = texture_resource.get(),
			.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
			.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		},
	};
	command_list->ResourceBarrier(1, &tex_upload_resource_barrier);
	command_list->Close();
	ID3D12CommandList* cmd_list = command_list.get();
	command_queue->ExecuteCommandLists(1, &cmd_list);

	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {
		.Format = tex_resource_desc.Format,
		.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		.Texture2D = {
			.MostDetailedMip = 0,
			.MipLevels = 1,
			.PlaneSlice = 0,
			.ResourceMinLODClamp = 0.0f
		},
	};
	D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle = cbv_heap->GetCPUDescriptorHandleForHeapStart();
	cpu_desc_handle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	device->CreateShaderResourceView(texture_resource.get(), &srv_desc, cpu_desc_handle);

	winrt::check_hresult(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.put())));
	fence_value = 1;

	fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fence_event == nullptr) {
		winrt::check_hresult(HRESULT_FROM_WIN32(GetLastError()));
	}

	WaitForPreviousFrame();

	delete bmp_bits;
}

void D3DHandler::OnUpdate() {
	if (GetAsyncKeyState('A') < 0) {
		angle += ROTATION_SPEED;
	}
	if (GetAsyncKeyState('D') < 0) {
		angle -= ROTATION_SPEED;
	}
	if (GetAsyncKeyState('W') < 0) {
		pos_x -= sin(angle) * MOVE_SPEED;
		pos_z -= -cos(angle) * MOVE_SPEED;
	}
	if (GetAsyncKeyState('S') < 0) {
		pos_x += sin(angle) * MOVE_SPEED;
		pos_z += -cos(angle) * MOVE_SPEED;
	}

	XMMATRIX wvp_matrix;
	wvp_matrix = XMMatrixMultiply(
		XMMatrixTranslation(-pos_x, -pos_y, -pos_z),
		XMMatrixRotationY(angle)
	);
	wvp_matrix = XMMatrixMultiply(
		wvp_matrix,
		XMMatrixPerspectiveFovLH(
			45.0f, viewport.Width / viewport.Height, 1.0f, 100.0f
		)
	);
	wvp_matrix = XMMatrixTranspose(wvp_matrix);
	XMStoreFloat4x4(
		&const_buffer_data.matWorldViewProj,
		wvp_matrix
	);
	memcpy(cbv_data_begin, &const_buffer_data, sizeof(const_buffer_data));
}
