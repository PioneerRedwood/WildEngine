#pragma once
#include <cstdint>
#include <map>

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

#pragma pack(push, 1)
typedef struct {
  uint64_t pixelDataOffset;
  uint32_t index;
  uint32_t width;
  uint32_t height;
} FrameHeader;
#pragma pack(pop)

typedef struct {
  uint64_t size;
  uint32_t totalFrameCount;
  uint32_t fps;
} MovieHeader;

typedef struct {
  FrameHeader header;
  uint8_t* pixelData;
  SDL_Texture* texture;
  bool neededUpdate;
} Frame;

typedef struct {
  MovieHeader header;
  uint64_t startedTime;
  std::map<uint32_t, Frame> frames;
  FILE* fp;
  uint64_t currentFrameId;
} Movie;

typedef struct {
  uint32_t frameCount;
  uint32_t width;
  uint32_t height;
  uint32_t fps;
} VideoHeader;

typedef struct {
  VideoHeader header;
  uint64_t* framePixelOffsets;
} Video;