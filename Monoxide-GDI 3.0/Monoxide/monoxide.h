#pragma once
#pragma warning( disable: 4152 )
#pragma warning( disable: 4201 )

#define RGBQUAD _RGBQUAD
#pragma warning( push, 0 )
#include <windows.h>
#pragma warning( pop )
#undef RGBQUAD

#include <accctrl.h>
#include <aclapi.h>
#include <stdio.h>
#include <math.h>

VOID
WINAPI
TimerThread(
	VOID
);

VOID
GetRandomPath(
	_Inout_ PWSTR szRandom,
	_In_ INT nLength
);

VOID
WINAPI
MessageBoxThread(VOID);

VOID
WINAPI
EnumGlobalWnd(VOID);

VOID
WINAPI
CursorDraw(VOID);

BOOL
CALLBACK
MonitorEnumProc(
	_In_ HMONITOR hMonitor,
	_In_ HDC hDC,
	_In_ PRECT prcArea,
	_In_ LPARAM lParam
);

LRESULT
CALLBACK
NoDestroyWndProc(
	_In_ HWND hWnd,
	_In_ DWORD dwMsg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
);

VOID
CALLBACK
TimerProc(
	_In_ HWND hWnd,
	_In_ UINT nMsg,
	_In_ UINT nIDEvent,
	_In_ DWORD dwTime
);

typedef union
{
	COLORREF rgb;
	struct
	{
		BYTE b;
		BYTE g;
		BYTE r;
		BYTE unused;
	};
} RGBQUAD, * PRGBQUAD;

typedef struct {
	FLOAT h;
	FLOAT s;
	FLOAT l;
} HSLCOLOR;

HSLCOLOR
RGBToHSL(
	_In_ RGBQUAD rgb
);

RGBQUAD
HSLToRGB(
	_In_ HSLCOLOR hsl
);


typedef VOID(WINAPI GDI_SHADER)(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ HDC hdcTemp,
	_In_ HBITMAP hbmTemp,
	_In_ PRGBQUAD prgbSrc,
	_Inout_ PRGBQUAD prgbDst
	), * PGDI_SHADER;

typedef VOID(WINAPI GDI_SHADER_OPERATION)(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ RECT rcBounds,
	_In_ HDC hdcDst,
	_In_ HDC hdcTemp
	), * PGDI_SHADER_OPERATION;

typedef struct tagGDI_SHADER_PARAMS
{
	PGDI_SHADER pGdiShader;
	PGDI_SHADER_OPERATION pPreGdiShader;
	PGDI_SHADER_OPERATION pPostGdiShader;
} GDI_SHADER_PARAMS, * PGDISHADER_PARAMS;

#pragma region Public Variables
extern HWND hwndDesktop;
extern HDC hdcDesktop;
extern RECT rcScrBounds;
extern HHOOK hMsgHook;
extern INT nCounter;
#pragma endregion Public Variables

#define PI 3.141592f

VOID
SeedXorshift32(
	_In_ DWORD dwSeed
);

DWORD
Xorshift32(VOID);

VOID
Reflect2D(
	_Inout_ PINT x,
	_Inout_ PINT y,
	_In_ INT w,
	_In_ INT h
);

VOID
InitializeSine(VOID);

FLOAT
FastSine(
	_In_ FLOAT f
);

FLOAT
FastCosine(
	_In_ FLOAT f
);

#define TIMER_DELAY 100
#define PAYLOAD_MS 10000
#define PAYLOAD_TIME ( PAYLOAD_MS / TIMER_DELAY )

VOID
WINAPI
GdiShaderThread(
	_In_ PGDISHADER_PARAMS pGdiShaderParams
);

VOID
WINAPI
ExecuteGdiShader(
	_In_ HDC hdcDst,
	_In_ RECT rcBounds,
	_In_ INT nTime,
	_In_ INT nDelay,
	_In_ GDI_SHADER pGdiShader,
	_In_opt_ GDI_SHADER_OPERATION pPreGdiShader,
	_In_opt_ GDI_SHADER_OPERATION pPostGdiShader
);

INT nShaderThreeSeed;
GDI_SHADER GdiShader1;
GDI_SHADER GdiShader2;
GDI_SHADER GdiShader3;
GDI_SHADER GdiShader4;
GDI_SHADER GdiShader5;
GDI_SHADER GdiShader6;
GDI_SHADER GdiShader7;
GDI_SHADER GdiShader8;
GDI_SHADER GdiShader9;
GDI_SHADER GdiShader10;
GDI_SHADER GdiShader11;
GDI_SHADER GdiShader12;
GDI_SHADER GdiShader13;
GDI_SHADER GdiShader14;
GDI_SHADER GdiShader15;
GDI_SHADER GdiShader16;
GDI_SHADER GdiShader17;
GDI_SHADER GdiShader18;
GDI_SHADER GdiShader19;
GDI_SHADER GdiShader20;
GDI_SHADER GdiShader21;
GDI_SHADER FinalGdiShader;
GDI_SHADER_OPERATION PreGdiShader1;
GDI_SHADER_OPERATION PostGdiShader1;
GDI_SHADER_OPERATION PostGdiShader2;
GDI_SHADER_OPERATION PostGdiShader3;
GDI_SHADER_OPERATION PostGdiShader4;
GDI_SHADER_OPERATION PostGdiShader5;
GDI_SHADER_OPERATION PostGdiShader6;
GDI_SHADER_PARAMS pGdiShaders[25];