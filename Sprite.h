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
	Sprite() = default;
	
	~Sprite();

	FILE* file() {
		return m_fp;
	}

	bool readFromFile(const char* path);

private:
	FILE* m_fp = {};

	SpriteHeader m_header = {};
};