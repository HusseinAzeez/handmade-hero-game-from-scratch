#include <windows.h>

#define internal static
#define global_variable static

// TODO(Xizors): This is global for now.
global_variable bool running;

global_variable BITMAPINFO bitmapInfo;
global_variable void *bitmapMemory;
global_variable HBITMAP bitmapHandle;
global_variable HDC bitmapDeviceContext;


// DIB: Device Indpendent Bitmap
internal void win32ResizeDIBSection(int width, int height)
{
    // Delete the handle if it exists.
    if (bitmapHandle)
    {
        // TODO(Xizors): Bulletproof this.
        // Maybe don't free first, free after, then free first if that fails.
        DeleteObject(bitmapHandle);
    }

    // Create a new device context if none exists.
    if (!bitmapDeviceContext)
    {
        // TODO(Xizors): Should we recreate these under certain special cicumstances.
        bitmapDeviceContext = CreateCompatibleDC(0);
    }

    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    bitmapHandle = CreateDIBSection(
            bitmapDeviceContext,
            &bitmapInfo,
            DIB_RGB_COLORS,
            &bitmapMemory,
            0,
            0);
}

internal void win32UpdateWindow(HDC deviceContext, int x, int y, int width, int height)
{
    /* StretchDIBits function copies the color data for a rectangle of pixels
     * in a DIB, JPEG, or PNG image to the specified destination rectangle.
     * If the destination rectangle is larger than the source rectangle,
     * this function stretches the rows and columns of color data
     * to fit the destination rectangle. If the destination rectangleis smaller
     * than the source rectangle,
     * this function compresses the rows and columns by using the specified raster operation.
     */
    StretchDIBits(
            deviceContext,
            x, y, width, height,
            x, y, width, height,
            bitmapMemory,
            &bitmapInfo,
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
                win32ResizeDIBSection(width, height);
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
                win32UpdateWindow(deviceContext, x, y, width, height);
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
    windowClass.lpfnWndProc = win32MainWindowCallback;
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
            // TODO(Xizors): Logging
        }
    }
    else {
        // TODO(Xizors): Logging
    }
    return(0);
}

