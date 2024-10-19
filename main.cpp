#include <SDL.h>

#include <iostream>
#include <memory>
#include <list>
#include <iomanip>

#include <process.h> // _beginThread
#include <windows.h>
#include <synchapi.h> // CriticalSection

#include "Video.h"
#include "Sprite.h"

#pragma comment(lib, "winmm.lib")

// #define vs. constexpr auto?
constexpr auto MAX_PRELOAD_FRAME_COUNT = 16;
//#define MAX_PRELOAD_FRAME_COUNT 16

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

// 시간 관련 변수
uint64_t currentTime = 0;
uint64_t lastTime = 0;
double deltaTime = 0;

// 메인 루프 플래그
int quit = 0;

Video v = {};
Sprite s = {};
SDL_Texture* sdlTexture = {};
uint64_t videoStartTime = {};

// 생산자 스레드에서는 이 프리로드된 프레임을 생성
// 사용자 스레드에서는 이 프리로드된 프레임을 사용/제거
// 양방향 접근을 위해 vector가 아닌 list 사용
std::list<Frame> bufferingFrames = {};
std::size_t lastDrawFrameID = {};
SDL_Point* points = {};
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
		SDL_RenderDrawRects(renderer, rects, bufferingFrames.size());

		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
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

	if (points != nullptr) {
		free(points);
	}

	// 윈도우/렌더러 파괴
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);

	// SDL 종료
	SDL_Quit();
}

bool PrepareResource() {
	// 연속성이 눈에 잘 띄는 리소스
	if (not v.readVideoFromFile("resources/videos/american-town.mv")) {
	//if (not v.readVideoFromFile("resources/videos/american-town3.mv")) {
	//if (not v.readVideoFromFile("resources/videos/dresden.mv")) {
		std::cout << "Failed ReadBitmapMovie \n";
		ExitProgram();
		return false;
	}

	// 미리 로드 시작 - 단 하나의 텍스처만 생성하고 이를 그릴 때 픽셀데이터로 업데이트하는 것으로 한다 
	int count = MAX_PRELOAD_FRAME_COUNT > v.header().totalFrame ? v.header().totalFrame : MAX_PRELOAD_FRAME_COUNT;

	printf("Preloading started - %d \n", count);

	// TODO: preloads 픽셀 데이터 읽어오기
	for (int i = 0; i < count; ++i) {
		Frame fr = {};
		fr.index = i;
		fr.pixelData = (uint8_t*)malloc(v.frameSize());
		if (not v.readFrame(i, fr.pixelData)) {
			std::cout << "Video::readFrame loading preload frames failed \n";
			SDL_assert(false);
			return false;
		}

		bufferingFrames.push_back(fr);
	}

	// TODO: Video 내 단 하나의 텍스처만 생성하기
	sdlTexture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING,
		v.header().width, v.header().height);

	printf("Preloading ended - %d \n", count);

	// TODO: Sprite 로드하기
	//if (not s.readFromFile("resources/sprites/blue_bird.sp")) {
	if (not s.readFromFile("resources/sprites/red_bird.sp")) {
		ExitProgram();
		return false;
	}

	return true;
}

// 생산자 스레드. 
void LoadFrameThread(void* arg) {
	Video* v = (Video*)arg;
	const int maxFrameCount = MAX_PRELOAD_FRAME_COUNT > v->header().totalFrame ? v->header().totalFrame : MAX_PRELOAD_FRAME_COUNT;

	while (true) {

		// TODO: 정해진 수만큼 계속 채우는 것을 시도. 
		int nextPreloadFrameID = -1;
		EnterCriticalSection(&cs);

		// 0 1 2 3 4 5 6 -> 0 7
		if (bufferingFrames.empty() == false) {
			nextPreloadFrameID = (bufferingFrames.back().index + 1) % (v->header().totalFrame);
		}

		if (bufferingFrames.size() >= maxFrameCount) {
			LeaveCriticalSection(&cs);
			//Sleep(10);
			Sleep(rand() % 100);
			continue;
		}
		LeaveCriticalSection(&cs);

		// TODO: 읽어야할 프레임을 파일로부터 읽기.
		if (nextPreloadFrameID == -1) {
			// 만약 읽어놓은게 아무것도 없다면, 현재 프레임을 기준으로 읽는다 
			double elapsed = (double)(SDL_GetPerformanceCounter() - v->startTime()) / SDL_GetPerformanceFrequency() * 1000;
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
		else {
			std::cout << "LoadFrameThread readFrame from Video " << nextPreloadFrameID << std::endl;
		}

		// CriticalSection의 사이에 있는 것은 최소한으로 줄이는 것이 좋음
		EnterCriticalSection(&cs);
		bufferingFrames.push_back(fr);
		LeaveCriticalSection(&cs);
		
		//Sleep(10);
		// 랜덤한 슬립
		Sleep(rand() % 100);
	}
}

void RenderVideo() {
	if (v.startTime() == 0) {
		v.startTime(SDL_GetPerformanceCounter());
	}

	// 경과시간을 기준으로 현재 프레임 ID를 구한다
	// 특정 시간으로부터 경과한 밀리초 구하기
	double elapsed = (double)(SDL_GetPerformanceCounter() - v.startTime()) / SDL_GetPerformanceFrequency() * 1000;
	int currentFrameID = v.getCurrentFrameIDByElapsed(elapsed);

	//std::cout << "Elapsed since video started " << elapsed << "ms - " << currentFrameId << " / " << v.header().frameCount << std::endl;

	// 만약 이전 프레임과 현재 프레임이 같은 것이라면 업데이트할 필요 없음
	if (currentFrameID == lastDrawFrameID) {
		return;
	}

	// TODO: 현재 그릴 프레임의 텍스처를 가져온다. 미리 로드한 프레임 배열에서 탐색. 
	int debug_frameID = -1;
	EnterCriticalSection(&cs);
	Frame found = {};
	found.index = -1;
	while (true) {
		if (bufferingFrames.empty()) {
			break;
		}
		//std::cout << "Finding frameID - ";
		//for (auto& i : bufferingFrames) {
		//	std::cout << i.index << " ";
		//}
		//std::cout << std::endl;
		if (bufferingFrames.front().index == currentFrameID) {
			// Debug용 로그
			std::cout << "Found frame " << currentFrameID << " at " << debug_frameID;
			std::cout << " buffering - ";
			for (auto& i : bufferingFrames) {
				std::cout << i.index << " ";
			}
			std::cout << std::endl;
			found = bufferingFrames.front();
			break;
		}
		else {
			free(bufferingFrames.front().pixelData);
			bufferingFrames.pop_front();
		}
		debug_frameID++;
	}

	// TODO: 만약 로드한 프레임 중에서 그릴 프레임을 찾을 수 없다면 스킵. 모든 로드한 프레임 삭제. 
	// 프레임 스킵 처리
	if (found.index == -1) {
		std::cout << "Cannot find in buffering .. skip this frame currentFrameID: " << currentFrameID;
		std::cout << " buffering - ";
		for (auto& i : bufferingFrames) {
			std::cout << i.index << " ";
		}
		std::cout << std::endl;
		for (auto& i : bufferingFrames) {
			free(i.pixelData);
		}
		bufferingFrames.clear();
		LeaveCriticalSection(&cs);
		return;
	}

	LeaveCriticalSection(&cs);

	// TODO: 가져온 프레임의 픽셀 데이터로 텍스처를 업데이트
#if 1
	void* pixelData = {};
	int pitch = {};
	// SDL 소스 확인해보기
	// STATIC 은 락이 불가. 
	int updateResult = SDL_LockTexture(sdlTexture, nullptr, &pixelData, &pitch);
	if (updateResult == 0 && found.pixelData != nullptr) {
		// 통째로 복사하면 패딩에 의해서 이미지가 휘어진 것처럼 렌더됨 
		// - SDL_LockTexture을 통해 가져온 픽셀 데이터는 패딩이 존재하여 4바이트 정렬되어있기 때문. 
		//memcpy(pixelData, found.pixelData, (size_t)pitch * v.header().height);

		// SDL_LockTexture는 패딩이 있으나 로드한 픽셀 데이터에는 없음
		// 로드한 픽셀의 로우만큼 텍스처 로우 오프셋에 메모리 복사
		for (int y = 0; y < v.header().height; ++y) {
			int rowOffset = y * v.rowSize();
			uint8_t* dst = ((uint8_t*)pixelData + y * pitch);
			memcpy(dst, found.pixelData + rowOffset, (size_t)pitch);
		}
	}
	else {
		std::cout << "SDL_LockTexture failed error: " << SDL_GetError() << std::endl;
	}
	SDL_UnlockTexture(sdlTexture);

	// TODO: 실제 그리기 수행
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderClear(renderer);
	SDL_Rect rect = { 0 }; rect.x = 0, rect.y = 0, rect.w = v.header().width, rect.h = v.header().height;
	SDL_RenderCopy(renderer, sdlTexture, nullptr, &rect);

	EnterCriticalSection(&cs);
	// TODO: 디버깅 용으로 화면에 (전체 프리로드 / 현재 사용중인 프리로드) 형식으로 보여주기
	debug::DrawUsingFrameStatus(debug_frameID);
	LeaveCriticalSection(&cs);
	
#else // Use drawPoint

	if (points == nullptr) {
		points = (SDL_Point*)malloc(v.frameSize());
	}

	// 프래그먼트 쉐이더가 수행하는 동작. 
	// 각 픽셀 데이터에 대한 RGB 정보를 포인트에 저장하고 점찍기 수행
	for (auto y = 0; y < v.header().height; ++y) {
		//std::cout << y << ": ";
		for (auto x = 0; x < v.header().width; ++x) {
			int rowOffset = y * v.rowSize() + (x * 3);
			Uint8 rgb[ 3 ] = {
				found.pixelData[ rowOffset + 2 ], found.pixelData[ rowOffset + 1 ], found.pixelData[ rowOffset + 0 ]
			};
			//std::cout << std::hex 
			//	<< std::setw(2) << std::setfill('0') << static_cast<int>(rgb[ 0 ]) << " "
			//	<< std::setw(2) << std::setfill('0') << static_cast<int>(rgb[ 1 ]) << " "
			//	<< std::setw(2) << std::setfill('0') << static_cast<int>(rgb[ 2 ]) << " " << std::dec;
#if 1
			SDL_SetRenderDrawColor(renderer, rgb[0], rgb[1], rgb[2], 255);
#else
			// Grayscale
			// 0.299 ∙ Red + 0.587 ∙ Green + 0.114 ∙ Blue
			Uint8 g = (Uint8)(rgb[ 0 ] * 0.299 + rgb[ 1 ] * 0.587 + rgb[ 2 ] * 0.114);
			SDL_SetRenderDrawColor(renderer, g, g, g, 255);
#endif
			
			SDL_RenderDrawPoint(renderer, x, y);
		}
		//std::cout << " \n";
	}

	EnterCriticalSection(&cs);
	// TODO: 디버깅 용으로 화면에 (전체 프리로드 / 현재 사용중인 프리로드) 형식으로 보여주기
	debug::DrawUsingFrameStatus(debug_frameID);
	LeaveCriticalSection(&cs);
#endif

	lastDrawFrameID = currentFrameID;
}

void RenderSprite(double delta) {
	// TODO: 현재 그릴 픽셀 데이터 파일로부터 로드
	if (not s.prepareFrame(delta)) {
		// 로드 실패
		return;
	}

	const unsigned xPos = 150;
	const unsigned yPos = 150;

	// TODO: 정해진 위치에 그리기
	uint8_t* data = s.pixelData();
	if (data == nullptr) {
		// 아직 픽셀 데이터가 준비되어있지 않음
		return;
	}
	for (auto y = 0; y < s.height(); ++y) {
		for (auto x = 0; x < s.width(); ++x) {
#if USE_GRAYSCALE
			// Grayscale [ 0.299 ∙ Red + 0.587 ∙ Green + 0.114 ∙ Blue ]
			Uint8 g = (Uint8)(rgb[ 0 ] * 0.299 + rgb[ 1 ] * 0.587 + rgb[ 2 ] * 0.114);
			SDL_SetRenderDrawColor(renderer, g, g, g, alpha);
#else
			int rowOffset = y * s.rowSize() + (x * 3);
			Uint8 rgb[ 3 ] = {
				data[ rowOffset + 2 ], data[ rowOffset + 1 ], data[ rowOffset + 0 ]
			};

			// TODO: 칼라값이 미리 설정한 알파와 동일하다면 그리기 건너뛰기
			if (rgb[ 0 ] == 255 && rgb[ 1 ] == 0 && rgb[ 2 ] == 220) {
				continue;
			}
			else {
				SDL_SetRenderDrawColor(renderer, rgb[ 0 ], rgb[ 1 ], rgb[ 2 ], 255);
			}

#endif // USE_GRAYSCALE
			SDL_RenderDrawPoint(renderer, x + xPos, y + yPos);
		}
	}
}

int main(int argc, char** argv) {
	// 랜덤 초기화
	srand(timeGetTime());

	if (not InitProgram(640, 480)) {
		std::cout << "Exit.. \n";
		return 1;
	}

	PrepareResource();

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

		// 비디오를 먼저 그려야 할 것 같아 먼저 렌더를 호출하면
		// 그려지지 않고 스프라이트만 덩그러니 남겨짐

		// 렌더링 업데이트
		//RenderVideo();

		RenderSprite(deltaTime);

		RenderVideo();

		SDL_RenderPresent(renderer);

		//_sleep(100);
	}

	ExitProgram();

	return 0;
}