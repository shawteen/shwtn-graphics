#include <math.h>
#include <windows.h>

#include "win32_graphics.h"
#include "graphics_math.cpp"


global global_state GlobalState;

v2 ProjectPoint(v3 WorldPos)
{
    v2 Result = {};
    //NOTE: to NDC
    Result = WorldPos.xy/WorldPos.z;

    //NOTE: to pixels
    Result = 0.5f * (Result + V2(1.0f, 1.0f));
    Result = V2(GlobalState.FrameBufferWidth, GlobalState.FrameBufferHeight) * Result;
    return Result;
}

f32 CrossProduct2D(v2 A, v2 B)
{
    f32 Result = A.x * B.y - A.y * B.x;
    return Result;
}

LRESULT Win32WindowCallback (
  HWND WindowHandle,
  UINT Message,
  WPARAM WParam,
  LPARAM LParam
)
{
    LRESULT Result = {};

    switch (Message)
    {
        case WM_DESTROY:
        case WM_CLOSE:
        {
          GlobalState.IsRunning = false;
        } break;

        default:
        {
            Result = DefWindowProcA(WindowHandle, Message, WParam, LParam);
        } break;
    }

    return Result;
}

void DrawTriangle(v3* Points, v3* Colors)
{
    v2 PointA = ProjectPoint(Points[0]);
    v2 PointB = ProjectPoint(Points[1]);
    v2 PointC = ProjectPoint(Points[2]);

    i32 MinX = min(min((i32)PointA.x, (i32)PointB.x),(i32)PointC.x);
    i32 MaxX = max(max((i32)round(PointA.x), (i32)round(PointB.x)),(i32)round(PointC.x));
    i32 MinY = min(min((i32)PointA.y, (i32)PointB.y),(i32)PointC.y);
    i32 MaxY = max(max((i32)round(PointA.y), (i32)round(PointB.y)),(i32)round(PointC.y));

    MinX = max(0, MinX);
    MinX = min(GlobalState.FrameBufferWidth-1, MinX);
    MaxX = max(0, MaxX);
    MaxX = min(GlobalState.FrameBufferWidth-1, MaxX);
    MinY = max(0, MinY);
    MinY = min(GlobalState.FrameBufferHeight-1, MinY);
    MaxY = max(0, MaxY);
    MaxY = min(GlobalState.FrameBufferHeight-1, MaxY);

    v2 Edge0 = PointB - PointA;
    v2 Edge1 = PointC - PointB;
    v2 Edge2 = PointA - PointC;

    b32 IsTopLeft0 = (Edge0.x >= 0.0f && Edge0.y > 0.0f ) || (Edge0.x > 0.0f && Edge0.y == 0.0f);
    b32 IsTopLeft1 = (Edge1.x >= 0.0f && Edge1.y > 0.0f ) || (Edge1.x > 0.0f && Edge1.y == 0.0f);
    b32 IsTopLeft2 = (Edge2.x >= 0.0f && Edge2.y > 0.0f ) || (Edge2.x > 0.0f && Edge2.y == 0.0f);
    
    f32 BaryCentricDiv = CrossProduct2D(PointB - PointA, PointC - PointA);

    for (i32 Y = MinY; Y <= MaxY; ++Y)
    {
        for (i32 X = MinX; X <= MaxX; ++X)
        {
            v2 PixelPoint = V2(X, Y) + V2(0.5f, 0.5f);

            v2 PixelEdge0 = PixelPoint - PointA;
            v2 PixelEdge1 = PixelPoint - PointB;
            v2 PixelEdge2 = PixelPoint - PointC;

            f32 CrossLength0 = CrossProduct2D(PixelEdge0, Edge0);
            f32 CrossLength1 = CrossProduct2D(PixelEdge1, Edge1);
            f32 CrossLength2 = CrossProduct2D(PixelEdge2, Edge2);

            if ((CrossLength0 > 0.0f || (IsTopLeft0 && CrossLength0 == 0.0f)) &&
                (CrossLength1 > 0.0f || (IsTopLeft1 && CrossLength1 == 0.0f)) &&
                (CrossLength2 > 0.0f || (IsTopLeft2 && CrossLength2 == 0.0f)))
            {
                //NOTE: We are inside the triangle
                u32 PixelId = Y * GlobalState.FrameBufferWidth + X;

                f32 T0 = -CrossLength1 / BaryCentricDiv;
                f32 T1 = -CrossLength2 / BaryCentricDiv;
                f32 T2 = -CrossLength0 / BaryCentricDiv;
                v3 FinalColor = T0 * Colors[0] + T1 * Colors[1] + T2 * Colors[2];
                FinalColor = FinalColor * 255.0f;
                u32 FinalColorU32 = ((u32)0xFF << 24) | ((u32)FinalColor.r  << 16) | ((u32)FinalColor.g << 8) | (u32)FinalColor.b;

                GlobalState.FrameBufferPixels[PixelId] = FinalColorU32;
            }
        }
    }
}

int APIENTRY WinMain(HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR     lpCmdLine,
                    int       nShowCmd)
{
    GlobalState.IsRunning = true;
    LARGE_INTEGER TimerFrequency = {};
    Assert(QueryPerformanceFrequency(&TimerFrequency));

    {
        WNDCLASSA WindowClass = {};
        WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        WindowClass.lpfnWndProc = Win32WindowCallback;
        WindowClass.hInstance = hInstance;
        WindowClass.hCursor = LoadCursorA(NULL, IDC_ARROW);
        WindowClass.lpszClassName = "bufferA";
        if (!RegisterClassA(&WindowClass))
        {
            InvalidCodePath;
        }

            GlobalState.WindowHandle = CreateWindowExA(
            0,
            WindowClass.lpszClassName,
            "bufferA",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            1280,
            720,
            NULL,
            NULL,
            hInstance,
            NULL);

        if(!GlobalState.WindowHandle)
        {
            InvalidCodePath;
        }

        GlobalState.DeviceContext = GetDC(GlobalState.WindowHandle);
    }

    {
        RECT ClientRect = {};
        Assert(GetClientRect(GlobalState.WindowHandle, &ClientRect));
        //GlobalState.FrameBufferWidth = ClientRect.right - ClientRect.left;
        //GlobalState.FrameBufferHeight = ClientRect.bottom - ClientRect.top;
        GlobalState.FrameBufferWidth = 300;
        GlobalState.FrameBufferHeight = 300;
        GlobalState.FrameBufferPixels = (u32*)malloc(sizeof(u32) * GlobalState.FrameBufferWidth *
                                                            GlobalState.FrameBufferHeight);

    }
    LARGE_INTEGER BeginTime = {};
    LARGE_INTEGER EndTime = {};
    Assert(QueryPerformanceCounter(&BeginTime));

    while (GlobalState.IsRunning)
    {
        Assert(QueryPerformanceCounter(&EndTime));
        f32 FrameTime = f32(EndTime.QuadPart - BeginTime.QuadPart) / f32(TimerFrequency.QuadPart);
        BeginTime = EndTime;

        MSG Message = {};
        while(PeekMessageA(&Message, GlobalState.WindowHandle, 0, 0, PM_REMOVE))
        {
            switch(Message.message)
            {
                case WM_QUIT:
                {
                    GlobalState.IsRunning = false;
                }break;
                default:
                {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }break;
            }
        }
        // NOTE: clearing buffer by setting all pixels to 0
        for (u32 Y = 0; Y < GlobalState.FrameBufferHeight; Y++){
            for (u32 X = 0; X < GlobalState.FrameBufferWidth; X++)
            {
            u32 PixelId = Y * GlobalState.FrameBufferWidth + X;

            u8 Red = (u8)0;
            u8 Green = (u8)0;
            u8 Blue = 0;
            u8 Alpha = 255;
            u32 PixelColor = ((u32)Alpha << 25) | ((u32)Red << 16) | ((u32)Green << 8) | (u32)Blue;

            GlobalState.FrameBufferPixels[PixelId] = PixelColor;
            }
        }
        //NOTE: Projecting our triangles
        GlobalState.CurrTime = GlobalState.CurrTime + FrameTime;
        if (GlobalState.CurrTime > 2.0f * 3.14169f)
        {
           GlobalState.CurrTime -= 2.0f * 3.14169f;
        }

        v3 Colors[] =
        {
            V3(1,0,0),
            V3(0,1,0),
            V3(0,0,1),
        };

        for (i32 TriangleId = 9; TriangleId >= 0; -- TriangleId)
        {
            f32 Depth = powf(2.0f, TriangleId + 1);
            v3 Points[3] =
                {
                    V3(-1.0f, -0.5f, Depth),
                    V3(0.0f, 0.5f, Depth),
                    V3(1.0f, -0.5f, Depth),

                };

            //NOTE: Shifting tje triangle points in a circle
            for (u32 PointId = 0; PointId < ArrayCount(Points); ++PointId)
            {
                v3 ShiftedPoint = Points[PointId] + V3(cosf(GlobalState.CurrTime), sinf(GlobalState.CurrTime), 0);
                Points[PointId] = ShiftedPoint;
            }

            DrawTriangle(Points, Colors);
        }

        //win32
        RECT ClientRect = {};
        Assert(GetClientRect(GlobalState.WindowHandle, &ClientRect));
        u32 ClientWidth = ClientRect.right - ClientRect.left;
        u32 ClientHeight = ClientRect.bottom - ClientRect.top;


        BITMAPINFO BitmapInfo = {};
        BitmapInfo.bmiHeader.biSize = sizeof(tagBITMAPINFOHEADER);
        BitmapInfo.bmiHeader.biWidth = GlobalState.FrameBufferWidth;
        BitmapInfo.bmiHeader.biHeight = GlobalState.FrameBufferHeight;
        BitmapInfo.bmiHeader.biPlanes = 1;
        BitmapInfo.bmiHeader.biBitCount = 32;
        BitmapInfo.bmiHeader.biCompression = BI_RGB;

        Assert(StretchDIBits(GlobalState.DeviceContext,
                                0,
                                0,
                                ClientWidth,
                                ClientHeight,
                                0,
                                0,
                                GlobalState.FrameBufferWidth,
                                GlobalState.FrameBufferHeight,
                                GlobalState.FrameBufferPixels,
                                &BitmapInfo,
                                DIB_RGB_COLORS,
                                SRCCOPY));
    }
    return 0;
}
