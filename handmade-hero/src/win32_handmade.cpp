#include <Windows.h>
#include <stdint.h>

static bool running;
static BITMAPINFO bitmapInfo;
static void* bitmapMemory;
static int bitmapWidth;
static int bitmapHeight;
static int bytesPerPixel = 4;

static void RenderWeirdGradient(int xOffset, int yOffset)
{
    int width = bitmapWidth;
    int height = bitmapHeight;
    int pitch = width * bytesPerPixel;
    uint8_t *row = (uint8_t *) bitmapMemory;
    for (int y = 0; y < bitmapHeight; ++y)
    {
        uint32_t *pixel = (uint32_t *) row;
        for (int x = 0; x < bitmapWidth; ++x)
        {
            uint8_t blue= (x + xOffset);
            uint8_t green= (y + yOffset);
            *pixel++ = (green << 8) | blue;
        }

        row += pitch;
    }
}

static void Win32ResizeDIBSection(int width, int height)
{
    if (bitmapMemory)
    {
        VirtualFree(bitmapMemory, 0, MEM_RELEASE);
    }

    bitmapWidth = width;
    bitmapHeight = height;

    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    int bitmapMemorySize = bytesPerPixel * width * height;
    bitmapMemory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

static void Win32UpdateWindow(HDC deviceContext, RECT* clientRect, int x, int y, int width, int height)
{
    int windowWidth = clientRect->right - clientRect->left;
    int windowHeight = clientRect->bottom - clientRect->top;
    StretchDIBits(deviceContext,
                  /*0, 0, width, height,
                  x, y, width, height,*/
                  0, 0, bitmapWidth, bitmapHeight,
                  0, 0, windowWidth, windowHeight,
                  bitmapMemory, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}


LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch (message)
    {
        case WM_SIZE:
        {
            RECT clientRect;
            GetClientRect(window, &clientRect);
            int width = clientRect.right - clientRect.left;
            int height = clientRect.bottom - clientRect.top;
            Win32ResizeDIBSection(width, height);
        } break;
        case WM_DESTROY:
        {
            running = false;
        } break;
        case WM_CLOSE:
        {
            running = false;
        } break;
        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint(window, &paint);
            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;
            RECT clientRect;
            GetClientRect(window, &clientRect);
            Win32UpdateWindow(deviceContext, &clientRect, x, y, width, height);
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
    windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
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

    int xOffset = 0;
    int yOffset = 0;
    running = true;
    while (running)
    {
        MSG message;
        while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT)
            {
                running = false;
            }
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
        RenderWeirdGradient(xOffset, yOffset);
        HDC deviceContext = GetDC(window);
        RECT clientRect;
        GetClientRect(window, &clientRect);
        int windowWidth = clientRect.right - clientRect.left;
        int windowHeight = clientRect.bottom - clientRect.top;
        Win32UpdateWindow(deviceContext, &clientRect, 0, 0, windowWidth, windowHeight);
        ReleaseDC(window, deviceContext);
        xOffset++;
    }

    return 0;
}
