#pragma once
#include <cstdio>
#include <SDL.h>

struct SpriteHeader {
	uint32_t totalCount;
	uint32_t width;
	uint32_t height;
};

class Sprite {
public:
	// default fps is 12
	Sprite(int fps = 12);
	
	~Sprite();

	FILE* file() {
		return m_fp;
	}

	bool readFromFile(const char* path);

	bool prepareFrame(double delta);

	bool readFrame(int index, uint8_t* out);

private:
	FILE* m_fp = {};

	SpriteHeader m_header = {};

	uint8_t* m_frameData = {};

	int m_rowSize = { 0 };

	int m_fps = { 0 };
	
	int m_currentFrameIndex = { 0 };

	double m_elapsedTime = { 0.0 };

	double m_frameDuration = { 0.0 };
};