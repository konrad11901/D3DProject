#pragma once

#include "vertex.h"

using namespace DirectX;

class D3DHandler {
public:
	D3DHandler(UINT width, UINT height);

	void OnInit();
	void OnRender();
	void OnUpdate();
	void OnDestroy();

private:
	struct vs_const_buffer_t {
		XMFLOAT4X4 matWorldViewProj;
		XMFLOAT4 padding[(256 - sizeof(XMFLOAT4X4)) / sizeof(XMFLOAT4)];
	};

	static constexpr UINT FRAME_COUNT = 2;
	static constexpr std::size_t VERTEX_SIZE = sizeof(vertex_t) / sizeof(FLOAT);
	static constexpr UINT BMP_PX_SIZE = 4;
	static constexpr FLOAT ROTATION_SPEED = 0.03f;
	static constexpr FLOAT MOVE_SPEED = 0.05f;
	static constexpr std::size_t CONST_BUFFER_SIZE = sizeof(vs_const_buffer_t);

	static constexpr PCWSTR TEXTURE_PATH = L"Assets\\Texture.png";
	static constexpr char SCENE_PATH[] = "Assets\\SceneData.obj";

	winrt::com_ptr<IDXGISwapChain4> swap_chain;
	winrt::com_ptr<ID3D12Device9> device;
	winrt::com_ptr<ID3D12CommandQueue> command_queue;
	winrt::com_ptr<ID3D12DescriptorHeap> rtv_heap;
	winrt::com_ptr<ID3D12Resource2> render_targets[FRAME_COUNT];
	winrt::com_ptr<ID3D12CommandAllocator> command_allocator;
	winrt::com_ptr<ID3D12GraphicsCommandList6> command_list;
	winrt::com_ptr<ID3D12PipelineState> pipeline_state;

	winrt::com_ptr<ID3D12RootSignature> root_signature;
	winrt::com_ptr<ID3D12Resource2> vertex_buffer;
	D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;

	winrt::com_ptr<ID3D12DescriptorHeap> cbv_heap;
	winrt::com_ptr<ID3D12Resource2> constant_buffer;

	winrt::com_ptr<ID3D12DescriptorHeap> dsv_heap;
	winrt::com_ptr<ID3D12Resource2> depth_buffer;

	winrt::com_ptr<ID3D12Resource2> texture_resource;

	winrt::com_ptr<ID3D12Fence1> fence;
	HANDLE fence_event;
	UINT64 fence_value;

	UINT rtv_descriptor_size;
	UINT frame_index;
	CD3DX12_VIEWPORT viewport;
	CD3DX12_RECT scissor_rect;

	UINT8* cbv_data_begin;
	vs_const_buffer_t const_buffer_data;

	UINT width, height;
	std::vector<vertex_t> triangle_data;
	FLOAT pos_x = 1.0f, pos_y = 1.0f, pos_z = 0.0f;
	FLOAT angle = 0.0f;

	void LoadPipeline();
	void LoadAssets();
	void PopulateCommandList();
	void WaitForPreviousFrame();

	void CreateDevice();
	void CreateCommandQueue();
	void CreateSwapChain(IDXGIFactory7* factory);
	void CreateDescriptorHeaps();
	void CreateFrameResources();
	void CreateCommandAllocator();
	void CreateRootSignature();
	void CreatePipelineState();
	void CreateCommandList();
	void CreateVertexBuffer();
	void CreateConstantBuffer();
	void CreateDepthBuffer();
	void CreateTextureAndSynchronizationResources();

};
