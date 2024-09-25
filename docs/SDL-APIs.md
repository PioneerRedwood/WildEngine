# SDL APIs
https://wiki.libsdl.org/SDL2/CategoryAPI

## 기초/그래픽 구성
- [SDL_Init](https://wiki.libsdl.org/SDL2/SDL_Init)/[SDL_Quit](https://wiki.libsdl.org/SDL2/SDL_Quit): 라이브러리 생성 및 종료
- [SDL_CreateWindow](https://wiki.libsdl.org/SDL2/SDL_CreateWindow)/[DestroyWindow](https://wiki.libsdl.org/SDL2/SDL_DestroyWindow): 윈도우 생성 및 파괴
    - Apple의 macOS 환경에서 윈도우 생성 시 넘기는 flags를 `SDL_WINDOW_ALLOW_HIGHDPI`로 설정해야 한다. 
- [SDL_CreateRenderer](https://wiki.libsdl.org/SDL2/SDL_CreateRenderer)/[DestroyRenderer](https://wiki.libsdl.org/SDL2/SDL_DestroyRenderer): 렌더러 생성 및 파괴 (렌더러는 렌더링 컨텍스트)
- [SDL_CreateWindowAndRenderer](https://wiki.libsdl.org/SDL2/SDL_CreateWindowAndRenderer): 윈도우와 렌더러 모두 생성
- [SDL_CreateTexture](https://wiki.libsdl.org/SDL2/SDL_CreateTexture)/[DestroyTexture](https://wiki.libsdl.org/SDL2/SDL_DestroyTexture): 텍스처를 생성 및 파괴
    - 텍스처 파괴 시 `NULL`이거나 유효하지 않은 텍스처의 경우 에러 메시지 출력
- [SDL_UpdateTexture](https://wiki.libsdl.org/SDL2/SDL_UpdateTexture): 지정한 사각 영역의 텍스처 데이터 업데이트. 텍스처 생성 시 설정한 픽셀 포맷에 맞춰야 하며, 픽셀 데이터의 행 바이트 크기인 `pitch` 인자를 적절하게 설정해야 한다. 생성하는 데에 꽤 시간이 걸리며, 변경이 자주 발생하지 않는 정적으로 사용할 텍스처에 의도가 맞춰져있다. 
- [SDL_PixelFormatEnum](https://wiki.libsdl.org/SDL2/SDL_PixelFormatEnum): 픽셀 포맷 

## 이벤트
- SDL_PollEvent: 현재까지 쌓인 이벤트를 폴링

## 상태
- [SDL_SetRenderDrawColor](https://wiki.libsdl.org/SDL2/SDL_SetRenderDrawColor): 그리기 명령에 수행할 색상을 설정한다. (Rect, Line 그리고 Clear에 해당 - SDL_RenderDrawRect/Line/Clear)
- [SDL_SetRenderDrawBlendMode](https://wiki.libsdl.org/SDL2/SDL_SetRenderDrawBlendMode): 그리기 명령에 수행할 블렌드 모드를 설정한다. (Fill 그리고 Line에 해당)

## 그리기
SDL의 SDL_RenderDrawLine() 같은 렌더링 함수는 백버퍼에 수행하므로 스크린에 곧바로 그려지지 않는다. 대신 백버퍼를 업데이트한다. 그러므로 씬 전체를 구성한 뒤 이 구성된 백버퍼를 완전한 그림으로 화면에 표시한다. 이러한 작업은 프레임마다 단 한번만 수행하여 최종적으로 화면에 그린다. 

한번 표시되고 난 뒤 백버퍼는 더이상 유효하지 않는 것으로 간주된다; 프레임 사이에 이전 내용이 존재할 것이라 가정하지 않는다. 각 새로운 프레임에 그리거나 심지어 모든 픽셀을 덮어쓰려 한다해도, 이전에 SDL_RenderClear()를 통해 백버퍼를 초기화하는 것을 강력하게 권장한다. 

- [SDL_RenderPresent](https://wiki.libsdl.org/SDL2/SDL_RenderPresent): 지금까지 구성한 그리기 명령을 백버퍼에 수행. 메인 스레드에서만 호출해야 한다.
- [SDL_RenderClear](https://wiki.libsdl.org/SDL2/SDL_RenderClear): 현재 렌더링 타겟을 지정한 그리기 색상으로 초기화한다. 
- [SDL_RenderPoint](https://wiki.libsdl.org/SDL2/SDL_RenderDrawPoint): 현재 렌더링 타겟에 점을 그린다. 여러 점을 그리기 원하면 SDL_RenderPoints을 사용하라. 
- [SDL_RenderLine](https://wiki.libsdl.org/SDL2/SDL_RenderDrawLine): 현재 렌더링 타겟에 선을 그린다. 여러 선을 그리기 원하면 SDL_RenderLines를 사용하라. 
- [SDL_RenderRect](https://wiki.libsdl.org/SDL2/SDL_RenderDrawRect): 현재 렌더링 타겟에 사각형을 그린다. 여러 사각형을 그리기 원하면 SDL_RenderRects를 사용하라. 
- [SDL_RenderFillRect](https://wiki.libsdl.org/SDL2/SDL_RenderFillRect): 현재 렌더링 타겟에 지정한 그리기 색상으로 사각형을 채운다. 여러 사각형을 채우기 원하면 SDL_RenderFillRects를 사용하라. 
- [SDL_RenderCopy](https://wiki.libsdl.org/SDL2/SDL_RenderCopy): 현재 렌더링 타겟에 텍스처의 일부분을 복사한다.
