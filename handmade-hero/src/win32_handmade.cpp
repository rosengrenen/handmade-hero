#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>

#define internal static
#define local_persist static
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
};

struct win32WindowDimension
{
    int width;
    int height;
};

// XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

global_variable bool globalRunning;
global_variable win32OffscreenBuffer globalBackbuffer;

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN  pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void Win32LoadXInput()
{
    HMODULE xInputLibrary = LoadLibraryA("xinput1_4.dll");
    if (!xInputLibrary)
    {
        xInputLibrary = LoadLibraryA("xinput1_3.dll");
    }
    if (xInputLibrary)
    {
        XInputGetState = (x_input_get_state *) GetProcAddress(xInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state *) GetProcAddress(xInputLibrary, "XInputSetState");;
    }
}

internal win32WindowDimension Win32GetWindowDimension(HWND window)
{
    win32WindowDimension dimension;

    RECT clientRect;
    GetClientRect(window, &clientRect);
    dimension.width = clientRect.right - clientRect.left;
    dimension.height = clientRect.bottom - clientRect.top;

    return dimension;
}

internal void Win32InitDSound(HWND window, int32 samplesPerSecond, int32 bufferSize)
{
    HMODULE dSoundLibrary = LoadLibraryA("dsound.dll");

    if (dSoundLibrary)
    {
        direct_sound_create *DirectSoundCreate = (direct_sound_create *) GetProcAddress(dSoundLibrary, "DirectSoundCreate");
        LPDIRECTSOUND directSound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &directSound, 0)))
        {
            WAVEFORMATEX waveFormat = {};
            waveFormat.wFormatTag = WAVE_FORMAT_PCM;
            waveFormat.nChannels = 2;
            waveFormat.nSamplesPerSec = samplesPerSecond;
            waveFormat.wBitsPerSample = 16;
            waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
            waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
            waveFormat.cbSize = 0;

            if (SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
            {
                DSBUFFERDESC bufferDescription = {};
                bufferDescription.dwSize = sizeof(DSBUFFERDESC);
                bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                // Create primary buffer
                LPDIRECTSOUNDBUFFER primaryBuffer;
                if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0)))
                {
                    HRESULT Error = SUCCEEDED(primaryBuffer->SetFormat(&waveFormat));
                    if (Error)
                    {
                        OutputDebugStringA("Primary buffer format was set.\n");
                    }
                    else
                    {
                        // TODO: Diagnostic
                    }
                }
                else
                {
                    // TODO: Diagnostic
                }
            }
            else
            {
                // TODO: Diagnostic
            }
            DSBUFFERDESC bufferDescription = {};
            bufferDescription.dwSize = sizeof(DSBUFFERDESC);
            bufferDescription.dwBufferBytes = bufferSize;
            bufferDescription.lpwfxFormat = &waveFormat;
            LPDIRECTSOUNDBUFFER secondaryBuffer;
            HRESULT Error = SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &secondaryBuffer, 0));
            if (Error)
            {
                OutputDebugStringA("Secondary buffer created successfully.\n");
            }
            else
            {
                // TODO: Diagnostic
            }
        }
        else
        {
            // TODO: Diagnostic
        }
    }
}

internal void RenderWeirdGradient(win32OffscreenBuffer* buffer, int blueOffset, int greenOffset)
{
    uint8 *row = (uint8 *) buffer->memory;
    for (int y = 0; y < buffer->height; ++y)
    {
        uint32 *pixel = (uint32 *) row;
        for (int x = 0; x < buffer->width; ++x)
        {
            uint8 blue = (x + blueOffset);
            uint8 green = (y + greenOffset);
            *pixel++ = (green << 8) | blue;
        }

        row += buffer->pitch;
    }
}

internal void Win32ResizeDIBSection(win32OffscreenBuffer *buffer, int width, int height)
{
    if (buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    int bytesPerPixel = 4;

    buffer->width = width;
    buffer->height = height;

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = width;
    buffer->info.bmiHeader.biHeight = -height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    int bitmapMemorySize = bytesPerPixel * width * height;
    buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);


    buffer->pitch = buffer->width * bytesPerPixel;
}

internal void Win32DisplayBufferInWindow(win32OffscreenBuffer* buffer, HDC deviceContext, int windowWidth, int windowHeight)
{
    StretchDIBits(deviceContext,
                  0, 0, windowWidth, windowHeight,
                  0, 0, buffer->width, buffer->height,
                  buffer->memory, &buffer->info, DIB_RGB_COLORS, SRCCOPY);
}


internal LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch (message)
    {
        case WM_SIZE:
        {
        } break;
        case WM_DESTROY:
        {
            globalRunning = false;
        } break;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32 vkCode = (uint32) wParam;
            bool wasDown = ((lParam & (1 << 30)) != 0);
            bool isDown = ((lParam & (1 << 31)) == 0);
            if (wasDown != isDown)
            {
                if (vkCode == 'W')
                {
                }
                else if (vkCode == 'A')
                {
                }
                else if (vkCode == 'S')
                {
                }
                else if (vkCode == 'D')
                {
                }
                else if (vkCode == 'Q')
                {
                }
                else if (vkCode == 'E')
                {
                }
                else if (vkCode == VK_UP)
                {
                }
                else if (vkCode == VK_RIGHT)
                {
                }
                else if (vkCode == VK_DOWN)
                {
                }
                else if (vkCode == VK_LEFT)
                {
                }
                else if (vkCode == VK_ESCAPE)
                {
                }
                else if (vkCode == VK_SPACE)
                {
                }
            }

            bool32 altKeyWasDown = (lParam & (1 << 29));
            if (vkCode == VK_F4 && altKeyWasDown)
            {
                globalRunning = false;
            }
        } break;
        case WM_CLOSE:
        {
            globalRunning = false;
        } break;
        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint(window, &paint);
            win32WindowDimension dimension = Win32GetWindowDimension(window);
            Win32DisplayBufferInWindow(&globalBackbuffer, deviceContext, dimension.width, dimension.height);
            EndPaint(window, &paint);
        } break;
        default:
        {
            //OutputDebugStringA("Default");
            result = DefWindowProcA(window, message, wParam, lParam);
        } break;
    }
    return result;
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode)
{
    Win32LoadXInput();

    Win32ResizeDIBSection(&globalBackbuffer, 1280, 720);

    WNDCLASSA windowClass = {};
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    // windowClass.hIcon;
    windowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (!RegisterClassA(&windowClass))
    {
        return 1;
        // TODO: Logging
    }

    HWND window = CreateWindowEx(0, windowClass.lpszClassName, "Handmade Hero", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);
    if (!window)
    {
        return 1;
        // TODO: Logging
    }

    Win32InitDSound(window, 48000, 48000 * sizeof(int16) * 2);

    int xOffset = 0;
    int yOffset = 0;

    globalRunning = true;
    while (globalRunning)
    {
        MSG message;
        while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT)
            {
                globalRunning = false;
            }
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        for (DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT; ++controllerIndex)
        {
            XINPUT_STATE controllerState;
            if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
            {
                XINPUT_GAMEPAD* pad = &controllerState.Gamepad;

                bool dPadUp = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                bool dPadDown = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                bool dPadLeft = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                bool dPadRight = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                bool start = (pad->wButtons & XINPUT_GAMEPAD_START);
                bool back = (pad->wButtons & XINPUT_GAMEPAD_BACK);
                bool leftShoulder = (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                bool rightShoulder = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                bool aButton = (pad->wButtons & XINPUT_GAMEPAD_A);
                bool bButton = (pad->wButtons & XINPUT_GAMEPAD_B);
                bool xButton = (pad->wButtons & XINPUT_GAMEPAD_X);
                bool yButton = (pad->wButtons & XINPUT_GAMEPAD_Y);

                int16 stickX = pad->sThumbLX;
                int16 stickY = pad->sThumbLY;

                xOffset += stickX >> 12;
                yOffset -= stickY >> 12;
            }
            else
            {
                // Controller not available
            }
        }

        RenderWeirdGradient(&globalBackbuffer, xOffset, yOffset);
        HDC deviceContext = GetDC(window);
        win32WindowDimension dimension = Win32GetWindowDimension(window);
        Win32DisplayBufferInWindow(&globalBackbuffer, deviceContext, dimension.width, dimension.height);

        ReleaseDC(window, deviceContext);
    }

    return 0;
}
