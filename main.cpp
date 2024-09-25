#include <SDL.h>
#include <iostream>
#include <memory>

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
int quit = 0; // 메인 루프 플래그

static SDL_Texture* CreateTextureFromBitmap(SDL_Renderer* renderer, 
																						const char* path) {

}

static void Update(SDL_Renderer* renderer) {
	// 지정한 그리기 색상으로 클리어
	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	SDL_RenderClear(renderer);

	// 점 그리기
	// 그리기 색 설정
	//SDL_SetRenderDrawColor(renderer, 255, 0, 0, 127);
	//for (int y = 0; y < 200; ++y) {
	//	for (int x = 0; x < 100; ++x) {
	//		SDL_RenderDrawPoint(renderer, x, y);
	//	}
	//}


	// 이전 호출 이후 수행된 그리기 명령을 화면에 적용 (SDL_Render* 패밀리에 해당)
	SDL_RenderPresent(renderer);
}

int main(int argc, char** argv) {
	// SDL 초기화
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
		return 1;
	}

	// SDL 윈도우 및 렌더러 생성
	if(SDL_CreateWindowAndRenderer(640, 480, 0, &window, &renderer) != 0) {
		std::cout << "Window/Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}

	// 메인 루프
	while (not quit) {
		SDL_Event e;
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT) {
				quit = 1;
			}
		}

		// 일부 오브젝트 상태 업데이트

		// 현재 프레임 그리기 수행
		Update(renderer);
	}

	// 윈도우/렌더러 파괴
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);

	// SDL 종료
	SDL_Quit();

	return 0;
}