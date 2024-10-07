#include <SDL.h>

#include <iostream>
#include <memory>
#include <vector>

#include <process.h> // _beginThread
#include <windows.h>
#include <synchapi.h> // CriticalSection

#include "Video.h"

#define MAX_PRELOAD_FRAME_COUNT 1

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
Frame preloadFrames[ MAX_PRELOAD_FRAME_COUNT ] = { 0 };
std::vector<int> neededUpdateFrameIDs;
int lastDrawFrameID = -1;
CRITICAL_SECTION cs;

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

// 텍스처 접근 모드는 자주 변경되지 않는 텍스처라면 STATIC, 자주 변경된다면 STREAMING 사용 권장
SDL_Texture* CreateTextureFromPixel(SDL_Renderer* renderer, int width, int height, const uint8_t* data) {
	int stride = ((width * 3 + 3) & ~3);

	// 스태틱은 락이 요구되어 느림. 스트리밍은 쉐어드 메모리 개념이므로 락이 요구되지 않아 변경이 자주 발생하면 상대적으로 빠름. 
	// 이 락은 드라이버 레벨의 락임. 메모리 액세스와 CPU - GPU 간 대역폭

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
		return nullptr;
	}
	
	// 실제로 업데이트할 때 락을 거는건지
	// TODO: SDL_LockTexture
	void* pixelData;
	int pitch;
	int updateResult = SDL_LockTexture(tex, nullptr, &pixelData, &stride);
	if (updateResult != 0) {
		std::cout << "SDL failed to update SDL_Texture SDL_Error: " << SDL_GetError() << std::endl;
		return nullptr;
	}

	return tex;
}

// 생산자 스레드. 
void LoadFrameThread(void* arg) {
	Video* v = (Video*)arg;

	int w = v->header().width;
	int h = v->header().height;
	int stride = ((w * 3 + 3) & ~3);
	int size = stride * h;

	// TODO: 정해진 수만큼 계속 채우는 것을 시도. 
	
	// TODO: 읽어야할 프레임을 파일로부터 읽기.

	// TODO: std::vector에 픽셀 데이터만 저장. 
	
	// CriticalSection의 사이에 있는 것은 최소한으로 줄이는 것이 좋음
	
	// TODO: CriticalSection 시작

	// TODO: CriticalSection 종료

}

void Update(double delta) {
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

	// TODO: 가져온 프레임의 픽셀 데이터로 텍스처를 업데이트

	// TODO: 실제 그리기 수행
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderClear(renderer);

	// TODO: 디버깅 용으로 화면에 (전체 프리로드 / 현재 사용중인 프리로드) 형식으로 보여주기

	SDL_Rect rect = { 0 }; rect.x = 0, rect.y = 0, rect.w = v.header().width, rect.h = v.header().height;
	SDL_RenderCopy(renderer, fr->texture, nullptr, &rect);
	SDL_RenderPresent(renderer);

	lastDrawFrameID = currentFrameId;
}

int main(int argc, char** argv) {
	if (not InitProgram(640, 480)) {
		std::cout << "Exit.. \n";
		return 1;
	}

	// 적어도 256개 이상의 연속성이 눈에 잘 띄는 리소스로 준비할 것. 
	if (not v.readVideoFromFile("resources/videos/castle.mv")) {
	//if (not v.readVideoFromFile("resources/videos/dresden.mv")) {
		//if (not v.readVideoFromFile("resources/videos/medium.mv")) {
		std::cout << "Failed ReadBitmapMovie \n";
		ExitProgram();
		return 1;
	}
	else {
		// 미리 로드 시작 - 단 하나의 텍스처만 생성하고 이를 그릴 때 픽셀데이터로 업데이트하는 것으로 한다 
		int count = MAX_PRELOAD_FRAME_COUNT > v.header().frameCount ? v.header().frameCount : MAX_PRELOAD_FRAME_COUNT;

		printf("Preloading started - %d \n", count);

		// TODO: preloads 픽셀 데이터 읽어오기

		// TODO: Video 내 단 하나의 텍스처만 생성하기
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