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

// 상하반전을 위한 임시 픽셀 데이터 포인터
uint8_t* tempMemory = NULL;

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

	fclose(fp);

	printf("LoadBitmap %s success!\n", path);

	return SUCCESS;
}

#if 0
#define BUF_SIZE 255

//#define BMP_PREFIX "../resources/test/ironman/"
//#define OUTFILE "../resources/bms/ironman.bm"
//#define BMP_PREFIX "../resources/small/"
//#define OUTFILE "../resources/bms/small.bm"
//#define BMP_PREFIX "../resources/random/"
//#define OUTFILE "../resources/bms/random.bm"
//#define BMP_PREFIX "../resources/dresden/"
//#define OUTFILE "../resources/bms/dresden.bm"
#define BMP_PREFIX "../resources/castle/"
#define OUTFILE "../resources/bms/castle.bm"

#define FPS 12

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

#else

// 새로운 구조의 BitmapMovie 선언
// 입력: 1, 2, 3, ... n.bmp 디렉토리 경로. 
// 출력: videos/아래에 새로운 Video 파일 쓰기. 
// 시나리오: 비트맵을 읽을 수 없을 때까지 계속 로드, 동시에 픽셀데이터를 파일에 저장. 
// 마지막 프레임을 저장하고 난 뒤 헤더 작성. 헤더에는 프레임 수, 프레임 크기, 초당 프레임 수, 프레임 별 픽셀 오프셋 저장
// 사실상 프레임의 픽셀 데이터 오프셋은 

typedef struct {
	uint32_t frameCount;
	uint32_t width;
	uint32_t height;
	uint32_t fps;
} VideoHeader;

#define OUTFILE "../resources/videos/castle.mv"
#define IN_DIR "../resources/test/castle/"

//#define OUTFILE "../resources/videos/dresden.mv"
//#define IN_DIR "../resources/dresden/"

//#define OUTFILE "../resources/videos/medium.mv"
//#define IN_DIR "../resources/medium/"

#define FPS 12

int main(int argc, char** argv) {
	FILE* fp = fopen(OUTFILE, "wb");
	if(fp == NULL) { printf("Cannot open file %s \n", OUTFILE); return 1; }

	VideoHeader header = { 0 };
	int i = 1;
	while(true) {
		char filepathBuf[256];
		snprintf(filepathBuf, 256, "%s%d.bmp", IN_DIR, i);

		bitmap bmp = { 0 };
		if(LoadBitmap(&bmp, filepathBuf) == 0) {
			// success
			int stride = ((bmp.header.width * 3 + 3) & ~3);
			int size = stride * bmp.header.height;
			fwrite(bmp.pixel_data, size, 1, fp); // 패딩 필요 없음
			free(bmp.pixel_data); // 자원낭비
			header.frameCount++;
			header.width = bmp.header.width;
			header.height = bmp.header.height;
		} else {
			// failure
			free(tempMemory);
			break;
		}

		++i;
	}

	// 헤더 파일에 쓰기
	header.fps = FPS;
	fwrite(&header, sizeof(VideoHeader), 1, fp);

	fclose(fp);
	return 0;
}

#endif