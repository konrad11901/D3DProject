#include "pch.h"
#include "D3DHandler.h"
#include "Win32Application.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
	RECT desktop;
	GetClientRect(GetDesktopWindow(), &desktop);

	D3DHandler sample(desktop.right - desktop.left, desktop.bottom - desktop.top);
	return Win32Application::Run(&sample, hInstance, nCmdShow);
}
