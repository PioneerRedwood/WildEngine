# SDL APIs
https://wiki.libsdl.org/SDL2/CategoryAPI

## 기초
- SDL_Init/SDL_Quit: 라이브러리 생성 및 종료
- SDL_CreateWindow/DestroyWindow: 윈도우 생성 및 파괴
- SDL_CreateRenderer/DestroyRenderer: 렌더러 생성 및 파괴 (렌더러는 렌더링 컨텍스트)
- SDL_CreateSurface/DestroySurface: 텍스처를 그리기 위해 

## 이벤트
- SDL_PollEvent: 현재까지 쌓인 이벤트를 폴링

## 렌더
- SDL_RenderClear
- SDL_Render