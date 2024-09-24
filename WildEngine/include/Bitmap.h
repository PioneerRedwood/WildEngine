#pragma once
#include <cstdint>

#pragma pack(push, 1)
typedef struct {
  char header_field[2];
  uint32_t size;
  uint16_t reserved1;
  uint16_t reserved2;
  uint32_t pixel_start_offset;
} BitmapHeaderInfo;
#pragma pack(pop)

typedef struct {
  uint32_t size;
  uint32_t width;
  uint32_t height;
  uint16_t num_color_planes;
  uint16_t bits_per_pixel;
  uint32_t compression_method;
  uint32_t image_size;
  int32_t horizontal_resolution;
  int32_t vertical_resolution;
  uint32_t color_palette;
  uint32_t num_important_colors;
} BitmapHeader;

typedef struct {
  BitmapHeaderInfo headerInfo;
  BitmapHeader header;
  uint8_t* pixelData;
} Bitmap;

