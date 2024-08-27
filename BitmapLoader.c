#include <stdio.h>
#include <memory.h>

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
  u_int16_t header_field;
  
  u_int32_t size;

  u_int16_t reserved1;
  
  u_int16_t reserved2;
  
  u_int32_t pixel_start_offset;
} bitmap_header_info;

// Fixed-size (7 different versions exist)
typedef enum {
  // OS21X_BITMAP_HEADER (12 bytes)
  // No longer supported after Windows 2000
  BITMAP_CORE_HEADER = 0,
  
  // (64 bytes)
  // No longer supported after Windows 2000
  OS22X_BITMAP_HEADER, 
  
  // This variant of the previous header contains 
  // only the first 16 bytes and the remaining bytes are assumed to be zero values. (16 bytes)
  // No longer supported after Windows 2000
  OS22X_BITMAP_HEADER_2, 
  
  // 40 bytes
  // The common Windows format, Windows NT, 3.1x or later
  BITMAP_INFO_HEADER,
  
  // 52 bytes
  // Not officially documented
  // Written by Adobe Photoshop
  BITMAP_V2_INFO_HEADER,
  
  // 56 bytes
  // Windows NT 4.0, 95 or later
  // Written by Adobe Photoshop
  BITMAP_V3_INFO_HEADER,
  
  // 108 bytes
  // Windows NT 5.0, 98 or later
  // Written by The GIMP
  BITMAP_V4_INFO_HEADER,
  
  // 124 bytes
  BITMAP_V5_INFO_HEADER,

} bitmap_header_type;

// BITMAPCOREHEADER
typedef struct {
  // offset: 14
  u_int32_t header_size;

  // offset: 18
  u_int16_t width;

  // offset: 20
  u_int16_t height;

  // offset: 22
  // must be 1
  u_int16_t num_color_plane;

  // offset: 24
  u_int16_t bits_per_pixel;

} bitmap_core_header;

typedef enum {
  BI_RGB = 0,
  BI_RLE8 = 1,
  BI_RLE4 = 2,
  BI_BITFIELDS = 3,
  BI_JPEG = 4,
  BI_PNG = 5,
  BI_ALPHABITFIELDS = 6,
  BI_CMYK = 11,
  BI_CMYKRLE8 = 12,
  BI_CMYKRLE4 = 13,
} bitmap_compression_method;

// BITMAPINFOHEADER
typedef struct {
  // offset: 14, size: 4
  u_int32_t size;

  // offset: 18, size: 4
  u_int32_t width;

  // offset: 22, size: 4
  u_int32_t height;

  // offset: 26, size: 2
  // Must be 1
  u_int16_t num_color_planes;

  // offset: 28, size: 2
  // Same as the color depths of the image
  // Typical values are 1, 4, 8, 16, 24 and 32
  u_int16_t bits_per_pixel;

  // offset: 30, size: 4
  bitmap_compression_method compression_method;

  // offset: 34, size: 4
  // The size of the raw bitmap data; a dummy 0 can be for BI_RGB bitmaps. 
  u_int32_t image_size;

  // offset: 38, size: 4
  int32_t horizontal_resolution;

  // offset: 42, size: 4
  int32_t vertical_resolution;

  // offset: 46, size: 4
  // 0 to default to 2^n
  u_int32_t color_palette;

  // offset: 50, size: 4
  // The number of important colors used, or 0 when every color is important; generally ignored
  u_int32_t num_important_colors;
} bitmap_info_header;

// FIXME: Need to be packed 2bytes from integer (default size of enum in C)
typedef enum {
  NONE = 0,
  ERROR_DIFFUSION = 1,
  PANDA = 2,
  SUPER_CIRCLE = 3
} bitmap_halftoning_algorithm_type;

// OS/2 OS22XBITMAPHEADER (BITMAPINFOHEADER2)
typedef struct {
  // offset: 54, size: 2
  // An enumerated value specifying the units for the horizontal and vertical resolutions
  // (offsets 38 and 42). The only defined value is 0, meaning pixels per metre
  u_int16_t pixels_per_metre;
  
  // offset: 56, size: 2
  // Ignored and should be zero
  u_int16_t padding;
  
  // offset: 58, size: 2
  // An enumerated value indicating the direction in which the bits fill the bitmap.
  // The only defined value is 0, meaning the origin is the lower-left corner.
  // Bits fill from left-to-right, then bottom-to-top.
  u_int16_t fill_direction;
  
  // offset: 60, size: 2
  // FIXME: See bitmap_halftoning_algorithm_type to use the enum directly.
  u_int16_t halftoning_algorithm;

  // offset: 64, size: 4
  u_int32_t halftoning_parameter_1;
  
  // offset: 68, size: 4
  u_int32_t halftoning_parameter_2;

  // offset: 72, size: 4
  // An enumerated value indicating the color encoding for each entry in the color table. 
  // The only defined value is 0, indicating RGB.
  u_int32_t color_encoding;
  
  // offset: 76, size: 4
  // An application-defined identifier. Not used for image rendering
  u_int32_t application_defined_id;

} bitmap_info_header2;

typedef struct {
  // Not needed after the file is loaded in memory
  bitmap_header_info* header_info;

  // Identify the type of its dib_header
  bitmap_header_type header_type;
  
  // Immediately follows the Bitmap file header
  void* dib_header;
  
  // 3 or 4 DWORDS (12 or 16 bytes)
  // Present only in case the DIB header is the BITMAPINFOHEADER 
  // and the Compression Method member is set to either BI_BITFIELDS 
  // or BI_ALPHABITFIELDS
  void* extra_bit_masks;

  // Variable size, mandatory for color depths <= 8 bits
  void* color_table;

  // Variable size
  void* gap1;

  // Variable size
  void* pixel_data;

  // Variable size
  // An artifact of the ICC profile data offset field in the DIB header
  void* gap2;

  // Variable size
  // Can also contain a path to an external file containing the color profile. 
  // When loaded in memory as "non-packed DIB", 
  // it is located between the color table and Gap1
  void* icc_color_profile;

} bitmap;

int main(char argc, char** argv) {
  

  return 0;
}