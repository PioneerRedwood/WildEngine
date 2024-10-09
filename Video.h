﻿#pragma once
#include <cstdio>   // FILE
#include <cstdint>  // uint family
#include <SDL.h>    // SDL_Texture*

struct Frame{
  uint32_t index;
  uint8_t* pixelData;
};

struct VideoHeader {
  uint32_t frameCount;
  uint32_t width;
  uint32_t height;
  uint32_t fps;
};

class Video {
public:
  Video() = default;

  ~Video();

  VideoHeader& header() {
    return m_header;
  }

  FILE* file() {
    return m_fp;
  }

  int getCurrentFrameIDByElapsed(double elapsed) const;

  bool readVideoFromFile(const char* path);

  bool readFrame(uint32_t frameId, uint8_t* out);

  uint64_t startTime() const {
    return m_startTime;
  }

  void startTime(uint64_t time) {
    if (m_startTime == 0) {
      m_startTime = time;
    }
  }

  int stride() const {
    return m_stride;
  }

  int frameSize() const {
    return m_stride * m_header.height;
  }

private:
  VideoHeader m_header = { 0 };

  uint64_t* m_framePixelOffsets = { nullptr };

  FILE* m_fp = { nullptr };

  float m_indexUnit = { 0.0f };

  uint64_t m_startTime = { 0 };

  int m_stride = {};
};