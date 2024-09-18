#pragma once
#include <stdint.h>

//////////////////////////////////////////////////////////////////////

#pragma pack(push, 4)
typedef enum {
  //BI_RGB = 0,
  //BI_RLE8 = 1,
  //BI_RLE4 = 2,
  //BI_BITFIELDS = 3,
  //BI_JPEG = 4,
  //BI_PNG = 5,
  BI_ALPHABITFIELDS = 6,
  BI_CMYK = 11,
  BI_CMYKRLE8 = 12,
  BI_CMYKRLE4 = 13,
} bitmap_compression_method;
#pragma pack(pop)

#pragma pack(push, 2)
typedef enum {
  NONE = 0,
  ERROR_DIFFUSION = 1,
  PANDA = 2,
  SUPER_CIRCLE = 3
} bitmap_halftoning_algorithm_type;
#pragma pack(pop)

#pragma pack(push, 4)
// Fixed-size (7 different versions exist)
typedef enum {
  // OS21X_BITMAP_HEADER (12 bytes)
  // No longer supported after Windows 2000
  BITMAP_CORE_HEADER = 12,
  
  // (64 bytes)
  // No longer supported after Windows 2000
  // Adds halftoning. Adds RLE and Huffman 1D compression from BITMAP_CORE_HEADER
  OS22X_BITMAP_HEADER_2 = 64, 
  
  // This variant of the previous header contains 
  // only the first 16 bytes and the remaining bytes are assumed to be zero values. (16 bytes)
  // No longer supported after Windows 2000
  OS22X_BITMAP_HEADER_SMALL = 16, 
  
  // 40 bytes
  // The common Windows format, Windows NT, 3.1x or later
  BITMAP_INFO_HEADER = 40,
  
  // 52 bytes
  // Not officially documented
  // Written by Adobe Photoshop
  BITMAP_V2_INFO_HEADER = 52,
  
  // 56 bytes
  // Windows NT 4.0, 95 or later
  // Written by Adobe Photoshop
  BITMAP_V3_INFO_HEADER = 56,
  
  // 108 bytes
  // Windows NT 5.0, 98 or later
  // Written by The GIMP
  BITMAP_V4_INFO_HEADER = 108,
  
  // 124 bytes
  BITMAP_V5_INFO_HEADER = 124,

} bitmap_header_type;
#pragma pack(pop)

// TODO: Not fully understand..
// Referred: https://en.wikipedia.org/wiki/Gamma_correction, 
// https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapv5header
// https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-ciexyztriple
// https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-ciexyz
// 4 bytes
typedef struct {
  uint16_t int_val;
  uint16_t float_val;
} fixed_point_2_dot_30;

// 12 bytes
typedef struct {
  fixed_point_2_dot_30 x;
  fixed_point_2_dot_30 y;
  fixed_point_2_dot_30 z;
} cie_xyz;

// 36 bytes
typedef struct {
  cie_xyz red;
  cie_xyz green;
  cie_xyz blue;
} cie_xyz_triple;

//////////////////////////////////////////////////////////////////////

#pragma pack(push, 1)
// 14 bytes
typedef struct {
  // Identify the BMP and DIB file is 0x42 0x4D in hexadecimal
  // These are reserved 2 bytes ASCII value to identify the BMP. 
  // BM (Windows 3.1x, 95, NT, ... etc)
  // BA (OS/2 struct bitmap array)
  // CI (OS/2 struct color icon)
  // CP (OS/2 const color pointer)
  // IC (OS/2 struct icon)
  // PT (OS/2 pointer)
  char header_field[2];
  
  uint32_t size;

  uint16_t reserved1;
  
  uint16_t reserved2;
  
  uint32_t pixel_start_offset;

} bitmap_header_info;
#pragma pack(pop)

// BITMAP_CORE_HEADER (12 bytes)
// Not supported
typedef struct {
  // offset: 14, size: 4
  uint32_t header_size;

  // offset: 18, size: 2
  uint16_t width;

  // offset: 20, size: 2
  uint16_t height;

  // offset: 22, size: 2
  // must be 1
  uint16_t num_color_planes;

  // offset: 24, size: 2
  uint16_t bits_per_pixel;

} bitmap_core_header;

// OS22X_BITMAP_HEADER_2 (64 bytes)
// Not supported
typedef struct {
  // offset: 54, size: 2
  // An enumerated value specifying the units for the horizontal and vertical resolutions
  // (offsets 38 and 42). The only defined value is 0, meaning pixels per metre
  uint16_t pixels_per_metre;
  
  // offset: 56, size: 2
  // Ignored and should be zero
  uint16_t padding;
  
  // offset: 58, size: 2
  // An enumerated value indicating the direction in which the bits fill the bitmap.
  // The only defined value is 0, meaning the origin is the lower-left corner.
  // Bits fill from left-to-right, then bottom-to-top.
  uint16_t fill_direction;
  
  // offset: 60, size: 2
  // FIXME: See bitmap_halftoning_algorithm_type to use the enum directly.
  uint16_t halftoning_algorithm;

  // offset: 64, size: 4
  uint32_t halftoning_parameter_1;
  
  // offset: 68, size: 4
  uint32_t halftoning_parameter_2;

  // offset: 72, size: 4
  // An enumerated value indicating the color encoding for each entry in the color table. 
  // The only defined value is 0, indicating RGB.
  uint32_t color_encoding;
  
  // offset: 76, size: 4
  // An application-defined identifier. Not used for image rendering
  uint32_t application_defined_id;

} os22x_bitmap_header_2;

// OS22X_BITMAP_HEADER_SMALL (16 bytes)
// Not supported
typedef struct {
  // offset: 14, size: 4
  uint32_t header_size;

  // offset: 18, size: 4
  uint32_t width;

  // offset: 22, size: 4
  uint32_t height;

  // offset: 26, size: 2
  // must be 1
  uint16_t num_color_planes;

  // offset: 28, size: 2
  uint16_t bits_per_pixel;

} os22x_bitmap_header_small;

// BITMAP_INFO_HEADER (40 bytes)
typedef struct {
  // offset: 14, size: 4
  uint32_t size;

  // offset: 18, size: 4
  uint32_t width;

  // offset: 22, size: 4
  uint32_t height;

  // offset: 26, size: 2
  // Must be 1
  uint16_t num_color_planes;

  // offset: 28, size: 2
  // Same as the color depths of the image
  // Typical values are 1, 4, 8, 16, 24 and 32
  uint16_t bits_per_pixel;

  // offset: 30, size: 4
  bitmap_compression_method compression_method;

  // offset: 34, size: 4
  // The size of the raw bitmap data; a dummy 0 can be for BI_RGB bitmaps. 
  uint32_t image_size;

  // offset: 38, size: 4
  int32_t horizontal_resolution;

  // offset: 42, size: 4
  int32_t vertical_resolution;

  // offset: 46, size: 4
  // 0 to default to 2^n
  // Colors in color table
  uint32_t color_palette;

  // offset: 50, size: 4
  // The number of important colors used, or 0 when every color is important; generally ignored
  uint32_t num_important_colors;

} bitmap_info_header;
  
// BITMAP_V2_INFO_HEADER (52 bytes)
// Added RGB bit masks(12 bytes) from BITMAP_INFO_HEADER (40 bytes)
typedef struct {
  // offset: 14, size: 4
  uint32_t size;

  // offset: 18, size: 4
  uint32_t width;

  // offset: 22, size: 4
  uint32_t height;

  // offset: 26, size: 2
  // Must be 1
  uint16_t num_color_planes;

  // offset: 28, size: 2
  // Same as the color depths of the image
  // Typical values are 1, 4, 8, 16, 24 and 32
  uint16_t bits_per_pixel;

  // offset: 30, size: 4
  bitmap_compression_method compression_method;

  // offset: 34, size: 4
  // The size of the raw bitmap data; a dummy 0 can be for BI_RGB bitmaps. 
  uint32_t image_size;

  // offset: 38, size: 4
  int32_t horizontal_resolution;

  // offset: 42, size: 4
  int32_t vertical_resolution;

  // offset: 46, size: 4
  // 0 to default to 2^n
  // Colors in color table
  uint32_t color_palette;

  // offset: 50, size: 4
  // The number of important colors used, or 0 when every color is important; generally ignored
  uint32_t num_important_colors;

  // offset: 54, size: 4
  uint32_t red_bit_mask;
  
  // offset: 58, size: 4
  uint32_t green_bit_mask;
  
  // offset: 62, size 4
  uint32_t blue_bit_mask;

} bitmap_v2_info_header;

// BITMAP_V3_INFO_HEADER (56 bytes)
// Added alpha bit masks(4 bytes) from BITMAP_V2_INFO_HEADER (52 bytes)
typedef struct {
  // offset: 14, size: 4
  uint32_t size;

  // offset: 18, size: 4
  uint32_t width;

  // offset: 22, size: 4
  uint32_t height;

  // offset: 26, size: 2
  // Must be 1
  uint16_t num_color_planes;

  // offset: 28, size: 2
  // Same as the color depths of the image
  // Typical values are 1, 4, 8, 16, 24 and 32
  uint16_t bits_per_pixel;

  // offset: 30, size: 4
  bitmap_compression_method compression_method;

  // offset: 34, size: 4
  // The size of the raw bitmap data; a dummy 0 can be for BI_RGB bitmaps. 
  uint32_t image_size;

  // offset: 38, size: 4
  int32_t horizontal_resolution;

  // offset: 42, size: 4
  int32_t vertical_resolution;

  // offset: 46, size: 4
  // 0 to default to 2^n
  // Colors in color table
  uint32_t color_palette;

  // offset: 50, size: 4
  // The number of important colors used, or 0 when every color is important; generally ignored
  uint32_t num_important_colors;

  // offset: 54, size: 4
  uint32_t red_bit_mask;
  
  // offset: 58, size: 4
  uint32_t green_bit_mask;
  
  // offset: 62, size 4
  uint32_t blue_bit_mask;

  // offset: 66, size: 4
  uint32_t alpha_bit_mask;

} bitmap_v3_info_header;

// BITMAP_V4_INFO_HEADER (108 bytes)
// Added color type and gamma correction (56 bytes) from BITMAP_V2_INFO_HEADER (52 bytes)
// TODO: Not sure the offset of it.. figure it out first
typedef struct {
  // offset: 14, size: 4
  uint32_t size;

  // offset: 18, size: 4
  uint32_t width;

  // offset: 22, size: 4
  uint32_t height;

  // offset: 26, size: 2
  // Must be 1
  uint16_t num_color_planes;

  // offset: 28, size: 2
  // Same as the color depths of the image
  // Typical values are 1, 4, 8, 16, 24 and 32
  uint16_t bits_per_pixel;

  // offset: 30, size: 4
  bitmap_compression_method compression_method;

  // offset: 34, size: 4
  // The size of the raw bitmap data; a dummy 0 can be for BI_RGB bitmaps. 
  uint32_t image_size;

  // offset: 38, size: 4
  int32_t horizontal_resolution;

  // offset: 42, size: 4
  int32_t vertical_resolution;

  // offset: 46, size: 4
  // 0 to default to 2^n
  // Colors in color table
  uint32_t color_palette;

  // offset: 50, size: 4
  // The number of important colors used, or 0 when every color is important; generally ignored
  uint32_t num_important_colors;

  // offset: 54, size: 4
  uint32_t red_bit_mask;
  
  // offset: 58, size: 4
  uint32_t green_bit_mask;
  
  // offset: 62, size 4
  uint32_t blue_bit_mask;
  
  // offset: 66, size: 4
  uint32_t alpha_bit_mask;

  // offset: 70, size: 4
  uint32_t color_space_type;

  // offset: 74, size: 36
  cie_xyz_triple color_space_endpoints;

  // offset: 110, size: 4
  uint32_t gamma_red;

  // offset: 114, size: 4
  uint32_t gamma_green;

  // offset: 118, size: 4
  uint32_t gamma_blue;

} bitmap_v4_info_header;

// BITMAP_V5_INFO_HEADER (124 bytes)
// Added intent, icc_profile_data, icc_profile_size and reserved (16 bytes) from BITMAP_V4_INFO_HEADER (108 bytes)
typedef struct {
  // offset: 14, size: 4
  uint32_t size;

  // offset: 18, size: 4
  uint32_t width;

  // offset: 22, size: 4
  uint32_t height;

  // offset: 26, size: 2
  // Must be 1
  uint16_t num_color_planes;

  // offset: 28, size: 2
  // Same as the color depths of the image
  // Typical values are 1, 4, 8, 16, 24 and 32
  uint16_t bits_per_pixel;

  // offset: 30, size: 4
  bitmap_compression_method compression_method;

  // offset: 34, size: 4
  // The size of the raw bitmap data; a dummy 0 can be for BI_RGB bitmaps. 
  uint32_t image_size;

  // offset: 38, size: 4
  int32_t horizontal_resolution;

  // offset: 42, size: 4
  int32_t vertical_resolution;

  // offset: 46, size: 4
  // 0 to default to 2^n
  // Colors in color table
  uint32_t color_palette;

  // offset: 50, size: 4
  // The number of important colors used, or 0 when every color is important; generally ignored
  uint32_t num_important_colors;

  // offset: 54, size: 4
  uint32_t red_bit_mask;
  
  // offset: 58, size: 4
  uint32_t green_bit_mask;
  
  // offset: 62, size 4
  uint32_t blue_bit_mask;
  
  // offset: 66, size: 4
  uint32_t alpha_bit_mask;

  // offset: 70, size: 4
  uint32_t color_space_type;

  // offset: 74, size: 36
  cie_xyz_triple color_space_endpoints;

  // offset: 110, size: 4
  uint32_t gamma_red;

  // offset: 114, size: 4
  uint32_t gamma_green;

  // offset: 118, size: 4
  uint32_t gamma_blue;

  // offset: 122, size: 4
  uint32_t intent;

  // offset: 126, size: 4
  uint32_t icc_profile_data;

  // offset: 130, size: 4
  uint32_t icc_profile_size;

  // offset: 134, size: 4
  uint32_t reserved;

} bitmap_v5_info_header;

//////////////////////////////////////////////////////////////////////

typedef struct {
  // Not needed after the file is loaded in memory
  bitmap_header_info header_info;

  // Identify the type of its dib_header
  bitmap_header_type header_type;
  
  // Immediately follows the Bitmap file header
  // void* dib_header;
  bitmap_v5_info_header header;
  
  // 3 or 4 DWORDS (12 or 16 bytes)
  // Present only in case the DIB header is the BITMAPINFOHEADER 
  // and the Compression Method member is set to either BI_BITFIELDS 
  // or BI_ALPHABITFIELDS
  void* extra_bit_masks;

  // Variable size, mandatory for color depths <= 8 bits (semi-optional)
  void* color_table;

  // Variable size (optional)
  void* gap1;

  // Variable size 
#if 0
  uint8_t** pixel_data;
#else
  uint8_t* pixel_data;
#endif

  // Variable size (optional)
  // An artifact of the ICC profile data offset field in the DIB header
  void* gap2;

  // Variable size (optional)
  // Can also contain a path to an external file containing the color profile. 
  // When loaded in memory as "non-packed DIB", 
  // it is located between the color table and Gap1
  // https://en.wikipedia.org/wiki/ICC_profile
  void* icc_color_profile;

} bitmap;

//////////////////////////////////////////////////////////////////////

typedef struct {
  uint32_t width;
  uint32_t height;
} frame_header;

typedef struct {
  uint32_t size;
  uint16_t total_frame_count;
  uint16_t fps;
} movie_header;

typedef struct {
  frame_header header;
  uint8_t* pixel_data;
  HBITMAP bmp;
} frame;

typedef struct {
  movie_header header;
  frame* frames;
} movie;
