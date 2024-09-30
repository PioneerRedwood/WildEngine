#include <SDL.h>

#include <iostream>
#include <memory>
#include <vector>

#include <process.h> // _beginThread
#include <windows.h>
#include <synchapi.h> // CriticalSection

#include "Bitmap.h"

// fopen 경고 끄기
#pragma warning(disable : 4996)

// TODO: 일부러 낮은 프레임만 미리 로드
#define MAX_PRELOAD_FRAME_COUNT 64

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

// 시간 관련 변수
uint64_t currentTime = 0;
uint64_t lastTime = 0;
double deltaTime = 0;

// 메인 루프 플래그
int quit = 0;

//Bitmap bitmap = { 0 };
Movie mv = { 0 };
uint64_t mvStartedTime = 0;
SDL_Texture* texture = nullptr;

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
	if (fp == nullptr) return false;

	fread(&bmp.headerInfo, 1, sizeof(BitmapHeaderInfo), fp);

	fread(&bmp.header, 1, sizeof(BitmapHeader), fp);

	int stride = ((bmp.header.width * 3 + 3) & ~3);
	int totalSize = stride * bmp.header.height;

	// 파일 포인터 픽셀 데이터 위치로 이동
	fseek(fp, bmp.headerInfo.pixelStartOffset, SEEK_SET);
	bmp.pixelData = (uint8_t*)malloc(totalSize);
	if (bmp.pixelData == nullptr) {
		fclose(fp);
		return false;
	}
	memset(bmp.pixelData, 0, totalSize);
	fread(bmp.pixelData, 1, totalSize, fp);

	fclose(fp);
	return true;
}

// 픽셀데이터로부터 상하반전된 텍스처를 생성함
// 텍스처 접근 모드는 자주 변경되지 않는 텍스처라면 STATIC, 자주 변경된다면 STREAMING 사용 권장
static SDL_Texture* CreateTextureFromPixel(SDL_Renderer* renderer, int width, int height, const uint8_t* data) {
	// 이미지 상하 반전
	int stride = ((width * 3 + 3) & ~3);
	uint8_t* flippedPixelData = (uint8_t*)malloc(stride * height);

	if (flippedPixelData == nullptr) {
		std::cout << "상하 반전을 위한 메모리 할당 실패 \n";
		return nullptr;
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
	if (tex == nullptr) {
		std::cout << "SDL 텍스처 생성 실패 SDL_Error: " << SDL_GetError() << std::endl;
		free(flippedPixelData);
		return nullptr;
	}

	int updateResult = SDL_UpdateTexture(tex, nullptr, flippedPixelData, stride);
	if (updateResult != 0) {
		std::cout << "SDL 텍스처 업데이트 실패 SDL_Error: " << SDL_GetError() << std::endl;
		free(flippedPixelData);
		return nullptr;
	}

	// 메모리 해제
	free(flippedPixelData);

	return tex;
}

static SDL_Texture* CreateTextureFromPath(SDL_Renderer* renderer, const char* path, Bitmap& out) {
	if (!ReadBitmap(out, path)) {
		return nullptr;
	}

	SDL_Texture* tex = CreateTextureFromPixel(renderer, out.header.width, out.header.height, out.pixelData);
	free(out.pixelData);
	return tex;
}

static bool UpdateTexture(SDL_Renderer* renderer, SDL_Texture* target, int w, int h, const uint8_t* data) {
	int stride = ((w * 3 + 3) & ~3);
	// 텍스처를 픽셀데이터로 업데이트, 이 동작은 느리므로, 자주 변경되는 것은 STREAMING 모드로 텍스처를 생성할 것을 권장한다 
	// 메모리 할당도 어떻게 이루어지는지?
	int result = SDL_UpdateTexture(target, nullptr, data, stride);
	if (result != 0) {
		std::cout << "UpdateTexture failed due to " << SDL_GetError() << std::endl;
		return false;
	}
	return true;
}

static bool ReadBitmapMovie(Movie& mv, const char* path) {
	mv.fp = fopen(path, "rb");
	if (mv.fp == nullptr) {
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
		// 값 복사
		preloads[i] = mv.frames[i];
		Frame* fr = &preloads[ i ];

		fseek(mv.fp, fr->header.pixelDataOffset, SEEK_SET);

		int stride = ((fr->header.width * 3 + 3) & ~3);
		int size = fr->header.height * stride;

		fr->pixelData = (uint8_t*)malloc(size);
		if (fr->pixelData == nullptr) {
			// 메모리 할당 실패 처리
			std::cout << "Failed to pre-load frames \n";
			break;
		}

		fread(fr->pixelData, size, 1, mv.fp);

		// 여기서는 픽셀 데이터만 설정하고 넘어간다 - 실제 텍스처를 생성하지 않도록
		auto tex = CreateTextureFromPixel(renderer, fr->header.width, fr->header.height, fr->pixelData);
		free(fr->pixelData);
		fr->pixelData = nullptr;
		fr->texture = tex;
		fr->neededUpdate = false;
	}

	std::cout << "ReadBitmapMovie preload: " << count << " " << mv.header.totalFrameCount << " completed \n";

	return true;
}

static int GetCurrentFrameIDByElapsed(uint64_t elapsed) {
	//const float indexUnit = (float)mv.header.fps / 1000;
	const float indexUnit = (float)1 / 1000;
	int frameId = (int)(elapsed * indexUnit);
	frameId %= mv.header.totalFrameCount;
	return frameId;
}

// 생산자 스레드. 
static void LoadFrameThread(void* ignored) {
	int maxPreloadCount = mv.header.totalFrameCount < MAX_PRELOAD_FRAME_COUNT ? mv.header.totalFrameCount : MAX_PRELOAD_FRAME_COUNT;
	while (true) {
		// 만약 더 미리 로드할 필요가 없다면?
		if (mv.header.totalFrameCount <= MAX_PRELOAD_FRAME_COUNT) {
			Sleep(20);
			continue;
		}

		EnterCriticalSection(&cs);

		// 이 시점까지 플래그가 설정된 것들은 다음 프레임을 미리 로드해야 한다. 

		// 다음 프레임 정보를 어떻게 알지?
		// 현재 프레임으로부터 미리로드된 프레임 배열 그 다음 프레임부터 로드해야 함
		uint64_t elapsed = (uint64_t)((SDL_GetPerformanceCounter() - mvStartedTime) / 1000);
		int nextFrameId = GetCurrentFrameIDByElapsed(elapsed);
		std::vector<Frame*> neededUpdatePixelFrames;
		for (int i = 0; i < MAX_PRELOAD_FRAME_COUNT; ++i) {
			if (preloads[ i ].neededUpdate) {
				neededUpdatePixelFrames.push_back(&preloads[ i ]);
			}

			// 미리 로드된 프레임 배열 그 다음 프레임부터 로드.
			// 이 코드의 맹점은 다시 재생 시 정상적으로 작동하지 않을 것임. 
			if (preloads[ i ].header.index > nextFrameId) {
				nextFrameId = preloads[ i ].header.index;
			}
		}

		// 사실상 미리 로드된 배열의 프레임 정보를 변경하는 것
		for (Frame* fr : neededUpdatePixelFrames) {
			// 헤더 정보 변경
			fr->header = mv.frames[ nextFrameId ].header;
			
			// 바디 정보 변경
			int w = fr->header.width, h = fr->header.height, stride = ((w * 3 + 3) & ~3), size = stride * h;
			fseek(mv.fp, fr->header.pixelDataOffset, SEEK_SET);
			fr->pixelData = (uint8_t*)malloc(size);
			fread(fr->pixelData, size, 1, mv.fp);

			// 그 다음 프레임으로 설정
			nextFrameId = (++nextFrameId % mv.header.totalFrameCount);
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
	int currentFrameId = GetCurrentFrameIDByElapsed(elapsed);

	std::cout << "UpdateMovie frame " << currentFrameId << " / " << mv.header.totalFrameCount << std::endl;

	// TODO: 현재 그릴 프레임의 텍스처를 가져온다. 
	EnterCriticalSection(&cs);

	// TODO: 미리 로드한 프레임 배열의 준비된 것들의 텍스처를 업데이트

	// TODO: 미리 로드한 프레임 배열에서 탐색
	Frame* fr = nullptr;
	for(int i = 0; i < MAX_PRELOAD_FRAME_COUNT; ++i) {
		if (preloads[ i ].header.index == currentFrameId) {
			fr = &preloads[ i ];
			break;
		}
	}
	if (fr == nullptr) {
		// TODO: 만약 미리 로드된 리스트에 존재하지 않으면 그리지 않고 넘어간다. 
		std::cout << "UpdateMovie Skip this frame \n";
		LeaveCriticalSection(&cs);
		return;
	}

	// TODO: 픽셀 데이터로 텍스처 업데이트
	if (fr->neededUpdate && fr->pixelData != nullptr) {
		//fr->texture = CreateTextureFromPixel(renderer, fr->header.width, fr->header.height, fr->pixelData);
		if (!UpdateTexture(renderer, fr->texture, fr->header.width, fr->header.height, fr->pixelData)) {
			// 텍스처 업데이트 실패
			std::cout << "UpdateTexture failed in UpdateMovie skip this frame \n";
			LeaveCriticalSection(&cs);
			return;
		}
		fr->neededUpdate = false;
	}
	else if (not fr->neededUpdate && fr->pixelData != nullptr) {

		fr->texture = CreateTextureFromPixel(renderer, fr->header.width, fr->header.height, fr->pixelData);
		if (fr->texture == nullptr) { 
			// 텍스처 생성 실패
			std::cout << "CreateTextureFromPixel failed, skip this frame [ error: " << SDL_GetError() << " ]\n";
			LeaveCriticalSection(&cs);
			return;
		}
		// 픽셀 데이터 삭제
		free(fr->pixelData);
	}
	// TODO: 실제 그리기 수행
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderClear(renderer);
	SDL_Rect rect = { 0 }; rect.x = 0, rect.y = 0, rect.w = fr->header.width, rect.h = fr->header.height;
	SDL_RenderCopy(renderer, fr->texture, nullptr, &rect);
	SDL_RenderPresent(renderer);

	// TODO: 텍스처 삭제
	//SDL_DestroyTexture(fr->texture);
	//fr->texture = nullptr;

	// TODO: 현재 사용한 프레임이 사용되었음을 설정
	fr->neededUpdate = true;

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
	HANDLE readBitmapMovieThread = (HANDLE)_beginthread(LoadFrameThread, 0, nullptr);

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