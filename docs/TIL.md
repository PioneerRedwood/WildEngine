# TIL
오늘 내가 배운 것 (Today I Learned)

## 구조체 크기 정렬
32비트 어플리케이션은 CPU가 32비트씩 읽는 것이 효율적이므로 구조체 역시 크기가 32비트에 맞춰서 정렬된다. 
예를 들어, 비트맵의 헤더 크기는 14바이트인데, 아래 구조체 헤더는 16바이트로 정렬된다. (4의 배수) 

```C++
#pragma pack(push, 1)
// 14 bytes
typedef struct {
  char header_field[2]; // 2bytes
  uint32_t size; // 4bytes
  uint16_t reserved1; // 2bytes
  uint16_t reserved2; // 2bytes
  uint32_t pixel_start_offset; // 4bytes
} bitmap_header_info;
#pragma pack(pop)
```

아래는 패딩 값을 쉽게 구할 수 있는 수식이다. 
```C++
int stride = ((headerinfo.width * 3 + 3) & ~3);
```
https://learn.microsoft.com/ko-kr/cpp/preprocessor/pack?view=msvc-170 

이와 관련해서 찾아볼 만한 것: CPU Cache L1, L2, L3, Cache Hit/Miss

## Windows 비트맵 렌더링
1. `SetPixel(hdc, x, y, color)` 를 사용하여 지정한 픽셀에 값을 설정. 가로, 세로 루프를 수행하므로 느림.
2. `BitBlit`를 사용하여 Bitmap 픽셀 데이터를 화면 영역에 복사하는 방법. 메모리 복사이므로 빠름. 

## 알파 채널 있는 이미지 포맷
PNG: Portable Network Graphics 

- 무손실 압축, 알파 채널 존재
- zip 알고리즘 사용

TGA: TrueVision Graphics Adapter
- 압축 없음, 알파 채널 존재

## Texture Compression Format
Platform 별 Texture Compression
- Windows - DDS (DirectDraw Surface)
- Android - ETC (더 이상 권장하지 않으나 오래된 장비를 지원), ETC2 (OpenGL ES 3.0 이상을 지원하는 모든 기기에서 지원 - 활성 상태의 거의 모든 Android 장비 포함), ASTC (이전 버전을 대체하도록 설계됨)
- iOS - PVRTC(PowerVR GPU 사용 칩셋 한정), ETC/ETC2 (A7 칩 이후), ATSC(A8 칩 이후)

## 개발 효율 향상에 도움 되는 프로그램
- [HxD](https://mh-nexus.de/en/hxd/) - 무료 16진수 에디터
- [Paint.Net](https://www.getpaint.net/doc/latest/index.html) - 무료 이미지 에디터
- [FFMPEG](https://www.ffmpeg.org/) - 무료 멀티미디어 인코더/디코더
- [ImageMacick](https://imagemagick.org/index.php) - 무료 이미지 에디터

## FFMPEG 사용하여 동영상을 비트맵으로 변환
- [메인 옵션](https://ffmpeg.org/ffmpeg.html#Main-options)

영상을 입력(`-i .\Dresden.mp4`)으로 하여 초당 2프레임(`-r 2`), 크기를 너비와 높이 둘다 절반으로 (`-vf scale=iw/2:ih/2`) 출력 비트맵 픽셀 포맷은 BGR24로 (`-pix_fmt bgr24`) 변환하는 명령어다. 
```shell
ffmpeg -i .\Dresden.mp4 -r 2 -vf scale=iw/2:ih/2 -pix_fmt bgr24 .\dresden-museum-clock\clock%02d.bmp
```

입력 영상의 0초 구간부터 10초 동안 초당 12프레임으로 스케일은 절반으로 줄인 BGR24 픽셀 포맷 비트맵 추출
```shell
ffmpeg -ss 00:00:00 -i .\castle_on_the_hill.MOV -t 10 -r 12 -vf scale=iw/2:ih/2 -pix_fmt bgr24 .\castle\%d.bmp
```

## 시간 베이스로 접근
- 프레임 단위로 렌더링하는 비트맵 무비 작업 시 `SetTimer`의 해상도 한계를 경험
- 해상도의 한계 뿐 아니라, 프레임을 스킵해야하는 경우 등 여러 상황을 위해서는 시간 베이스 접근이 반드시 요구됨
- 예를 들어, 이동 및 충돌 등의 사건 역시 절대 변하지 않는 시간을 베이스로 계산해야 쉽고 정확
- Windows 환경에서 주로 사용되는 타이머는 정확한 타이머인 `QueryPerformanceCounter` 사용

## SDL 스트리밍
- SDL로 비트맵 무비를 스트리밍하는 기능을 완성
- SDL 윈도우 생성, 그래픽 그리기 등 기초부터 시작
- 멀티 스레드; 비트맵을 스트리밍하기 위해서는 읽고 사용하는 스레드가 달라야 함
  - beginThread, Critical Section 사용

## SDL 사용 on Windows
- [SDL2 다운로드](https://github.com/libsdl-org/SDL/releases/tag/release-2.30.7) 를 통해 `SDL2-devel-2.30.7-VC.zip`을 다운 받는다. 해당 압축 파일은 개발을 위한 용도로 Visual C++을 위한 패키지가 들어있다. 
- 압축을 해제한 뒤 응용 프로그램이 실행되는 곳에 `SDL2.dll` 파일을 위치시킨다. 링크 시 필요. 
- 프로젝트 파일의 속성에서 `Include directory`에 `SDL2/include` 경로를 추가한다. 

## 어떻게 영상을 스트리밍하는가?
- 목표: 메인 스레드에서는 로드한 프레임을 실행, 워커 스레드에서는 정해진 수만큼 프레임을 미리 로드해놓는다. 
- 구현할 부분:
  - 워커 스레드는 어떤 방식으로 몇번째 프레임을 로드해야하는지 결정하는가?
    - 일정 주기마다 루프를 돌며 사용한 수만큼 큐에 미리 로드. 사용한 수는 어떻게 구하는지?
    - 메인 스레드는 이 큐에 있는 프레임을 꺼내 사용. 
  - 프레임 스킵이 발생해서 메인 스레드에서 현재 미리 로드한 프레임에 실행할 것이 없다면 어떻게 해야 하는가?
    - 스킵된 프레임을 어떻게 보여줘야하는지?

- 시나리오
  - 총 5개의 프레임이 있으며, 초당 1프레임, 파일 로드하는 시간은 1초보다 낮을 것이라 가정하자.
1) 1번 프레임을 사용하고 1번 프레임의 텍스처를 사용했음으로 표시. 큐의 형태 
  - [1 2 3 4 5] -> [1' 2 3 4 5]
2) 1번 프레임이 사용되었으니, 생산자 스레드에선 미리 로드된 프레임 중 마지막의 그 다음 프레임을 미리 로드. 
  - [1' 2 3 4 5] -> [2 3 4 5 6]
3) 1 + 2 반복. 

preloads - 벡터로 할것
텍스처는 한 장만 미리 만들어둘것 

## static 사용 예시와 그 이유
- 함수: 
- 변수: 

