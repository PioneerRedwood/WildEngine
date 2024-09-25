#include <SDL.h>
#include <iostream>
#include <memory>

#include "Bitmap.h"

// fopen 경고 끄기
#pragma warning(disable : 4996)

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
int quit = 0; // 메인 루프 플래그

Bitmap bitmap = {0};
SDL_Texture* texture = NULL;

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

static SDL_Texture* CreateTextureFromBitmap(SDL_Renderer* renderer, 
																						const char* path) {
	if (!ReadBitmap(bitmap, path)) {
		return NULL;
	}

	// TODO: 텍스처가 뒤집혀서 보임. 수정 바람. 
	SDL_Texture* tex = SDL_CreateTexture(renderer, 
		SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STATIC, 
		bitmap.header.width, bitmap.header.height);
	if (tex == NULL) {
		std::cout << "SDL 텍스처 생성 실패 SDL_Error: " << SDL_GetError() << std::endl;
		free(bitmap.pixelData);
		return NULL;
	}

	int stride = ((bitmap.header.width * 3 + 3) & ~3);
	int updateResult = SDL_UpdateTexture(tex, NULL, bitmap.pixelData, stride);
	if (updateResult != 0) {
		std::cout << "SDL 텍스처 업데이트 실패 SDL_Error: " << SDL_GetError() << std::endl;
		free(bitmap.pixelData);
		return NULL;
	}

	return tex;
}

static void Update(SDL_Renderer* renderer) {
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
	if (texture != NULL) {
		SDL_Rect rect = { 0 };
		rect.x = 0, rect.y = 0, rect.w = bitmap.header.width, rect.h = bitmap.header.height;
		SDL_RenderCopy(renderer, texture, NULL, &rect);
	}

	// 이전 호출 이후 수행된 그리기 명령을 화면에 적용 (SDL_Render* 패밀리에 해당)
	SDL_RenderPresent(renderer);
}

int main(int argc, char** argv) {
	if (not InitProgram(640, 480)) {
		std::cout << "프로그램 종료\n";
		return 1;
	}

	const char* filepath = "resources/dresden/1.bmp";
	texture = CreateTextureFromBitmap(renderer, filepath);
	if (texture == NULL) {
		ExitProgram();
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

	ExitProgram();

	return 0;
}