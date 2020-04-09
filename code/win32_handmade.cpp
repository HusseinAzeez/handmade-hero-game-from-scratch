#include <windows.h>

#define global_variable static

// TODO(Xizors): This is global for now.
global_variable bool running;

LRESULT CALLBACK mainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	switch (message)
	{
	case WM_ACTIVATEAPP:
	{
		OutputDebugString("WM_ACTIVATEAPP\n");
		break;
	}
	case WM_SIZE:
	{
		OutputDebugString("WM SIZE\n");
		break;
	}
	case WM_CLOSE:
	{
		// TODO(Xizors): Handle this with a message to the user?
		running = false;
		break;
	}
	case WM_DESTROY:
	{
		// TODO(Xizors): Handle this as an error, recreate the window?
		running = false;
		break;
	}
	case WM_PAINT:
	{
		// Draw something to the window screen
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
		result = DefWindowProcA(window, message, wParam, lParam);
		break;
	}
	}
	return(result);
}

// Main Windows entry point
int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int cmdShow)
{
	WNDCLASS windowClass = {};
	windowClass.hInstance = instance;
	windowClass.lpfnWndProc = mainWindowCallback;
	// TODO(Xizors): Add game icon. 
	// Windowclass.hIcon;
	windowClass.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClass(&windowClass))
	{
		HWND windowHandle = CreateWindowExA(
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
			running = true;
			while (running)
			{
				// Message struct
				MSG message;
				// HWND window handle to get the messages that bound to that handler
				// ZERO to get all the messages.
				// msgFilterMin, msgFilterMax = zeros to get all the messages
				BOOL messageResult = GetMessageA(&message, 0, 0, 0);
				if (messageResult > 0)
				{
					TranslateMessage(&message);
					DispatchMessageA(&message);
				}
				else
				{
					break;
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

