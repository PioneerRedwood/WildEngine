# TIL; Today I Learned

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

## Inline Assembly - 인라인 어셈블리
- [C 코드에서 인라인 어셈블리를 사용하는 방법](https://gcc.gnu.org/onlinedocs/gcc/extensions-to-the-c-language-family/how-to-use-inline-assembly-language-in-c-code.html)
- 예전에는 픽셀 단위로 렌더링하는 것을 사용해왔다. 이중 루프이므로 속도가 매우 느리다. 그래서 이 그리는 기능의 일부를 어셈블리로 인라인하여 사용하는 경우가 있었다고 한다. 

## 32bit vs. 64bit 아키텍처의 레지스터
- CPU에서 직접적으로 접근 가능한 raw 메모리를 보관
- CPU마다 다른 변수 정의 (x86 AX BX CX EX ...)
- [X86 Architecture Microsoft Doc](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/x86-architecture)
- [X64 Architecture Microsoft Doc](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/x64-architecture)

## SIMD (Single Instruction Multiple Data)
- ARM - SIMD (NEON)
- 엔진에서는 CPU 연산으로 벡터, 메트릭스 연산 빠르게 가능
- DirectX 3D - SIMD 매트릭스 적용되어있음

## 256 컬러
- 팔레트 사용
- 256 컬러를 쓰는 게임: 스타1, 디아블로 1, 디아블로 2(버전이 다름) - 테크닉 (물 흐르는 것을 팔레트 교체로 표현)
- 예를 들어, 100 X 100 크기의 텍스처가 있을 때, RGB 각각 1바이트씩 표현하면 전체 100 X 100 X 3 = 30,000 바이트가 요구된다.
  그러나 1바이트 (0 ~ 255) 256가지의 컬러 팔레트를 사용한다면, 한 색상당 1바이트씩만 사용하기 때문에 10,000 바이트만 요구된다. 
  (팔레트 크기 제외)
- PCX (256 컬러 확장자) 전용 툴이 존재
- 던파, 리니지 1(예전)에서 사용
