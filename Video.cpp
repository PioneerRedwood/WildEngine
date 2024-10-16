#include "Video.h"
#include <stdio.h>
#include <memory>
#include <iostream>

namespace {
	constexpr auto BytesPerPixel = 3; // BI_RGB Pixel bytes;
}

Video::~Video() {
}

void Video::startTime(uint64_t time) {
  if (m_startTime == 0) {
    m_startTime = time;
  }
}

int Video::frameSize() const {
  //return m_stride * m_header.height;
  return m_rowSize * m_header.height;
}

/// <summary>
/// 경과시간을 바탕으로 현재 프레임 아이디를 가져옴
/// </summary>
/// <param name="elapsed">밀리초 단위 경과 시간</param>
/// <returns></returns>
int Video::getCurrentFrameIDByElapsed(double elapsed) const {
	int frameId = (int)(elapsed * m_indexUnit);
	frameId %= m_header.totalFrame;
	return frameId;
}

/// <summary>
/// 입력 받은 파일 경로의 비디오 파일 읽기
/// </summary>
/// <param name="path">입력 파일 경로</param>
/// <returns>성공 시 true, 그렇지 않으면 false</returns>
bool Video::readVideoFromFile(const char* path) {
	std::cout << "Video::readVideoFromFile started filepath:" << path << std::endl;
	// 파일 포인터 생성
	errno_t err = fopen_s(&m_fp, path, "rb");
	if (err != 0) {
		// 파일 읽기 형식으로 열기 실패
		return false;
	}

	// 파일의 끝자락에서 헤더 정보 읽기
	int headerStartOffset = (-1) * (int)sizeof(VideoHeader);
	fseek(m_fp, headerStartOffset, SEEK_END);
	fread(&m_header, sizeof(VideoHeader), 1, m_fp);
	fseek(m_fp, 0, SEEK_SET);

	// 프레임 크기
	m_rowSize = (std::size_t)m_header.width * BytesPerPixel;

	//m_indexUnit = (float)1 / 1000; // 느리게
	//m_indexUnit = (float)0.1 / 1000; // 많이 느리게
	m_indexUnit = (float)m_header.fps / 1000;

	std::cout << "Video::readVideoFromFile finished filepath:" << path << std::endl;

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

	//printf("Video::readFrame %u - offset %llu \n", frameId, offset);

	const uint64_t frameSize = (uint64_t)(m_rowSize * m_header.height);
	long offset = frameSize * frameId;
	fseek(m_fp, offset, SEEK_SET);
	fread(out, frameSize, 1, m_fp);
	
	return true;
}
