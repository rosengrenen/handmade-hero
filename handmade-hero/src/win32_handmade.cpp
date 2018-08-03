#include <Windows.h>

static bool running;
static BITMAPINFO bitmapInfo;
static void* bitmapMemory;
static HBITMAP bitmapHandle;
static HDC bitmapDeviceContext;

static void Win32ResizeDIBSection(int width, int height)
{
    if (bitmapHandle)
    {
        DeleteObject(bitmapHandle);
    }

    if (!bitmapDeviceContext)
    {
        bitmapDeviceContext = CreateCompatibleDC(0);
    }

    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    bitmapHandle = CreateDIBSection(bitmapDeviceContext, &bitmapInfo, DIB_RGB_COLORS, &bitmapMemory, NULL, 0);
}

static void Win32UpdateWindow(HDC deviceContext, int x, int y, int width, int height)
{
    StretchDIBits(deviceContext, x, y, width, height, x, y, width, height,
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
            Win32UpdateWindow(deviceContext, x, y, width, height);
            static DWORD operation = WHITENESS;
            if (operation == WHITENESS)
            {
                operation = BLACKNESS;
            }
            else
            {
                operation = WHITENESS;
            }
            PatBlt(deviceContext, x, y, width, height, operation);
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

    HWND windowHandle = CreateWindowEx(0, windowClass.lpszClassName, "Handmade Hero", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);
    if (!windowHandle)
    {
        return 1;
        // TODO: Logging
    }

    running = true;
    while (running)
    {
        MSG message;
        BOOL messageResult = GetMessage(&message, 0, 0, 0);
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

    return 0;
}
