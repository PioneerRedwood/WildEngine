#include "Video.h"
#include <stdio.h>

Video::~Video() {
	if (m_framePixelOffsets != nullptr) {
		free(m_framePixelOffsets);
	}
}

/// <summary>
/// 경과시간을 바탕으로 현재 프레임 아이디를 가져옴
/// </summary>
/// <param name="elapsed">밀리초 단위 경과 시간</param>
/// <returns></returns>
int Video::getCurrentFrameIDByElapsed(uint64_t elapsed) const {
	int frameId = (int)(elapsed * m_indexUnit);
	frameId %= m_header.frameCount;
	return frameId;
}

/// <summary>
/// 입력 받은 파일 경로의 비디오 파일 읽기
/// </summary>
/// <param name="path">입력 파일 경로</param>
/// <returns>성공 시 true, 그렇지 않으면 false</returns>
bool Video::readVideoFromFile(const char* path) {
	// 파일 포인터 생성
	errno_t err = fopen_s(&m_fp, path, "rb");
	if (err != 0) {
		// 파일 읽기 형식으로 열기 실패
		return false;
	}

	// 파일의 끝자락에서 헤더 정보 읽기
	int headerStartOffset = (-1) * sizeof(VideoHeader);
	fseek(m_fp, headerStartOffset, SEEK_END);
	fread(&m_header, sizeof(VideoHeader), 1, m_fp);
	fseek(m_fp, 0, SEEK_SET);

	// 전체 프레임을 순회하면서 각 프레임의 오프셋을 저장
	m_framePixelOffsets = (uint64_t*)malloc(sizeof(uint64_t) * m_header.frameCount);
	if (m_framePixelOffsets == nullptr) {
		// 프레임 픽셀 오프셋 저장할 메모리 할당 실패
		return false;
	}

	// 프레임 크기
	int stride = ((m_header.width * 3 + 3) & ~3);
	uint64_t frameSize = (uint64_t)(stride * m_header.height);
	for (uint64_t i = 0; i < m_header.frameCount; ++i) {
		m_framePixelOffsets[ i ] = (uint64_t)(i * frameSize);
	}

	//m_indexUnit = (float)1 / 1000; // 느리게
	//m_indexUnit = (float)0.1 / 1000; // 많이 느리게
	m_indexUnit = (float)m_header.fps / 1000;

	// 성공 시 true 반환
	return true;
}

/// <summary>
/// 입력 비디오의 특정 프레임의 픽셀 데이터를 읽어오기
/// </summary>
/// <param name="frameId">픽셀 데이터를 구할 프레임 아이디</param>
/// <param name="out">특정 프레임의 픽셀 데이터</param>
/// <returns>성공 시 true, 그렇지 않으면 false</returns>
bool Video::readFrame(uint32_t frameId, uint8_t* out) {
	if (out == nullptr) {
		SDL_assert(out != nullptr);
		return false;
	}

	uint64_t offset = m_framePixelOffsets[ frameId ];

	printf("Video::readFrame %u - offset %llu \n", frameId, offset);

	fseek(m_fp, offset, SEEK_SET);
	int stride = ((m_header.width * 3 + 3) & ~3);
	uint64_t frameSize = (uint64_t)(stride * m_header.height);
	fread(out, frameSize, 1, m_fp);

	return true;
}
