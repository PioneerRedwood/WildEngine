#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <stdbool.h>
#include <string.h>

// 비트맵 로드에 필요한 구조체 선언
#pragma pack(push, 1)
typedef struct {
	char header_field[2];
	uint32_t size;
	uint16_t reserved1, reserved2;
	uint32_t pixel_start_offset;
} bitmap_header_info;
#pragma pack(pop)

typedef struct {
	uint64_t size;
	uint32_t total_frame_count;
	uint32_t fps;
} movie_header;

#pragma pack(push, 1)
typedef struct {
	uint64_t pixel_data_offset;
	uint32_t index;
	uint32_t width;
	uint32_t height;
} frame_header;
#pragma pack(pop)

typedef struct {
	frame_header header;
	uint8_t* pixel_data;
} frame;

typedef struct {
	movie_header header;
	frame* frames;
} movie;

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
} bitmap_header; // 40 bytes

typedef struct {
	bitmap_header_info header_info;
	bitmap_header header;
	uint8_t* pixel_data;
} bitmap; // 58bytes

typedef enum {
	SUCCESS = 0,
	INVALID_PATH = 1,
	FILE_NOT_FOUND = 2,
	ALLOCATION_FAILED = 3,
} LoadResult;

int LoadBitmap(bitmap* bmp, const char* path) {
	// TODO: 파일 읽기 형식으로 열기
	if (path == NULL) { return INVALID_PATH; }

	FILE* fp = fopen(path, "rb");
	if (fp == NULL) {
		printf("잘못된 파일 경로: %s 파일을 열 수 없습니다. \n", path);
		return FILE_NOT_FOUND;
	}

	// TODO: 헤더 정보 읽기
	fread(&bmp->header_info, 1, sizeof(bitmap_header_info), fp);

	// TODO: 헤더 읽기
	fread(&bmp->header, 1, sizeof(bitmap_header), fp);

	// TODO: 데이터 읽기
	fseek(fp, bmp->header_info.pixel_start_offset, SEEK_SET);

	int stride = ((bmp->header.width * 3 + 3) & ~3);
	int size = stride * bmp->header.height;

	bmp->pixel_data = (uint8_t*)malloc(size);
	if (bmp->pixel_data == NULL) {
		printf("비트맵 로드에 필요한 데이터 할당에 실패했습니다. \n");
		return ALLOCATION_FAILED;
	}
	memset(bmp->pixel_data, 0, size);
	fread(bmp->pixel_data, 1, size, fp);

	fclose(fp);

	return SUCCESS;
}

#define BUF_SIZE 255

//#define BMP_PREFIX "../resources/test/ironman/"
//#define OUTFILE "../resources/bms/ironman.bm"
//#define BMP_PREFIX "../resources/small/"
//#define OUTFILE "../resources/bms/small.bm"
//#define BMP_PREFIX "../resources/random/"
//#define OUTFILE "../resources/bms/random.bm"
#define BMP_PREFIX "../resources/dresden/"
#define OUTFILE "../resources/bms/dresden.bm"

#define FPS 30

// 헤더가 꼭 앞에 있을 필요는 없음 - 발상의 전환
// 한 프레임씩 저장 
// 메모리 많이 쓰지 말기

int main(int argc, char** argv)
{
	// 출력 파일 포인터 생성
	FILE* fp = fopen(OUTFILE, "wb");
	if (fp == NULL) {
		printf("파일을 쓰기 형식으로 열 수 없습니다. \n");
		return 1;
	}

	movie mv = { 0 };
	
	int num_frames = 0;
	while(true) {
		char filepath[BUF_SIZE];
		snprintf(filepath, BUF_SIZE, "%s%d.bmp", BMP_PREFIX, num_frames + 1);

		bitmap bmp = { 0 };
		LoadResult result = LoadBitmap(&bmp, filepath);

		if (result != 0) {
			printf("비트맵 로드에 실패했습니다 [ ID: %d 파일: %s ] \n", num_frames, filepath);
			break;
		}
		else {
			// 현재 프레임에 대한 정보를 먼저 저장
			frame fr = { 0 };
			fr.header.pixel_data_offset = mv.header.size + sizeof(frame_header);
			fr.header.index = mv.header.total_frame_count;
			fr.header.width = bmp.header.width;
			fr.header.height = bmp.header.height;
			fr.pixel_data = bmp.pixel_data;

			// 성공 시 데이터 업데이트
			int frame_stride = ((fr.header.width * 3 + 3) & ~3);
			int num_pixel_bytes = (frame_stride * fr.header.height);
			mv.header.size += (sizeof(frame_header) + num_pixel_bytes);

			printf("비트맵 로드 성공 [ ID: %d 파일: %s 쌓은 크기: %llu ] \n", num_frames, filepath, mv.header.size);

			mv.header.total_frame_count++;

			// 프레임 정보 저장
			fwrite(&fr.header, sizeof(frame_header), 1, fp);
			fwrite(fr.pixel_data, num_pixel_bytes, 1, fp);

			free(bmp.pixel_data);
		}

		num_frames++;
	}

	// 파일 크기, 프레임 수, 초 당 프레임
	mv.header.size += sizeof(movie_header);
	mv.header.fps = FPS;

	// 파일 헤더 쓰기
	fwrite(&mv.header, sizeof(movie_header), 1, fp);

	fclose(fp);

	printf("bm 파일 쓰기 성공 [ 파일: %s 쌓은 크기: %llu ] \n", OUTFILE, mv.header.size);

	return 0;
}