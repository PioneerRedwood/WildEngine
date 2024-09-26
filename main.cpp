#include <SDL.h>

#include <iostream>
#include <memory>
#include <deque>
#include <map>

#include <process.h> // _beginThread
//#include <synchapi.h> // CriticalSection

#include "Bitmap.h"

// fopen 경고 끄기
#pragma warning(disable : 4996)

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

// 시간 관련 변수
uint64_t currentTime = 0;
uint64_t lastTime = 0;
double deltaTime = 0;

// 메인 루프 플래그
int quit = 0;

//Bitmap bitmap = { 0 };
Movie mv = { 0 };
SDL_Texture* texture = NULL;

#define MAX_READY_FRAME 64
std::map<unsigned, Frame> readyFrameMap;
//CRITICAL_SECTION criticalSection;

static bool InitProgram(int width, int height) {
	// SDL 초기화
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cout << "SDL Init 실패! SDL_Error: " << SDL_GetError() << std::endl;
		return false;
	}

	// SDL 윈도우 및 렌더러 생성
	if (SDL_CreateWindowAndRenderer(width, height, 0, &window, &renderer) != 0) {
		std::cout << "SDL 윈도우 및 렌더러 생성 실패! SDL_Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return false;
	}

	return true;
}

static void ExitProgram() {
	std::cout << "프로그램 종료\n";

	// 윈도우/렌더러 파괴
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);

	// SDL 종료
	SDL_Quit();
}

static bool ReadBitmap(Bitmap& bmp, const char* path) {
	FILE* fp = fopen(path, "rb");
	if (fp == NULL) return false;

	fread(&bmp.headerInfo, 1, sizeof(BitmapHeaderInfo), fp);

	fread(&bmp.header, 1, sizeof(BitmapHeader), fp);

	int stride = ((bmp.header.width * 3 + 3) & ~3);
	int totalSize = stride * bmp.header.height;

	// 파일 포인터 픽셀 데이터 위치로 이동
	fseek(fp, bmp.headerInfo.pixelStartOffset, SEEK_SET);
	bmp.pixelData = (uint8_t*)malloc(totalSize);
	if (bmp.pixelData == NULL) {
		fclose(fp);
		return false;
	}
	memset(bmp.pixelData, 0, totalSize);
	fread(bmp.pixelData, 1, totalSize, fp);

	fclose(fp);
	return true;
}

static SDL_Texture* CreateTextureFromPixel(SDL_Renderer* renderer, int width, int height, const uint8_t* data) {
	// 이미지 상하 반전
	int stride = ((width * 3 + 3) & ~3);
	uint8_t* flippedPixelData = (uint8_t*)malloc(stride * height);

	if (flippedPixelData == NULL) {
		std::cout << "상하 반전을 위한 메모리 할당 실패 \n";
		return NULL;
	}

	for (int y = 0; y < height; ++y) {
		memcpy(
			flippedPixelData + (y * stride),
			data + ((height - 1 - y) * stride),
			stride);
	}

	// 만약 픽셀 데이터가 자주 변경되는 것이라면 SDL_TEXTUREACCESS_STREAMING을 사용해야 함.
	// SDL_LockTexture, SDL_UnlockTexture 참고
	SDL_Texture* tex = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STATIC,
		width, height);
	//SDL_Texture* tex = SDL_CreateTexture(renderer, 
	//	SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING, 
	//	width, height);
	if (tex == NULL) {
		std::cout << "SDL 텍스처 생성 실패 SDL_Error: " << SDL_GetError() << std::endl;
		free(flippedPixelData);
		return NULL;
	}

	int updateResult = SDL_UpdateTexture(tex, NULL, flippedPixelData, stride);
	if (updateResult != 0) {
		std::cout << "SDL 텍스처 업데이트 실패 SDL_Error: " << SDL_GetError() << std::endl;
		free(flippedPixelData);
		return NULL;
	}

	// 메모리 해제
	free(flippedPixelData);

	return tex;
}

static SDL_Texture* CreateTextureFromPath(SDL_Renderer* renderer, const char* path, Bitmap& out) {
	if (!ReadBitmap(out, path)) {
		return NULL;
	}

	SDL_Texture* tex = CreateTextureFromPixel(renderer, out.header.width, out.header.height, out.pixelData);
	free(out.pixelData);
	return tex;
}

static bool ReadBitmapMovie(Movie& mv, const char* path) {
	mv.fp = fopen(path, "rb");
	if (mv.fp == NULL) { 
		std::cout << "ReadBitmapMovie 실패. 파일을 읽을 수 없습니다: " << path << std::endl;
		return false; 
	}

	// 헤더 읽기
	const long headerStartOffset = (-1) * sizeof(MovieHeader);
	fseek(mv.fp, headerStartOffset, SEEK_END);
	fread(&mv.header, sizeof(MovieHeader), 1, mv.fp);
	fseek(mv.fp, 0, SEEK_SET);

	// #1 pass
	// 지정한 프레임을 읽으려면 프레임의 픽셀 시작 오프셋을 알아야 한다. 처음 읽을 때는 헤더만 읽는다.
	for (int i = 0; i < mv.header.totalFrameCount; ++i) {
		Frame fr = { 0 };
		fread(&fr.header, sizeof(FrameHeader), 1, mv.fp);
		int stride = ((fr.header.width * 3 + 3) & ~3);
		int size = fr.header.height * stride;
		fseek(mv.fp, size, SEEK_CUR);
		mv.frames.emplace(i, std::move(fr));
	}

	int count = mv.header.totalFrameCount < MAX_READY_FRAME ? mv.header.totalFrameCount : MAX_READY_FRAME;

	// #2 pass - 지정한 사전 읽을 수만큼 픽셀 데이터 읽어오기
	for (int i = 0; i < count; ++i) {
		Frame& fr = mv.frames[ i ];
		fseek(mv.fp, fr.header.pixelDataOffset, SEEK_SET);

		int stride = ((fr.header.width * 3 + 3) & ~3);
		int size = fr.header.height * stride;

		fr.pixelData = (uint8_t*)malloc(size);
		if (fr.pixelData == NULL) {
			// 메모리 할당 실패 처리
			std::cout << "사전 프레임 읽기에 필요한 메모리 할당에 실패했습니다. \n";
			break;
		}

		fread(fr.pixelData, size, 1, mv.fp);

		auto tex = CreateTextureFromPixel(renderer, fr.header.width, fr.header.height, fr.pixelData);
		free(fr.pixelData);
		fr.pixelData = NULL;
		fr.texture = tex;

		//EnterCriticalSection(&criticalSection);
		readyFrameMap.emplace(i, fr);
		//LeaveCriticalSection(&criticalSection);
	}

	return true;
}

static int GetCurrentFrameIDByElapsed(const Movie& mv, double delta) {
	const float indexUnit = (float)mv.header.fps / 1000;
	int frameId = ((int)(delta * indexUnit) % mv.header.totalFrameCount);
	return frameId;
}

void ReadBitmapMovieThread(void* ignored) {
	//while (true) {
	//	EnterCriticalSection(&criticalSection);

	//	// 만약 준비된 프레임 수가 임계치보다 낮아진 경우
	//	if (readyFrameMap.size() < MAX_READY_FRAME) {
	//		// 현재 프레임이후로부터의 프레임을 읽어오기

	//	}
	//	LeaveCriticalSection(&criticalSection);
	//	Sleep(100);
	//}
}

static void Update(double delta) {
	// 지정한 그리기 색상으로 클리어
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderClear(renderer);

	// 점 그리기
	// 그리기 색 설정
	//SDL_SetRenderDrawColor(renderer, 255, 0, 0, 127);
	//for (int y = 0; y < 200; ++y) {
	//	for (int x = 0; x < 100; ++x) {
	//		SDL_RenderDrawPoint(renderer, x, y);
	//	}
	//}

	// 텍스처 렌더링
	//if (texture != NULL) {
	//	SDL_Rect rect = { 0 };
	//	rect.x = 0, rect.y = 0, rect.w = bitmap.header.width, rect.h = bitmap.header.height;
	//	SDL_RenderCopy(renderer, texture, NULL, &rect);
	//}
	
	// 이때 픽셀데이터로 텍스처를 생성하기에는 퍼포먼스가 제대로 나지 않을 것이다..
	// 그래도 메인 스레드에서 텍스처 생성을 수행해야 한다..
	
	int currentFrameId = GetCurrentFrameIDByElapsed(mv, delta);
	Frame& fr = mv.frames[ currentFrameId ];

	SDL_Rect rect = { 0 };
	rect.x = 0, rect.y = 0, rect.w = fr.header.width, rect.h = fr.header.height;
	SDL_RenderCopy(renderer, fr.texture, NULL, &rect);

	// 이전 호출 이후 수행된 그리기 명령을 화면에 적용 (SDL_Render* 패밀리에 해당)
	SDL_RenderPresent(renderer);
}

int main(int argc, char** argv) {
	if (not InitProgram(640, 480)) {
		std::cout << "프로그램 종료\n";
		return 1;
	}

	// CS 초기화
	//InitializeCriticalSection(&criticalSection);

	//HANDLE readBitmapMovieThread = (HANDLE)_beginthread(ReadBitmapMovieThread, 0, NULL);

	// 텍스처 로드
	//const char* filepath = "resources/dresden/1.bmp";
	//texture = CreateTextureFromBitmap(renderer, filepath);
	//if (texture == NULL) {
	//	ExitProgram();
	//	return 1;
	//}
	if (!ReadBitmapMovie(mv, "resources/bms/dresden.bm")) {
		std::cout << "ReadBitmapMovie 실패 \n";
		ExitProgram();
		return 1;
	}

	// 시작 시간
	currentTime = SDL_GetPerformanceCounter();

	// 메인 루프
	while (not quit) {
		lastTime = currentTime;
		currentTime = SDL_GetPerformanceCounter();

		deltaTime = (double)( (currentTime - lastTime) * 1000 / (double)SDL_GetPerformanceFrequency() );
		
		SDL_Event e;
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT) {
				quit = 1;
			}
		}

		// 일부 오브젝트 상태 업데이트

		// 현재 프레임 그리기 수행
		Update(deltaTime);
	}

	ExitProgram();

	return 0;
}