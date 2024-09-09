// BitmapLoader.c : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "framework.h"
#include "BitmapLoader.h"
#include "Bitmap.h"
#include <stdio.h>
#include <math.h>

#pragma warning(disable: 4996)

#define MAX_LOADSTRING 100
#define MAX_ZOOM_LEVEL 8

#define USE_BILINEAR_INTERPOLATION true

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

bitmap bmp;
HBITMAP hBitmap = NULL;
HBITMAP scaledBitmap[8] = { 0 };
double scaleFactors[8] = { 0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0, 2.5 };
int zoomLevel = 0;

HBITMAP Create24BitHBITMAP(HDC hdc, int width, int height, uint8_t* data)
{
	BITMAPINFOHEADER h = { 0 };
	h.biSize = sizeof(BITMAPINFOHEADER);
	h.biWidth = width;
	h.biHeight = height;
	h.biPlanes = 1;
	h.biBitCount = 24;
	h.biCompression = BI_RGB;

	HBITMAP hBmp = CreateDIBitmap(hdc, &h, CBM_INIT, data, (BITMAPINFO*)&h, DIB_RGB_COLORS);
	return hBmp;
}

// 아래 조사한 것으로 구현한 것과 다른점은 스케일값을 입력 받아 새로운 비트맵을 생성하는 것.
HBITMAP StretchBitmap(HDC hdc, bitmap* bmp, float scale)
{
	if (bmp->pixel_data == NULL) return NULL;

	int originalWidth = bmp->header.width;
	int originalHeight = bmp->header.height;
	uint8_t* src = bmp->pixel_data;

	// 스케일 값을 곱하는 것은 버퍼 오버런을 발생시킴. 스케일값이 1 초과시 원본 데이터의 범위를 넘어감.
	int scaledWidth = (int)(originalWidth * scale);
	int scaledHeight = (int)(originalHeight * scale);

	int newBitmapSize = scaledWidth * scaledHeight * 3;
	uint8_t* newPixelData = (uint8_t*)malloc(newBitmapSize); // BI_RGB
	if (newPixelData == NULL)
	{
		return NULL;
	}
	memset(newPixelData, 0, newBitmapSize);

	// 검은 결과만 출력
	for (int y = 0; y < scaledHeight; ++y)
	{
		for (int x = 0; x < scaledWidth; ++x)
		{
			// 새로 스케일된 픽셀 값은 원본 픽셀로부터 가져와야 함
			// 확대: 기존 픽셀이 여러 픽셀로 쪼개지는 듯 보임 
			// 축소: 기존 여러 픽셀이 하나의 픽셀로 합쳐지는 듯 보임

			// 아래 수정 바람.
			int scaledX = (int)(x * scale);
			int scaledY = (int)(y * scale);

			int srcIdx = (scaledY * originalWidth + scaledX) * 3;
			int scaledIdx = (y * scaledWidth + x) * 3;

			newPixelData[scaledIdx] = src[srcIdx];
			newPixelData[scaledIdx + 1] = src[srcIdx + 1];
			newPixelData[scaledIdx + 2] = src[srcIdx + 2];
		}
	}

	BITMAPINFOHEADER h = { 0 };
	h.biSize = sizeof(BITMAPINFOHEADER);
	h.biWidth = scaledWidth;
	h.biHeight = scaledHeight;
	h.biPlanes = 1;
	h.biBitCount = 24;
	h.biCompression = BI_RGB;

	HBITMAP hBmp = CreateDIBitmap(hdc, &h, CBM_INIT, newPixelData, (BITMAPINFO*)&h, DIB_RGB_COLORS);
	return hBmp;
}

// 보간
HBITMAP ResizeBitmap(HDC hdc, bitmap* bmp, int newWidth, int newHeight)
{
	if (bmp->pixel_data == NULL) return NULL;

	int originalWidth = bmp->header.width;
	int originalHeight = bmp->header.height;
	uint8_t* src = bmp->pixel_data;

	float xRatio = (float)originalWidth / (float)newWidth;
	float yRatio = (float)originalHeight / (float)newHeight;

	uint8_t* newPixelData = (uint8_t*)malloc(newWidth * newHeight * 3); // BI_RGB
	if (newPixelData == NULL)
	{
		return NULL;
	}
	memset(newPixelData, 0, newWidth * newHeight * 3);

#if 1
	// 가장 가까운 이웃값 사용
	for (int y = 0; y < newHeight; ++y)
	{
		for (int x = 0; x < newWidth; ++x)
		{
			// 변경할 크기 비율에 따라 근사한 좌표를 구함
			int nearestX = (int)(x * xRatio);
			int nearestY = (int)(y * yRatio);

			// 원본 인덱스
			int originalIndex = (nearestY * originalWidth + nearestX) * 3;

			// 새로운 인덱스
			int newIndex = (y * newWidth + x) * 3;

			newPixelData[newIndex] = src[originalIndex];
			newPixelData[newIndex + 1] = src[originalIndex + 1];
			newPixelData[newIndex + 2] = src[originalIndex + 2];
		}
	}
#else
	// 이진선형 보간법 사용 - 오류 있으니 수정 바람
	for (int y = 0; y < newHeight; ++y)
	{
		for (int x = 0; x < newWidth; ++x)
		{
			float gx = x * xRatio;
			float gy = y * yRatio;
			int gxi = (int)gx;
			int gyi = (int)gy;

			// 인접한 네 개의 픽셀 인덱스 계산
			int i00 = (gyi * originalWidth + gxi) * 3;
			int i10 = (gyi * originalWidth + (gxi + 1)) * 3;
			int i01 = ((gyi + 1) * originalWidth + gxi) * 3;
			int i11 = ((gyi + 1) * originalWidth + (gxi + 1)) * 3;

			// 비율 계산
			float xDiff = gx - gxi;
			float yDiff = gy - gyi;

			for (int i = 0; i < 3; ++i)
			{
				// 이진 선형 보간 공식 적용
				float value = (src[i00 + i] * (i - xDiff) * (1 - yDiff)) +
											(src[i10 + i] * xDiff * (1 - yDiff)) +
											(src[i01 + i] * (1 - xDiff) * yDiff) +
											(src[i11 + i] * xDiff * yDiff);
				newPixelData[(y * newWidth + x) * 3 + i] = (uint8_t)value;
			}
		}
	}
#endif
	BITMAPINFOHEADER h = { 0 };
	h.biSize = sizeof(BITMAPINFOHEADER);
	h.biWidth = newWidth;
	h.biHeight = newHeight;
	h.biPlanes = 1;
	h.biBitCount = 24;
	h.biCompression = BI_RGB;

	HBITMAP hBmp = CreateDIBitmap(hdc, &h, CBM_INIT, newPixelData, (BITMAPINFO*)&h, DIB_RGB_COLORS);
	return hBmp;
}

void ReadBitmap(HWND hWnd, const char* path)
{
	FILE* fp = fopen(path, "rb");
	if (fp == NULL) return 1;

	fread(&bmp.header_info, 1, sizeof(bitmap_header_info), fp);

	fread(&bmp.header, 1, sizeof(bitmap_v5_info_header), fp);

	int stride = ((bmp.header.width * 3 + 3) & ~3);

	int totalSize = bmp.header.height * stride;

	// 파일 포인터 픽셀 데이터 위치로 이동
	fseek(fp, bmp.header_info.pixel_start_offset, SEEK_SET);
	bmp.pixel_data = (uint8_t*)malloc(totalSize);
	if (bmp.pixel_data == NULL) {
		fclose(fp);
		return;
	}
	ZeroMemory(bmp.pixel_data, totalSize);
	fread(bmp.pixel_data, 1, totalSize, fp);

	fclose(fp);

	HDC hdc = GetDC(hWnd);
	hBitmap = Create24BitHBITMAP(hdc, bmp.header.width, bmp.header.height, bmp.pixel_data);
	ReleaseDC(hWnd, hdc);
}

void DrawBitmap(HDC hdc, int width, int height, HBITMAP hBmp)
{
	HDC hdcMem = CreateCompatibleDC(hdc);
	HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hBmp);
	BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);
	SelectObject(hdcMem, hbmOld);
	DeleteDC(hdcMem);
}

void ZoomInOut(HDC hdc)
{
	double scale = scaleFactors[zoomLevel];

	int newWidth = (int)(bmp.header.width * scale);
	int newHeight = (int)(bmp.header.height * scale);

	HBITMAP bmpWillBeDrew = scaledBitmap[zoomLevel];
#if 1
	
	if (bmpWillBeDrew) {
		// 이미 존재하면 할당 해제, 여전히 메모리 계속 늘어남 수정 바람.
		DeleteObject(bmpWillBeDrew);
	}

	//bmpWillBeDrew = ResizeBitmap(hdc, &bmp, newWidth, newHeight);
	bmpWillBeDrew = StretchBitmap(hdc, &bmp, (float)scale);
	DrawBitmap(hdc, newWidth, newHeight, bmpWillBeDrew);
#else
	BITMAPINFOHEADER h = { 0 };
	h.biSize = sizeof(BITMAPINFOHEADER);
	h.biWidth = bmp.header.width;
	h.biHeight = bmp.header.height;
	h.biPlanes = 1;
	h.biBitCount = 24;
	h.biCompression = BI_RGB;

	StretchDIBits(hdc,
		0, 0, newWidth, newHeight,
		0, 0, bmp.header.width, bmp.header.height,
		bmp.pixel_data, (BITMAPINFO*)&h, DIB_RGB_COLORS, SRCCOPY);
#endif
}

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void								LoadBitmapWindow(HWND hWnd);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: 여기에 코드를 입력합니다.

	// 전역 문자열을 초기화합니다.
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_BITMAPLOADER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_BITMAPLOADER));

	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BITMAPLOADER));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_BITMAPLOADER);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance;

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// 메뉴 선택을 구문 분석합니다:
		switch (wmId)
		{
		case ID_FILE_LOADBITMAPFILE:
			LoadBitmapWindow(hWnd);
			break;
		case IDM_ABOUT:
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: 여기에 hdc를 사용하는 그리기 코드를 추가합니다...
#if 0
		if (bmp != NULL && bmp->pixel_data != NULL)
		{
			uint32_t w = bmp->dib_header.width;
			uint32_t h = bmp->dib_header.height;
			uint8_t** pixel = bmp->pixel_data;
			uint32_t bpp = (bmp->dib_header.bits_per_pixel / 8);

			for (int y = 0; y < h; ++y)
			{
				uint8_t* row = pixel[y];
				for (int x = 0; x < w; ++x)
				{
					uint8_t r = row[x * bpp + 0];
					uint8_t g = row[x * bpp + 1];
					uint8_t b = row[x * bpp + 2];

					// RGB -> BGR
					COLORREF color = RGB(b, g, r);

					SetPixel(hdc, x, y, color);
				}
			}
		}
#else
		//DrawBitmap(hdc, bmp.header.width, bmp.header.height, hBitmap);
		ZoomInOut(hdc);
#endif

		EndPaint(hWnd, &ps);
	}
	break;
	case WM_MOUSEWHEEL:
		if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
		{
			if (zoomLevel < 7) zoomLevel++;
		}
		else
		{
			if (zoomLevel > 0) zoomLevel--;
		}
		InvalidateRect(hWnd, NULL, TRUE);
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void LoadBitmapWindow(HWND hWnd)
{
	OPENFILENAME ofn;
	WCHAR szFile[260] = { 0 };

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = L"Bitmap Files\0*.bmp\0All Files\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn) == TRUE)
	{
#if 0
		HANDLE hFile = CreateFileW(szFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			DWORD error = GetLastError();
			if (error == ERROR_ACCESS_DENIED)
			{
				MessageBox(hWnd, L"Access denied to file: %s, Error: %lu \n", hFile, error, L"Error", MB_OK);
			}
			else
			{
				MessageBox(hWnd, L"Failed to open file: %s, Error: %lu \n", szFile, error, L"Error", MB_OK);
			}

			CloseHandle(hFile);

			return NULL;
		}
		else
		{
			//MessageBox(hWnd, L"Read access to file: %s \n", szFile, L"Error", MB_OK);
			if (ReadBitmap(hFile, bmp) != 0)
			{
				MessageBox(hWnd, L"No read access to file: %s \n", szFile, L"Error", MB_OK);
			}
			else
			{
				// 창 내부 사각영역을 무효화시키는 명령어, 
				// 만약 사각형이 없으면 전부 무효화하는데 이는 WndProc의 메시지 루프에 WM_PAINT 메시지가 전달되도록 함.
				InvalidateRect(hWnd, NULL, TRUE);
			}
		}
#else
		char path[256];
		int nLen = (int)wcslen(ofn.lpstrFile);
		wcstombs(path, ofn.lpstrFile, nLen + 1);

		ReadBitmap(hWnd, path);
		InvalidateRect(hWnd, NULL, TRUE);
#endif
	}
}
