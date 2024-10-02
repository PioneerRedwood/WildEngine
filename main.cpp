#include <SDL.h>

#include <iostream>
#include <memory>
#include <vector>

#include <process.h> // _beginThread
#include <windows.h>
#include <synchapi.h> // CriticalSection

#include "Video.h"

#define MAX_PRELOAD_FRAME_COUNT 2

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

// 시간 관련 변수
uint64_t currentTime = 0;
uint64_t lastTime = 0;
double deltaTime = 0;

// 메인 루프 플래그
int quit = 0;

Video v;
uint64_t videoStartTime = 0;

// 프리로드 수만큼의 프레임을 로드할 배열
// 생산자 스레드에서는 이 프리로드된 프레임을 채우게 될 것이고
// 사용자 스레드에서는 이 프리로드된 프레임을 사용한 뒤 제거할 것
Frame preloadFrames[ MAX_PRELOAD_FRAME_COUNT ] = {0};
std::vector<int> neededUpdateFrameIDs;
int lastDrawFrameID = -1;
CRITICAL_SECTION cs;

static bool InitProgram(int width, int height) {
	// SDL 초기화
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cout << "SDL Init failed! SDL_Error: " << SDL_GetError() << std::endl;
		return false;
	}

	// SDL 윈도우 및 렌더러 생성
	if (SDL_CreateWindowAndRenderer(width, height, 0, &window, &renderer) != 0) {
		std::cout << "SDL Create window and render failed! SDL_Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return false;
	}

	return true;
}

static void ExitProgram() {
	std::cout << "Program exit \n";

	// 윈도우/렌더러 파괴
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);

	// SDL 종료
	SDL_Quit();
}

// 픽셀데이터로부터 상하반전된 텍스처를 생성함
// 텍스처 접근 모드는 자주 변경되지 않는 텍스처라면 STATIC, 자주 변경된다면 STREAMING 사용 권장
static SDL_Texture* CreateTextureFromPixel(SDL_Renderer* renderer, int width, int height, const uint8_t* data) {
	// 이미지 상하 반전
	int stride = ((width * 3 + 3) & ~3);
	uint8_t* flippedPixelData = (uint8_t*)malloc(stride * height);

	if (flippedPixelData == nullptr) {
		std::cout << "CreateTextureFromPixel failed to allocate to flip the pixel data \n";
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
	//SDL_Texture* tex = SDL_CreateTexture(renderer,
	//	SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STATIC,
	//	width, height);
	SDL_Texture* tex = SDL_CreateTexture(renderer, 
		SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING, 
		width, height);
	if (tex == nullptr) {
		std::cout << "SDL failed to create SDL_Texture SDL_Error: " << SDL_GetError() << std::endl;
		free(flippedPixelData);
		return nullptr;
	}

	int updateResult = SDL_UpdateTexture(tex, nullptr, flippedPixelData, stride);
	if (updateResult != 0) {
		std::cout << "SDL failed to update SDL_Texture SDL_Error: " << SDL_GetError() << std::endl;
		free(flippedPixelData);
		return nullptr;
	}

	// 메모리 해제
	free(flippedPixelData);

	return tex;
}

// 생산자 스레드. 
static void LoadFrameThread(void* arg) {
	Video* v = (Video*)arg;
	
	int w = v->header().width;
	int h = v->header().height; 
	int stride = ((w * 3 + 3) & ~3);
	int size = stride * h;

	// 뒤집기 위한 여분의 프레임 변수
	uint8_t* flippedTemp = (uint8_t*)malloc(size);
	if (flippedTemp == nullptr) { 
		// 픽셀 뒤집기 위한 여분의 프레임 메모리 할당 실패
		return;
	}
	memset(flippedTemp, 0, size);

	while (true) {
		// 만약 더 미리 로드할 필요가 없다면 그냥 탈출.
		if (v->header().frameCount <= MAX_PRELOAD_FRAME_COUNT) {
			break;
		}

		EnterCriticalSection(&cs);

		if (neededUpdateFrameIDs.empty()) {
			LeaveCriticalSection(&cs);
			continue;
		}

		// 다음 프레임 = (직전 프레임 + 미리 로드된 크기 + 1) % 전체 프레임 수 
		// 특정 시간으로부터 경과한 밀리초 구하기
		double elapsed = (double)(SDL_GetPerformanceCounter() - videoStartTime) / SDL_GetPerformanceFrequency() * 1000;
		int nextFrameId = v->getCurrentFrameIDByElapsed(elapsed);
		nextFrameId %= v->header().frameCount;

		int lastFrameId = neededUpdateFrameIDs.back();
		Frame* fr = &preloadFrames[ lastFrameId ];

		uint8_t* data = (uint8_t*)malloc(size);
		if (data == nullptr) {
			// 메모리 할당 실패
			SDL_assert(false);
			LeaveCriticalSection(&cs);
			continue;
		}
		if (not v->readFrame(nextFrameId, data)) {
			// 읽기 실패
			SDL_assert(false);
			LeaveCriticalSection(&cs);
			continue;
		}
		fr->index = nextFrameId;

		// 상하반전 - 만약 파일을 구성할때부터 이걸 하면 런타임시 필요없을 것 같다
		for (int y = 0; y < h; ++y) {
			memcpy(
				flippedTemp + (y * stride),
				data + ((h - 1 - y) * stride),
				stride);
		}

		void* pixels = nullptr;
		int pitch = 0;
		if (SDL_LockTexture(fr->texture, nullptr, &pixels, &pitch) == 0) {
			memcpy(pixels, flippedTemp, size);
		}
		SDL_UnlockTexture(fr->texture);
		free(data);
		neededUpdateFrameIDs.pop_back();

		LeaveCriticalSection(&cs);
	}
}

static void Update(double delta) {
	if (videoStartTime == 0) { videoStartTime = SDL_GetPerformanceCounter(); }

	// 경과시간을 기준으로 현재 프레임 ID를 구한다
	// 특정 시간으로부터 경과한 밀리초 구하기
	double elapsed = (double)(SDL_GetPerformanceCounter() - videoStartTime) / SDL_GetPerformanceFrequency() * 1000;
	int currentFrameId = v.getCurrentFrameIDByElapsed(elapsed);

	//std::cout << "Elapsed since video started " << elapsed << "ms - " << currentFrameId << " / " << v.header().frameCount << std::endl;

	// 만약 이전 프레임과 현재 프레임이 같은 것이라면 업데이트할 필요 없음
	if (currentFrameId == lastDrawFrameID) {
		return;
	}

	EnterCriticalSection(&cs);

	// TODO: 현재 그릴 프레임의 텍스처를 가져온다. 미리 로드한 프레임 배열에서 탐색. 
	Frame* fr = nullptr;
	int preloadFrameIndex = 0;
	for (int i = 0; i < MAX_PRELOAD_FRAME_COUNT; ++i) {
		if (preloadFrames[ i ].index == currentFrameId) {
			fr = &preloadFrames[ i ];
			preloadFrameIndex = i;
			break;
		}
	}
	
	if (fr == nullptr) {
		// TODO: 만약 미리 로드된 리스트에 존재하지 않으면 그리지 않고 넘어간다. 
		//std::cout << "UpdateMovie Skip this frame, there is no frame to render [ FrameID: " << currentFrameId << " ]\n";
		neededUpdateFrameIDs.push_back(0);
		LeaveCriticalSection(&cs);
		return;
	}

	// TODO: 실제 그리기 수행
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderClear(renderer);
	SDL_Rect rect = { 0 }; rect.x = 0, rect.y = 0, rect.w = v.header().width, rect.h = v.header().height;
	SDL_RenderCopy(renderer, fr->texture, nullptr, &rect);
	SDL_RenderPresent(renderer);

	lastDrawFrameID = currentFrameId;

	// TODO: 현재 사용한 프레임이 사용되었음을 설정
	neededUpdateFrameIDs.push_back(preloadFrameIndex);
	LeaveCriticalSection(&cs);
}

int main(int argc, char** argv) {
	if (not InitProgram(640, 480)) {
		std::cout << "Exit.. \n";
		return 1;
	}

	if (not v.readVideoFromFile("resources/videos/castle.mv")) {
	//if (not v.readVideoFromFile("resources/videos/dresden.mv")) {
	//if (not v.readVideoFromFile("resources/videos/medium.mv")) {
		std::cout << "Failed ReadBitmapMovie \n";
		ExitProgram();
		return 1;
	}
	else {
		// 미리 로드 시작
		int count = MAX_PRELOAD_FRAME_COUNT > v.header().frameCount ? v.header().frameCount : MAX_PRELOAD_FRAME_COUNT;

		printf("Preloading started - %d \n", count);
		for (int i = 0; i < count; ++i) {
			Frame* fr = &preloadFrames[ i ];
			fr->index = i;

			int size = ((v.header().width * 3 + 3) & ~3) * v.header().height;
			uint8_t* data = (uint8_t*)malloc(size);
			if (not v.readFrame(i, data)) {
				// 프레임 읽기 실패
				std::cout << "Failed to readFrame of video! [ FrameID: " << i << " ]\n";
				ExitProgram();
				return 1;
			}
			fr->texture = CreateTextureFromPixel(renderer, v.header().width, v.header().height, data);
			
			// 임시로 텍스처 데이터 삭제하지 않도록 함
			free(data);
		}
		printf("Preloading ended - %d \n", count);
	}

	// CS 초기화
	InitializeCriticalSection(&cs);
	HANDLE readBitmapMovieThread = (HANDLE)_beginthread(LoadFrameThread, 0, &v);

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