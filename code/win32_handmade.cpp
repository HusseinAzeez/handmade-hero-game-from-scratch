#include <windows.h>
#include <stdint.h>

#define internal static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

struct win32OffscreenBuffer
{
	BITMAPINFO info;
	void* memory;
	int width;
	int height;
	int pitch;
	int bytesPerPixel;
};

// TODO(Xizors): This is global for now.
global_variable bool running;
global_variable win32OffscreenBuffer backBuffer;

internal void renderGradient(win32OffscreenBuffer buffer, int blueOffset, int greenOffset)
{
	uint8* row = (uint8*)buffer.memory;

	for (int y = 0; y < buffer.height; y++)
	{
		uint32* pixel = (uint32*)row;
		for (int x = 0; x < buffer.width; x++)
		{
			/*
			 * Pixel (32-bits)
			 * LITILE ENDIAN ARCHITECTURE
			 * Memory:      BB GG RR xx
			 * Register:    xx RR GG BB
			 * 0x xxRRGGBB
			 */
			uint8 blue = (x + blueOffset);
			uint8 green = (y + greenOffset);
			*pixel++ = ((green << 8) | blue);
		}
		row += buffer.pitch;
	}
}

// DIB: Device Indpendent Bitmap
internal void win32ResizeDIBSection(win32OffscreenBuffer* buffer, int width, int height)
{
	if (buffer->memory)
	{
		VirtualFree(buffer->memory, 0, MEM_RELEASE);
	}

	buffer->width = width;
	buffer->height = height;
	buffer->bytesPerPixel = 4;

	buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
	buffer->info.bmiHeader.biWidth = buffer->width;
	buffer->info.bmiHeader.biHeight = -buffer->height;
	buffer->info.bmiHeader.biPlanes = 1;
	buffer->info.bmiHeader.biBitCount = 32;
	buffer->info.bmiHeader.biCompression = BI_RGB;

	int bitmapMemorySize = (buffer->width * buffer->height) * buffer->bytesPerPixel;
	buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
	buffer->pitch = width * buffer->bytesPerPixel;
}

internal void win32DisplayBufferInWindow(HDC deviceContext,
	win32OffscreenBuffer buffer,
	RECT clientRect, int x, int y, int width, int height)
{
	/* StretchDIBits function copies the color data for a rectangle of pixels
	 * in a DIB, JPEG, or PNG image to the specified destination rectangle.
	 * If the destination rectangle is larger than the source rectangle,
	 * this function stretches the rows and columns of color data
	 * to fit the destination rectangle. If the destination rectangleis smaller
	 * than the source rectangle,
	 * this function compresses the rows and columns by using the specified raster operation.
	 */
	int windowWidth = clientRect.right - clientRect.left;
	int windowHeight = clientRect.bottom - clientRect.top;
	StretchDIBits(
		deviceContext,
		/*
		   x, y, width, height,
		   x, y, width, height,
		   */
		0, 0, buffer.width, buffer.height,
		0, 0, windowWidth, windowHeight,
		buffer.memory,
		&buffer.info,
		DIB_RGB_COLORS,
		SRCCOPY);
}

LRESULT CALLBACK win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
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
		RECT clientRect;
		GetClientRect(window, &clientRect);
		int width = clientRect.right - clientRect.left;
		int height = clientRect.bottom - clientRect.top;
		win32ResizeDIBSection(&backBuffer, width, height);
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

		RECT clientRect;
		GetClientRect(window, &clientRect);
		win32DisplayBufferInWindow(deviceContext, backBuffer,
			clientRect, x, y, width, height);
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
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.hInstance = instance;
	windowClass.lpfnWndProc = win32MainWindowCallback;
	// TODO(Xizors): Add game icon.
	// Windowclass.hIcon;
	windowClass.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClass(&windowClass))
	{
		HWND window = CreateWindowExA(
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

		if (window)
		{
			int xOffset = 0;
			int yOffset = 0;
			running = true;
			while (running)
			{
				MSG message;
				while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
				{
					if (message.message == WM_QUIT)
					{
						running = false;
					}
					TranslateMessage(&message);
					DispatchMessageA(&message);
				}
				renderGradient(backBuffer, xOffset, yOffset);

				HDC deviceContext = GetDC(window);
				RECT clientRect;
				GetClientRect(window, &clientRect);
				int windowWidth = clientRect.right - clientRect.left;
				int windowHeight = clientRect.bottom - clientRect.top;
				win32DisplayBufferInWindow(deviceContext, backBuffer,
					clientRect, 0, 0, windowWidth, windowHeight);
				ReleaseDC(window, deviceContext);

				++xOffset;
				++yOffset;
			}
		}
		else {
			// TODO(Xizors): Logging
		}
	}
	else {
		// TODO(Xizors): Logging
	}
	return(0);
}

