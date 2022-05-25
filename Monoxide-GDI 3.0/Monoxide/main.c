#include "monoxide.h"

#pragma region Public Variables
HWND hwndDesktop;
HDC hdcDesktop;
RECT rcScrBounds;
HHOOK hMsgHook;
INT nCounter = 0;
#pragma endregion Public Variables

DWORD xs;
static FLOAT pfSinVals[4096];

BOOL
CALLBACK
MonitorEnumProc(
	_In_ HMONITOR hMonitor,
	_In_ HDC hDC,
	_In_ PRECT prcArea,
	_In_ LPARAM lParam
)
{
	UNREFERENCED_PARAMETER(hMonitor);
	UNREFERENCED_PARAMETER(hDC);
	UNREFERENCED_PARAMETER(lParam);

	rcScrBounds.left = min(rcScrBounds.left, prcArea->left);
	rcScrBounds.top = min(rcScrBounds.top, prcArea->top);
	rcScrBounds.right = max(rcScrBounds.right, prcArea->right);
	rcScrBounds.bottom = max(rcScrBounds.bottom, prcArea->bottom);

	return TRUE;
}

LRESULT
CALLBACK
NoDestroyWndProc(
	_In_ HWND hWnd,
	_In_ DWORD dwMsg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
)
{
	switch (dwMsg)
	{
	default:
		return DefWindowProcW(hWnd, dwMsg, wParam, lParam);
	case WM_DESTROY:
	case WM_CLOSE:
	case WM_QUIT:
		return CALLBACK_NULL;
	}
}

VOID
CALLBACK
TimerProc(
	_In_ HWND hWnd,
	_In_ UINT nMsg,
	_In_ UINT nIDEvent,
	_In_ DWORD dwTime
)
{
	UNREFERENCED_PARAMETER(hWnd);
	UNREFERENCED_PARAMETER(nMsg);
	UNREFERENCED_PARAMETER(nIDEvent);
	UNREFERENCED_PARAMETER(dwTime);

	nCounter++;
}

HSLCOLOR
RGBToHSL(
	_In_ RGBQUAD rgb
)
{
	HSLCOLOR hsl;

	BYTE r = rgb.r;
	BYTE g = rgb.g;
	BYTE b = rgb.b;

	FLOAT _r = (FLOAT)r / 255.f;
	FLOAT _g = (FLOAT)g / 255.f;
	FLOAT _b = (FLOAT)b / 255.f;

	FLOAT rgbMin = min(min(_r, _g), _b);
	FLOAT rgbMax = max(max(_r, _g), _b);

	FLOAT fDelta = rgbMax - rgbMin;
	FLOAT deltaR;
	FLOAT deltaG;
	FLOAT deltaB;

	FLOAT h = 0.f;
	FLOAT s = 0.f;
	FLOAT l = (FLOAT)((rgbMax + rgbMin) / 2.f);

	if (fDelta != 0.f)
	{
		s = l < .5f ? (FLOAT)(fDelta / (rgbMax + rgbMin)) : (FLOAT)(fDelta / (2.f - rgbMax - rgbMin));
		deltaR = (FLOAT)(((rgbMax - _r) / 6.f + (fDelta / 2.f)) / fDelta);
		deltaG = (FLOAT)(((rgbMax - _g) / 6.f + (fDelta / 2.f)) / fDelta);
		deltaB = (FLOAT)(((rgbMax - _b) / 6.f + (fDelta / 2.f)) / fDelta);

		if (_r == rgbMax)
		{
			h = deltaB - deltaG;
		}
		else if (_g == rgbMax)
		{
			h = (1.f / 3.f) + deltaR - deltaB;
		}
		else if (_b == rgbMax)
		{
			h = (2.f / 3.f) + deltaG - deltaR;
		}

		if (h < 0.f)
		{
			h += 1.f;
		}
		if (h > 1.f)
		{
			h -= 1.f;
		}
	}

	hsl.h = h;
	hsl.s = s;
	hsl.l = l;
	return hsl;
}

RGBQUAD
HSLToRGB(
	_In_ HSLCOLOR hsl
)
{
	RGBQUAD rgb;

	FLOAT r = hsl.l;
	FLOAT g = hsl.l;
	FLOAT b = hsl.l;

	FLOAT h = hsl.h;
	FLOAT sl = hsl.s;
	FLOAT l = hsl.l;
	FLOAT v = (l <= .5f) ? (l * (1.f + sl)) : (l + sl - l * sl);

	FLOAT m;
	FLOAT sv;
	FLOAT fract;
	FLOAT vsf;
	FLOAT mid1;
	FLOAT mid2;

	INT sextant;

	if (v > 0.f)
	{
		m = l + l - v;
		sv = (v - m) / v;
		h *= 6.f;
		sextant = (INT)h;
		fract = h - sextant;
		vsf = v * sv * fract;
		mid1 = m + vsf;
		mid2 = v - vsf;

		switch (sextant)
		{
		case 0:
			r = v;
			g = mid1;
			b = m;
			break;
		case 1:
			r = mid2;
			g = v;
			b = m;
			break;
		case 2:
			r = m;
			g = v;
			b = mid1;
			break;
		case 3:
			r = m;
			g = mid2;
			b = v;
			break;
		case 4:
			r = mid1;
			g = m;
			b = v;
			break;
		case 5:
			r = v;
			g = m;
			b = mid2;
			break;
		}
	}

	rgb.r = (BYTE)(r * 255.f);
	rgb.g = (BYTE)(g * 255.f);
	rgb.b = (BYTE)(b * 255.f);

	return rgb;
}

VOID
WINAPI
TimerThread(
	VOID
)
{
	SetTimer(NULL, 0, TIMER_DELAY, (TIMERPROC)TimerProc);

	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

VOID
SeedXorshift32(
	_In_ DWORD dwSeed
)
{
	xs = dwSeed;
}

DWORD
Xorshift32(VOID)
{
	xs ^= xs << 13;
	xs ^= xs >> 17;
	xs ^= xs << 5;
	return xs;
}

VOID
Reflect2D(
	_Inout_ PINT x,
	_Inout_ PINT y,
	_In_ INT w,
	_In_ INT h
)
{
#define FUNCTION(v, maxv) ( abs( v ) / ( maxv ) % 2 ? ( maxv ) - abs( v ) % ( maxv ) : abs( v ) % ( maxv ) );
	*x = FUNCTION(*x, w - 1);
	*y = FUNCTION(*y, h - 1);
#undef FUNCTION
}

FLOAT
FastSine(
	_In_ FLOAT f
)
{
	INT i = (INT)(f / (2.f * PI) * (FLOAT)_countof(pfSinVals));
	return pfSinVals[i % _countof(pfSinVals)];
}

FLOAT
FastCosine(
	_In_ FLOAT f
)
{
	return FastSine(f + PI / 2.f);
}

VOID
InitializeSine(VOID)
{
	for (INT i = 0; i < _countof(pfSinVals); i++)
		pfSinVals[i] = sinf((FLOAT)i / (FLOAT)_countof(pfSinVals) * PI * 2.f);
}

VOID
WINAPI
Initialize( VOID )
{
	HMODULE hModUser32 = LoadLibraryW( L"user32.dll" );
	BOOL( WINAPI * SetProcessDPIAware )( VOID ) = ( BOOL( WINAPI * )( VOID ) )GetProcAddress( hModUser32, "SetProcessDPIAware" );
	if ( SetProcessDPIAware )
		SetProcessDPIAware( );
	FreeLibrary( hModUser32 );

	hwndDesktop = HWND_DESKTOP;
	hdcDesktop = GetDC( hwndDesktop );

	SeedXorshift32( ( DWORD )__rdtsc( ) );
	InitializeSine( );
	EnumDisplayMonitors( NULL, NULL, &MonitorEnumProc, 0 );
	CloseHandle( CreateThread( NULL, 0, ( PTHREAD_START_ROUTINE )TimerThread, NULL, 0, NULL ) );
}

INT
WINAPI
wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ PWSTR pszCmdLine,
	_In_ INT nShowCmd
)
{
	UNREFERENCED_PARAMETER( hInstance );
	UNREFERENCED_PARAMETER( hPrevInstance );
	UNREFERENCED_PARAMETER( pszCmdLine );
	UNREFERENCED_PARAMETER( nShowCmd );

	HANDLE hCursorDraw, hGdiThread;
	
	Initialize( );

	Sleep( 5000 );
	//CloseHandle( CreateThread( NULL, 0, ( PTHREAD_START_ROUTINE )MessageBoxThread, NULL, 0, NULL ) );
	Sleep( 1000 );

	//CloseHandle( CreateThread( NULL, 0, ( PTHREAD_START_ROUTINE )EnumGlobalWnd, NULL, 0, NULL ) );

	hCursorDraw = CreateThread( NULL, 0, ( PTHREAD_START_ROUTINE )CursorDraw, NULL, 0, NULL );

	CreateMutexW( NULL, TRUE, L"Monoxide.exe" );

	if ( GetLastError( ) == ERROR_ALREADY_EXISTS )
	{
		exit;
		return 0;
	}

	pGdiShaders[ 0 ]  = ( GDI_SHADER_PARAMS ) { GdiShader1,  NULL,          PostGdiShader1 };
	pGdiShaders[ 1 ]  = ( GDI_SHADER_PARAMS ) { GdiShader2,  NULL,          PostGdiShader2 };
	pGdiShaders[ 2 ]  = ( GDI_SHADER_PARAMS ) { GdiShader3,  NULL,          PostGdiShader3 };
	pGdiShaders[ 3 ]  = ( GDI_SHADER_PARAMS ) { GdiShader4,  NULL,          PostGdiShader2 };
	pGdiShaders[ 4 ]  = ( GDI_SHADER_PARAMS ) { GdiShader5,  NULL,          PostGdiShader4 };
	pGdiShaders[ 5 ]  = ( GDI_SHADER_PARAMS ) { GdiShader6,  NULL,          PostGdiShader2 };
	pGdiShaders[ 6 ]  = ( GDI_SHADER_PARAMS ) { GdiShader7,  NULL,          PostGdiShader5 };
	pGdiShaders[ 7 ]  = ( GDI_SHADER_PARAMS ) { GdiShader8,  PreGdiShader1, PostGdiShader6 };
	pGdiShaders[ 8 ]  = ( GDI_SHADER_PARAMS ) { GdiShader9,  NULL,          NULL };
	pGdiShaders[ 9 ]  = ( GDI_SHADER_PARAMS ) { GdiShader10, NULL,          NULL };
	pGdiShaders[ 10 ] = ( GDI_SHADER_PARAMS ) { GdiShader11, NULL,          NULL };
	pGdiShaders[ 11 ] = ( GDI_SHADER_PARAMS ) { GdiShader12, NULL,          NULL };
	pGdiShaders[ 12 ] = ( GDI_SHADER_PARAMS ) { GdiShader13, NULL,          NULL };
	pGdiShaders[ 13 ] = ( GDI_SHADER_PARAMS ) { GdiShader14, NULL,          PostGdiShader2 };
	pGdiShaders[ 14 ] = ( GDI_SHADER_PARAMS ) { GdiShader15, NULL,          NULL };
	pGdiShaders[ 15 ] = ( GDI_SHADER_PARAMS ) { GdiShader16, NULL,          NULL };
	pGdiShaders[ 16 ] = ( GDI_SHADER_PARAMS ) { GdiShader17, NULL,          NULL };
	pGdiShaders[ 17 ] = ( GDI_SHADER_PARAMS ) { GdiShader18, NULL,          NULL };
	pGdiShaders[ 18 ] = ( GDI_SHADER_PARAMS ) { GdiShader19, NULL,          NULL };
	pGdiShaders[ 19 ] = ( GDI_SHADER_PARAMS ) { GdiShader20, NULL,          PostGdiShader2 };
	pGdiShaders[ 20 ] = ( GDI_SHADER_PARAMS ) { GdiShader21, NULL,          PostGdiShader2 };
	pGdiShaders[ 24 ] = ( GDI_SHADER_PARAMS ) { FinalGdiShader, NULL, NULL };
	
	for ( ;; )
	{
		hGdiThread = CreateThread( NULL, 0, ( PTHREAD_START_ROUTINE )GdiShaderThread, &pGdiShaders[ Xorshift32( ) % 21 ], 0, NULL );
		WaitForSingleObject( hGdiThread, ( Xorshift32( ) % 3 ) ? PAYLOAD_MS : ( ( Xorshift32( ) % 5 ) * ( PAYLOAD_MS / 4 ) ) );
		CloseHandle( hGdiThread );

		if ( nCounter >= ( ( 180 * 1000 ) / TIMER_DELAY ) )
		{
			break;
		}
	}

	TerminateThread( hCursorDraw, 0 );
	CloseHandle( hCursorDraw );

	CloseHandle( CreateThread( NULL, 0, ( PTHREAD_START_ROUTINE )GdiShaderThread, &pGdiShaders[ 24 ], 0, NULL ) );

	for ( ;; )
	{
		exit;
	}
}

VOID
WINAPI
GdiShaderThread(
	_In_ PGDISHADER_PARAMS pGdiShaderParams
)
{
	if (pGdiShaderParams->pGdiShader == GdiShader3)
	{
		nShaderThreeSeed = Xorshift32();
	}

	ExecuteGdiShader(hdcDesktop, rcScrBounds, PAYLOAD_TIME, 5, pGdiShaderParams->pGdiShader,
		pGdiShaderParams->pPreGdiShader, pGdiShaderParams->pPostGdiShader);
}

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
)
{
	BITMAPINFO bmi = { 0 };
	PRGBQUAD prgbSrc, prgbDst;
	HANDLE hHeap;
	HDC hdcTemp;
	HBITMAP hbmTemp;
	SIZE_T nSize;
	INT nWidth;
	INT nHeight;

	nWidth = rcBounds.right - rcBounds.left;
	nHeight = rcBounds.bottom - rcBounds.top;
	nSize = nWidth * nHeight * sizeof(COLORREF);

	bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biWidth = nWidth;
	bmi.bmiHeader.biHeight = nHeight;

	hHeap = GetProcessHeap();
	prgbSrc = (PRGBQUAD)HeapAlloc(hHeap, 0, nSize);

	hdcTemp = CreateCompatibleDC(hdcDst);
	hbmTemp = CreateDIBSection(hdcDst, &bmi, 0, &prgbDst, NULL, 0);
	SelectObject(hdcTemp, hbmTemp);

	for (INT i = 0, j = nCounter; (j + nTime) > nCounter; i++)
	{
		if (pPreGdiShader == NULL)
		{
			BitBlt(hdcTemp, 0, 0, nWidth, nHeight, hdcDst, rcBounds.left, rcBounds.top, SRCCOPY);
		}
		else
		{
			pPreGdiShader(i, nWidth, nHeight, rcBounds, hdcDst, hdcTemp);
		}

		RtlCopyMemory(prgbSrc, prgbDst, nSize);

		pGdiShader(i, nWidth, nHeight, hdcDst, hbmTemp, prgbSrc, prgbDst);

		if (pPostGdiShader == NULL)
		{
			BitBlt(hdcDst, rcBounds.left, rcBounds.top, nWidth, nHeight, hdcTemp, 0, 0, SRCCOPY);
		}
		else
		{
			pPostGdiShader(i, nWidth, nHeight, rcBounds, hdcDst, hdcTemp);
		}

		if (nDelay)
		{
			Sleep(nDelay);
		}
	}

	HeapFree(hHeap, 0, prgbSrc);
	DeleteObject(hbmTemp);
	DeleteDC(hdcTemp);
}

VOID
WINAPI
GdiShader1(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ HDC hdcTemp,
	_In_ HBITMAP hbmTemp,
	_In_ PRGBQUAD prgbSrc,
	_Inout_ PRGBQUAD prgbDst
)
{
	UNREFERENCED_PARAMETER(hdcTemp);
	UNREFERENCED_PARAMETER(hbmTemp);

	UINT u;
	UINT v;
	FLOAT div = (FLOAT)t / 30.f;
	FLOAT a = FastSine(div) * 5.f;
	FLOAT b = FastCosine(div) * 5.f;
	RGBQUAD rgbDst;

	for (INT y = 0; y < h; y++)
	{
		for (INT x = 0; x < w; x++)
		{
			u = x + (INT)a, v = y + (INT)b;
			u %= w;
			v %= h;

			rgbDst = prgbSrc[v * w + u];
			rgbDst.r += ~prgbSrc[y * w + x].r / 32;
			rgbDst.g += ~prgbSrc[y * w + x].g / 32;
			rgbDst.b += ~prgbSrc[y * w + x].b / 32;
			prgbDst[y * w + x] = rgbDst;
		}
	}
}

VOID
WINAPI
PostGdiShader1(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ RECT rcBounds,
	_In_ HDC hdcDst,
	_In_ HDC hdcTemp
)
{
	if (!(t % 256))
	{
		RedrawWindow(NULL, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
	}
	else
	{
		BitBlt(hdcDst, rcBounds.left, rcBounds.top, w, h, hdcTemp, 0, 0, SRCCOPY);
	}
}

VOID
WINAPI
GdiShader2(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ HDC hdcTemp,
	_In_ HBITMAP hbmTemp,
	_In_ PRGBQUAD prgbSrc,
	_Inout_ PRGBQUAD prgbDst
)
{
	UNREFERENCED_PARAMETER(hdcTemp);
	UNREFERENCED_PARAMETER(hbmTemp);

	UINT u;
	UINT v;
	RGBQUAD rgbDst;

	for (INT y = 0; y < h; y++)
	{
		FLOAT _y = (FLOAT)y / 20.f;

		for (INT x = 0; x < w; x++)
		{
			FLOAT div = (FLOAT)t / 4.f;
			FLOAT a = FastSine(div + _y) * 10.f;
			FLOAT b = FastCosine(div + (FLOAT)x / 20.f) * 40.f;

			u = x + (INT)a, v = y + (INT)b;
			u %= w;
			v %= h;

			rgbDst = prgbSrc[y * w + u];

			DWORD rgb = prgbSrc[v * w + x].rgb / ((0x101010 | (t & y) | ((t & x) << 8) | (t << 16)) + 1);
			if (!rgb)
			{
				rgb = 2;
			}

			rgbDst.rgb /= rgb;
			if (!rgbDst.rgb)
			{
				rgbDst.rgb = 0xFFFFFF;
			}

			prgbDst[y * w + x] = rgbDst;
		}
	}
}

VOID
WINAPI
PostGdiShader2(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ RECT rcBounds,
	_In_ HDC hdcDst,
	_In_ HDC hdcTemp
)
{
	if (!(t % 16))
	{
		RedrawWindow(NULL, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
	}
	else
	{
		BitBlt(hdcDst, rcBounds.left, rcBounds.top, w, h, hdcTemp, 0, 0, SRCCOPY);
	}
}

VOID
WINAPI
GdiShader3(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ HDC hdcTemp,
	_In_ HBITMAP hbmTemp,
	_In_ PRGBQUAD prgbSrc,
	_Inout_ PRGBQUAD prgbDst
)
{
	UNREFERENCED_PARAMETER(hdcTemp);
	UNREFERENCED_PARAMETER(hbmTemp);

	UINT u;
	UINT v;
	RGBQUAD rgbDst;
	HSLCOLOR hsl;
	FLOAT _t = (FLOAT)t / 10.f;

	for (INT y = 0; y < h; y++)
	{
		FLOAT _y = (FLOAT)y / 25.f;

		for (INT x = 0; x < w; x++)
		{
			FLOAT a = FastCosine(_y + _t) * 16.f;

			u = x + (INT)a, v = y;
			u %= w;
			v %= h;

			rgbDst = prgbSrc[v * w + u];
			FLOAT f = 1.f / 8.f;
			FLOAT r = (FLOAT)prgbSrc[y * w + x].r * f + (FLOAT)rgbDst.r * (1.f - f);
			FLOAT g = (FLOAT)prgbSrc[y * w + x].g * f + (FLOAT)rgbDst.g * (1.f - f);
			FLOAT b = (FLOAT)prgbSrc[y * w + x].b * f + (FLOAT)rgbDst.b * (1.f - f);

			rgbDst.rgb = ((BYTE)b | ((BYTE)g << 8) | ((BYTE)r << 16));
			hsl = RGBToHSL(rgbDst);
			hsl.h = (FLOAT)fmod((DOUBLE)hsl.h + 1.0 / 45.0 + ((FLOAT)x + (FLOAT)y) / (((FLOAT)w + (FLOAT)h) * 64.f), 1.0);
			prgbDst[y * w + x] = HSLToRGB(hsl);
		}
	}
}

VOID
WINAPI
PostGdiShader3(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ RECT rcBounds,
	_In_ HDC hdcDst,
	_In_ HDC hdcTemp
)
{
	INT x, y;
	HBRUSH hbrBall;
	HPEN hpenBall;

	BitBlt(hdcDst, rcBounds.left, rcBounds.top, w, h, hdcTemp, 0, 0, SRCCOPY);

	t += nShaderThreeSeed;
	x = t * 16;
	y = t * 16;

	for (INT i = 64; i > 8; i -= 8)
	{
		hbrBall = CreateSolidBrush(0x0000FF);
		hpenBall = CreatePen(PS_SOLID, 2, 0xFFFFFF);

		SelectObject(hdcDst, hbrBall);
		SelectObject(hdcDst, hpenBall);
		Reflect2D(&x, &y, w, h);
		Ellipse(hdcDst, x + rcBounds.left - i, y + rcBounds.top - i, x + rcBounds.left + i, y + rcBounds.top + i);
		DeleteObject(hbrBall);
		DeleteObject(hpenBall);
	}
}

VOID
WINAPI
GdiShader4(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ HDC hdcTemp,
	_In_ HBITMAP hbmTemp,
	_In_ PRGBQUAD prgbSrc,
	_Inout_ PRGBQUAD prgbDst
)
{
	UNREFERENCED_PARAMETER(hdcTemp);
	UNREFERENCED_PARAMETER(hbmTemp);

	UINT u;
	UINT v;
	RGBQUAD rgbDst;
	RGBQUAD rgbSrc;
	FLOAT _t = (FLOAT)t / 5000.f;
	FLOAT a = (FLOAT)t / 50.f;
	FLOAT b = FastSine(a) * _t;
	FLOAT c = FastCosine(a) * _t;
	FLOAT centerX = (FLOAT)w / 2;
	FLOAT centerY = (FLOAT)h / 2;

	while (b < 0.f)
	{
		b += PI * 2.f;
	}

	while (c < 0.f)
	{
		c += PI * 2.f;
	}

	for (INT y = 0; y < h; y++)
	{
		for (INT x = 0; x < w; x++)
		{
			u = (UINT)((x - centerX) * FastCosine(b) - (y - centerY) * FastSine(c) + centerX);
			v = (UINT)((x - centerX) * FastSine(c) + (y - centerY) * FastCosine(b) + centerY);

			Reflect2D((PINT)&u, (PINT)&v, w, h);

			rgbDst = prgbSrc[v * w + u];
			rgbSrc = prgbSrc[y * w + x];
			rgbDst.rgb ^= rgbSrc.rgb;

			if ((t / 32) % 2)
			{
				rgbDst.rgb = ~rgbDst.rgb;
			}

			prgbDst[y * w + x] = rgbDst;
		}
	}
}

VOID
WINAPI
GdiShader5(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ HDC hdcTemp,
	_In_ HBITMAP hbmTemp,
	_In_ PRGBQUAD prgbSrc,
	_Inout_ PRGBQUAD prgbDst
)
{
	UNREFERENCED_PARAMETER(hdcTemp);
	UNREFERENCED_PARAMETER(hbmTemp);

	UINT u;
	UINT v;
	RGBQUAD rgbDst;
	RGBQUAD rgbSrc;

	for (INT y = 0; y < h; y++)
	{
		for (INT x = 0; x < w; x++)
		{
			u = ~((x + t) & y);
			v = ~((y + t) & x);

			u %= w;
			v %= h;

			rgbDst = prgbSrc[v * w + u];
			rgbSrc = prgbSrc[y * w + x];
			rgbDst.rgb ^= rgbSrc.rgb;

			prgbDst[y * w + x] = rgbDst;
		}
	}
}

VOID
WINAPI
PostGdiShader4(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ RECT rcBounds,
	_In_ HDC hdcDst,
	_In_ HDC hdcTemp
)
{
	UNREFERENCED_PARAMETER(hdcDst);
	UNREFERENCED_PARAMETER(hdcTemp);

	BitBlt(hdcDst, rcBounds.left, rcBounds.top, w, h, hdcTemp, 0, 0, SRCCOPY);

	if (!(t % 8))
	{
		RedrawWindow(NULL, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
	}
}

VOID
WINAPI
GdiShader6(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ HDC hdcTemp,
	_In_ HBITMAP hbmTemp,
	_In_ PRGBQUAD prgbSrc,
	_Inout_ PRGBQUAD prgbDst
)
{
	UNREFERENCED_PARAMETER(hdcTemp);
	UNREFERENCED_PARAMETER(hbmTemp);

	UINT u;
	UINT v;
	FLOAT div = (FLOAT)t / 20.f;
	FLOAT a = FastCosine(div) * 2.f * PI;
	BOOL bShiftDir = (BOOL)(Xorshift32() & 1);
	BYTE bChannels = (BYTE)(Xorshift32() & 0b111);
	RGBQUAD rgbSrc;
	RGBQUAD rgbDst;

	for (INT y = 0; y < h; y++)
	{
		for (INT x = 0; x < w; x++)
		{
			FLOAT b = (FLOAT)(x + y + t * 32) / 100.f;
			FLOAT c = FastSine(a + b) * 10.f;

			u = x + (INT)a, v = y + (INT)c;
			Reflect2D((PINT)&u, (PINT)&v, w, h);

			rgbDst = prgbSrc[v * w + x];

			if (bShiftDir)
			{
				rgbDst.rgb <<= 1;
			}
			else
			{
				rgbDst.rgb >>= 1;
			}

			rgbSrc = prgbSrc[v * w + x];
			rgbDst.rgb ^= rgbSrc.rgb;

			if (bChannels & 0b001)
			{
				rgbDst.b |= rgbSrc.b;
			}

			if (bChannels & 0b010)
			{
				rgbDst.g |= rgbSrc.g;
			}

			if (bChannels & 0b100)
			{
				rgbDst.r |= rgbSrc.r;
			}

			prgbDst[y * w + u] = rgbDst;
		}
	}
}

VOID
WINAPI
GdiShader7(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ HDC hdcTemp,
	_In_ HBITMAP hbmTemp,
	_In_ PRGBQUAD prgbSrc,
	_Inout_ PRGBQUAD prgbDst
)
{
	UNREFERENCED_PARAMETER(hdcTemp);
	UNREFERENCED_PARAMETER(hbmTemp);

	UINT u;
	UINT v;
	RGBQUAD rgbDst;
	RGBQUAD rgbSrc;
	BOOL bOperation = (BOOL)(Xorshift32() % 3);

	for (INT y = 0; y < h; y++)
	{
		for (INT x = 0; x < w; x++)
		{
			u = (x + t * 2) ^ (y + t * 8) ^ t;
			v = (x + t * 8) + (y + t * 2) ^ t;

			u %= w;
			v %= h;

			rgbDst = prgbSrc[v * w + u];
			rgbSrc = prgbSrc[y * w + x];

			if (bOperation)
			{
				rgbDst.rgb |= rgbSrc.rgb;
			}
			else
			{
				rgbDst.rgb &= rgbSrc.rgb;
			}

			prgbDst[y * w + x] = rgbDst;
		}
	}
}

VOID
WINAPI
PostGdiShader5(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ RECT rcBounds,
	_In_ HDC hdcDst,
	_In_ HDC hdcTemp
)
{
	if (!(t % 4))
	{
		RedrawWindow(NULL, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
	}
	else
	{
		BitBlt(hdcDst, rcBounds.left, rcBounds.top, w, h, hdcTemp, 0, 0, NOTSRCCOPY);
	}
}

VOID
WINAPI
PreGdiShader1(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ RECT rcBounds,
	_In_ HDC hdcDst,
	_In_ HDC hdcTemp
)
{
	UNREFERENCED_PARAMETER(t);

	BitBlt(hdcTemp, 0, 0, w, h, hdcDst, rcBounds.left, rcBounds.top, SRCCOPY);

	for (INT i = 0; i < 5; i++)
	{
		INT nBlockSize = Xorshift32() % 129 + 128;
		INT nNewBlockSize = nBlockSize + (Xorshift32() % 17 + 16);
		INT x = Xorshift32() % (w - nBlockSize);
		INT y = Xorshift32() % (h - nBlockSize);

		StretchBlt(hdcTemp, x - (nNewBlockSize - nBlockSize) / 2, y - (nNewBlockSize - nBlockSize) / 2,
			nNewBlockSize, nNewBlockSize, hdcTemp, x, y, nBlockSize, nBlockSize, SRCCOPY);
	}
}

VOID
WINAPI
GdiShader8(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ HDC hdcTemp,
	_In_ HBITMAP hbmTemp,
	_In_ PRGBQUAD prgbSrc,
	_Inout_ PRGBQUAD prgbDst
)
{
	UNREFERENCED_PARAMETER(hdcTemp);
	UNREFERENCED_PARAMETER(hbmTemp);

	UINT u;
	UINT v;
	FLOAT div = (FLOAT)t / 10.f;
	FLOAT a = FastSine(div) * 32.f;
	FLOAT b = FastCosine(div) * 32.f;
	FLOAT f = 1.f / 4.f;
	RGBQUAD rgbDst;
	RGBQUAD rgbSrc;
	HSLCOLOR hsl;

	for (INT y = 0; y < h; y++)
	{
		for (INT x = 0; x < w; x++)
		{
			u = x + (INT)a + (INT)((y, 10, h) * 16.f);
			v = y + (INT)b;

			Reflect2D((PINT)&u, (PINT)&v, w, h);

			rgbDst = prgbSrc[v * w + u];
			rgbSrc = prgbSrc[y * w + x];

			if (!rgbSrc.rgb)
			{
				rgbSrc.rgb = 1;
			}

			rgbDst.rgb &= rgbDst.rgb % ((rgbSrc.rgb << 8) + 1);
			FLOAT _r = (FLOAT)rgbDst.r * f + (FLOAT)rgbSrc.r * (1.f - f);
			FLOAT _g = (FLOAT)rgbDst.g * f + (FLOAT)rgbSrc.g * (1.f - f);
			FLOAT _b = (FLOAT)rgbDst.b * f + (FLOAT)rgbSrc.b * (1.f - f);
			rgbDst.rgb = ((BYTE)_b | ((BYTE)_g << 8) | ((BYTE)_r << 16));

			hsl = RGBToHSL(rgbDst);
			hsl.h = (FLOAT)fmod((DOUBLE)hsl.h + (DOUBLE)(x + y) / 100000.0 + 0.05, 1.0);
			hsl.s = 1.f;

			if (hsl.l < .2f)
			{
				hsl.l += .2f;
			}

			rgbDst = HSLToRGB(hsl);
			prgbDst[y * w + x] = rgbDst;
		}
	}
}

VOID
WINAPI
PostGdiShader6(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ RECT rcBounds,
	_In_ HDC hdcDst,
	_In_ HDC hdcTemp
)
{
	if (!(t % 32))
	{
		RedrawWindow(NULL, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
	}
	else
	{
		BitBlt(hdcDst, rcBounds.left, rcBounds.top, w, h, hdcTemp, 0, 0, SRCCOPY);
	}
}

VOID
WINAPI
GdiShader9(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ HDC hdcTemp,
	_In_ HBITMAP hbmTemp,
	_In_ PRGBQUAD prgbSrc,
	_Inout_ PRGBQUAD prgbDst
)
{
	UNREFERENCED_PARAMETER(hdcTemp);
	UNREFERENCED_PARAMETER(hbmTemp);

	UINT u;
	UINT v;
	FLOAT div = (FLOAT)t / 10.f;
	FLOAT a = FastSine(div) * 32.f;
	FLOAT b = FastCosine(div) * 32.f;
	FLOAT f = 1.f / 32.f;
	RGBQUAD rgbDst;
	RGBQUAD rgbSrc;
	HSLCOLOR hsl;

	for (INT y = 0; y < h; y++)
	{
		for (INT x = 0; x < w; x++)
		{
			u = x + (INT)a + (INT)((y, 10, h) * 16.f);
			v = y + (INT)b;

			Reflect2D((PINT)&u, (PINT)&v, w, h);

			rgbDst = prgbSrc[v * w + u];
			rgbSrc = prgbSrc[y * w + x];

			if (!rgbSrc.rgb)
			{
				rgbSrc.rgb = 1;
			}

			rgbDst.rgb &= rgbDst.rgb % ((rgbSrc.rgb << 8) + 1);
			FLOAT _r = (FLOAT)rgbDst.r * f + (FLOAT)rgbSrc.r * (1.f - f);
			FLOAT _g = (FLOAT)rgbDst.g * f + (FLOAT)rgbSrc.g * (1.f - f);
			FLOAT _b = (FLOAT)rgbDst.b * f + (FLOAT)rgbSrc.b * (1.f - f);
			rgbDst.rgb = ((BYTE)_b | ((BYTE)_g << 8) | ((BYTE)_r << 16));

			hsl = RGBToHSL(rgbDst);
			hsl.h /= 1.0125f;
			hsl.s /= 1.0125f;
			hsl.l /= 1.0125f;
			rgbDst = HSLToRGB(hsl);

			prgbDst[y * w + x] = rgbDst;
		}
	}
}

VOID
WINAPI
GdiShader10(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ HDC hdcTemp,
	_In_ HBITMAP hbmTemp,
	_In_ PRGBQUAD prgbSrc,
	_Inout_ PRGBQUAD prgbDst
)
{
	UNREFERENCED_PARAMETER(hdcTemp);
	UNREFERENCED_PARAMETER(hbmTemp);

	UINT u;
	UINT v;
	FLOAT f = 1.f / 64.f;
	RGBQUAD rgbDst;
	RGBQUAD rgbSrc;
	HSLCOLOR hsl;

	for (INT y = 0; y < h; y++)
	{
		for (INT x = 0; x < w; x++)
		{
			u = x + ((t + y) % 64) * -1;
			v = y + (t + x) % 64;

			u %= w;
			v %= h;

			rgbDst = prgbSrc[v * w + u];
			rgbSrc = prgbSrc[y * w + x];

			FLOAT _r = (FLOAT)rgbDst.r * f + (FLOAT)rgbSrc.r * (1.f - f);
			FLOAT _g = (FLOAT)rgbDst.g * f + (FLOAT)rgbSrc.g * (1.f - f);
			FLOAT _b = (FLOAT)rgbDst.b * f + (FLOAT)rgbSrc.b * (1.f - f);
			rgbDst.rgb = ((BYTE)_b | ((BYTE)_g << 8) | ((BYTE)_r << 16));

			hsl = RGBToHSL(rgbDst);
			hsl.s = .5f;
			hsl.l *= 1.125f;

			if (hsl.l > .5f)
			{
				hsl.l -= .5f;
			}

			if (hsl.l < .25f)
			{
				hsl.l += .25f;
			}

			rgbDst = HSLToRGB(hsl);

			prgbDst[y * w + x] = rgbDst;
		}
	}
}

VOID
WINAPI
GdiShader11(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ HDC hdcTemp,
	_In_ HBITMAP hbmTemp,
	_In_ PRGBQUAD prgbSrc,
	_Inout_ PRGBQUAD prgbDst
)
{
	UNREFERENCED_PARAMETER(hdcTemp);
	UNREFERENCED_PARAMETER(hbmTemp);

	UINT u;
	UINT v;
	FLOAT f = 1.f / 4.f;
	RGBQUAD rgbDst;
	RGBQUAD rgbSrc;
	HSLCOLOR hsl;

	for (INT y = 0; y < h; y++)
	{
		for (INT x = 0; x < w; x++)
		{
			u = x + y / (h / 16);
			v = y + u / (w / 16);
			u = x + v / (h / 16);

			u %= w;
			v %= h;

			rgbDst = prgbSrc[v * w + u];
			rgbSrc = prgbSrc[y * w + x];

			FLOAT _r = (FLOAT)rgbDst.r * f + (FLOAT)rgbSrc.r * (1.f - f);
			FLOAT _g = (FLOAT)rgbDst.g * f + (FLOAT)rgbSrc.g * (1.f - f);
			FLOAT _b = (FLOAT)rgbDst.b * f + (FLOAT)rgbSrc.b * (1.f - f);
			rgbDst.rgb = ((BYTE)_b | ((BYTE)_g << 8) | ((BYTE)_r << 16));

			hsl = RGBToHSL(rgbDst);

			if (hsl.s < .5f)
			{
				hsl.s = .5f;
			}

			if ((roundf(hsl.h * 10.f) / 10.f) != (roundf((FLOAT)((Xorshift32() + t) % 257) / 256.f * 10.f) / 10.f))
			{
				hsl.h = (FLOAT)fmod((DOUBLE)hsl.h + .1, 1.0);
			}
			else
			{
				hsl.h = (FLOAT)fmod((DOUBLE)hsl.h + .5, 1.0);
			}

			rgbDst = HSLToRGB(hsl);

			prgbDst[y * w + x] = rgbDst;
		}
	}
}

VOID
WINAPI
GdiShader12(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ HDC hdcTemp,
	_In_ HBITMAP hbmTemp,
	_In_ PRGBQUAD prgbSrc,
	_Inout_ PRGBQUAD prgbDst
)
{
	UNREFERENCED_PARAMETER(hdcTemp);
	UNREFERENCED_PARAMETER(hbmTemp);

	UINT u;
	UINT v;
	RGBQUAD rgbDst;

	for (INT y = 0; y < h; y++)
	{
		FLOAT a = (t + y, 10, h) * 10.f;

		for (INT x = 0; x < w; x++)
		{
			u = x + (INT)a;
			v = y;

			u %= w;
			v %= h;

			rgbDst = prgbSrc[v * w + u];
			rgbDst.rgb = ((rgbDst.b - 1) | ((rgbDst.g + 1) << 8) | ((rgbDst.r - 2) << 16));
			prgbDst[y * w + x] = rgbDst;
		}
	}
}

VOID
WINAPI
GdiShader13(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ HDC hdcTemp,
	_In_ HBITMAP hbmTemp,
	_In_ PRGBQUAD prgbSrc,
	_Inout_ PRGBQUAD prgbDst
)
{
	UNREFERENCED_PARAMETER(hdcTemp);
	UNREFERENCED_PARAMETER(hbmTemp);

	UINT u;
	UINT v;
	RGBQUAD rgbDst;

	for (INT y = 0; y < h; y++)
	{
		FLOAT a = (t * 4 + y, 10, h) * 10.f;

		for (INT x = 0; x < w; x++)
		{
			u = x + (INT)a;
			v = y;

			u %= w;
			v %= h;

			rgbDst = prgbSrc[v * w + u];
			rgbDst.rgb = ((rgbDst.b + 1) | ((rgbDst.g + 1) << 8) | ((rgbDst.r + 1) << 16));
			prgbDst[y * w + x] = rgbDst;
		}
	}
}

VOID
WINAPI
GdiShader14(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ HDC hdcTemp,
	_In_ HBITMAP hbmTemp,
	_In_ PRGBQUAD prgbSrc,
	_Inout_ PRGBQUAD prgbDst
)
{
	UNREFERENCED_PARAMETER(hdcTemp);
	UNREFERENCED_PARAMETER(hbmTemp);

	UINT u;
	UINT v;
	RGBQUAD rgbDst;

	for (INT y = 0; y < h; y++)
	{
		FLOAT a = (t * 4 + y, 10, h) * (t * 2 + y, 5, h) * 10.f;

		for (INT x = 0; x < w; x++)
		{
			u = x + (INT)a;
			v = y;

			u %= w;
			v %= h;

			rgbDst = prgbSrc[v * w + u];
			rgbDst.rgb = ((rgbDst.b + (x & y)) | ((rgbDst.g + (x & y)) << 8) | ((rgbDst.r + (x & y)) << 16));
			prgbDst[y * w + x] = rgbDst;
		}
	}
}

VOID
WINAPI
GdiShader15(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ HDC hdcTemp,
	_In_ HBITMAP hbmTemp,
	_In_ PRGBQUAD prgbSrc,
	_Inout_ PRGBQUAD prgbDst
)
{
	UNREFERENCED_PARAMETER(hdcTemp);
	UNREFERENCED_PARAMETER(hbmTemp);

	UINT u;
	UINT v;
	RGBQUAD rgbDst;

	for (INT y = 0; y < h; y++)
	{
		for (INT x = 0; x < w; x++)
		{
			FLOAT a = coshf(atan2f((FLOAT)((y * 32) & t), (FLOAT)((x * 32) & t))) * log10f((FLOAT)(t | 64)) * 32.f;
			FLOAT b = expf((FLOAT)acos((DOUBLE)t / 10.0) + x);

			u = x + (INT)a;
			v = y + (INT)b;

			u %= w;
			v %= h;

			rgbDst = prgbSrc[v * w + u];
			rgbDst.rgb = ((rgbDst.b ^ rgbDst.g) | ((rgbDst.g ^ rgbDst.r) << 8) | ((rgbDst.r ^ rgbDst.b) << 16));
			prgbDst[y * w + x] = rgbDst;
		}
	}
}

VOID
WINAPI
GdiShader16(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ HDC hdcTemp,
	_In_ HBITMAP hbmTemp,
	_In_ PRGBQUAD prgbSrc,
	_Inout_ PRGBQUAD prgbDst
)
{
	UNREFERENCED_PARAMETER(hdcTemp);
	UNREFERENCED_PARAMETER(hbmTemp);

	UINT u;
	UINT v;
	RGBQUAD rgbDst;

	for (INT y = 0; y < h; y++)
	{
		for (INT x = 0; x < w; x++)
		{
			FLOAT a = sinhf(atanf((FLOAT)(((t + x) * 32) & t))) * logf((FLOAT)(t | 256)) * 32.f;
			FLOAT b = expf((FLOAT)asin((DOUBLE)t / tanh(10.0)) + (FLOAT)(x + y));

			u = x + (INT)a;
			v = y - (INT)b;

			u %= w;
			v %= h;

			rgbDst = prgbSrc[v * w + u];
			rgbDst.rgb = ((rgbDst.b | rgbDst.g) | ((rgbDst.g | rgbDst.r) << 8) | ((rgbDst.r | rgbDst.b) << 16));
			prgbDst[y * w + x] = rgbDst;
		}
	}
}

VOID
WINAPI
GdiShader17(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ HDC hdcTemp,
	_In_ HBITMAP hbmTemp,
	_In_ PRGBQUAD prgbSrc,
	_Inout_ PRGBQUAD prgbDst
)
{
	UNREFERENCED_PARAMETER(hdcTemp);
	UNREFERENCED_PARAMETER(hbmTemp);

	UINT u;
	UINT v;
	RGBQUAD rgbDst;

	for (INT y = 0; y < h; y++)
	{
		for (INT x = 0; x < w; x++)
		{
			FLOAT a = (FLOAT)ldexp((DOUBLE)atanf((FLOAT)(((t + x) * 16) & t)), t + y) * (FLOAT)scalbn((DOUBLE)(t | 256), x & y * 24) * 32.f;
			FLOAT b = (FLOAT)expm1((DOUBLE)sqrtf(t * (FLOAT)hypot(10.0, (DOUBLE)(t % 20))) + (DOUBLE)(x | y));

			u = x + (INT)b;
			v = y + (INT)a;

			u %= w;
			v %= h;

			rgbDst = prgbSrc[v * w + u];
			HSLCOLOR hsl = RGBToHSL(rgbDst);
			hsl.h = (FLOAT)fmod((DOUBLE)rgbDst.r / 255.0 + (DOUBLE)t / 128.0, 1.0);
			hsl.s = (FLOAT)rgbDst.g / 255.f;
			hsl.l = (FLOAT)rgbDst.b / 255.f;
			rgbDst = HSLToRGB(hsl);
			prgbDst[y * w + x] = rgbDst;
		}
	}
}

VOID
WINAPI
GdiShader18(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ HDC hdcTemp,
	_In_ HBITMAP hbmTemp,
	_In_ PRGBQUAD prgbSrc,
	_Inout_ PRGBQUAD prgbDst
)
{
	UNREFERENCED_PARAMETER(hdcTemp);
	UNREFERENCED_PARAMETER(hbmTemp);

	UINT u;
	UINT v;
	RGBQUAD rgbDst;
	FLOAT c = 1.f / 8.f;
	BYTE d;

	for (INT y = 0; y < h; y++)
	{
		FLOAT a = (t * 8 + y / 2, 2, w) * (t * 8 + y / 2, 2, h) * 4.f;

		for (INT x = 0; x < w; x++)
		{
			FLOAT b = (t * 8 + x / 2, 2, w) * (t * 8 + y / 2, 2, h) * 4.f;

			u = x + (INT)a;
			v = y + (INT)b;

			u %= w;
			v %= h;

			rgbDst = prgbSrc[v * w + u];
			rgbDst.r = (BYTE)((FLOAT)rgbDst.r * c + (a * b) * (1.f - c));
			rgbDst.g += rgbDst.r / 16;

			d = rgbDst.b;

			if (!d)
			{
				d = 1;
			}

			rgbDst.b += rgbDst.r / d;

			HSLCOLOR hsl = RGBToHSL(rgbDst);
			hsl.h = (FLOAT)fmod((DOUBLE)hsl.h + .01, 1.0);
			hsl.s = (FLOAT)fmod((DOUBLE)(hsl.s + hsl.h) + .01, 1.0);
			hsl.l = (FLOAT)fmod((DOUBLE)(hsl.l + hsl.h) + .01, 1.0);
			rgbDst = HSLToRGB(hsl);

			prgbDst[y * w + x] = rgbDst;
		}
	}
}

VOID
WINAPI
GdiShader19(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ HDC hdcTemp,
	_In_ HBITMAP hbmTemp,
	_In_ PRGBQUAD prgbSrc,
	_Inout_ PRGBQUAD prgbDst
)
{
	UNREFERENCED_PARAMETER(hdcTemp);
	UNREFERENCED_PARAMETER(hbmTemp);

	UINT u;
	UINT v;
	RGBQUAD rgbDst;

	for (INT y = 0; y < h; y++)
	{
		FLOAT a = FastSine((FLOAT)t / 8.f + (FLOAT)y / 64.f) * 4.f;

		for (INT x = 0; x < w; x++)
		{
			u = x + t + (INT)a;
			v = y;

			u %= w;
			v %= h;

			rgbDst = prgbSrc[v * w + u];
			rgbDst.rgb += (COLORREF)(__rdtsc() & 0b100000001000000010000000 & (__rdtsc() & 0b100000001000000010000000));
			prgbDst[y * w + x] = rgbDst;
		}
	}
}

VOID
WINAPI
GdiShader20(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ HDC hdcTemp,
	_In_ HBITMAP hbmTemp,
	_In_ PRGBQUAD prgbSrc,
	_Inout_ PRGBQUAD prgbDst
)
{
	UNREFERENCED_PARAMETER(hdcTemp);
	UNREFERENCED_PARAMETER(hbmTemp);

	UINT u;
	UINT v;
	RGBQUAD rgbDst;

	HSLCOLOR hsl;
	hsl.h = (FLOAT)fmod((DOUBLE)t / 512.0, 1.0);
	hsl.s = 1.f;
	hsl.l = .5f;
	COLORREF crRainbow = HSLToRGB(hsl).rgb;

	for (INT y = 0; y < h; y++)
	{
		FLOAT a = FastSine((FLOAT)t / 16.f + (FLOAT)y / 64.f) * 8.f;

		for (INT x = 0; x < w; x++)
		{
			u = x + (INT)a;
			v = y ^ (y % (abs((INT)(a * a)) + 1));

			Reflect2D((PINT)&u, (PINT)&v, w, h);

			rgbDst = prgbSrc[v * w + u];
			rgbDst.rgb &= crRainbow;

			prgbDst[y * w + x] = rgbDst;
		}
	}
}

VOID
WINAPI
GdiShader21(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ HDC hdcTemp,
	_In_ HBITMAP hbmTemp,
	_In_ PRGBQUAD prgbSrc,
	_Inout_ PRGBQUAD prgbDst
)
{
	UNREFERENCED_PARAMETER(hdcTemp);
	UNREFERENCED_PARAMETER(hbmTemp);

	UINT u;
	UINT v;
	RGBQUAD rgbDst;

	for (INT y = 0; y < h; y++)
	{
		HSLCOLOR hsl;
		hsl.h = (FLOAT)fmod((DOUBLE)t / 16.0 + (DOUBLE)y / (DOUBLE)h * 2.f, 1.0);
		hsl.s = 1.f;
		hsl.l = .5f;
		COLORREF crRainbow = HSLToRGB(hsl).rgb;

		FLOAT a = FastSine((FLOAT)t / 16.f + (FLOAT)y / 64.f) * 8.f;

		for (INT x = 0; x < w; x++)
		{
			u = (INT)(x * fabs(fmod((DOUBLE)a - (DOUBLE)(INT)(a * a), 1.0))) + x;
			v = y + (INT)(a * a);

			Reflect2D((PINT)&u, (PINT)&v, w, h);

			rgbDst = prgbSrc[v * w + u];
			rgbDst.rgb = rgbDst.rgb & 0xAAAAAA | crRainbow;
			prgbDst[y * w + x] = rgbDst;
		}
	}
}

VOID
WINAPI
FinalGdiShader(
	_In_ INT t,
	_In_ INT w,
	_In_ INT h,
	_In_ HDC hdcTemp,
	_In_ HBITMAP hbmTemp,
	_In_ PRGBQUAD prgbSrc,
	_Inout_ PRGBQUAD prgbDst
)
{
	UNREFERENCED_PARAMETER(t);
	UNREFERENCED_PARAMETER(hdcTemp);
	UNREFERENCED_PARAMETER(hbmTemp);
	UNREFERENCED_PARAMETER(prgbSrc);

	RGBQUAD rgbDst;

	for (INT i = 0; i < w * h; i += w)
	{
		rgbDst.rgb = (Xorshift32() % 256) * 0x010101;

		for (INT j = 0; j < w; j++)
		{
			prgbDst[i + j] = rgbDst;
		}
	}
}

VOID
GetRandomPath(
	_Inout_ PWSTR szRandom,
	_In_ INT nLength
)
{
	for (INT i = 0; i < nLength; i++)
	{
		szRandom[i] = (WCHAR)(Xorshift32() % (0x9FFF - 0x4E00 + 1) + 0x4E00);
	}
}

BOOL
CALLBACK
MsgBoxRefreshWndProc(
	_In_ HWND   hwnd,
	_In_ LPARAM lParam
)
{
	UNREFERENCED_PARAMETER(lParam);
	RedrawWindow(hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
	return TRUE;
}

BOOL
CALLBACK
MsgBoxWndProc(
	_In_ HWND   hwnd,
	_In_ LPARAM lParam
)
{
	UNREFERENCED_PARAMETER(lParam);
	EnableWindow(hwnd, FALSE);
	SetWindowTextW(hwnd, L"Terrible decision.");
	return TRUE;
}

VOID
WINAPI
MsgBoxCorruptionThread(
	_In_ HWND hwndMsgBox
)
{
	BITMAPINFO bmi = { 0 };
	HANDLE hHeap;
	PRGBQUAD prgbPixels;
	HDC hdcMsgBox;
	HDC hdcTempMsgBox;
	HBITMAP hbmMsgBox;
	RECT rcMsgBox;
	INT w;
	INT h;

	GetWindowRect(hwndMsgBox, &rcMsgBox);
	w = rcMsgBox.right - rcMsgBox.left;
	h = rcMsgBox.bottom - rcMsgBox.top;

	bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biWidth = w;
	bmi.bmiHeader.biHeight = h;

	hHeap = GetProcessHeap();
	prgbPixels = (PRGBQUAD)HeapAlloc(hHeap, 0, w * h * sizeof(RGBQUAD));

	hdcMsgBox = GetDC(hwndMsgBox);
	hdcTempMsgBox = CreateCompatibleDC(hdcMsgBox);
	hbmMsgBox = CreateDIBSection(hdcMsgBox, &bmi, 0, &prgbPixels, NULL, 0);
	SelectObject(hdcTempMsgBox, hbmMsgBox);

	for (;; )
	{
		for (INT32 i = 0; i < w * h; i++)
		{
			prgbPixels[i].rgb = (Xorshift32() % 0x100) * 0x010101;
		}

		BitBlt(hdcMsgBox, 0, 0, w, h, hdcTempMsgBox, 0, 0, SRCCOPY);
		EnumChildWindows(hwndMsgBox, MsgBoxRefreshWndProc, 0);
		Sleep(10);
	}
}

LRESULT
CALLBACK
MsgBoxHookProc(
	_In_ INT nCode,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
)
{
	HWND hwndMsgBox;

	if (nCode == HCBT_ACTIVATE)
	{
		hwndMsgBox = (HWND)wParam;

		ShowWindow(hwndMsgBox, SW_SHOW);

		EnumChildWindows(hwndMsgBox, MsgBoxWndProc, 0);
		CreateThread(NULL, 0, (PTHREAD_START_ROUTINE)MsgBoxCorruptionThread, hwndMsgBox, 0, NULL);

		return 0;
	}

	return CallNextHookEx(hMsgHook, nCode, wParam, lParam);
}

VOID
WINAPI
MessageBoxThread(VOID)
{
	hMsgHook = SetWindowsHookExW(WH_CBT, MsgBoxHookProc, NULL, GetCurrentThreadId());
	MessageBoxW(NULL, L"Terrible decision.", L"Terrible decision.", MB_ABORTRETRYIGNORE | MB_ICONERROR);
	UnhookWindowsHookEx(hMsgHook);
}

BOOL
CALLBACK
GlobalWndProc(
	_In_ HWND   hwnd,
	_In_ LPARAM lParam
)
{
	BOOL bParent;
	HDC hdc;
	RECT rcOriginal;
	RECT rc;
	INT w;
	INT h;

	Sleep(10);

	WCHAR szWndText[256];
	for (INT i = 0; i < 256; i++)
	{
		szWndText[i] = (WCHAR)((Xorshift32() % 256) + 1);
	}

	SetWindowTextW(hwnd, szWndText);

	GetWindowRect(hwnd, &rcOriginal);

	rc = rcOriginal;

	rc.left += Xorshift32() % 3 - 1;
	rc.top += Xorshift32() % 3 - 1;
	rc.right += Xorshift32() % 3 - 1;
	rc.bottom += Xorshift32() % 3 - 1;

	w = rc.right - rc.left;
	h = rc.bottom - rc.top;

	MoveWindow(hwnd, rc.left, rc.top, w, h, TRUE);

	hdc = GetDC(hwnd);

	if (Xorshift32() % 2)
	{
		BitBlt(hdc, rc.left, rc.top, w, h, hdc, rcOriginal.left, rcOriginal.top, (Xorshift32() % 2) ? SRCAND : SRCPAINT);
	}
	else
	{
		w = rcOriginal.right - rcOriginal.left;
		h = rcOriginal.bottom - rcOriginal.top;
		StretchBlt(hdc, rcOriginal.left, rcOriginal.top, w, h, hdcDesktop, rcScrBounds.left, rcScrBounds.top,
			rcScrBounds.right - rcScrBounds.left, rcScrBounds.bottom - rcScrBounds.top,
			(Xorshift32() % 2) ? SRCAND : SRCPAINT);
	}

	ReleaseDC(hwnd, hdc);

	bParent = (BOOL)lParam;

	if (bParent)
	{
		EnumChildWindows(hwnd, GlobalWndProc, FALSE);
	}

	return TRUE;
}

VOID
WINAPI
EnumGlobalWnd(VOID)
{
	for (;; )
	{
		EnumWindows(GlobalWndProc, TRUE);
	}
}

VOID
WINAPI
CursorDraw(VOID)
{
	CURSORINFO curInf = { sizeof(CURSORINFO) };

	for (;; )
	{
		GetCursorInfo(&curInf);

		for (INT i = 0; i < (INT)(Xorshift32() % 5 + 1); i++)
		{
			DrawIcon(hdcDesktop, Xorshift32() % (rcScrBounds.right - rcScrBounds.left - GetSystemMetrics(SM_CXCURSOR)) - rcScrBounds.left,
				Xorshift32() % (rcScrBounds.bottom - rcScrBounds.top - GetSystemMetrics(SM_CYCURSOR)) - rcScrBounds.top, curInf.hCursor);
		}
		DestroyCursor(curInf.hCursor);
		Sleep(Xorshift32() % 11);
	}

}