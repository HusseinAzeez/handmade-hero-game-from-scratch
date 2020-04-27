#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>

#define internal static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;
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

struct win32WindowDimension
{
	int width;
	int height;
};

// TODO(Xizors): This is global for now.
global_variable bool running;
global_variable win32OffscreenBuffer backBuffer;
global_variable LPDIRECTSOUNDBUFFER secondaryBuffer;

win32WindowDimension win32GetWindowDimension(HWND window)
{
	win32WindowDimension result;
	RECT clientRect;
	GetClientRect(window, &clientRect);
	result.width = clientRect.right - clientRect.left;
	result.height = clientRect.bottom - clientRect.top;
	return(result);
}

internal void renderGradient(win32OffscreenBuffer* buffer, int blueOffset, int greenOffset)
{
	uint8* row = (uint8*)buffer->memory;

	for (int y = 0; y < buffer->height; y++)
	{
		uint32* pixel = (uint32*)row;
		for (int x = 0; x < buffer->width; x++)
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
		row += buffer->pitch;
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

internal void win32DisplayBufferInWindow(win32OffscreenBuffer* buffer,
	HDC deviceContext,
	int windowWidth, int windowHeight)
{
	/* StretchDIBits function copies the color data for a rectangle of pixels
	* in a DIB, JPEG, or PNG image to the specified destination rectangle.
	* If the destination rectangle is larger than the source rectangle,
	* this function stretches the rows and columns of color data
	* to fit the destination rectangle. If the destination rectangleis smaller
	* than the source rectangle,
	* this function compresses the rows and columns
	* by using the specified raster operation.
	*/
	// TODO(Xizors): Aspect ratio correction.
	StretchDIBits(
		deviceContext,
		0, 0, windowWidth, windowHeight,
		0, 0, buffer->width, buffer->height,
		buffer->memory,
		&buffer->info,
		DIB_RGB_COLORS,
		SRCCOPY);
}

internal void win32InitDSound(HWND window, int32 samplesPerSecond, int32 bufferSize)
{
	// NOTE(Xizors): Get a DirectSound object
	// TODO(Xizors): Double-check that this works on XP - DirectSound8 or 7
	LPDIRECTSOUND directSound;
	if (SUCCEEDED(DirectSoundCreate(0, &directSound, 0)))
	{
		WAVEFORMATEX waveFormat = {};
		waveFormat.wFormatTag = WAVE_FORMAT_PCM;
		waveFormat.nChannels = 2;
		waveFormat.nSamplesPerSec = samplesPerSecond;
		waveFormat.wBitsPerSample = 16;
		waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
		waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
		waveFormat.cbSize = 0;

		if (SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
		{
			// NOTE(Xizors): Create a primary buffer
			DSBUFFERDESC bufferDescription = {};
			bufferDescription.dwSize = sizeof(bufferDescription);
			bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

			LPDIRECTSOUNDBUFFER primaryBuffer;
			if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0)))
			{
				HRESULT error = primaryBuffer->SetFormat(&waveFormat);
				if (SUCCEEDED(error))
				{
					OutputDebugStringA("Primary buffer created successfully\n");
				}
				else
				{
					// TODO(Xizors): Diagnostic
				}
			}
		}
		else
		{
			// TODO(Xizors): Diagnostic
		}
		// NOTE(Xizors): Create a secondary buffer
		DSBUFFERDESC bufferDescription = {};
		bufferDescription.dwSize = sizeof(bufferDescription);
		bufferDescription.dwFlags = 0;
		bufferDescription.dwBufferBytes = bufferSize;
		bufferDescription.lpwfxFormat = &waveFormat;

		HRESULT error = directSound->CreateSoundBuffer(&bufferDescription, &secondaryBuffer, 0);
		if (SUCCEEDED(error))
		{
			OutputDebugStringA("Secondary buffer created successfully\n");
			// NOTE(Xizors): Start it playing!
		}
	}
	else
	{
		// TODO(Xizors): Diagnostic
	}
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
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	{
		uint32 VKCode = wParam;
		bool wasDown = ((lParam & (1 << 30)) != 0);
		bool isDown = ((lParam & (1 << 31)) == 0);

		if (isDown != wasDown)
		{
			if (VKCode == 'W')
			{
			}
			else if (VKCode == 'A')
			{
			}
			else if (VKCode == 'S')
			{
			}
			else if (VKCode == 'D')
			{
			}
			else if (VKCode == 'Q')
			{
			}
			else if (VKCode == 'E')
			{
			}
			else if (VKCode == VK_UP)
			{
			}
			else if (VKCode == VK_DOWN)
			{
			}
			else if (VKCode == VK_LEFT)
			{
			}
			else if (VKCode == VK_RIGHT)
			{
			}
			else if (VKCode == VK_ESCAPE)
			{
			}
			else if (VKCode == VK_SPACE)
			{
			}
		}
		bool32 altKeyWasDown = (lParam & (1 << 29));
		if ((VKCode == VK_F4) && altKeyWasDown)
		{
			running = false;
		}
	}
	case WM_PAINT:
	{
		PAINTSTRUCT paint;
		HDC deviceContext = BeginPaint(window, &paint);
		win32WindowDimension dimension = win32GetWindowDimension(window);
		win32DisplayBufferInWindow(&backBuffer, deviceContext,
			dimension.width, dimension.height);
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
	WNDCLASSA windowClass = {};
	win32ResizeDIBSection(&backBuffer, 1280, 720);
	windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
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
			// NOTE(Xizors): Since we specified CS_OWNDC, we can just
			// get one device context and use it forever because we
			// are not shearing it with anyone.
			HDC deviceContext = GetDC(window);
			// Display buffer variables
			int xOffset = 0;
			int yOffset = 0;
			// DirectSound variables
			int toneHz = 256; // ~Middle C HZ
			int toneVolume = 3000;
			int samplesPerSecond = 48000;
			uint32 runningSampleIndex = 0;
			int sequareWavePeriod = samplesPerSecond / toneHz;
			int halfSequareWavePeriod = sequareWavePeriod / 2;
			int bytesPerSample = sizeof(int16) * 2;
			int secondaryBufferSize = samplesPerSecond * bytesPerSample;

			// Init Direct Sound
			win32InitDSound(window, samplesPerSecond, secondaryBufferSize);
			secondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

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
				for (DWORD controllerIndex = 0;
					controllerIndex < XUSER_MAX_COUNT;
					controllerIndex++)
				{
					XINPUT_STATE controllerState;
					if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
					{
						// NOTE(Xizors): Controller is connected
						XINPUT_GAMEPAD* gamepad = &controllerState.Gamepad;
						bool up = (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool down = (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool left = (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool right = (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool start = (gamepad->wButtons & XINPUT_GAMEPAD_START);
						bool back = (gamepad->wButtons & XINPUT_GAMEPAD_BACK);
						bool leftSoulder = (gamepad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool rightSoulder = (gamepad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool aButton = (gamepad->wButtons & XINPUT_GAMEPAD_A);
						bool bButton = (gamepad->wButtons & XINPUT_GAMEPAD_B);
						bool xButton = (gamepad->wButtons & XINPUT_GAMEPAD_X);
						bool yButton = (gamepad->wButtons & XINPUT_GAMEPAD_Y);

						int16 stickX = gamepad->sThumbLX;
						int16 stickY = gamepad->sThumbLY;

						if (aButton)
						{
							yOffset += stickX << 12;
							xOffset += stickY << 12;
						}
					}
					else
					{
						// NOTE(Xizors): Controller is not connected
					}
				}
				renderGradient(&backBuffer, xOffset, yOffset);
				// TODO(Xizors): DirectSound output test
				// TODO(Xizors): More test here!.
				DWORD playCursor;
				DWORD writeCursor;
				if (SUCCEEDED(secondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
				{
					DWORD bytesToWrite;
					DWORD bytesToLock = runningSampleIndex * bytesPerSample % secondaryBufferSize;
					if (bytesToLock > playCursor)
					{
						bytesToWrite = (secondaryBufferSize - bytesToLock);
						bytesToWrite += playCursor;
					}
					else
					{
						bytesToWrite = playCursor - bytesToLock;
					}
					/*
					* [int16 int16] [int16 int16] [int16 int16] [int16 int16] ...
					* [LEFT  RIGHT] [LEFT  RIGHT] [LEFT  RIGHT] [LEFT  RIGHT] ...
					*/
					void* region1;
					DWORD region1Size;
					void* region2;
					DWORD region2Size;
					if (SUCCEEDED(secondaryBuffer->Lock(bytesToLock, bytesToWrite,
						&region1, &region1Size,
						&region2, &region2Size,
						0)))
					{
						// TODO(Xizors): Assert that region1Size/region2Size is valid
						int16* sampleOut = (int16*)region1;
						DWORD region1SampleCount = region1Size / bytesPerSample;
						DWORD region2SampleCount = region2Size / bytesPerSample;

						for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount; sampleIndex++)
						{
							int16 sampleValue = ((runningSampleIndex++ / halfSequareWavePeriod) % 2) ? toneVolume : -toneVolume;
							*sampleOut++ = sampleValue;
							*sampleOut++ = sampleValue;
						}

						for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount; sampleIndex++)
						{
							int16 sampleValue = ((runningSampleIndex++ / halfSequareWavePeriod) % 2) ? toneVolume : -toneVolume;
							*sampleOut++ = sampleValue;
							*sampleOut++ = sampleValue;
						}
						secondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
					}
				}
				win32WindowDimension dimension = win32GetWindowDimension(window);
				win32DisplayBufferInWindow(&backBuffer, deviceContext,
					dimension.width, dimension.height);
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

