#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <Windows.h>
#include "Bitmap.h"

//////////////////////////////////////////////////////////////////////
/// Read Data
//////////////////////////////////////////////////////////////////////

int fread_by_offset(void* data, int size, int nitems, int offset, FILE* fp) {
	if (fp == NULL)
	{
		fprintf(stderr, "Error: File pointer is NULL. \n");
		return 0;
	}

	clearerr(fp);

	if (ferror(fp))
	{
		fprintf(stderr, "Error: File stream error detected before fseek. Clearing errors. \n");
		clearerr(fp);
		return 0;
	}

	if (fseek(fp, offset, SEEK_SET) != 0)
	{
		fprintf(stderr, "Error: fseek failed. Unable to set file position to offset %d. \n", offset);
		perror("fseek");
		return 0;
	}

	int items_read = fread(data, size, nitems, fp);

	if (items_read < nitems) {
		if (feof(fp))
		{
			fprintf(stderr, "Warning: End of file reached before reading the expected number of items. \n");
		}
		else if (ferror(fp))
		{
			fprintf(stderr, "Error: File read error occurred. \n");
			perror("fread");
		}
	}

	return items_read;

	//return fread(data, size, nitems, fp);
}

int fpread(void* data, int size, int offset, FILE* fp) {
	if (fseek(fp, offset, SEEK_SET) != 0)
		return 0;
	if (fread(data, size, 1, fp) != 1) {
		return 0;
	}
	return 1;
}

int read_pixels(FILE* fp, bitmap* bmp) {
	// TODO: get the dib header to local pointer variable

	switch (bmp->header_type) {
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

int read_bitmap_header_info(FILE* fp, bitmap* bmp, int* offset) {
	bitmap_header_info* h = &(bmp->header_info);
	int ret_code = 0;
	ret_code = fread_by_offset(&h->header_field, sizeof(char), 2, *offset, fp);
	if (ret_code != 2) {
		// TODO: return the reason of failure
		return 1;
	}
	*offset += ret_code;

	if (fpread(&h->size, sizeof(uint32_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint32_t);

	if (fpread(&h->reserved1, sizeof(uint16_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint16_t);

	if (fpread(&h->reserved2, sizeof(uint16_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint16_t);

	if (fpread(&h->pixel_start_offset, sizeof(uint32_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint32_t);

	return 0;
}

int read_bitmap_core_header(FILE* fp, bitmap* bmp, int* offset) {
	bitmap_v5_info_header* h = &bmp->dib_header;

	if (fpread(&h->width, sizeof(uint16_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint16_t);

	if (fpread(&h->height, sizeof(uint16_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint16_t);

	if (fpread(&h->num_color_planes, sizeof(uint16_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint16_t);
	// TODO: The number of color plane must be 1
	/*
	if (h->num_color_planes != 1) {
		return 1;
	}
	*/

	if (fpread(&h->bits_per_pixel, sizeof(uint16_t), 1, *offset, fp) == 0) return 1;
	*offset += sizeof(uint16_t);

	return 0;
}

/**
* Read BITMAP_INFO_HEADER
* Start offset of file: 14, total expected header size: 40 bytes
*/
int read_bitmap_info_header(FILE* fp, bitmap* bmp, int* offset) {
	bitmap_v5_info_header* h = &bmp->dib_header;
	h->size = (uint32_t)bmp->header_type;

	// width (4)
	if (fpread(&h->width, sizeof(uint32_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint32_t);

	// height (4)
	if (fpread(&h->height, sizeof(uint32_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint32_t);

	// num_color_planes (2)
	if (fpread(&h->num_color_planes, sizeof(uint16_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint16_t);

	// bits_per_pixel (2)
	if (fpread(&h->bits_per_pixel, sizeof(uint16_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint16_t);

	// compression_method (4)
	if (fpread(&h->compression_method, sizeof(bitmap_compression_method), *offset, fp) == 0) return 1;
	*offset += sizeof(bitmap_compression_method);

	// image_size (4)
	if (fpread(&h->image_size, sizeof(uint32_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint32_t);

	// horizontal_resolution (4)
	if (fpread(&h->horizontal_resolution, sizeof(int32_t), *offset, fp) == 0) return 1;
	*offset += sizeof(int32_t);

	// vertical_resolution (4)
	if (fpread(&h->vertical_resolution, sizeof(int32_t), *offset, fp) == 0) return 1;
	*offset += sizeof(int32_t);

	// color_palette (4)
	if (fpread(&h->color_palette, sizeof(uint32_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint32_t);
	if (h->color_palette > 0) {
		// TODO: Prepare to read color table 
		printf("[DEBUG] this bmp has a non-zero color_palette, prepare to read its color table \n");
	}

	// num_important_colors (4)
	if (fpread(&h->num_important_colors, sizeof(uint32_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint32_t);

	return 0;
}

int read_bitmap_v2_info_header(FILE* fp, bitmap* bmp, int* offset) {
	if (read_bitmap_info_header(fp, bmp, offset) != 0) {
		// TODO: failed to read bitmap v1 info header
		return 1;
	}

	bitmap_v5_info_header* h = &bmp->dib_header;

	// red_bit_mask (4)
	if (fpread(&h->red_bit_mask, sizeof(uint32_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint32_t);

	// green_bit_mask (4)
	if (fpread(&h->green_bit_mask, sizeof(uint32_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint32_t);

	// blue_bit_mask (4)
	if (fpread(&h->blue_bit_mask, sizeof(uint32_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint32_t);

	return 0;
}

int read_bitmap_v3_info_header(FILE* fp, bitmap* bmp, int* offset) {
	if (read_bitmap_v2_info_header(fp, bmp, offset) != 0) {
		// TODO: failed to read bitmap v1 info header
		return 1;
	}

	bitmap_v5_info_header* h = &bmp->dib_header;

	// alpha_bit_mask (4)
	if (fpread(&h->alpha_bit_mask, sizeof(uint32_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint32_t);

	return 0;
}

int read_bitmap_v4_info_header(FILE* fp, bitmap* bmp, int* offset) {
	if (read_bitmap_v3_info_header(fp, bmp, offset) != 0) {
		// TODO: failed to read bitmap v1 info header
		return 1;
	}

	bitmap_v5_info_header* h = &bmp->dib_header;

	// color_space_type (4)
	if (fpread(&h->color_space_type, sizeof(uint32_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint32_t);

	// color_space_endpoints (36)
	if (fpread(&h->color_space_endpoints, sizeof(cie_xyz_triple), *offset, fp) == 0) return 1;
	*offset += sizeof(cie_xyz_triple);

	// gamma_red (4)
	if (fpread(&h->gamma_red, sizeof(uint32_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint32_t);

	// gamma_green (4)
	if (fpread(&h->gamma_green, sizeof(uint32_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint32_t);

	// gamma_blue (4)
	if (fpread(&h->gamma_blue, sizeof(uint32_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint32_t);

	return 0;
}

int read_bitmap_v5_info_header(FILE* fp, bitmap* bmp, int* offset) {
	if (read_bitmap_v4_info_header(fp, bmp, offset) != 0) {
		// TODO: failed to read bitmap v1 info header
		return 1;
	}

	bitmap_v5_info_header* h = &bmp->dib_header;

	// intent (4)
	if (fpread(&h->intent, sizeof(uint32_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint32_t);

	// icc_profile_data (4)
	if (fpread(&h->icc_profile_data, sizeof(uint32_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint32_t);
	// TODO: If bigger then zero, prepare to read icc_profile_data as icc_profile_size

	// icc_profile_size (4)
	if (fpread(&h->icc_profile_size, sizeof(uint32_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint32_t);

	// reserved (4)
	if (fpread(&h->reserved, sizeof(uint32_t), *offset, fp) == 0) return 1;
	*offset += sizeof(uint32_t);

	// TODO: Before read pixels, know its pixel format. 
	// bits_per_pixel, compression and RGBA masks are the factors which determine the pixel format. 
	printf("[DEBUG] read_bitmap_v5_info_header determine its pixel format [ bits_per_pixel: %u compression: %d, RGBA masks: %u %u %u %u ] \n",
		h->bits_per_pixel, h->compression_method, h->red_bit_mask, h->green_bit_mask, h->blue_bit_mask, h->alpha_bit_mask);

	switch (h->compression_method) {
	case BI_RGB: {
		// bits per pixel: 24 = 3 bytes
		// There is no compression
		bmp->pixel_data = (uint8_t**)malloc(h->height * sizeof(uint8_t*));
		uint8_t rgb_masks[3];
		rgb_masks[0] = (h->red_bit_mask >> 16) & 0xFF;
		rgb_masks[1] = (h->green_bit_mask >> 8) & 0xFF;
		rgb_masks[2] = (h->blue_bit_mask) & 0xFF;

		// int padding = 0;
		// Read 1st row
		for (int i = h->height - 1; i > 0; --i) {
			int row_size = h->width * (h->bits_per_pixel / 8); // bytes
			uint8_t* row = (uint8_t*)malloc(row_size); // 1bytes * row_size
			// |<--------------------width------------------->|-------|
			// |Pixel[0, h-1]|Pixel[1, h-1]...|Pixel[w-1, h-1]|Padding|
			int ret_code = fread_by_offset(row, sizeof(uint8_t), row_size, *offset, fp);
			if (ret_code != row_size) {
				printf("[ERROR] read pixel data failed read size = %d offset: %d \n", ret_code, *offset);
				return 1;
			}
			else {
				// padding = (row_size % 4); // think it's not necessary
				*offset += (row_size + sizeof(uint32_t));

				// printf("[DEBUG] before masking row: %02X %02X %02X \n", row[0], row[1], row[2]);
				// 0..255 &mpersand with red_bit_mask
				for (int j = 0; j < row_size; j += 3) {
					row[j + 0] = row[j + 0] & rgb_masks[0]; // R
					row[j + 1] = row[j + 1] & rgb_masks[1]; // G
					row[j + 2] = row[j + 2] & rgb_masks[2]; // B
				}
				// printf("[DEBUG] after masking row: %02X %02X %02X \n", row[0], row[1], row[2]);

				((uint8_t**)bmp->pixel_data)[i] = row;
			}
		}
		// Deduction
		break;
	}
	case BI_RLE8: {
		// https://en.wikipedia.org/wiki/Run-length_encoding
		// RLE 8-bit/pixel
		// TODO: Carry on the following steps:
		//  1. Read 
		break;
	}
	case BI_RLE4: {
		// RLE 4-bit/pixel
		break;
	}
	case BI_BITFIELDS: {
		// BITMAP_V2_INFO_HEADER: RGB bit fields mask
		// BITMAP_V3_INFO_HEADER+: RGBA
		break;
	}
	case BI_JPEG: {
		// RLE-24 BITMAP_V4_INFO_HEADER+: JPEG image for printing
		break;
	}
	case BI_PNG: {
		// BITMAP_V4_INFO_HEADER+: PNG image for printing
		break;
	}
	case BI_ALPHABITFIELDS: {
		// Only Windows CE 5.0 with .NET 4.0 or later
		// RGBA bit field masks
		break;
	}
	case BI_CMYK: {
		// Only Windos Metafile CMYK
		// None
		break;
	}
	case BI_CMYKRLE8: {
		// Only Windows Metafile CMYK
		// RLE-8
		break;
	}
	case BI_CMYKRLE4: {
		// Only Windows Metafile CMYK
		// RLE-4
		break;
	}
	default: break;
	}
	return 0;
}

/// <summary>
/// Read bitmap Device-Independent Bitmap(DIB) header.
/// Not supported header type: BITMAP_CORE_HEADER, OS22X_BITMAP_HEADER_2, OS22X_BITMAP_HEADER_SMALL return -1
/// Supported header type: BITMAP_INFO_HEADER, V2~V5 INFO HEADER return 0
/// If error occurred, return 1
/// </summary>
/// <param name="fp">File pointer</param>
/// <param name="bmp">Bitmap pointer</param>
/// <param name="offset">File offset</param>
/// <returns></returns>
int read_bitmap_dib_header(FILE* fp, bitmap* bmp, int* offset) {
	int result = -1;
	switch (bmp->header_type) {
	case BITMAP_CORE_HEADER: {
		// Not supported
		break;
	}
	case OS22X_BITMAP_HEADER_2: {
		// Not supported
		break;
	}
	case OS22X_BITMAP_HEADER_SMALL: {
		// Not supported
		break;
	}
	case BITMAP_INFO_HEADER: {
		result = read_bitmap_info_header(fp, bmp, offset);
		break;
	}
	case BITMAP_V2_INFO_HEADER: {
		result = read_bitmap_v2_info_header(fp, bmp, offset);
		break;
	}
	case BITMAP_V3_INFO_HEADER: {
		result = read_bitmap_v3_info_header(fp, bmp, offset);
		break;
	}
	case BITMAP_V4_INFO_HEADER: {
		result = read_bitmap_v4_info_header(fp, bmp, offset);
		break;
	}
	case BITMAP_V5_INFO_HEADER: {
		result = read_bitmap_v5_info_header(fp, bmp, offset);
		break;
	}
	default: {
		break;
	}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////
/// End of Read Headers 
//////////////////////////////////////////////////////////////////////

int read_bitmap(FILE* fp, bitmap* bmp) 
{
	int offset = 0;

	if (read_bitmap_header_info(fp, bmp, &offset) != 0) 
	{
		printf("[ERROR] failed to read bitmap header info \n");
		return 1;
	}

	if (fpread(&bmp->header_type, sizeof(uint32_t), offset, fp) == 0) {
		printf("[ERROR] failed to read dib header size \n");
		return 1;
	}
	offset += sizeof(uint32_t);

	if (read_bitmap_dib_header(fp, bmp, &offset) != 0) {
		printf("[ERROR] failed tp read dib_header \n");
		return 1;
	}
	
	// TODO: Read pixels

	return 0;
}


//int main(int argc, char** argv) {
//	printf("[DEBUG] program initiated .. \n");
//
//	if (argc != 2) {
//		printf("[ERROR] no file to read \n");
//		return 1;
//	}
//	else {
//		printf("[DEBUG] filepath: %s \n", argv[1]);
//	}
//
//	FILE* fp = fopen(argv[1], "rb");
//	if (fp == NULL) {
//		printf("[ERROR] failed to open file \n");
//		return 1;
//	}
//
//	bitmap bmp;
//	int file_offset = 0;
//
//	// TODO: Read bitmap header info
//	int ret_code = read_bitmap_header_info(fp, &bmp, &file_offset);
//	if (ret_code != 0) {
//		printf("[ERROR] failed to read bitmap_header_info \n");
//		fclose(fp);
//		return 1;
//	}
//	else {
//		printf("[DEBUG] succeed to read bitmap_header_info size: %u \n", bmp.header_info.size);
//	}
//
//	if (fpread(&bmp.header_type, sizeof(uint32_t), file_offset, fp) == 0) {
//		printf("[ERROR] failed to read dib_header_size \n");
//		fclose(fp);
//		return 1;
//	}
//	file_offset += sizeof(uint32_t);
//
//	ret_code = read_bitmap_dib_header(fp, &bmp, &file_offset);
//	if (ret_code != 0) {
//		printf("[ERROR] failed tp read dib_header \n");
//		fclose(fp);
//		return 1;
//	}
//
//	// TODO: Read Pixels
//	// So far, reading pixels done. 
//
//	// TODO: Render to screen
//
//	// TODO: Release all resources
//
//	// TODO: Exit the program gracefully
//
//	printf("[DEBUG] program finished .. \n");
//	return 0;
//}