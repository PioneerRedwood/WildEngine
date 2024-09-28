#include <SDL.h>

#include <iostream>
#include <memory>
#include <deque>
#include <map>

#include <process.h> // _beginThread
#include <windows.h>
#include <synchapi.h> // CriticalSection

#include "Bitmap.h"

// fopen 경고 끄기
#pragma warning(disable : 4996)

// TODO: 일부러 낮은 프레임만 미리 로드
#define MAX_PRELOAD_FRAME_COUNT 64

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
uint64_t mvStartedTime = 0;
SDL_Texture* texture = NULL;

// 프리로드 수만큼의 프레임을 로드할 배열
// 생산자 스레드에서는 이 프리로드된 프레임을 채우게 될 것이고
// 사용자 스레드에서는 이 프리로드된 프레임을 사용한 뒤 제거할 것
Frame preloads[ MAX_PRELOAD_FRAME_COUNT ] = {0};
uint32_t lastDrawFrameID = 0;
CRITICAL_SECTION cs;

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
		std::cout << "ReadBitmapMovie Failed to read file: " << path << std::endl;
		return false;
	}
	std::cout << "ReadBitmapMovie started file path " << path << std::endl;

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

	std::cout << "ReadBitmapMovie " << mv.frames.size() << " frames header read \n";

	// TODO: 미리 로드하는 크기는 전체 프레임 수보다 항상 작도록 설정
	int count = mv.header.totalFrameCount < MAX_PRELOAD_FRAME_COUNT ? mv.header.totalFrameCount : MAX_PRELOAD_FRAME_COUNT;

	// #2 pass - 지정한 사전 읽을 수만큼 픽셀 데이터 읽어오기
	for (int i = 0; i < count; ++i) {
		Frame* fr = &mv.frames[ i ];
		fseek(mv.fp, fr->header.pixelDataOffset, SEEK_SET);

		int stride = ((fr->header.width * 3 + 3) & ~3);
		int size = fr->header.height * stride;

		fr->pixelData = (uint8_t*)malloc(size);
		if (fr->pixelData == NULL) {
			// 메모리 할당 실패 처리
			std::cout << "Failed to pre-load frames \n";
			break;
		}

		fread(fr->pixelData, size, 1, mv.fp);

		auto tex = CreateTextureFromPixel(renderer, fr->header.width, fr->header.height, fr->pixelData);
		free(fr->pixelData);
		fr->pixelData = NULL;
		fr->texture = tex;

		preloadedFrames.emplace(fr->header.index, fr);
	}

	std::cout << "ReadBitmapMovie preload: " << count << " " << mv.header.totalFrameCount << " completed \n";

	return true;
}

static int GetCurrentFrameIDByElapsed(const Movie& mv, uint64_t elapsed) {
	//const float indexUnit = (float)mv.header.fps / 1000;
	const float indexUnit = (float)1 / 1000;
	int frameId = (int)(elapsed * indexUnit);
	frameId %= mv.header.totalFrameCount;
	std::cout << "Bmp frame " << frameId << " / " << mv.header.totalFrameCount << std::endl;
	return frameId;
}

// 생산자 스레드. 
void ReadBitmapMovieThread(void* ignored) {
	int maxPreloadCount = mv.header.totalFrameCount < MAX_PRELOAD_FRAME_COUNT ? mv.header.totalFrameCount : MAX_PRELOAD_FRAME_COUNT;
	while (true) {
		EnterCriticalSection(&cs);

		int numPreloaded = preloadedFrames.size();
		if (numPreloaded < maxPreloadCount) {
			// 현재 프레임이후로부터의 프레임을 읽어오기

			// 단순하게 마지막에 있는 프레임 그 다음의 프레임을 로드
			uint32_t startFrameId = (lastDrawFrameID + 1) % mv.header.totalFrameCount;

			auto found = mv.frames.find(startFrameId);
			if (found == mv.frames.end()) {
				SDL_assert(false);
			}

			Frame* startFrame = &mv.frames[ startFrameId ];
			for (int i = 0; i < maxPreloadCount - numPreloaded; ++i) {
				Frame* fr = &startFrame[ i ];
				fseek(mv.fp, fr->header.pixelDataOffset, SEEK_SET);

				int stride = ((fr->header.width * 3 + 3) & ~3);
				int frSize = fr->header.height * stride;
				fr->pixelData = (uint8_t*)malloc(frSize);
				if (fr->pixelData != NULL) {
					fread(fr->pixelData, frSize, 1, mv.fp);
				}

				// 해당 스레드에서 텍스처를 생성하면 스레드가 멈춰버린다
				//fr->texture = CreateTextureFromPixel(renderer, fr->header.width, fr->header.height, fr->pixelData);

				preloadedFrames.emplace(fr->header.index, fr);
			}
		}

		LeaveCriticalSection(&cs);

		// 해당 스레드가 많이 갖지 못하도록 대기
		Sleep(20);
	}
}

static void UpdateMovie(double delta) {
	if (mvStartedTime == 0) { mvStartedTime = SDL_GetPerformanceCounter(); }

	// TODO: 경과시간을 기준으로 현재 프레임 ID를 구한다
	uint64_t elapsed = (uint64_t)((SDL_GetPerformanceCounter() - mvStartedTime) / 1000);
	int currentFrameId = GetCurrentFrameIDByElapsed(mv, elapsed);

	// TODO: 현재 그릴 프레임의 텍스처를 가져온다. 
	EnterCriticalSection(&cs);

	auto found = preloadedFrames.find(currentFrameId);
	Frame* fr = nullptr;
	if (found != preloadedFrames.end()) {
		fr = found->second;
	}
	else {
		// TODO: 만약 미리 로드된 리스트에 존재하지 않으면 그리지 않고 넘어간다. 
		LeaveCriticalSection(&cs);
		return;
	}

	if (fr->texture == NULL && fr->pixelData != NULL) {
		// TODO: 픽셀 데이터로 텍스처 생성 - 메인 스레드에서 수행
		fr->texture = CreateTextureFromPixel(renderer, fr->header.width, fr->header.height, fr->pixelData);
	}
	// TODO: 실제 그리기 수행
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderClear(renderer);
	SDL_Rect rect = { 0 }; rect.x = 0, rect.y = 0, rect.w = fr->header.width, rect.h = fr->header.height;
	SDL_RenderCopy(renderer, fr->texture, NULL, &rect);
	SDL_RenderPresent(renderer);

	// TODO: 텍스처 삭제
	SDL_DestroyTexture(fr->texture);
	fr->texture = nullptr;

	// TODO: 현재 사용한 프레임이 사용되었음을 설정 및 마지막으로 그린 프레임을 갖는 변수 업데이트
	preloadedFrames.erase(currentFrameId);
	lastDrawFrameID = currentFrameId;

	LeaveCriticalSection(&cs);
}

static void Update(double delta) {
	UpdateMovie(delta);
}

int main(int argc, char** argv) {
	if (not InitProgram(640, 480)) {
		std::cout << "Exit.. \n";
		return 1;
	}

	//if (!ReadBitmapMovie(mv, "resources/bms/castle.bm")) {
		if (!ReadBitmapMovie(mv, "resources/bms/dresden.bm")) {
		std::cout << "Failed ReadBitmapMovie \n";
		ExitProgram();
		return 1;
	}

	// CS 초기화
	InitializeCriticalSection(&cs);
	HANDLE readBitmapMovieThread = (HANDLE)_beginthread(ReadBitmapMovieThread, 0, NULL);

	// 시작 시간
	currentTime = SDL_GetPerformanceCounter();

	// 메인 루프
	while (not quit) {
		lastTime = currentTime;
		currentTime = SDL_GetPerformanceCounter();

		deltaTime = (double)((currentTime - lastTime) * 1000 / (double)SDL_GetPerformanceFrequency());

		SDL_Event e;
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT) {
				quit = 1;
			}
		}

		// 일부 오브젝트 상태 업데이트

		// 현재 프레임 그리기 수행
		Update(deltaTime);

		//_sleep(100);
	}

	ExitProgram();

	return 0;
}