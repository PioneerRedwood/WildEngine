#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "BitmapHeaderInfo.h"

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

int fread_by_offset(void* data, int size, int nitems, int offset, FILE* fp) {
  if(fseek(fp, offset, SEEK_SET) != 0)
    return 0;
  return fread(data, size, nitems, fp);
}

int read_bitmap_header_info(FILE* fp, bitmap_header_info* info) {
  int ret_code = 0, offset = 0;
  ret_code = fread_by_offset(info->header_field, sizeof(char), 2, offset, fp);
  if(ret_code != 2) {
    return 1;
  }

  offset += ret_code;
  ret_code = fread_by_offset((void*)&(info->size), sizeof(u_int32_t), 1, offset, fp);
  if(ret_code != 1) {
    return 1;
  }

  offset += ret_code;
  ret_code = fread_by_offset((void*)&(info->reserved1), sizeof(u_int16_t), 1, offset, fp);
  if(ret_code != 1) {
    return 1;
  }

  offset += ret_code;
  ret_code = fread_by_offset((void*)&(info->reserved2), sizeof(u_int16_t), 1, offset, fp);
  if(ret_code != 1) {
    return 1;
  }

  offset += ret_code;
  ret_code = fread_by_offset((void*)&(info->pixel_start_offset), sizeof(u_int32_t), 1, offset, fp);
  if(ret_code != 1) {
    return 1;
  }
  
  return 0;
}

int read_bitmap_dib_header(FILE* fp, bitmap* bmp, int* offset) {
  switch(bmp->header_type) {
    case BITMAP_CORE_HEADER: {
      // bitmap_core_header
      break;
    }
    case OS22X_BITMAP_HEADER: {
      // 
      break;
    }
    case OS22X_BITMAP_HEADER_2: {
      // bitmap_info_header2
      break;
    }
    case BITMAP_INFO_HEADER: {
      // bitmap_info_header
      break;
    }
    case BITMAP_V2_INFO_HEADER: {
      break;
    }
    case BITMAP_V3_INFO_HEADER: {
      break;
    }
    case BITMAP_V4_INFO_HEADER: {
      break;
    }
    case BITMAP_V5_INFO_HEADER: {
      break;
    }
    default: {
      break;
    }
  }
  return 0;
}

int main(int argc, char** argv) {
  printf("[DEBUG] program initiated .. \n");

  if(argc != 2) {
    printf("[ERROR] no file to read \n");
    return 1;
  }
  
  FILE* fp = fopen(argv[1], "rb");
  if(fp == NULL) {
    printf("[ERROR] failed to open file \n");
    return 1;
  }

  bitmap bmp;
  int file_offset = 0;

  // TODO: Read bitmap header info
  bitmap_header_info header_info;
  printf("[DEBUG] bitmap_header_info size: %ld \n", sizeof(bitmap_header_info));
  int ret_code = read_bitmap_header_info(fp, &header_info);
  if(ret_code != 0) {
    printf("[ERROR] failed to read bitmap_header_info \n");
    fclose(fp);
    return 1;
  } else {
    printf("[DEBUG] succeed to read bitmap_header_info size: %u \n", header_info.size);
    file_offset += 14; // 14 bytes
    bmp.header_info = (bitmap_header_info*)malloc(sizeof(header_info));
    memcpy(bmp.header_info, &header_info, sizeof(header_info));
  }
  
  // TODO: Read DIB header info
  u_int32_t dib_header_size = 0;
  ret_code = fread_by_offset(&dib_header_size, sizeof(u_int32_t), 1, file_offset, fp);
  if(ret_code != 1) {
    printf("[ERROR] failed to read dib_header_size \n");
    fclose(fp);
    free(bmp.header_info);
    return 1;
  }
  printf("[DEBUG] succced to read dib_header_size %u \n", dib_header_size);
  file_offset += sizeof(u_int32_t);
  bmp.header_type = (bitmap_header_type)dib_header_size;
  ret_code = read_bitmap_dib_header(fp, &bmp, &file_offset);

  // TODO: Read Pixels

  // TODO: Render to screen

  // TODO: Release all resources

  // TODO: Exit the program gracefully

  printf("[DEBUG] program finished .. \n");
  return 0;
}