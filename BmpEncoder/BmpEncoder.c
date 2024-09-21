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

bool LoadBitmap(bitmap* bmp, const char* path) {
	// TODO: 파일 읽기 형식으로 열기
	if (path == NULL) { return false; }

	FILE* fp = fopen(path, "rb");
	if (fp == NULL) {
		printf("잘못된 파일 경로: %s 파일을 열 수 없습니다. \n", path);
		return false;
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
		return false;
	}
	memset(bmp->pixel_data, 0, size);
	fread(bmp->pixel_data, 1, size, fp);

	fclose(fp);

	return true;
}

/*
입력 인자 예시
small.bm - 

dresden-clock.bm 
- PREFIX: "../resources/dresden/"
- OUTFILE: "bms/dresden.bm"
- NUM_FRAMES 11
- FPS 11

// 사이즈가 다양한 비트맵
random.bm
- PREFIX: "../resources/random/"
- OUTPUT: "bms/random.bm"
- NUM_FRMES 5
- FPS 5

// 사이즈가 큰 것, 많은 양의 비트맵도 시도해볼것
castle3.bm
- PREFIX: "../resources/test/castle3/"
- OUTPUT: "bms/castle3.bm"
- NUM_FRAMES 5400
- FPS 30

*/

#define BUF_SIZE 255
#define BMP_PREFIX "../resources/test/castle4/"
#define OUTFILE "bms/castle4.bm"
#define NUM_FRAMES 5400
#define FPS 60
#define LOAD_HUGE_FILES true

char** file_names = NULL;

bool LoadFileNames() {
	const int num_files = NUM_FRAMES;
	file_names = (char**)malloc(num_files * sizeof(char*));
	if (file_names == NULL) {
		printf("파일 이름을 담기 위한 배열 할당 실패\n");
		return false;
	}
	memset(file_names, 0, num_files * sizeof(char*));

	char buf[BUF_SIZE];

	for (int i = 0; i < num_files; ++i) {
		memset(buf, 0, BUF_SIZE);
		snprintf(buf, BUF_SIZE, "%s%d.bmp", BMP_PREFIX, i + 1);

		file_names[i] = (char*)malloc(BUF_SIZE + 1);
		if (file_names[i] == NULL) {
			printf("파일 이름 동적 할당 실패\n");

			// 중복 해제 방지
			for (int j = 0; j < i; ++j) {
				free(file_names[j]);
				file_names[j] = NULL;
			}
			free(file_names);
			return false;
		}
		strcpy_s(file_names[i], BUF_SIZE, buf);

		printf("파일 이름: %s\n", file_names[i]);
	}

	return true;
}

int main(int argc, char** argv)
{
	const char* outfile = OUTFILE;
	int num_frames = NUM_FRAMES;
	if (!LoadFileNames()) {
		return 1;
	}	

	// 출력 파일 포인터 생성
	FILE* fp = fopen(outfile, "wb");
	if (fp == NULL) {
		printf("파일을 쓰기 형식으로 열 수 없습니다. \n");
		return 1;
	}

	// 비트맵 정보 저장할 프레임 배열 생성
	movie mv = { 0 };
	mv.frames = (frame*)malloc(num_frames * sizeof(frame));
	if (mv.frames == NULL) { return 1; }
	memset(mv.frames, 0, num_frames * sizeof(frame));

	// 파일 크기, 프레임 수, 초 당 프레임
	mv.header.size += sizeof(uint32_t) * 3;
	mv.header.fps = FPS;

	int idx = 0;
	while (idx < num_frames) {
		const char* filepath = file_names[idx];
		bitmap bmp = { 0 };
		bool result = LoadBitmap(&bmp, filepath);

		// 로드한 비트맵 정보 옮기기
		frame* fr = &mv.frames[idx];
		if (result) {
			fr->header.width = bmp.header.width;
			fr->header.height = bmp.header.height;
			fr->pixel_data = bmp.pixel_data;

			// 성공 시 데이터 업데이트
			int frame_stride = ((fr->header.width * 3 + 3) & ~3);
			int num_pixel_bytes = frame_stride * fr->header.height;
			mv.header.size += (uint16_t)sizeof(frame_header) + num_pixel_bytes;
			printf("비트맵 로드 성공[ %d / %d ]. 현재 파일: %s - 쌓은 크기: %d \n", idx + 1, num_frames, filepath, mv.header.size);

			mv.header.total_frame_count++;
		}
		else {
			fr->pixel_data = NULL;
			printf("비트맵 로드에 실패했습니다 %s \n", filepath);
		}
		idx++;
	}

	// 파일 헤더 쓰기
	fwrite(&mv.header, sizeof(movie_header), 1, fp);

	// 루프 돌면서 프레임 정보 파일에 쓰기
	idx = 0;
	while (idx < num_frames) {
		frame* fr = &mv.frames[idx];
		if (fr != NULL && fr->pixel_data) {
			// 프레임 헤더 저장
			fwrite(&fr->header, sizeof(frame_header), 1, fp);

			// 프레임 바디 저장
			int frame_stride = ((fr->header.width * 3 + 3) & ~3);
			int num_pixel_bytes = frame_stride * fr->header.height;
			fwrite(fr->pixel_data, num_pixel_bytes, 1, fp);
		}
		idx++;
	}
	fclose(fp);

	// TODO: 할당한 파일 이름 메모리 해제
	for (int i = 0; i < num_frames; ++i) {
		if (file_names[i] != NULL) {
			free(file_names[i]);
			file_names[i] = NULL;
		}
	}
	free(file_names);
	file_names = NULL;

	return 0;
}