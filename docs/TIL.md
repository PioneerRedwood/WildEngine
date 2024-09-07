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

## 비트맵 렌더링
Windows API인 GDI를 사용해서 비트맵을 렌더링하기 위해서는 다양한 방법이 있다. 

1. `SetPixel(hdc, x, y, color)` 를 사용하여 지정한 픽셀에 값을 설정. (매우 느림)
2. `BitBlit`를 사용하여 Bitmap 픽셀 데이터를 화면 영역에 복사하는 방법. (매우 빠름)

