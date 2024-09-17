// BitmapLoader.c : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "framework.h"
#include "BitmapLoader.h"
#include "Bitmap.h"
#include <stdio.h>
#include <math.h>

#pragma warning(disable: 4996)

#define MAX_LOADSTRING 100
#define MAX_ZOOM_LEVEL 16

#define USE_BILINEAR_INTERPOLATION true

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

// 비트맵 관련 함수 선언:
void								LoadBitmapWindow(HWND hWnd);
HBITMAP							Create24BitHBITMAP(HDC hdc, int width, int height, uint8_t* data);
void								DrawResizedBitmap(HDC hdc, double scale);
void								ClearCachedScaledBitmaps(HDC hdc);
void								ReadBitmap(HWND hWnd, const char* path);
void								DrawBitmap(HDC hdc, int width, int height, HBITMAP hBmp);
void								MosaicBitmap(HWND hWnd);

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

bitmap bmp;
HBITMAP hBitmap = NULL;

HBITMAP zoomedBitmaps[MAX_ZOOM_LEVEL] = { NULL };
int currentZoomLevel = (int)(MAX_ZOOM_LEVEL / 2);
double zoomFactors[MAX_ZOOM_LEVEL] = { 0.01, 0.05, 0.1, 0.2, 0.25, 0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0, 3.0, 4.0, 6.0, 8.0 };

HBITMAP hMosaicBitmap = NULL;
BOOL isMosaic = FALSE;

HBITMAP Create24BitHBITMAP(HDC hdc, int width, int height, uint8_t* data)
{
	BITMAPINFOHEADER h = { 0 };
	h.biSize = sizeof(BITMAPINFOHEADER);
	h.biWidth = width;
	h.biHeight = height;
	h.biPlanes = 1;
	h.biBitCount = bmp.header.bits_per_pixel;
	h.biCompression = BI_RGB;

	HBITMAP hBmp = CreateDIBitmap(hdc, &h, CBM_INIT, data, (BITMAPINFO*)&h, DIB_RGB_COLORS);
	return hBmp;
}

uint8_t GetColor(int w, int x, int y, int colorPosition) {
	return bmp.pixel_data[w * y + (x * 3) + colorPosition];
}

void SetColor(int w, int x, int y, int colorPosition, int color, uint8_t* dst) {
	dst[w * y + (x * 3) + colorPosition] = color;
}

void DrawResizedBitmap(HDC hdc, double scale)
{
	if (bmp.pixel_data == NULL) return;

	int width = bmp.header.width;
	int height = bmp.header.height;
	uint8_t* src = bmp.pixel_data;

	int newWidth = (int)(width * scale);
	int newHeight = (int)(height * scale);

	HBITMAP hCachedScaledBitmap = zoomedBitmaps[currentZoomLevel];

	if (zoomedBitmaps[currentZoomLevel] != NULL) {
		DrawBitmap(hdc, newWidth, newHeight, hCachedScaledBitmap);
	}

	float xRatio = (float)width / (float)newWidth;
	float yRatio = (float)height / (float)newHeight;

	// 4의 배수를 맞춰주기 위한 변수 설정
	int oldStride = ((bmp.header.width * 3 + 3) & ~3);
	int newStride = ((newWidth * 3 + 3) & ~3);

	int bbp = bmp.header.bits_per_pixel / 8;
	const int totalSize = newStride * newHeight * bbp;

	uint8_t* newPixelData = (uint8_t*)malloc(totalSize); // BI_RGB
	if (newPixelData == NULL) return;
	memset(newPixelData, 0, totalSize);

#if 1
	// 가장 가까운 이웃값 사용
	for (int y = 0; y < newHeight; ++y) {
		for (int x = 0; x < newWidth; ++x) {
			// 변경할 크기 비율에 따라 근사한 좌표를 구함
			int nearestX = (int)(x * xRatio);
			int nearestY = (int)(y * yRatio);

			uint8_t b = GetColor(oldStride, nearestX, nearestY, 0);
			uint8_t g = GetColor(oldStride, nearestX, nearestY, 1);
			uint8_t r = GetColor(oldStride, nearestX, nearestY, 2);

			SetColor(newStride, x, y, 0, b, newPixelData);
			SetColor(newStride, x, y, 1, g, newPixelData);
			SetColor(newStride, x, y, 2, r, newPixelData);
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
			int i00 = (gyi * width + gxi) * 3;
			int i10 = (gyi * width + (gxi + 1)) * 3;
			int i01 = ((gyi + 1) * width + gxi) * 3;
			int i11 = ((gyi + 1) * width + (gxi + 1)) * 3;

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
	h.biBitCount = bmp.header.bits_per_pixel;
	h.biCompression = BI_RGB;

	HBITMAP hScaledBitmap = CreateDIBitmap(hdc, &h, CBM_INIT, newPixelData, (BITMAPINFO*)&h, DIB_RGB_COLORS);
	DrawBitmap(hdc, newWidth, newHeight, hScaledBitmap);
	zoomedBitmaps[currentZoomLevel] = hScaledBitmap;
}

// 캐시된 스케일된 비트맵 삭제
void ClearCachedScaledBitmaps(HDC hdc) {
	HDC hdcMem = CreateCompatibleDC(hdc);
	
	for (int i = 0; i < MAX_ZOOM_LEVEL; ++i) {
		HBITMAP hCachedBitmap = zoomedBitmaps[i];
		if (hCachedBitmap != NULL) {
			HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hCachedBitmap);
			DeleteObject(hbmOld);
			zoomedBitmaps[i] = NULL;
		}			
	}
	DeleteDC(hdcMem);
}

void ReadBitmap(HWND hWnd, const char* path)
{
	FILE* fp = fopen(path, "rb");
	if (fp == NULL) return;

	fread(&bmp.header_info, 1, sizeof(bitmap_header_info), fp);

	fread(&bmp.header, 1, sizeof(bitmap_v5_info_header), fp);

	int stride = ((bmp.header.width * 3 + 3) & ~3);
	int totalSize = stride * bmp.header.height;

	// 파일 포인터 픽셀 데이터 위치로 이동
	fseek(fp, bmp.header_info.pixel_start_offset, SEEK_SET);
	bmp.pixel_data = (uint8_t*)malloc(totalSize);
	if (bmp.pixel_data == NULL) {
		fclose(fp);
		return;
	}
	memset(bmp.pixel_data, 0, totalSize);
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

// Windows GDI API 사용
void DrawScaledBitmap(HDC hdc)
{
	double scale = zoomFactors[currentZoomLevel];
	int newWidth = (int)(bmp.header.width * scale);
	int newHeight = (int)(bmp.header.height * scale);

	BITMAPINFOHEADER h = { 0 };
	h.biSize = sizeof(BITMAPINFOHEADER);
	h.biWidth = bmp.header.width;
	h.biHeight = bmp.header.height;
	h.biPlanes = 1;
	h.biBitCount = bmp.header.bits_per_pixel;
	h.biCompression = BI_RGB;

	StretchDIBits(hdc,
		0, 0, newWidth, newHeight,
		0, 0, bmp.header.width, bmp.header.height,
		bmp.pixel_data, (BITMAPINFO*)&h, DIB_RGB_COLORS, SRCCOPY);
}

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
		{
			HDC hdc = GetDC(hWnd);
			ClearCachedScaledBitmaps(hdc);
			ReleaseDC(hWnd, hdc);

			LoadBitmapWindow(hWnd);
		}
		break;
		case ID_FILE_MOSAIC:
			MosaicBitmap(hWnd);
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
		//if (isMosaic) {
		//	DrawBitmap(hdc, bmp.header.width, bmp.header.height, hMosaicBitmap);
		//}
		//else {
		//	DrawBitmap(hdc, bmp.header.width, bmp.header.height, hBitmap);
		//}

		//DrawBitmap(hdc, bmp.header.width, bmp.header.height, hBitmap);

		double scale = zoomFactors[currentZoomLevel];
		DrawResizedBitmap(hdc, scale);

		EndPaint(hWnd, &ps);
	}
	break;
	case WM_MOUSEWHEEL:
		if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
		{
			// Zoom in
			if (currentZoomLevel < (MAX_ZOOM_LEVEL - 1)) currentZoomLevel++;
		}
		else
		{
			// Zoom out
			if (currentZoomLevel > 0) currentZoomLevel--;
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

// 모자이크 레벨이 높을수록 선명도가 낮아지는 것. 최대 5단계까지
void MosaicBitmap(HWND hWnd)
{
	if (bmp.pixel_data == NULL) return;

	// 토글 모자이크 변수
	if (isMosaic) {
		isMosaic = FALSE;
	}
	else {
		isMosaic = TRUE;
	}

	//if (mosaicLevel <= 0) {
	//	mosaicLevel = 1;
	//}
	//else if (mosaicLevel >= 5) {
	//	mosaicLevel = 5;
	//}

	const uint8_t* src = bmp.pixel_data;
	const int width = bmp.header.width;
	const int height = bmp.header.height;
	const int bmpSize = width * height * 3;

	uint8_t* newPixelData = (uint8_t*)malloc(bmpSize);
	if (newPixelData == NULL) return;
	memset(newPixelData, 0, bmpSize);

#if 0
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			// 가까운 픽셀 값을 계산해서 곱하기 적용
			// 상 - 하 - 좌 - 우 픽셀 값 구하기
			int index = (y * width + x) * 3;

			// 처음 시도한 것: 오히려 이상한 노이즈처럼 보임
			// [up down left right] [x y] 
			// 최소/최대 범위에 벗어나지 않는 인덱스 값 구하기 - 벗어나는 경우 자신의 픽셀 값으로 적용
			int indice[4][2] = { {0} };
			if (x <= 0) {
				indice[0][0] = 0;
				indice[1][0] = 0;
				indice[2][0] = 0;
				indice[3][0] = 0;
			}
			else if (x == (width - 1)) {
				indice[0][0] = width - 1;
				indice[1][0] = width - 1;
				indice[2][0] = width - 1;
				indice[3][0] = width - 1;
			}
			else {
				indice[0][0] = x;
				indice[1][0] = x;
				indice[2][0] = x - 1;
				indice[3][0] = x + 1;
			}

			if (y == 0) {
				indice[0][1] = 0;
				indice[1][1] = 0;
				indice[2][1] = 0;
				indice[3][1] = 0;
			}
			else if (y == (height - 1)) {
				indice[0][1] = height - 1;
				indice[1][1] = height - 1;
				indice[2][1] = height - 1;
				indice[3][1] = height - 1;
			}
			else {
				indice[0][1] = y - 1;
				indice[1][1] = y + 1;
				indice[2][1] = y;
				indice[3][1] = y;
			}

			int up = (indice[0][1] * width + indice[0][0]) * 3;
			int down = (indice[1][1] * width + indice[1][0]) * 3;
			int left = (indice[2][1] * width + indice[2][0]) * 3;
			int right = (indice[3][1] * width + indice[3][0]) * 3;

			newPixelData[index + 0] = (uint8_t)(src[up + 0] * src[down + 0] * src[left + 0] * src[right + 0]);
			newPixelData[index + 1] = (uint8_t)(src[up + 1] * src[down + 1] * src[left + 1] * src[right + 1]);
			newPixelData[index + 2] = (uint8_t)(src[up + 2] * src[down + 2] * src[left + 2] * src[right + 2]);
		}
	}
#elif 1
	// 모자이크에 대한 이해가 필요..
	// 두번째 시도: 
	const int n = 2;
	int* indice = (int*)malloc(n * n);
	for (int y = 0; y < height; y += n) {
		for (int x = 0; x < width; x += n) {
			// 본인 인덱스와 그 주변 (n-1) 영역 만큼 영향을 줌.
			// 가까운 픽셀 값을 계산해서 곱하기 적용
			// 상 - 하 - 좌 - 우 픽셀 값 구하기
			int index = (y * width + x) * 3;

			// 인덱스가 배열을 벗어났다면 나머지 처리

			// 영향을 받는 픽셀 인덱스 구하기

		}
	}
	free(indice);
#elif 2

#endif

	HDC hdc = GetDC(hWnd);
	hMosaicBitmap = Create24BitHBITMAP(hdc, width, height, newPixelData);
	ReleaseDC(hWnd, hdc);

	InvalidateRect(hWnd, NULL, TRUE);

	return;
}
