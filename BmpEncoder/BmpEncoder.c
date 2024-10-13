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


// 새로운 구조의 BitmapMovie 선언
// 입력: 1, 2, 3, ... n.bmp 디렉토리 경로. 
// 출력: videos/아래에 새로운 Video 파일 쓰기. 
// 시나리오: 비트맵을 읽을 수 없을 때까지 계속 로드, 동시에 픽셀데이터를 파일에 저장. 
// 마지막 프레임을 저장하고 난 뒤 헤더 작성. 헤더에는 프레임 수, 프레임 크기, 초당 프레임 수

typedef struct {
	uint32_t totalFrame;
	uint32_t width;
	uint32_t height;
	uint32_t fps;
} VideoHeader;

uint8_t* pixelData = NULL;
bool isFirstLoading = true;
int realPixelSize = 0, rowSize = 0;
VideoHeader header = { 0 };

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
#if 0
	int size = stride * bmp->header.height;

	// TODO: 읽은 픽셀 데이터 상하 반전 위한 메모리 할당
	if (tempMemory == NULL) {
		tempMemory = (uint8_t*)malloc(size);
		if (tempMemory == NULL) {
			printf("상하반전에 필요한 데이터 할당에 실패했습니다.\n");
			return ALLOCATION_FAILED;
		}
	}
	fread(tempMemory, 1, size, fp);

	bmp->pixel_data = (uint8_t*)malloc(size);
	if (bmp->pixel_data == NULL) {
		printf("비트맵 로드에 필요한 데이터 할당에 실패했습니다. \n");
		return ALLOCATION_FAILED;
	}

	// TODO: 상하반전 메모리 복사
	for (int y = 0; y < bmp->header.height; ++y) {
		memcpy(
			bmp->pixel_data + (y * stride),
			tempMemory + ((bmp->header.height - 1 - y) * stride),
			stride);
	}
#else
	int bbp = bmp->header.bits_per_pixel / 8;
	rowSize = bmp->header.width * bbp;
	int padding = stride - rowSize;

	if (isFirstLoading) {
		rowSize = bmp->header.width * (bmp->header.bits_per_pixel / 8);
		realPixelSize = rowSize * bmp->header.height;

		pixelData = (uint8_t*)malloc(realPixelSize);
		header.width = bmp->header.width;
		header.height = bmp->header.height;
		isFirstLoading = false;
	}

	if (pixelData == NULL) {
		printf("LoadBitmap - Allocating PixelData failed \n");
		return ALLOCATION_FAILED;
	}

	// TODO: 읽은 픽셀 데이터 상하 반전 위해 메모리 반대로 할당
	unsigned long offset = 0;
	for (int y = bmp->header.height - 1; y >= 0; --y) {
		offset = y * rowSize;
		fread(pixelData + offset, rowSize, 1, fp);
		fseek(fp, padding, SEEK_CUR);
	}

#endif

	fclose(fp);

	printf("LoadBitmap %s success!\n", path);

	return SUCCESS;
}

//#define OUTFILE "../resources/videos/castle.mv"
//#define IN_DIR "../resources/test/castle/"

//#define OUTFILE "../resources/videos/american-town.mv"
//#define OUTFILE "../resources/videos/american-town3.mv"
//#define IN_DIR "../resources/test/american-town/"

//#define OUTFILE "../resources/videos/dresden.mv"
//#define IN_DIR "../resources/dresden/"

//#define OUTFILE "../resources/videos/medium.mv"
//#define IN_DIR "../resources/medium/"

#define OUTFILE "../resources/videos/small.mv"
#define IN_DIR "../resources/small/"

#define FPS 12

int main(int argc, char** argv) {
	FILE* fp = fopen(OUTFILE, "wb");
	if(fp == NULL) { printf("Cannot open file %s \n", OUTFILE); return 1; }

	int i = 1;

	while(true) {
		char filepathBuf[256];
		snprintf(filepathBuf, 256, "%s%d.bmp", IN_DIR, i);

		bitmap bmp = { 0 };
		if(LoadBitmap(&bmp, filepathBuf) == 0) {
			// success
			fwrite(pixelData, realPixelSize, 1, fp);
			header.frameCount++;
		} else {
			// failure
			break;
		}

		++i;
	}
	if (pixelData != NULL) {
		free(pixelData);
	}

	// 헤더 파일에 쓰기
	header.fps = FPS;
	fwrite(&header, sizeof(VideoHeader), 1, fp);

	fclose(fp);
	return 0;
}
