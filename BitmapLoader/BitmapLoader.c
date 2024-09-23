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
#define TIMER_ID			 1

#define USE_BILINEAR_INTERPOLATION true

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

// 비트맵 관련 함수 선언:
void								OpenBitmapSelectWindow(HWND hWnd);
void								OpenBitmapMovieSelectWindow(HWND hWnd);
HBITMAP							Create24BitHBITMAP(HDC hdc, int width, int height, uint8_t* data);
void								DrawResizedBitmap(HDC hdc, double scale);
void								ClearCachedScaledBitmaps(HDC hdc);
void								ReadBitmap(HWND hWnd, const char* path);
void								DrawBitmap(HDC hdc, int width, int height, HBITMAP hBmp);
void								MosaicBitmap(HWND hWnd);
void								DrawBitmapMovie(HDC hdc);

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

bitmap bmp = {0};
HBITMAP hBitmap = NULL;

movie mv = {0};
FILE* mv_fp = NULL;
int current_frame_id = 0;

HBITMAP zoomedBitmaps[MAX_ZOOM_LEVEL] = { NULL };
int currentZoomLevel = (int)(MAX_ZOOM_LEVEL / 2);
double zoomFactors[MAX_ZOOM_LEVEL] = { 0.01, 0.05, 0.1, 0.2, 0.25, 0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0, 3.0, 4.0, 6.0, 8.0 };

HBITMAP hMosaicBitmap = NULL;
BOOL isMosaic = FALSE;

DWORD mvStartTime = 0;
#define MAX_PRELOAD 128;
int num_preload = MAX_PRELOAD;

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

// 스트리밍으로 바꾸기
void ReadBmpMovieWithRange(HWND hWnd, int from, int to) {
	if (from > to) { int temp = from; from = to; to = temp; }

	if (abs(to - from) > num_preload) {
		to = from + num_preload;
	}

	HDC hdc = GetDC(hWnd);
	for (int idx = from; idx < to; ++idx) {
		if (idx >= mv.header.total_frame_count) { break; }

		frame* fr = &mv.frames[idx];
		fseek(mv_fp, fr->header.pixel_data_offset, SEEK_SET);
		//fread(&fr->header, sizeof(frame_header), 1, mv_fp);

		int stride = ((fr->header.width * 3 + 3) & ~3);
		int size = stride * fr->header.height;

		// 프레임 픽셀 데이터 할당
		fr->pixel_data = (uint8_t*)malloc(size);
		if (fr->pixel_data == NULL) { break; }
		memset(fr->pixel_data, 0, size);

		fread(fr->pixel_data, size, 1, mv_fp);

		// 로드한 픽셀로 비트맵 생성
		fr->bmp = Create24BitHBITMAP(hdc, fr->header.width, fr->header.height, fr->pixel_data);
	}
	ReleaseDC(hWnd, hdc);
}

void LoadBitmapMovie(HWND hWnd, const char* path) {
	num_preload = MAX_PRELOAD;

	mv_fp = fopen(path, "rb");
	if (mv_fp == NULL) { return; }

	// 저장된 데이터는 끝부분에 헤더가 위치해 있으므로, 먼저 끝부분을 읽기
	const long header_start_offset = (-1) * sizeof(movie_header);
	fseek(mv_fp, header_start_offset, SEEK_END);
	fread(&mv.header, sizeof(movie_header), 1, mv_fp);
	fseek(mv_fp, 0, SEEK_SET);

	mv.frames = (frame*)malloc(sizeof(frame) * mv.header.total_frame_count);
	if (mv.frames == NULL) { return; }
	memset(mv.frames, 0, sizeof(frame) * mv.header.total_frame_count);

#if 1
	// 스트리밍 지원하도록

	int idx = 0;
	// 1st 읽기 - 프레임 정보 (크기, 오프셋)
	while (idx < mv.header.total_frame_count) {
		frame* fr = &mv.frames[idx];
		fread(&fr->header, sizeof(frame_header), 1, mv_fp);
		
		if ((idx + 1) < mv.header.total_frame_count) {
			int stride = ((fr->header.width * 3 + 3) & ~3);
			int size = stride * fr->header.height;
			fseek(mv_fp, size, SEEK_CUR);
		}
		idx++;
	}

	if (num_preload > mv.header.total_frame_count) { 
		num_preload = mv.header.total_frame_count;
	}
	else {
		num_preload = MAX_PRELOAD;
	}

	HDC hdc = GetDC(hWnd);

	// 2nd 프레임 픽셀 데이터 사전읽기
	for(idx = 0; idx < num_preload; ++idx) {
		if (idx >= mv.header.total_frame_count) { break; }

		frame* fr = &mv.frames[idx];
		fseek(mv_fp, fr->header.pixel_data_offset, SEEK_SET);
		//fread(&fr->header, sizeof(frame_header), 1, mv_fp);

		int stride = ((fr->header.width * 3 + 3) & ~3);
		int size = stride * fr->header.height;

		// 프레임 픽셀 데이터 할당
		fr->pixel_data = (uint8_t*)malloc(size);
		if (fr->pixel_data == NULL) { break; }
		memset(fr->pixel_data, 0, size);

		fread(fr->pixel_data, size, 1, mv_fp);

		// 로드한 픽셀로 비트맵 생성
		fr->bmp = Create24BitHBITMAP(hdc, fr->header.width, fr->header.height, fr->pixel_data);
	}
	ReleaseDC(hWnd, hdc);
#else
	// 파일이 큰 경우 오래 걸림 - 메모리에 다 올리는 방식
	HDC hdc = GetDC(hWnd);

	for (int i = 0; i < mv.header.total_frame_count; ++i) {
		frame* fr = &mv.frames[i];
		fread(&fr->header, sizeof(frame_header), 1, mv_fp);

		int stride = ((fr->header.width * 3 + 3) & ~3);
		int size = stride * fr->header.height;

		// 프레임 픽셀 데이터 할당
		fr->pixel_data = (uint8_t*)malloc(size);
		if (fr->pixel_data == NULL) break;
		memset(fr->pixel_data, 0, size);

		fread(fr->pixel_data, size, 1, mv_fp);

		// 로드한 픽셀로 비트맵 생성
		fr->bmp = Create24BitHBITMAP(hdc, fr->header.width, fr->header.height, fr->pixel_data);
	}
	ReleaseDC(hWnd, hdc);
#endif
}

void DrawBitmap(HDC hdc, int width, int height, HBITMAP hBmp)
{
	HDC hdcMem = CreateCompatibleDC(hdc);
	HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hBmp);
	BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);
	SelectObject(hdcMem, hbmOld);
	DeleteDC(hdcMem);
}

void DrawBitmapMovie(HDC hdc) {
	// 무비 프레임 로드 완료됐는지 검사
	if (mv.frames == NULL) { return; }

	frame* fr = &mv.frames[current_frame_id];
	DrawBitmap(hdc, fr->header.width, fr->header.height, fr->bmp);
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

VOID CALLBACK DrawBmpMovieCallback(HWND hWnd, UINT message, UINT_PTR idTimer, DWORD dwTime) {	
	// 타이머 시작 이후로 경과 시간
	DWORD elapsed = timeGetTime() - mvStartTime;

	// 몇번째 프레임을 보여줘야 하는지 계산 
	const float index_unit = (float)mv.header.fps / 1000;
	
	int prev_frame_id = current_frame_id;
	current_frame_id = (int)(elapsed * index_unit);
	current_frame_id %= mv.header.total_frame_count;

	// 해당 프레임이 로드되지 않았다면 사전 로드 크기만큼 로드
	frame* fr = &mv.frames[current_frame_id];
	if (fr->pixel_data == NULL) {
		ReadBmpMovieWithRange(hWnd, current_frame_id, current_frame_id + num_preload);
	}

	InvalidateRect(hWnd, NULL, FALSE);

#if 0
	// 크기가 다른 프레임이 담겨있는 경우 처리
	int prev_frame_id = currentFrameId;
	currentFrameId = (++currentFrameId % mv.header.total_frame_count);

	// TODO: 만약 다음 프레임이 현재 프레임과 다른 크기이면 bErase TRUE 설정
	frame* prev_fr = &mv.frames[prev_frame_id];
	frame* curr_fr = &mv.frames[currentFrameId];

	if (prev_fr->header.width != curr_fr->header.width
		|| prev_fr->header.height != curr_fr->header.height) {
		//RECT rect = { .left = 0, };
		InvalidateRect(hWnd, NULL, TRUE);
	}
	else {
		InvalidateRect(hWnd, NULL, FALSE);
	}
#endif
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

			OpenBitmapSelectWindow(hWnd);
		}
		break;
		case ID_FILE_LOADBITMAPMOVIEFILE:
		{
			OpenBitmapMovieSelectWindow(hWnd);

			if (mv.header.total_frame_count != 0) {
				// 업데이트 주기
				// 밀리초 단위이므로 1000밀리초(1초)를 fps로 나눔
				int interval = (int)(1000 / mv.header.fps);
				// SetTimer 해상도의 한계
				mvStartTime = timeGetTime();
				SetTimer(hWnd, TIMER_ID, interval, (TIMERPROC)DrawBmpMovieCallback);
			}
		}
		break;
		case ID_FILE_MOSAIC:
			//MosaicBitmap(hWnd);
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

		// #1 로드한 비트맵 그리기
		//DrawBitmap(hdc, bmp.header.width, bmp.header.height, hBitmap);

		// #2 비트맵 확대/축소
		//double scale = zoomFactors[currentZoomLevel];
		//DrawResizedBitmap(hdc, scale);

		// #3 비트맵 무비 그리기
		DrawBitmapMovie(hdc);

		EndPaint(hWnd, &ps);
	}
	break;
	case WM_KEYDOWN:
		// TODO: 프레임 로드 완료된 경우, Space바 입력 시 다음 프레임으로 이동하는 기능
		if (wParam == VK_SPACE)
		{
			if (mv.header.total_frame_count != 0) {
				current_frame_id++;
				current_frame_id %= mv.header.total_frame_count;
				InvalidateRect(hWnd, NULL, TRUE);
			}
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
	// TODO: 해당 방식 말고, 다른 방법으로 깜빡거리는 현상 수정 바람
	//case WM_ERASEBKGND:
	//	return 1;  // 배경 지우기 방지
	case WM_DESTROY:
		KillTimer(hWnd, TIMER_ID);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void OpenBitmapSelectWindow(HWND hWnd)
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
		char path[256];
		int nLen = (int)wcslen(ofn.lpstrFile);
		wcstombs(path, ofn.lpstrFile, nLen + 1);

		ReadBitmap(hWnd, path);
		InvalidateRect(hWnd, NULL, TRUE);
	}
}

void OpenBitmapMovieSelectWindow(HWND hWnd) {
	OPENFILENAME ofn;
	WCHAR szFile[260] = { 0 };

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = L"BitmapMovie Files\0*.bm\0All Files\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn) == TRUE)
	{
		char path[256];
		int nLen = (int)wcslen(ofn.lpstrFile);
		wcstombs(path, ofn.lpstrFile, nLen + 1);

		LoadBitmapMovie(hWnd, path);
		InvalidateRect(hWnd, NULL, TRUE);
	}
}

// 모자이크 레벨이 높을수록 선명도가 낮아지는 것. 최대 5단계까지
void MosaicBitmap(HWND hWnd) {

}
