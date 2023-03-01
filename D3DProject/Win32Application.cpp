#include "pch.h"
#include "Win32Application.h"
#include "D3DHandler.h"

HWND Win32Application::hwnd = nullptr;

int Win32Application::Run(D3DHandler* d3d_handler, HINSTANCE instance, int cmd_show) {
	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = instance;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = L"DXProjectClass";
	RegisterClassEx(&windowClass);

	RECT desktop;
	GetClientRect(GetDesktopWindow(), &desktop);

	hwnd = CreateWindowEx(
		0,                      // Optional window styles
		L"DXProjectClass",      // Window class
		L"DXProject",           // Window text
		WS_POPUP,               // Window style

		// Size and position
		0, 0,
		desktop.right - desktop.left,
		desktop.bottom - desktop.top,

		nullptr,        // Parent window    
		nullptr,        // Menu
		instance,       // Instance handle
		d3d_handler // Additional application data
	);
	winrt::check_pointer(hwnd);

	d3d_handler->OnInit();

	ShowWindow(hwnd, cmd_show);

	MSG msg = {};
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	d3d_handler->OnDestroy();

	return static_cast<char>(msg.wParam);
}

LRESULT Win32Application::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	D3DHandler* d3d_handler = reinterpret_cast<D3DHandler*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

	switch (message) {
	case WM_CREATE: {
		LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
	}
	return 0;

	case WM_PAINT:
		if (d3d_handler) {
			d3d_handler->OnUpdate();
			d3d_handler->OnRender();
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) {
			DestroyWindow(hwnd);
		}
		return 0;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}
