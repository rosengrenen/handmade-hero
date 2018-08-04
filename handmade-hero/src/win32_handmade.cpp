#include <Windows.h>
#include <stdint.h>

struct win32OffscreenBuffer
{
    BITMAPINFO info;
    void* memory;
    int width;
    int height;
    int pitch;
};

static bool globalRunning;
static win32OffscreenBuffer globalBackbuffer;


struct win32WindowDimension
{
    int width;
    int height;
};

win32WindowDimension Win32GetWindowDimension(HWND window)
{
    win32WindowDimension dimension;

    RECT clientRect;
    GetClientRect(window, &clientRect);
    dimension.width = clientRect.right - clientRect.left;
    dimension.height = clientRect.bottom - clientRect.top;

    return dimension;
}

static void RenderWeirdGradient(win32OffscreenBuffer buffer, int blueOffset, int greenOffset)
{
    uint8_t *row = (uint8_t *) buffer.memory;
    for (int y = 0; y < buffer.height; ++y)
    {
        uint32_t *pixel = (uint32_t *) row;
        for (int x = 0; x < buffer.width; ++x)
        {
            uint8_t blue = (x + blueOffset);
            uint8_t green = (y + greenOffset);
            *pixel++ = (green << 8) | blue;
        }

        row += buffer.pitch;
    }
}

static void Win32ResizeDIBSection(win32OffscreenBuffer *buffer, int width, int height)
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

static void Win32DisplayBufferInWindow(HDC deviceContext, int windowWidth, int windowHeight, win32OffscreenBuffer buffer)
{
    StretchDIBits(deviceContext,
                  0, 0, windowWidth, windowHeight,
                  0, 0, buffer.width, buffer.height,
                  buffer.memory, &buffer.info, DIB_RGB_COLORS, SRCCOPY);
}


LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
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
            Win32DisplayBufferInWindow(deviceContext, dimension.width, dimension.height, globalBackbuffer);
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
    WNDCLASS windowClass = {};
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

    win32WindowDimension dimension = Win32GetWindowDimension(window);
    Win32ResizeDIBSection(&globalBackbuffer, dimension.width, dimension.height);

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
        RenderWeirdGradient(globalBackbuffer, xOffset, yOffset);
        HDC deviceContext = GetDC(window);
        win32WindowDimension dimension = Win32GetWindowDimension(window);
        Win32DisplayBufferInWindow(deviceContext, dimension.width, dimension.height, globalBackbuffer);

        ReleaseDC(window, deviceContext);
        xOffset++;
    }

    return 0;
}
