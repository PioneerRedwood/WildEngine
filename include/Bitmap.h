#pragma once
#include <cstdint>

#pragma pack(push, 1)
typedef struct {
  char header_field[2];
  uint32_t size;
  uint16_t reserved1;
  uint16_t reserved2;
  uint32_t pixelStartOffset;
} BitmapHeaderInfo;
#pragma pack(pop)

typedef struct {
  uint32_t size;
  uint32_t width;
  uint32_t height;
  uint16_t numColorPlanes;
  uint16_t bitsPerPixel;
  uint32_t compressionMethod;
  uint32_t imageSize;
  int32_t horizontalResolution;
  int32_t verticalResolution;
  uint32_t colorPalette;
  uint32_t numImportantColors;
} BitmapHeader;

typedef struct {
  BitmapHeaderInfo headerInfo;
  BitmapHeader header;
  uint8_t* pixelData;
} Bitmap;

