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