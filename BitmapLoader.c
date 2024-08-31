#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "Bitmap.h"

//////////////////////////////////////////////////////////////////////
/// Read Data
//////////////////////////////////////////////////////////////////////

int fread_by_offset(void* data, int size, int nitems, int offset, FILE* fp) {
  if(fseek(fp, offset, SEEK_SET) != 0)
    return 0;
  return fread(data, size, nitems, fp);
}

int fpread(void* data, int size, int offset, FILE* fp) {
  if(fseek(fp, offset, SEEK_SET) != 0)
    return 0;
  if(fread(data, size, 1, fp) != 1) {
    return 0;
  }
  return 1;
}

int read_pixels(FILE* fp, bitmap* bmp) {
  // TODO: get the dib header to local pointer variable

  switch(bmp->header_type) {
    case BITMAP_CORE_HEADER:
      break;
  }
}

//////////////////////////////////////////////////////////////////////
/// End of Read Data
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
/// Read Headers
//////////////////////////////////////////////////////////////////////

int read_bitmap_header_info(FILE* fp, bitmap_header_info* info) {
  int ret_code = 0, offset = 0;
  ret_code = fread_by_offset(info->header_field, sizeof(char), 2, offset, fp);
  if(ret_code != 2) {
    // TODO: return the reason of failure
    return 1;
  }

  offset += ret_code;
  ret_code = fread_by_offset((void*)&(info->size), sizeof(u_int32_t), 1, offset, fp);
  if(ret_code != 1) {
    // TODO: return the reason of failure
    return 1;
  }

  offset += ret_code;
  ret_code = fread_by_offset((void*)&(info->reserved1), sizeof(u_int16_t), 1, offset, fp);
  if(ret_code != 1) {
    // TODO: return the reason of failure
    return 1;
  }

  offset += ret_code;
  ret_code = fread_by_offset((void*)&(info->reserved2), sizeof(u_int16_t), 1, offset, fp);
  if(ret_code != 1) {
    // TODO: return the reason of failure
    return 1;
  }

  offset += ret_code;
  ret_code = fread_by_offset((void*)&(info->pixel_start_offset), sizeof(u_int32_t), 1, offset, fp);
  if(ret_code != 1) {
    // TODO: return the reason of failure
    return 1;
  }
  
  return 0;
}

int read_bitmap_core_header(FILE* fp, bitmap* bmp, int* offset) {
  bmp->dib_header = (bitmap_core_header*)malloc(sizeof(bitmap_core_header));
  
  bitmap_core_header h;
  int ret_code = 0;

  ret_code = fread_by_offset(&h.width, sizeof(u_int16_t), 1, *offset, fp);
  if(ret_code != 1) {
    return 1;
  }
  offset += sizeof(u_int16_t);

  ret_code = fread_by_offset(&h.height, sizeof(u_int16_t), 1, *offset, fp);
  if(ret_code != 1) {
    return 1;
  }
  offset += sizeof(u_int16_t);

  ret_code = fread_by_offset(&h.num_color_plane, sizeof(u_int16_t), 1, *offset, fp);
  if(ret_code != 1) {
    return 1;
  } else if(h.num_color_plane != 1) {
    // TODO: The number of color plane must be 1
    return 1;
  }
  offset += sizeof(u_int16_t);

  ret_code = fread_by_offset(&h.bits_per_pixel, sizeof(u_int16_t), 1, *offset, fp);
  if(ret_code != 1) {
    return 1;
  }
  offset += sizeof(u_int16_t);

  memcpy(bmp->dib_header, &h, sizeof(bitmap_core_header));
  return 0;
}

int read_os22x_bitmap_header(FILE* fp, bitmap* bmp, int* offset) {
  return 0;
}

int read_os22x_bitmap_header_small(FILE* fp, bitmap* bmp, int* offset) {
  return 0;
}

int read_os22x_bitmap_header_2(FILE* fp, bitmap* bmp, int* offset) {
  return 0;
}

int read_bitmap_info_header(FILE* fp, bitmap* bmp, int* offset) {
  return 0;
}

int read_bitmap_v2_info_header(FILE* fp, bitmap* bmp, int* offset) {
  return 0;
}

int read_bitmap_v3_info_header(FILE* fp, bitmap* bmp, int* offset) {
  return 0;
}

int read_bitmap_v4_info_header(FILE* fp, bitmap* bmp, int* offset) {
  return 0;
}

int read_bitmap_v5_info_header(FILE* fp, bitmap* bmp, int* offset) {
  bmp->dib_header = (bitmap_v5_info_header*)malloc(sizeof(bitmap_v5_info_header));

  bitmap_v5_info_header* h = (bitmap_v5_info_header*)bmp->dib_header;
  h->size = (uint32_t)bmp->header_type;
  // width (4)
  if(fpread(&h->width, sizeof(u_int32_t), *offset, fp) == 0) return 1;
  *offset += sizeof(u_int32_t);

  // height (4)
  if(fpread(&h->height, sizeof(u_int32_t), *offset, fp) == 0) return 1;
  *offset += sizeof(u_int32_t);

  // num_color_planes (2)
  if(fpread(&h->num_color_planes, sizeof(u_int16_t), *offset, fp) == 0) return 1;
  *offset += sizeof(u_int16_t);

  // bits_per_pixel (2)
  if(fpread(&h->bits_per_pixel, sizeof(u_int16_t), *offset, fp) == 0) return 1;
  *offset += sizeof(u_int16_t);

  // compression_method (4)
  if(fpread(&h->compression_method, sizeof(bitmap_compression_method), *offset, fp) == 0) return 1;
  *offset += sizeof(bitmap_compression_method);

  // image_size (4)
  if(fpread(&h->image_size, sizeof(u_int32_t), *offset, fp) == 0) return 1;
  *offset += sizeof(u_int32_t);

  // horizontal_resolution (4)
  if(fpread(&h->horizontal_resolution, sizeof(int32_t), *offset, fp) == 0) return 1;
  *offset += sizeof(int32_t);

  // vertical_resolution (4)
  if(fpread(&h->vertical_resolution, sizeof(int32_t), *offset, fp) == 0) return 1;
  *offset += sizeof(int32_t);

  // color_palette (4)
  if(fpread(&h->color_palette, sizeof(u_int32_t), *offset, fp) == 0) return 1;
  *offset += sizeof(u_int32_t);
  if(h->color_palette > 0) {
    // TODO: Prepare to read color table 
    printf("[DEBUG] this bmp has a non-zero color_palette, prepare to read its color table \n");
  }

  // num_important_colors (4)
  if(fpread(&h->num_important_colors, sizeof(u_int32_t), *offset, fp) == 0) return 1;
  *offset += sizeof(u_int32_t);

  // red_bit_mask (4)
  if(fpread(&h->red_bit_mask, sizeof(u_int32_t), *offset, fp) == 0) return 1;
  *offset += sizeof(u_int32_t);

  // green_bit_mask (4)
  if(fpread(&h->green_bit_mask, sizeof(u_int32_t), *offset, fp) == 0) return 1;
  *offset += sizeof(u_int32_t);

  // blue_bit_mask (4)
  if(fpread(&h->blue_bit_mask, sizeof(u_int32_t), *offset, fp) == 0) return 1;
  *offset += sizeof(u_int32_t);

  // alpha_bit_mask (4)
  if(fpread(&h->alpha_bit_mask, sizeof(u_int32_t), *offset, fp) == 0) return 1;
  *offset += sizeof(u_int32_t);

  // color_space_type (4)
  if(fpread(&h->color_space_type, sizeof(u_int32_t), *offset, fp) == 0) return 1;
  *offset += sizeof(u_int32_t);

  // color_space_endpoints (36)
  if(fpread(&h->color_space_endpoints, sizeof(cie_xyz_triple), *offset, fp) == 0) return 1;
  *offset += sizeof(cie_xyz_triple);

  // gamma_red (4)
  if(fpread(&h->gamma_red, sizeof(u_int32_t), *offset, fp) == 0) return 1;
  *offset += sizeof(u_int32_t);

  // gamma_green (4)
  if(fpread(&h->gamma_green, sizeof(u_int32_t), *offset, fp) == 0) return 1;
  *offset += sizeof(u_int32_t);

  // gamma_blue (4)
  if(fpread(&h->gamma_blue, sizeof(u_int32_t), *offset, fp) == 0) return 1;
  *offset += sizeof(u_int32_t);

  // intent (4)
  if(fpread(&h->intent, sizeof(u_int32_t), *offset, fp) == 0) return 1;
  *offset += sizeof(u_int32_t);

  // icc_profile_data (4)
  if(fpread(&h->icc_profile_data, sizeof(u_int32_t), *offset, fp) == 0) return 1;
  *offset += sizeof(u_int32_t);
  // TODO: If bigger then zero, prepare to read icc_profile_data as icc_profile_size

  // icc_profile_size (4)
  if(fpread(&h->icc_profile_size, sizeof(u_int32_t), *offset, fp) == 0) return 1;
  *offset += sizeof(u_int32_t);

  // reserved (4)
  if(fpread(&h->reserved, sizeof(u_int32_t), *offset, fp) == 0) return 1;
  *offset += sizeof(u_int32_t);

  // TODO: Before read pixels, know its pixel format. 
  // bits_per_pixel, compression and RGBA masks are the factors which determine the pixel format. 
  printf("[DEBUG] read_bitmap_v5_info_header determine its pixel format [ bits_per_pixel: %u compression: %d, RGBA masks: %u %u %u %u ] \n",
    h->bits_per_pixel, h->compression_method, h->red_bit_mask, h->green_bit_mask, h->blue_bit_mask, h->alpha_bit_mask);

  switch(h->compression_method) {
    case BI_RGB: {
      // bits per pixel: 24 = 3 bytes
      // There is no compression
      bmp->pixel_data = (u_int8_t**)malloc(sizeof(h->height));
      u_int8_t rgb_masks[3];
      rgb_masks[0] = (h->red_bit_mask >> 16) & 0xFF;
      rgb_masks[1] = (h->green_bit_mask >> 8) & 0xFF;
      rgb_masks[2] = (h->blue_bit_mask) & 0xFF;
      
      // int padding = 0;
      // Read 1st row
      for(int i = h->height - 1; i > 0; --i) {
        u_int8_t* row = &(bmp->pixel_data[i]);
        // |<--------------------width------------------->|-------|
        // |Pixel[0, h-1]|Pixel[1, h-1]...|Pixel[w-1, h-1]|Padding|
        int row_size = h->width * (h->bits_per_pixel / 8); // bytes
        row = (u_int8_t*)malloc(row_size); // 1bytes * row_size

        int ret_code = fread_by_offset(row, sizeof(u_int8_t), row_size, *offset, fp);
        if(ret_code != row_size) {
          printf("[ERROR] read pixel data failed read size = %d offset: %d \n", ret_code, *offset);
          return 1;
        } else {
          // padding = (row_size % 4); // think it's not necessary
          *offset += (row_size + sizeof(u_int32_t));

          // printf("[DEBUG] before masking row: %02X %02X %02X \n", row[0], row[1], row[2]);
          // 0..255 &mpersand with red_bit_mask
          for(int j = 0; j < row_size; j+=3) {
            row[j+0] = row[j+0] & rgb_masks[0]; // R
            row[j+1] = row[j+1] & rgb_masks[1]; // G
            row[j+2] = row[j+2] & rgb_masks[2]; // B
          }
          // printf("[DEBUG] after masking row: %02X %02X %02X \n", row[0], row[1], row[2]);
        }
      }
      // Deduction
      break;
    }
    // https://en.wikipedia.org/wiki/Run-length_encoding
    // RLE 8-bit/pixel
    case BI_RLE8: {
      // TODO: Carry on the following steps:
      //  1. Read 
      break;
    }
    // RLE 4-bit/pixel
    case BI_RLE4: {
      break;
    }
    // BITMAP_V2_INFO_HEADER: RGB bit fields mask
    // BITMAP_V3_INFO_HEADER+: RGBA
    case BI_BITFIELDS: {
      break;
    }
    // RLE-24 BITMAP_V4_INFO_HEADER+: JPEG image for printing
    case BI_JPEG: {
      break;
    }
    // BITMAP_V4_INFO_HEADER+: PNG image for printing
    case BI_PNG: {
      break;
    }
    // Only Windows CE 5.0 with .NET 4.0 or later
    // RGBA bit field masks
    case BI_ALPHABITFIELDS: {
      break;
    }
    // Only Windos Metafile CMYK
    // None
    case BI_CMYK: {
      break;
    }
    // Only Windows Metafile CMYK
    // RLE-8
    case BI_CMYKRLE8: {
      break;
    }
    // Only Windows Metafile CMYK
    // RLE-4
    case BI_CMYKRLE4: {
      break;
    }
    default: break;
  }
  return 0;
}

int read_bitmap_dib_header(FILE* fp, bitmap* bmp, int* offset) {
  switch(bmp->header_type) {
    case BITMAP_CORE_HEADER: {
      return read_bitmap_core_header(fp, bmp, offset);
      break;
    }
    case OS22X_BITMAP_HEADER_2: {
      return read_os22x_bitmap_header_2(fp, bmp, offset);
      break;
    }
    case OS22X_BITMAP_HEADER_SMALL: {
      return read_os22x_bitmap_header_small(fp, bmp, offset);
      break;
    }
    case BITMAP_INFO_HEADER: {
      return read_bitmap_info_header(fp, bmp, offset);
      break;
    }
    case BITMAP_V2_INFO_HEADER: {
      return read_bitmap_v2_info_header(fp, bmp, offset);
      break;
    }
    case BITMAP_V3_INFO_HEADER: {
      return read_bitmap_v3_info_header(fp, bmp, offset);
      break;
    }
    case BITMAP_V4_INFO_HEADER: {
      return read_bitmap_v4_info_header(fp, bmp, offset);
      break;
    }
    case BITMAP_V5_INFO_HEADER: {
      return read_bitmap_v5_info_header(fp, bmp, offset);
      break;
    }
    default: {
      return 1;
      break;
    }
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////
/// End of Read Headers 
//////////////////////////////////////////////////////////////////////



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
  if(ret_code != 0) {
    printf("[ERROR] failed tp read dib_header \n");
    fclose(fp);
    free(bmp.header_info);
    return 1;
  }

  // TODO: Read Pixels
  // So far, reading pixels done. 

  // TODO: Render to screen

  // TODO: Release all resources

  // TODO: Exit the program gracefully

  printf("[DEBUG] program finished .. \n");
  return 0;
}