#include "Sprite.h"
#include <iostream>

namespace {
	constexpr auto BytesPerPixel = 3; // BI_RGB Pixel bytes;
}

Sprite::Sprite(int fps)
: m_fps(fps) {

}

Sprite::~Sprite() {

}

bool Sprite::readFromFile(const char* path) {
	//std::cout << "Sprite::readFromFile started path: " << path << std::endl;

	errno_t err = fopen_s(&m_fp, path, "rb");
	if (err != 0) {
		std::cout << "File open failed err: " << err << std::endl;
		return false;
	}

	int headerStartOffset = (-1) * (int)sizeof(SpriteHeader);
	fseek(m_fp, headerStartOffset, SEEK_END);
	fread(&m_header, sizeof(SpriteHeader), 1, m_fp);
	fseek(m_fp, 0, SEEK_SET);

	m_rowSize = m_header.width * BytesPerPixel;

	m_pixelData = (uint8_t*)malloc(m_rowSize * m_header.height);
	if (m_pixelData == nullptr) {
		std::cout << "malloc failed \n";
		return false;
	}

	//m_frameDuration = (double)m_fps / 1000;
	m_frameDuration = (double)500;
	
	//std::cout << "Sprite::readFromFile finished path: " << path << std::endl;

	return true;
}

bool Sprite::prepareFrame(double delta) {
	// 경과한 시간을 기반으로 그려야 할 프레임의 픽셀 데이터를 준비
	// 이전 프레임으로부터의 경과시간으로 현재 준비해야할 프레임을 결정할 수 있을까?
	// 만약 초당 10프레임이라 했을때 프레임당 100밀리초씩 사용..
	// 현재 프레임이 0일때 경과시간이 100 초과 200 미만인 경우 1번 프레임 사용

	// TODO: 다음 프레임을 계산하는 방식이 이상함 - 수정바람
	// 경과시간 누적
	m_elapsedTime += delta;

	// 프레임 변경이 필요한지, 경과시간이 프레임 지속시간보다 크면 변경해야 함
	if (m_elapsedTime >= m_frameDuration) {
		// 다음 프레임으로 이동
		m_currentFrameIndex = (m_currentFrameIndex + 1) % m_header.totalCount; // 루프
		//m_elapsedTime -= m_frameDuration; // 경과시간에서 프레임 지속시간을 빼기
		m_elapsedTime = 0; // 경과시간에서 프레임 지속시간을 빼기
	}

	std::cout << "Sprite::prepareFrame delta: " << delta
		<< " elapsed: " << m_elapsedTime
		<< " m_currentFrameIndex: " << m_currentFrameIndex << std::endl;

	if (not readFrame(m_currentFrameIndex, m_pixelData)) {
		std::cout << "Sprite::prepareFrame failed \n";
		return false;
	}

	return true;
}

bool Sprite::readFrame(int index, uint8_t* out) {
	if (out == nullptr) {
		SDL_assert(false);
		return false;
	}

	const uint64_t frameSize = (uint64_t)(m_rowSize * m_header.height);
	long offset = (frameSize) * index;
	fseek(m_fp, offset, SEEK_SET);
	fread(out, frameSize, 1, m_fp);

	return true;
}
