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

// TODO(Xizors): This is global for now.
global_variable bool running;
global_variable BITMAPINFO bitmapInfo;
global_variable void *bitmapMemory;
global_variable int bitmapWidth;
global_variable int bitmapHeight;
global_variable int bytesPerPixel = 4;

internal void renderGradient(int xOffset, int yOffset)
{
    int width = bitmapWidth;
    int height = bitmapHeight;
    int pitch = width * bytesPerPixel;
    uint8 *row = (uint8 *) bitmapMemory;

    for (int y = 0; y < bitmapHeight ; y++)
    {
        uint32 *pixel = (uint32 *) row;
        for (int x = 0; x < bitmapWidth; x++)
        {
            /*
             * Pixel (32-bits)
             * LITILE ENDIAN ARCHITECTURE
             * Memory:      BB GG RR xx
             * Register:    xx RR GG BB
             * 0x xxRRGGBB
             */
            uint8 blue = (x + xOffset);
            uint8 green = (y + yOffset);
            *pixel++ = ((green << 8) | blue);
        }
        row += pitch;
    }
}

// DIB: Device Indpendent Bitmap
internal void win32ResizeDIBSection(int width, int height)
{
    if (bitmapMemory)
    {
        VirtualFree(bitmapMemory, 0, MEM_RELEASE);
    }

    bitmapWidth = width;
    bitmapHeight = height;
    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = bitmapWidth;
    bitmapInfo.bmiHeader.biHeight = -bitmapHeight;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    int bitmapMemorySize = (bitmapWidth * bitmapHeight) * bytesPerPixel;
    bitmapMemory = VirtualAlloc(0, bitmapMemorySize,  MEM_COMMIT, PAGE_READWRITE);
}

internal void win32UpdateWindow(HDC deviceContext, RECT *clientRect, int x, int y, int width, int height)
{
    /* StretchDIBits function copies the color data for a rectangle of pixels
     * in a DIB, JPEG, or PNG image to the specified destination rectangle.
     * If the destination rectangle is larger than the source rectangle,
     * this function stretches the rows and columns of color data
     * to fit the destination rectangle. If the destination rectangleis smaller
     * than the source rectangle,
     * this function compresses the rows and columns by using the specified raster operation.
     */
    int windowWidth = clientRect -> right - clientRect -> left;
    int windowHeight = clientRect -> bottom - clientRect -> top;
    StretchDIBits(
            deviceContext,
            /*
               x, y, width, height,
               x, y, width, height,
               */
            0, 0, bitmapWidth, bitmapHeight,
            0, 0, windowWidth, windowHeight,
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

                RECT clientRect;
                GetClientRect(window, &clientRect);
                win32UpdateWindow(deviceContext, &clientRect, x, y, width, height);
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
                while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
                {
                    if (message.message == WM_QUIT)
                    {
                        running = false;
                    }
                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                }
                renderGradient(xOffset, yOffset);

                HDC deviceContext = GetDC(window);
                RECT clientRect;
                GetClientRect(window, &clientRect);
                int windowWidth = clientRect.right - clientRect.left;
                int windowHeight = clientRect.bottom - clientRect.top;
                win32UpdateWindow(deviceContext, &clientRect, 0, 0, windowWidth, windowHeight);
                ReleaseDC(window, deviceContext);

                ++xOffset;
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

