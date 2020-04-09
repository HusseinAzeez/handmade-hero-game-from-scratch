#include <windows.h>

LRESULT CALLBACK mainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	switch (message)
	{
	case WM_SIZE:
	{
		OutputDebugString("WM SIZE\n");
		break;
	}
	case WM_DESTROY:
	{
		OutputDebugString("WM_DESTROY\n");
		break;
	}
	case WM_CLOSE:
	{
		OutputDebugString("WM_CLOSE\n");
		break;
	}
	case WM_ACTIVATEAPP:
	{
		OutputDebugString("WM_ACTIVATEAPP\n");
		break;
	}
	// Draw something to the window screen
	case WM_PAINT:
	{
		PAINTSTRUCT paint;
		HDC deviceContext = BeginPaint(window, &paint);
		int x = paint.rcPaint.left;
		int y = paint.rcPaint.top;
		int width = paint.rcPaint.right - paint.rcPaint.left;
		int height = paint.rcPaint.bottom - paint.rcPaint.top;
		PatBlt(deviceContext, x, y, width, height, WHITENESS);
		EndPaint(window, &paint);
		break;
	}
	default:
	{
		result = DefWindowProc(window, message, wParam, lParam);
		break;
	}
	}
	return(result);
}

// Main Windows entry point
int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int cmdShow)
{
	WNDCLASS windowClass = {};
	// TODO: Check if HREDRAW/VREDRAW/OWNDC still matter.
	windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	windowClass.hInstance = instance;
	windowClass.lpfnWndProc = mainWindowCallback;
	// TODO: Add game icon. // Windowclass.hIcon;
	windowClass.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClass(&windowClass))
	{
		HWND windowHandle = CreateWindowEx(
			0,
			windowClass.lpszClassName,
			"Handmade Hero",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			instance,
			0);

		if (windowHandle)
		{
			// Struct
			MSG message;
			BOOL messageResult;
			// HWND window handle to get the messages that bound to that handler
			// ZERO to get all the messages.
			// // msgFilterMin, msgFilterMax = zeros to get all the messages
			while ((messageResult = GetMessage(&message, 0, 0, 0)) != 0)
			{
				if (messageResult == -1)
				{
					break;
				}
				else
				{
					TranslateMessage(&message);
					DispatchMessage(&message);
				}
			}
		}
		else {
			// TODO: Logging
		}
	}
	else {
		// TODO: Logging
	}
	return(0);
}

