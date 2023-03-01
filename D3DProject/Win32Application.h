#pragma once

class D3DHandler;

class Win32Application
{
public:
	static int Run(D3DHandler* d3d_handler, HINSTANCE instance, int cmd_show);
	static HWND GetHwnd() { return hwnd; }

protected:
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	static HWND hwnd;
};
