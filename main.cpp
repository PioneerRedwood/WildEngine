﻿#include <SDL.h>

#include <iostream>
#include <memory>
#include <vector>
#include <list>
#include <deque>

#include <process.h> // _beginThread
#include <windows.h>
#include <synchapi.h> // CriticalSection

#include "Video.h"

#define MAX_PRELOAD_FRAME_COUNT 32

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

// 시간 관련 변수
uint64_t currentTime = 0;
uint64_t lastTime = 0;
double deltaTime = 0;

// 메인 루프 플래그
int quit = 0;

Video v = {};
SDL_Texture* sdlTexture = {};
uint64_t videoStartTime = {};

// 생산자 스레드에서는 이 프리로드된 프레임을 생성
// 사용자 스레드에서는 이 프리로드된 프레임을 사용/제거
std::deque<Frame> loadedFrames = {};
std::size_t lastDrawFrameID = {};
CRITICAL_SECTION cs;

// Debug
namespace debug {
	SDL_Rect rects[ MAX_PRELOAD_FRAME_COUNT ] = {};
	SDL_Rect indicatorRect;
	const int x = 5;
	const int y = 5;
	const int margin = (int)(640 / MAX_PRELOAD_FRAME_COUNT) + 1;
	const int scale = 5;

	void InitRect() {
		for (int i = 0; i < MAX_PRELOAD_FRAME_COUNT; ++i) {
			rects[ i ].x = x + i * margin;
			rects[ i ].y = y;
			rects[ i ].w = scale;
			rects[ i ].h = scale;
		}

		indicatorRect.y = y;
		indicatorRect.w = scale;
		indicatorRect.h = scale;
	}

	void DrawUsingFrameStatus(int currentPreloadFrameID) {
		// TODO: 화면에 현재 사용중인 프레임과 전체 프레임을 사각형으로 그린다.
		// 전체 프리로드 수가 너무 많으면 화면에 다 그리기 힘듬. 적정 개수만 할것. 

		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderDrawRects(renderer, rects, MAX_PRELOAD_FRAME_COUNT);

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		indicatorRect.x = x + currentPreloadFrameID * margin;
		SDL_RenderDrawRect(renderer, &indicatorRect);
	}
}


bool InitProgram(int width, int height) {
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

	std::cout << "InitProgram succeeded\n";

	return true;
}

void ExitProgram() {
	std::cout << "Program exit \n";

	// 윈도우/렌더러 파괴
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);

	// SDL 종료
	SDL_Quit();
}

// 생산자 스레드. 
void LoadFrameThread(void* arg) {
	Video* v = (Video*)arg;

	while (true) {

		// TODO: 정해진 수만큼 계속 채우는 것을 시도. 
		int nextPreloadFrameID = -1;
		EnterCriticalSection(&cs);

		if (not loadedFrames.empty()) {
			nextPreloadFrameID = (loadedFrames.back().index + 1) % (v->header().frameCount);
		}

		if (loadedFrames.size() >= MAX_PRELOAD_FRAME_COUNT) {
			LeaveCriticalSection(&cs);
			Sleep(10);
			continue;
		}
		LeaveCriticalSection(&cs);

		// TODO: 읽어야할 프레임을 파일로부터 읽기.
		if (nextPreloadFrameID == -1) {
			// 만약 읽어놓은게 아무것도 없다면, 현재 프레임을 기준으로 읽는다 
			double elapsed = (double)(SDL_GetPerformanceCounter() - videoStartTime) / SDL_GetPerformanceFrequency() * 1000;
			nextPreloadFrameID = v->getCurrentFrameIDByElapsed(elapsed);
		}

		// TODO: std::vector에 픽셀 데이터만 저장. 
		Frame fr = { };
		fr.index = nextPreloadFrameID;
		fr.pixelData = (uint8_t*)malloc(v->frameSize());
		if (not v->readFrame(nextPreloadFrameID, fr.pixelData)) {
			std::cout << "Video::readFrame failed \n";
			SDL_assert(false);
		}

		// CriticalSection의 사이에 있는 것은 최소한으로 줄이는 것이 좋음
		EnterCriticalSection(&cs);
		loadedFrames.push_back(fr);
		LeaveCriticalSection(&cs);
		
		Sleep(10);
	}
}

void Update(double delta) {
	if (videoStartTime == 0) { videoStartTime = SDL_GetPerformanceCounter(); }

	// 경과시간을 기준으로 현재 프레임 ID를 구한다
	// 특정 시간으로부터 경과한 밀리초 구하기
	double elapsed = (double)(SDL_GetPerformanceCounter() - videoStartTime) / SDL_GetPerformanceFrequency() * 1000;
	int currentFrameID = v.getCurrentFrameIDByElapsed(elapsed);

	//std::cout << "Elapsed since video started " << elapsed << "ms - " << currentFrameId << " / " << v.header().frameCount << std::endl;

	// 만약 이전 프레임과 현재 프레임이 같은 것이라면 업데이트할 필요 없음
	if (currentFrameID == lastDrawFrameID) {
		return;
	}

	EnterCriticalSection(&cs);

	// TODO: 현재 그릴 프레임의 텍스처를 가져온다. 미리 로드한 프레임 배열에서 탐색. 
	int indexOfPreloads = -1;
	Frame found = {};
	found.index = -1;
	for (auto& i : loadedFrames) {
		if (i.index == currentFrameID) {
			found = i;
			break;
		}
		indexOfPreloads++;
	}

	// TODO: 만약 로드한 프레임 중에서 그릴 프레임을 찾을 수 없다면 스킵. 모든 로드한 프레임 삭제. 
	// 프레임 스킵 처리
	if (found.index == -1) {
		for (auto& i : loadedFrames) {
			free(i.pixelData);
		}
		loadedFrames.clear();
		LeaveCriticalSection(&cs);
		return;
	}

	// TODO: 가져온 프레임의 픽셀 데이터로 텍스처를 업데이트
	{
		void* pixelData = {};
		int pitch = {};
		int updateResult = SDL_LockTexture(sdlTexture, nullptr, &pixelData, &pitch);
		if (updateResult == 0 && found.pixelData != nullptr) {
			int expectedPitch = v.rowSize();
			if (expectedPitch != pitch) {
				std::cout << "SDL_LockTexture pitch is different from expecting \n";
			}
			//memcpy(pixelData, found.pixelData, (size_t)pitch * v.header().height);
			// TODO: 피치는 패딩이 포함된 로우고 v.rowSize()는 그렇지 않음.. 논리적으로 되게 하려면.. 
			// 여기서 복사할때 트릭이 있어야 하지 않을까
			memcpy(pixelData, found.pixelData, v.rowSize() * v.header().height);
		}
		else {
			std::cout << "SDL_LockTexture failed error: " << SDL_GetError() << std::endl;
		}
		SDL_UnlockTexture(sdlTexture);
	}

	// TODO: 가져온 픽셀 데이터를 로드한 프레임 배열에서 삭제
	{
		std::cout << "Before [ ";
		for (auto& f : loadedFrames) {
			std::cout << f.index << " ";
		}
		std::cout << "] current: " << currentFrameID << " indexOfPreloads: " << indexOfPreloads << std::endl;

		// 단순 알고리즘 1: indexOfPreloads 이하에 있는 것들은 모두 지워버리기. 
		{
			for (int i = 0; i < indexOfPreloads; ++i) {
				auto& info = loadedFrames.front();
				free(info.pixelData);
				loadedFrames.pop_front();
			}
		}

		std::cout << "After [ ";
		for (auto& f : loadedFrames) {
			std::cout << f.index << " ";
		}
		std::cout << "] current: " << currentFrameID << " indexOfPreloads: " << indexOfPreloads << std::endl;
	}
	LeaveCriticalSection(&cs);

	// TODO: 실제 그리기 수행
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderClear(renderer);
	SDL_Rect rect = { 0 }; rect.x = 0, rect.y = 0, rect.w = v.header().width, rect.h = v.header().height;
	SDL_RenderCopy(renderer, sdlTexture, nullptr, &rect);

	// TODO: 디버깅 용으로 화면에 (전체 프리로드 / 현재 사용중인 프리로드) 형식으로 보여주기
	debug::DrawUsingFrameStatus(indexOfPreloads);

	SDL_RenderPresent(renderer);
	lastDrawFrameID = currentFrameID;
}

int main(int argc, char** argv) {
	if (not InitProgram(640, 480)) {
		std::cout << "Exit.. \n";
		return 1;
	}

	// 적어도 256개 이상의 연속성이 눈에 잘 띄는 리소스로 준비할 것. 
	//if (not v.readVideoFromFile("resources/videos/american-town.mv")) {
	if (not v.readVideoFromFile("resources/videos/american-town3.mv")) {
	//if (not v.readVideoFromFile("resources/videos/dresden.mv")) {
	//if (not v.readVideoFromFile("resources/videos/medium.mv")) {
		std::cout << "Failed ReadBitmapMovie \n";
		ExitProgram();
		return 1;
	}

	// 미리 로드 시작 - 단 하나의 텍스처만 생성하고 이를 그릴 때 픽셀데이터로 업데이트하는 것으로 한다 
	int count = MAX_PRELOAD_FRAME_COUNT > v.header().frameCount ? v.header().frameCount : MAX_PRELOAD_FRAME_COUNT;

	printf("Preloading started - %d \n", count);

	// TODO: preloads 픽셀 데이터 읽어오기
	for (int i = 0; i < count; ++i) {
		Frame fr = {};
		fr.index = i;
		fr.pixelData = (uint8_t*)malloc(v.frameSize());
		if (not v.readFrame(i, fr.pixelData)) {
			std::cout << "Video::readFrame loading preload frames failed \n";
			SDL_assert(false);
		}

		loadedFrames.push_back(fr);
	}

	// TODO: Video 내 단 하나의 텍스처만 생성하기
	sdlTexture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING,
		v.header().width, v.header().height);
		//v.rowSize(), v.header().height);

	printf("Preloading ended - %d \n", count);

	// Debug 기능 초기화
	debug::InitRect();

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

		// 이벤트 처리
		SDL_Event e;
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT) {
				quit = 1;
			}
		}

		// 오브젝트 업데이트

		// 렌더링 업데이트
		Update(deltaTime);

		//_sleep(100);
	}

	ExitProgram();

	return 0;
}