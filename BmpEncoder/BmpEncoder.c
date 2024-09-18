#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <stdbool.h>
#include <string.h>

// TODO: 비트맵 로드에 필요한 구조체 선언
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
small.bm - "C:\source\BitmapLoader\BitmapLoader\resources\small_1.bmp" "C:\source\BitmapLoader\BitmapLoader\resources\small_2.bmp" "C:\source\BitmapLoader\BitmapLoader\resources\small_3.bmp" "-o" "small.bm"
dresden-clock.bm - "C:\source\BitmapLoader\BitmapLoader\resources\dresden-museum-clock\clock01.bmp" "C:\source\BitmapLoader\BitmapLoader\resources\dresden-museum-clock\clock02.bmp" "C:\source\BitmapLoader\BitmapLoader\resources\dresden-museum-clock\clock03.bmp" "C:\source\BitmapLoader\BitmapLoader\resources\dresden-museum-clock\clock04.bmp" "C:\source\BitmapLoader\BitmapLoader\resources\dresden-museum-clock\clock05.bmp" "C:\source\BitmapLoader\BitmapLoader\resources\dresden-museum-clock\clock06.bmp" "C:\source\BitmapLoader\BitmapLoader\resources\dresden-museum-clock\clock07.bmp" "C:\source\BitmapLoader\BitmapLoader\resources\dresden-museum-clock\clock08.bmp" "C:\source\BitmapLoader\BitmapLoader\resources\dresden-museum-clock\clock09.bmp" "C:\source\BitmapLoader\BitmapLoader\resources\dresden-museum-clock\clock10.bmp" "C:\source\BitmapLoader\BitmapLoader\resources\dresden-museum-clock\clock11.bmp" "-o" "dresden-clock.bm"
// TODO: 사이즈가 다른 비트맵 로드할 것 (홀수 짝수)
// TODO: 사이즈가 엄청 큰 것도 해볼 것 엄청 많은 양의 비트맵도 시도해볼것 (300MB이상)
*/

int main(int argc, char** argv)
{
	// TODO: 입력 인자 갯수 확인
	if (argc < 2) {
		printf("잘못된 사용: 최소 한개의 비트맵 파일 경로를 입력하시오. 입력된 인자수 : %d \n", argc);
		return 1;
	}

	// TODO: 입력 인자 중에서 출력 파일이 지정되지 않은 경우 종료
	// -o 이후의 바로 다음의 입력 문자열은 파일 경로이며, 그 이후의 인자는 무시됨.
	const char* outfile = NULL;
	int num_frames = 0;
	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-o") == 0 && ((i + 1) < argc)) {
			outfile = argv[i + 1];
			num_frames = i - 1;
			break;
		}
	}

	if (outfile == NULL) {
		printf("잘못된 입력: -o 인자 뒤에 출력할 파일 경로를 입력하십시오. \n");
		return 1;
	}

	// TODO: 출력 파일 포인터 생성
	FILE* fp = fopen(outfile, "wb");
	if (fp == NULL) {
		printf("파일을 쓰기 형식으로 열 수 없습니다. \n");
		return 1;
	}

	// TODO: 비트맵 정보 저장할 프레임 배열 생성
	movie mv = { 0 };
	mv.frames = (frame*)malloc(num_frames * sizeof(frame));
	if (mv.frames == NULL) { return 1; }
	memset(mv.frames, 0, num_frames * sizeof(frame));

	// 파일 크기, 프레임 수, 초 당 프레임
	mv.header.size += sizeof(uint32_t) * 3;
	mv.header.fps = 3;

	// TODO: 입력받은 파일 수만큼 루프
	int idx = 0;
	while (idx < num_frames) {
		const char* filepath = argv[idx + 1];
		bitmap bmp = { 0 };
		bool result = LoadBitmap(&bmp, filepath);

		// TODO: 로드한 비트맵 정보 옮기기
		frame* fr = &mv.frames[idx];
		if (result) {
			fr->header.width = bmp.header.width;
			fr->header.height = bmp.header.height;
			fr->pixel_data = bmp.pixel_data;

			// TODD: 성공적으로 로드한 비트맵이라면 movie 파일 업데이트
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

	// TODO: 파일 헤더 쓰기
	fwrite(&mv.header, sizeof(movie_header), 1, fp);
	
	// TODO: 루프 돌면서 프레임 정보 파일에 쓰기
	idx = 0;
	while (idx < num_frames) {
		frame* fr = &mv.frames[idx];
		if (fr != NULL && fr->pixel_data) {
			// TODO: 프레임 헤더 저장
			fwrite(&fr->header, sizeof(frame_header), 1, fp);

			// TODO: 프레임 바디 저장
			int frame_stride = ((fr->header.width * 3 + 3) & ~3);
			int num_pixel_bytes = frame_stride * fr->header.height;
			fwrite(fr->pixel_data, num_pixel_bytes, 1, fp);
		}
		idx++;
	}

	fclose(fp);

	return 0;
}